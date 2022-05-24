#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "hardware/FT800/FT800.h"
#include "libraries/FT_SD/FT_SD.h"
//#include "libraries/FT_RTC/FT_RTC.h"
#include "libraries/FT_GC/FT_Transport_SPI/FT_Transport_SPI.h"
#include "libraries/FT_GC/FT_GC.h"
#include "libraries/FT_GC/FT800/FT800Impl.h"
#include "ScreenBase.h"
#include "Config.h"
#include <FT800_Onscreen_Keyboard.h>


#define TAG_BASE_SSID_LIST      100
#define TAG_BASE_SSID_LIST_MAX  104
#define TAG_SSD_SLIDER          130
#define TAG_SCAN                132
#define TAG_SAVE_CONNECT        133
#define TAG_CLOSE               134
#define TAG_EXIT                253
#define TAG_SSID                129
#define TAG_PASSWORD            135
#define TAG_NOTHING             0
#define TAG_UNKNOWN             255

class WiFiConfigScreen : public ScreenBase
{
    public:
      WiFiConfigScreen(OnscreenKeyboard *keyboard) {
        _name = "WiFiConfig";
        _onscreenKeyboard = keyboard;
        memset(_currentSSID, 0, sizeof(_currentSSID));
        memset(_currentPassword, 0, sizeof(_currentPassword));
      };
      bool TagSelected(sTagXY *tag, sTrackTag *trackTag);
      bool Update(FT800Impl<FT_Transport_SPI> *ftImpl);
      void SetConfig(WifiClockConfig *config);
      void Load();
      char * GetSSID() { return _currentSSID; };
      char * GetPassword() { return _currentPassword; };

    private:
      OnscreenKeyboard *_onscreenKeyboard;
      int _wifiScanNumber = 0;
      bool _scanning = false;
      int32_t _startScanTime = 0;
      bool _showingKeyboard = false;
      int _keyboardAction = -1;
      float _scrollBar = 0;
      char _currentSSID[100];
      char _currentPassword[100];
      WifiClockConfig *_config;

      String _SSIDList[150];
      int32_t _RSSI[150];
      wifi_auth_mode_t _SSIDAuth[150];

      void ScanNetworks();
      void ShowKeyboard(int action = -1);
      void CloseKeyboard(bool canceled = false);
};

void WiFiConfigScreen::Load()
{
  strcpy(_currentSSID, _config->SSID);
  strcpy(_currentPassword, _config->WifiPassword);
  ScanNetworks();
}

void WiFiConfigScreen::ScanNetworks()
{
  if (!_scanning)
  {
    _startScanTime = millis();
    _wifiScanNumber = 0;
    RedrawScreen();
    Serial.println("Starting Scan...");
    WiFi.setAutoReconnect(false);
    delay(500);
    _wifiScanNumber = WiFi.scanNetworks(true);
    _scanning = true;
  }
}

void WiFiConfigScreen::SetConfig(WifiClockConfig *config)
{
  _config = config;
  strcpy(_currentSSID, config->SSID);
  strcpy(_currentPassword, config->WifiPassword);
}

void WiFiConfigScreen::ShowKeyboard(int action)
{
  _showingKeyboard = true;
  _keyboardAction = action;
}

void WiFiConfigScreen::CloseKeyboard(bool canceled)
{
  _screenChangeDelay = 3000;
  _showingKeyboard = false;

  switch(_keyboardAction)
  {
    case TAG_PASSWORD:
      if (!canceled)
      {
        strcpy(_currentPassword, _onscreenKeyboard->GetText());
        Serial.println("Updating Password");
      }
      break;
    case TAG_SSID:
      if (!canceled)
      {
        strcpy(_currentSSID, _onscreenKeyboard->GetText());
        Serial.println("Updating SSID");
      }
      break;
  }

  _keyboardAction = -1;
}

bool WiFiConfigScreen::TagSelected(sTagXY *tag, sTrackTag *trackTag)
{
  if (_screenChangeDelay > 0)
  {
    return false;
  } else if (_showingKeyboard) {
    _onscreenKeyboard->UpdateScreen(tag->tag);

    if (_onscreenKeyboard->IsClosed())
    {
      CloseKeyboard(_onscreenKeyboard->IsCanceled());
      _onscreenKeyboard->Reset();
      RedrawScreen();
    }
  } else if (tag->tag >= TAG_BASE_SSID_LIST && tag->tag <= TAG_BASE_SSID_LIST_MAX) {
    int baseNumber = _scrollBar * _wifiScanNumber;
    int i = tag->tag - TAG_BASE_SSID_LIST + baseNumber;
    if (_SSIDAuth[i] != WIFI_AUTH_OPEN)
    {
      strcpy(_currentSSID, _SSIDList[i].c_str());
    }
    RedrawScreen();
  } else {
    switch(tag->tag)
    {
      case TAG_SSD_SLIDER:
        _scrollBar = trackTag->track / 65536.0f;
        RedrawScreen();
        break;

      case TAG_SCAN:
      {
        ScanNetworks();
        break;
      }

      case TAG_SAVE_CONNECT:
        strcpy(_config->SSID, _currentSSID);
        strcpy(_config->WifiPassword, _currentPassword);
        _status = SCREEN_STATUS_SAVE_CLOCK_CONFIG;
        CloseScreen();
        break;
      
      case TAG_PASSWORD:
        ShowKeyboard(TAG_PASSWORD);
        _onscreenKeyboard->SetName("WiFi Password");
        _onscreenKeyboard->UpdateScreen();
        break;

      case TAG_SSID:
        ShowKeyboard(TAG_SSID);
        _onscreenKeyboard->SetName("SSID");
        _onscreenKeyboard->UpdateScreen();
        break;

      case TAG_EXIT:
      case TAG_CLOSE:
        this->CloseScreen();
        break;

      case TAG_UNKNOWN:
      case TAG_NOTHING:
        break;

      default:
        Serial.print("Unhandled tag: ");
        Serial.print(tag->tag);
        Serial.print(" - ");
        Serial.println(trackTag->track / 65536);
        break;
    }
  }
  
  return true;
}

bool WiFiConfigScreen::Update(FT800Impl<FT_Transport_SPI> *ftImpl)
{
  if (_screenChangeDelay > 0)
  {
    _screenChangeDelay--;
  }

  if (_scanning && (_wifiScanNumber = WiFi.scanComplete()) > 0)
  {
    _scanning = false;
    for (int i = 0; i < _wifiScanNumber; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
      _SSIDList[i] = WiFi.SSID(i).c_str();
      _RSSI[i] = i;
      _SSIDAuth[i] = WiFi.encryptionType(i);
      delay(10);
    }
    WiFi.scanDelete();
    RedrawScreen();
  }

  if (_scanning && (_startScanTime + 20000) < millis())
  {
    _scanning = false;
    RedrawScreen();
  }

  if (_needRedraw && !_showingKeyboard)
  {
    _needRedraw = false;
    ftImpl->DLStart();
    
    ftImpl->Tag(TAG_SSID);
    ftImpl->Clear(1, 1, 1);
    ftImpl->ColorRGB(255, 255, 255);
    ftImpl->Cmd_Text(275, 52, 26, 0, "SSID:");
    
    ftImpl->Begin(FT_RECTS);
    ftImpl->ColorRGB(0, 157, 255);
    ftImpl->Vertex2ii(315, 49, 0, 0);
    ftImpl->Vertex2ii(463, 69, 0, 0);
    ftImpl->End();
    ftImpl->ColorRGB(255, 255, 255);
    ftImpl->Cmd_Text(318, 51, 26, 0, _currentSSID);
    
    ftImpl->Tag(TAG_PASSWORD);
    ftImpl->ColorRGB(255, 255, 255);
    ftImpl->Cmd_Text(243, 89, 26, 0, "Password:");
    
    ftImpl->Begin(FT_RECTS);
    ftImpl->ColorRGB(0, 157, 255);
    ftImpl->Vertex2ii(315, 85, 0, 0);
    ftImpl->Vertex2ii(462, 108, 0, 0);
    ftImpl->End();
    ftImpl->ColorRGB(255, 255, 255);
    
    ftImpl->Cmd_Text(319, 89, 26, 0, _currentPassword);

    ftImpl->Tag(TAG_SCAN);
    ftImpl->Cmd_Button(60, 173, 84, 36, 27, FT_OPT_FLAT, "Scan...");

    int baseNumber = _scrollBar * _wifiScanNumber;
    int tagCount = 0;
    for(int x = baseNumber; x < min(_wifiScanNumber, baseNumber + 4); x++)
    {
      char showSSID[100];
      sprintf(showSSID, "%s %s", _SSIDList[x].c_str(), (_SSIDAuth[x] == WIFI_AUTH_OPEN)?" ":"*"); 
      ftImpl->Tag(TAG_BASE_SSID_LIST + tagCount);
      ftImpl->Cmd_Button(2, 45 + (30 * (x - baseNumber)), 180, 25, 27, FT_OPT_FLAT, showSSID);
      tagCount++;
    }

    ftImpl->Tag(TAG_NONE);  
    ftImpl->Begin(FT_RECTS);
    ftImpl->Vertex2ii(477, 33, 0, 0);
    ftImpl->ColorRGB(0, 157, 255);
    ftImpl->Vertex2ii(2, 2, 0, 0);
    ftImpl->End();
    ftImpl->ColorRGB(255, 255, 255);
    ftImpl->Cmd_Text(6, 8, 28, 0, "WiFi Menu");

    ftImpl->Tag(TAG_SSD_SLIDER);
    ftImpl->Cmd_Scrollbar(200, 47, 19, 113, FT_OPT_FLAT, 100 * _scrollBar, 10, 100);
    ftImpl->Cmd_Track(198, 40, 23, 130, TAG_SSD_SLIDER);
    
    ftImpl->Tag(TAG_SAVE_CONNECT);
    ftImpl->Cmd_Button(6, 228, 127, 36, 27, FT_OPT_FLAT, "Save/Connect");
    ftImpl->Tag(TAG_CLOSE);
    ftImpl->Cmd_Button(401, 229, 72, 36, 27, FT_OPT_FLAT, "Close");
    ftImpl->Tag(TAG_EXIT);
    ftImpl->Cmd_Button(443, 4, 30, 24, 27, FT_OPT_FLAT, "X");
   
    if (_scanning)
    {
      ftImpl->Cmd_Spinner(233, 161, 0, 0);
    }

    ftImpl->DLEnd();
    ftImpl->Finish();
  }

  return true;
}

