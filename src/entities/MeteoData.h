#ifndef _METEODATA_h
#define _METEODATA_h

#include <Arduino.h>

struct Data {
  float temperature = 0;
  uint16_t preassure = 0; 
  uint8_t humidity = 0;
  uint16_t co2 = 0;
  float heading = 0.0;
};

#endif