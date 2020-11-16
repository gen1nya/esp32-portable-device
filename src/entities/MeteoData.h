#ifndef _METEODATA_h
#define _METEODATA_h

#include <Arduino.h>

struct Data {
  double temperature = 0;
  unsigned int preassure = 0; 
  int humidity = 0;
  uint16_t co2 = 0;
};

#endif