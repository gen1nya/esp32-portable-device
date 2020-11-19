#ifndef _WIFINETWORK_H
#define _WIFINETWORK_H

#include <Arduino.h>

typedef struct WifiNetwork {
    String ssid;
    int32_t rssi;
    uint8_t encryptionType;
    uint8_t* bssid;
    int32_t channel;
    int scanResult;
};

#endif