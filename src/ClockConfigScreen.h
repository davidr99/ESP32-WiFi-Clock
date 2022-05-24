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

#define TAG_GMT_UP              133
#define TAG_GMT_DOWN            132
#define TAG_SAVE                131
#define TAG_WIFI                130
#define TAG_RECALIBRATE         129
#define TAG_CLOSE               134
#define TAG_EXIT                253
#define TAG_NOTHING             0
#define TAG_UNKNOWN             255

class ClockConfigScreen : public ScreenBase
{
    public:
        ClockConfigScreen(OnscreenKeyboard *keyboard) {
            _name = "ClockConfig";
            _onscreenKeyboard = keyboard;
        };
        bool TagSelected(sTagXY *tag, sTrackTag *trackTag);
        bool Update(FT800Impl<FT_Transport_SPI> *ftImpl);
        void SetConfig(WifiClockConfig *config);
        void Load();

    private:
        OnscreenKeyboard * _onscreenKeyboard;
        bool _showingKeyboard = false;
        int _gmtOffset;
        WifiClockConfig * _config;
        char _wifiIP[25];
        //void ShowKeyboard(int action = -1);
        //void CloseKeyboard(bool canceled = false);
};

void ClockConfigScreen::SetConfig(WifiClockConfig *config)
{
    _config = config;
    _gmtOffset = config->GMT_Offset;
}

void ClockConfigScreen::Load()
{
    _gmtOffset = _config->GMT_Offset;
}

bool ClockConfigScreen::TagSelected(sTagXY *tag, sTrackTag *trackTag)
{
    if (_screenChangeDelay > 0)
    {
        return false;
    }

    switch(tag->tag)
    {
        case TAG_CLOSE:
        case TAG_EXIT:
            this->CloseScreen();
            break;

        case TAG_GMT_DOWN:
            _gmtOffset--;
            if (_gmtOffset < -12) {
                _gmtOffset = -12;
            }
            RedrawScreen();
            break;

        case TAG_GMT_UP:
            _gmtOffset++;
            if (_gmtOffset > 12) {
                _gmtOffset = 12;
            }
            RedrawScreen();
            break;

        case TAG_RECALIBRATE:
            // Tell Main program to recalibrate
            _status = SCREEN_STATUS_RECALIBRATE;
            delay(500);
            break;

        case TAG_SAVE:
            this->CloseScreen();
            _config->GMT_Offset = _gmtOffset;
            this->_status = SCREEN_STATUS_SAVE_CLOCK_CONFIG;
            break;

        case TAG_WIFI:
            this->_status = SCREEN_STATUS_WIFI_CONFIG;
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
  
  return true;
}

bool ClockConfigScreen::Update(FT800Impl<FT_Transport_SPI> *ftImpl)
{
    if (_screenChangeDelay > 0)
    {
        _screenChangeDelay--;
    }

    const char * wifiIP = WiFi.localIP().toString().c_str();

    // Move IP to local copy
    if (strcpy(_wifiIP, wifiIP) != 0)
    {
        strcpy(_wifiIP, WiFi.localIP().toString().c_str());
    }

    if (_needRedraw && !_showingKeyboard)
    {
        _needRedraw = false;
        char gmtOffsetString[4];
        sprintf(gmtOffsetString, "%d", _gmtOffset);

        ftImpl->DLStart();
        ftImpl->Tag(TAG_NONE);
        ftImpl->ColorRGB(255, 255, 255);
        ftImpl->Cmd_Text(171, 53, 28, 0, gmtOffsetString);
        ftImpl->Begin(FT_RECTS);
        ftImpl->Vertex2ii(477, 33, 0, 0);
        ftImpl->ColorRGB(0, 157, 255);
        ftImpl->Vertex2ii(2, 2, 0, 0);
        ftImpl->End();
        ftImpl->ColorRGB(255, 255, 255);
        ftImpl->Cmd_Text(6, 8, 28, 0, "Clock Menu");
        ftImpl->Cmd_Text(7, 52, 28, 0, "GMT Offset:");
        ftImpl->Tag(TAG_GMT_UP);
        ftImpl->Cmd_Button(126, 45, 40, 40, 27, FT_OPT_FLAT, "+");
        ftImpl->Tag(TAG_GMT_DOWN);
        ftImpl->Cmd_Button(211, 45, 40, 40, 27, FT_OPT_FLAT, "-");
        ftImpl->Tag(TAG_NONE);
        ftImpl->Cmd_Text(64, 100, 28, 0, "My IP:");
        
        ftImpl->ColorRGB(131, 222, 255);
        ftImpl->Begin(FT_RECTS);
        ftImpl->Vertex2ii(129, 99, 0, 0);
        ftImpl->Vertex2ii(307, 123, 0, 0);
        ftImpl->End();
        ftImpl->ColorRGB(0, 0, 0);
        ftImpl->Cmd_Text(131, 99, 28, 0, _wifiIP);
        
        ftImpl->ColorRGB(255, 222, 255);
        
        /*
        ftImpl->Tag(3);
        ftImpl->Cmd_Button(8, 142, 120, 36, 27, FT_OPT_FLAT, "RTC Sync");
        ftImpl->Tag(4);
        ftImpl->Cmd_Button(135, 142, 120, 36, 27, FT_OPT_FLAT, "NTP Sync");
        */
        ftImpl->Tag(TAG_SAVE);
        ftImpl->Cmd_Button(6, 228, 63, 36, 27, FT_OPT_FLAT, "Save");
        ftImpl->Tag(TAG_WIFI);
        ftImpl->Cmd_Button(6, 136, 107, 36, 27, FT_OPT_FLAT, "WiFi Config");
        ftImpl->Tag(TAG_RECALIBRATE);
        ftImpl->Cmd_Button(119, 136, 168, 36, 27, FT_OPT_FLAT, "Recalibrate Screen");

        
        
        ftImpl->Tag(TAG_CLOSE);
        ftImpl->Cmd_Button(401, 229, 72, 36, 27, FT_OPT_FLAT, "Close");
        ftImpl->Tag(TAG_EXIT);
        ftImpl->Cmd_Button(443, 4, 30, 24, 27, FT_OPT_FLAT, "X");
        
        ftImpl->DLEnd();
        ftImpl->Finish();
    }
    
    return true;
}
