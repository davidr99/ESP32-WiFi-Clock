#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "hardware/FT800/FT800.h"
#include "libraries/FT_SD/FT_SD.h"
//#include "libraries/FT_RTC/FT_RTC.h"
#include "libraries/FT_GC/FT_Transport_SPI/FT_Transport_SPI.h"
#include "libraries/FT_GC/FT_GC.h"
#include "libraries/FT_GC/FT800/FT800Impl.h"
#include "ScreenBase.h"
#include "Config.h"
#include <FT800_Onscreen_Keyboard.h>
#include "RTClib.h"

#define RTC

#define TAG_CLOSE               134
#define TAG_EXIT                253
#define TAG_BACKGROUND          252
#define TAG_NOTHING             0
#define TAG_UNKNOWN             255

time_t _lastNTPServerSync;

#ifdef RTC
    RTC_DS3231 _rtcClock;
    int32_t _lastRTCTime;
#endif

class ClockScreen : public ScreenBase
{
    public:
        ClockScreen();
        bool TagSelected(sTagXY *tag, sTrackTag *trackTag);
        bool Update(FT800Impl<FT_Transport_SPI> *ftImpl);
        void SetConfig(WifiClockConfig *config);
        void Load();
        void PenDown(uint16_t x, uint16_t y);

    private:
        int _lastUpdate = 0;
        int32_t _lastScreenTouch = 0;
#ifdef RTC
        void UpdateRTCTime();
#endif
};


void timeSyncNotificationCB(struct timeval *tv)
{
    Serial.println("Time sync to NTP Server!");
    _lastNTPServerSync = tv->tv_sec;
#ifdef RTC
    DateTime now;
    now = _lastNTPServerSync;
    _rtcClock.adjust(now);
#endif
}

ClockScreen::ClockScreen()
{
    _name = "Clock";
#ifdef RTC
    _rtcClock.begin();
    UpdateRTCTime();
#endif
    sntp_set_time_sync_notification_cb(&timeSyncNotificationCB);
}

#ifdef RTC
void ClockScreen::UpdateRTCTime()
{
    if (!_rtcClock.lostPower())
    {
        struct timeval tv;
        tv.tv_sec = _rtcClock.now().unixtime();
        tv.tv_usec = 0;
        settimeofday(&tv, NULL);
        _lastRTCTime = tv.tv_sec;
    }
}
#endif

void ClockScreen::PenDown(uint16_t x, uint16_t y)
{
    _lastScreenTouch = millis();
}

void ClockScreen::Load()
{
}

bool ClockScreen::TagSelected(sTagXY *tag, sTrackTag *trackTag)
{
    if (_screenChangeDelay > 0)
    {
        return false;
    }

    _lastScreenTouch = millis();

    switch(tag->tag)
    {
        case TAG_EXIT:
            this->CloseScreen();
            break;

        case TAG_BACKGROUND:
            // Show Clock Config Screen
            this->_status = SCREEN_STATUS_CLOCK_CONFIG;
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

bool ClockScreen::Update(FT800Impl<FT_Transport_SPI> *ftImpl)
{
    if (_screenChangeDelay > 0)
    {
        _screenChangeDelay--;
    }

    if (_lastUpdate < millis())
    {
        _lastUpdate = millis() + 250;
        RedrawScreen();
    }

/*
    if (_lastScreenTouch + (10) * 1000 < millis())
    {
        ftImpl->Write(REG_PWM_DUTY, 128);
    }
    else
    {
        ftImpl->Write(REG_PWM_DUTY, 255);
    }
*/

    if (_needRedraw)
    {
        _needRedraw = false;

        /* Change the below string for experimentation */
        char displayDate[15];
        char displayGMT[50];
        char displayLocal[50];
        time_t now;
        struct tm gmtTime;
        struct tm localTime;

        time(&now);
        gmtime_r(&now, &gmtTime);
        localtime_r(&now, &localTime);

#ifdef RTC
        if ((_lastRTCTime + 61 * 60) > now && (_lastNTPServerSync + 61 * 60) > now)
        {
            UpdateRTCTime();
        }
#endif

        sprintf(displayDate, "%02d/%02d/%04d", localTime.tm_mon + 1, localTime.tm_mday, 1900 + localTime.tm_year);
        sprintf(displayGMT,   "GMT:   %02d:%02d:%02d", gmtTime.tm_hour, gmtTime.tm_min, gmtTime.tm_sec);
        sprintf(displayLocal, "Local: %02d:%02d:%02d", localTime.tm_hour, localTime.tm_min, localTime.tm_sec);
        
        ftImpl->DLStart();//start the display list. Note DLStart and DLEnd are helper apis, Cmd_DLStart() and Display() can also be utilized.
        ftImpl->Tag(TAG_BACKGROUND);
        ftImpl->Begin(FT_RECTS);
        ftImpl->ColorRGB(0x00,0x00,0x00);
        ftImpl->Vertex2ii(0, 0, 0, 0);
        ftImpl->Vertex2ii(320, 240, 0, 0);
        ftImpl->End();
        ftImpl->ColorRGB(0x00,0xFF,0x00);//set the color of the string to while color
        ftImpl->Cmd_Text(FT_DISPLAYWIDTH/2, FT_DISPLAYHEIGHT/2 - 100, 0, FT_OPT_CENTER, displayDate);
        ftImpl->Cmd_Text(FT_DISPLAYWIDTH/2, FT_DISPLAYHEIGHT/2 - 20, 0, FT_OPT_CENTER, displayGMT);
        ftImpl->Cmd_Text(FT_DISPLAYWIDTH/2, FT_DISPLAYHEIGHT/2 + 40, 0, FT_OPT_CENTER, displayLocal);
        ftImpl->DLEnd();//end the display list
        ftImpl->Finish();//render the display list and wait for the completion of the DL
    }
    
    return true;
}
