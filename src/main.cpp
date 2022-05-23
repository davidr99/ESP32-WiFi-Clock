#define min(a,b) ((a)<(b)?(a):(b))

#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include "Wire.h"
#include <FT800_Onscreen_Keyboard.h>
#include <FT_VM800P43_50.h>
#include "DisplayCalibration.h"
#include "EEPROMWrapper.h"
#include "EEPROM.h"
#include "Config.h"
#include "ScreenBase.h"
#include "WiFiConfigScreen.h"
#include "ClockConfigScreen.h"
#include "ClockScreen.h"
#include "CustomFont.h"


//FT800IMPL_SPI FTImpl(4, 15);
FT800IMPL_SPI FTImpl(16, 21);
OnscreenKeyboard Keyboard(&FTImpl);

int lastTagId = 0;
bool penLastStatus = false;
bool newConfig = false;

WiFiConfigScreen wifiConfigScreen(&Keyboard);
ClockConfigScreen clockConfigScreen(&Keyboard);
ClockScreen clockScreen;

ScreenBase * currentScreen;
WifiClockConfig Config;


void SaveConfig()
{
  Serial.println("SaveConfig()");
  EEPROM_writeAnything(0, Config);
}

void LoadConfig()
{
  Serial.println("LoadConfig()");
  EEPROM_readAnything(0, Config);
}

/* API to display custom Font on screen */
/* Fonts are generated using FT800 specific font conversion tools */
void CustomFont()
{ 
  FTImpl.DLStart();
  FTImpl.Cmd_Inflate(RAM_FONTS_SF_DIGITAL_READOUT_MEDIUM);
  FTImpl.WriteCmdfromflash(FONTS_SF_DIGITAL_READOUT_MEDIUM, sizeof(FONTS_SF_DIGITAL_READOUT_MEDIUM));
  FTImpl.Finish();
  FTImpl.DLStart();
  FTImpl.BitmapHandle(0);
  FTImpl.BitmapSource(-7428);
  FTImpl.BitmapLayout(FT_L1, 4, 67);
  FTImpl.BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, 32, 67);
  FTImpl.Cmd_SetFont(0, 1000);
  FTImpl.DLEnd();
  FTImpl.Finish();
}

/* API for calibration on ft800 */
void Calibrate(bool reCalibrate = false)
{  
  /*************************************************************************/
  /* Below code demonstrates the usage of calibrate function. Calibrate    */
  /* function will wait untill user presses all the three dots. Only way to*/
  /* come out of this api is to reset the coprocessor bit.                 */
  /*************************************************************************/

  DisplayCalibrationConfig displayCalConfig;

  EEPROM_readAnything(sizeof(Config) + 1, displayCalConfig);    // Read next from config

  if (displayCalConfig.SaveVersion != 0x01 || reCalibrate)
  {
    /* Construct the display list with grey as background color, informative string "Please Tap on the dot" followed by inbuilt calibration command */
    FTImpl.DLStart();
    FTImpl.ClearColorRGB(64,64,64);
    FTImpl.Clear(1,1,1);    
    FTImpl.ColorRGB(0xff, 0xff, 0xff);
    FTImpl.Cmd_Text((FT_DISPLAYWIDTH/2), (FT_DISPLAYHEIGHT/2), 27, FT_OPT_CENTER, "Please Tap on the dot");
    FTImpl.Cmd_Calibrate(0);
    
    /* Wait for the completion of calibration - either finish can be used for flush and check can be used */
    FTImpl.Finish();

    // Read Cal data
    uint32_t tA = FTImpl.Read32(REG_TOUCH_TRANSFORM_A);
    uint32_t tB = FTImpl.Read32(REG_TOUCH_TRANSFORM_B);
    uint32_t tC = FTImpl.Read32(REG_TOUCH_TRANSFORM_C);
    uint32_t tD = FTImpl.Read32(REG_TOUCH_TRANSFORM_D);
    uint32_t tE = FTImpl.Read32(REG_TOUCH_TRANSFORM_E);
    uint32_t tF = FTImpl.Read32(REG_TOUCH_TRANSFORM_F);

    displayCalConfig.SaveVersion = 0x01;
    displayCalConfig.tA = tA;
    displayCalConfig.tB = tB;
    displayCalConfig.tC = tC;
    displayCalConfig.tD = tD;
    displayCalConfig.tE = tE;
    displayCalConfig.tF = tF;

    EEPROM_writeAnything(sizeof(Config) + 1, displayCalConfig);
  }
  else
  {
    // Read Cal data
    FTImpl.Write32(REG_TOUCH_TRANSFORM_A, displayCalConfig.tA);
    FTImpl.Write32(REG_TOUCH_TRANSFORM_B, displayCalConfig.tB);
    FTImpl.Write32(REG_TOUCH_TRANSFORM_C, displayCalConfig.tC);
    FTImpl.Write32(REG_TOUCH_TRANSFORM_D, displayCalConfig.tD);
    FTImpl.Write32(REG_TOUCH_TRANSFORM_E, displayCalConfig.tE);
    FTImpl.Write32(REG_TOUCH_TRANSFORM_F, displayCalConfig.tF);
  }
}

void WiFiScreenUpdate()
{
  sTagXY tag;
  sTrackTag trackTag;
  FTImpl.GetTagXY(tag);
  FTImpl.GetTrackTag(trackTag);

  if (FTImpl.IsPendown())
  {
    uint32_t readWord = FTImpl.Read32(REG_TOUCH_SCREEN_XY);
    uint16_t yPos = readWord;
    uint16_t xPos = ((uint16_t)(readWord >> 16));

    penLastStatus= true;
    currentScreen->PenDown(xPos, yPos);
  }
  else
  {
    if (penLastStatus)
    {
      penLastStatus = false;
      currentScreen->PenUp();
    }
  }

  if (tag.tag != lastTagId || trackTag.track != 0)
  {
    lastTagId = tag.tag;

    if (currentScreen != nullptr)
    {
      currentScreen->TagSelected(&tag, &trackTag);
    }

    Serial.print("Tag: ");
    Serial.print(tag.tag);
    Serial.print(" - ");
    Serial.println(trackTag.track / 65536);
  }

  currentScreen->Update(&FTImpl);
}

void SetupDisplay()
{
    uint32_t chipid = 0;
    FTImpl.Init(FT_DISPLAY_RESOLUTION);//configure the display to the WQVGA
  
    delay(20);//for safer side
    chipid = FTImpl.Read32(FT_ROM_CHIPID);

    /* Identify the chip */

    if(FT800_CHIPID != chipid)
    {
      Serial.print("Error in chip id read ");
      Serial.println(chipid,HEX);
    }
    FTImpl.DisplayOn();
    Serial.print("running");

    CustomFont();
    Calibrate();
}

void WiFiCallBack(system_event_t *sys_event, wifi_prov_event_t *prov_event)
{
  switch(sys_event->event_id)
  {
    case SYSTEM_EVENT_STA_CONNECTED:
      {
        Serial.println("We are connected to WiFi!");
        break;
      }
    case SYSTEM_EVENT_STA_DISCONNECTED:
      {
        Serial.println("We are disconnected from WiFi!");
        break;
      }
    case SYSTEM_EVENT_STA_GOT_IP:
      {
        Serial.println("We got an IP!");
        break; 
      }
    default:
      break;
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  EEPROM.begin(1024);

  LoadConfig();

  if (Config.StartDetection != 0x02)
  {
    newConfig = true;
    memset(&Config, 0, sizeof(Config));
    Config.StartDetection = 0x02;
    Config.SyncTime = 65;
    strcpy(Config.NTPServer, "pool.ntp.org");
    strcpy(Config.AdminPassword, "admin");
    strcpy(Config.SSID, "");
    strcpy(Config.WifiPassword, "");
    Config.GMT_Offset = -5; // EST
    SaveConfig();
  }   

  configTime(0, Config.GMT_Offset * 3600, Config.NTPServer);

  WiFi.setAutoReconnect(true);
  WiFi.setHostname("WIFI-Clock");
  WiFi.onEvent(&WiFiCallBack);

  // Try to connect if we have a SSID
  if (Config.SSID[0] != 0x00)
  {
    if (Config.WifiPassword[0] != 0x00)
    {
      WiFi.begin(Config.SSID, Config.WifiPassword);
    }
    else
    {
      WiFi.begin(Config.SSID);      
    }
  }

  Serial.print("Starting..");
  SetupDisplay();

  wifiConfigScreen.SetConfig(&Config);
  clockConfigScreen.SetConfig(&Config);

  if (newConfig)
  {
    currentScreen = (ScreenBase *) &wifiConfigScreen;
  }
  else
  {
    currentScreen = (ScreenBase *) &clockScreen;
  }

  currentScreen->Load();
  currentScreen->Update(&FTImpl);
}

void loop() {
  // Update screen
  WiFiScreenUpdate();

  switch (currentScreen->GetStatus())
  {
    case SCREEN_STATUS_WIFI_CONFIG:
      currentScreen = (ScreenBase *) &wifiConfigScreen;
      currentScreen->Reset();
      currentScreen->Load();
      break;
    case SCREEN_STATUS_CLOCK_CONFIG:
      currentScreen = (ScreenBase *) &clockConfigScreen;
      currentScreen->Reset();
      currentScreen->Load();
      break;
    case SCREEN_STATUS_RECALIBRATE:
      Calibrate(true);
      currentScreen->Reset();
      currentScreen->Load();
      break;
    case SCREEN_STATUS_SAVE_CLOCK_CONFIG:
      SaveConfig();
      if (!WiFi.isConnected())
      {
        if (Config.SSID[0] != 0x00)
        {
          if (Config.WifiPassword[0] != 0x00)
          {
            WiFi.begin(Config.SSID, Config.WifiPassword);
          }
          else
          {
            WiFi.begin(Config.SSID);      
          }
        }
      }
      configTime(0, Config.GMT_Offset * 3600, Config.NTPServer);
      break;
    
    default:
      break;
  }

  currentScreen->ClearStatus();

  if (currentScreen->IsClosed())
  {
    // If we are not on the clock screen bring us back here
    if (currentScreen != &clockScreen)
    {
      currentScreen = (ScreenBase *) &clockScreen;
      currentScreen->Reset();
      currentScreen->Load();
    }
  }
}