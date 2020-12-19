#ifndef _MAIN_H
#define _MAIN_H

#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include <Adafruit_ST7735.h>
#include "Adafruit_Sensor.h"
#include "Adafruit_BME280.h"
#include <TinyGPS++.h>
#include <libraries/CircularBuffer.h>
#include <libraries/MovingAverage.h>
#include <libraries/GyverEncoder.h>
#include <libraries/btAudio.h>

#include <EEPROM.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <string>
#include "time.h"
#include "Audio.h"
#include "SD.h"
#include "FS.h"

#include <config.h>
#include <icons.h>
#include <utils.h>
#include <entities/MeteoData.h>
#include <entities/UiState.h>
#include <entities/WifiNetwork.h>

/**
 * Method signatures;
 * */
int co2ppm(void);

// interrupts
void geigerCounterIsr(void);
void buttonOkIsr(void);
void buttonUpIsr(void);
void buttonDownIsr(void);

// RTOS tasks
void cpsUpdater(void * parameter);
void getSensorsData(void * parameter);
void ui(void * parameter);
void webServer(void * parameter);
void scanWifi(void * parameter);
void wifiSwitch(void * parameter);
void audioPlayer(void * parameter);

// ui methods
void drawHeader(void);
void drawMenu(char title[]);
void drawMainScreen(void);
void drawMenuScreen(void);
void drawWifiMenuScreen(void);
void drawEnableWifiScreen(void);
void drawCo2Scren(void);
void drawEnableCo2Scren(void);
void drawWifiScannerScreen(void);
void drawGeigerScreen(void);
void drawEnableGeigerScreen(void);
void drawMeteoSensorScren(Data meteoData);
void drawAudioScreen(void);
void drawBtAudioScreen(void);
void drawSdCardAudioScreen(void);
void drawGpsScreen(TinyGPSPlus gps);

void onUiStateChanged(void);

uint8_t convertRealValueToPx(int item, int measuringCounter);
int getMeasurmentArraySize(int item);


#endif