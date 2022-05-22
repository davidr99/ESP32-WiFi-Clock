#pragma once


struct WifiClockConfig
{
    int StartDetection;
    int SyncTime;
    char NTPServer[128];
    char AdminPassword[10];
    int GMT_Offset;
    char SSID[128];
    char WifiPassword[128];
};