#pragma once

#include "hardware/FT800/FT800.h"
#include "libraries/FT_SD/FT_SD.h"
//#include "libraries/FT_RTC/FT_RTC.h"
#include "libraries/FT_GC/FT_Transport_SPI/FT_Transport_SPI.h"
#include "libraries/FT_GC/FT_GC.h"
#include "libraries/FT_GC/FT800/FT800Impl.h"
#include "ScreenStatus.h"

//typedef void (*FuncDoneCallbackPtr)(void *);

class ScreenBase
{
    public:
        virtual bool TagSelected(sTagXY *tag, sTrackTag *trackTag) { return false; };
        virtual bool Update(FT800Impl<FT_Transport_SPI> *ftImpl) { return false; };
        virtual bool IsClosed() { return this->_isClosed; };
        virtual void Load() { };
        virtual void Reset() { ClearStatus(); _isClosed = false; _needRedraw = true; _screenChangeDelay = 3000; };
        virtual char * GetName() { _name; }
        virtual int GetStatus() { return _status; };
        virtual void ClearStatus() { _status = 0; };
        virtual void PenDown(uint16_t x, uint16_t y) { };
        virtual void PenUp() { };

    protected:
        char * _name = "";
        int _status = 0;
        bool _needRedraw = true;
        bool _isClosed = false;
        int _screenChangeDelay = 0;

        virtual void CloseScreen() { _isClosed = true; }
        virtual void RedrawScreen() { _needRedraw = true; }
};