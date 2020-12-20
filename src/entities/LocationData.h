#ifndef _LOCATIONDATA_H
#define _LOCATIONDATA_H

#include <Arduino.h>

struct LocationData {
    float lat;
    float lng;
    bool locationIsValid;

    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    bool timeIsValid;

    uint8_t day;
    uint8_t month;
    uint16_t year;
    bool dateIsValid;
};

#endif