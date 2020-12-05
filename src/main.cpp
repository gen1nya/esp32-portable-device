#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include <Adafruit_ST7735.h>
#include "Adafruit_Sensor.h"
#include "Adafruit_BME280.h"
#include <libraries/CircularBuffer.h>
#include <libraries/MovingAverage.h>
#include <libraries/GyverEncoder.h>
#include <libraries/btAudio.h>

#include <EEPROM.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <string> 
#include "time.h"

#include <config.h>
#include <icons.h>
#include <utils.h>
#include <entities/MeteoData.h>
#include <entities/UiState.h>
#include <entities/WifiNetwork.h>

volatile unsigned long cps = 0L;
MovingAverage<volatile unsigned long, GEIGER_CYCLE_SIZE> cpm;
MovingAverage<volatile uint16_t, CO2_CYCLE_SIZE> co2DataCycleArray;
MovingAverage<volatile uint16_t, PRESSURE_CYCLE_SIZE> pressureCycleArray;
MovingAverage<volatile float, TEMPERATURE_CYCLE_SIZE> temperatureCycleArray;
MovingAverage<volatile uint8_t, HUMIDITY_CYCLE_SIZE> humidityCycleArray;

struct tm timeinfo;

btAudio audio = btAudio(bluetoothName);
Adafruit_ST7735 oled = Adafruit_ST7735(CS_PIN, DC_PIN, RST_PIN);
//Adafruit_SSD1351 oled = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, CS_PIN, DC_PIN, RST_PIN);
Adafruit_BME280 bme;

UiState uiState = UiState(); 


bool co2Enabled = false;
bool geigerEnabled = false;
bool wifiEnabled = false;

QueueHandle_t handler;
QueueHandle_t meteodataQueueHandler;
QueueHandle_t buttonClickQueueHandler;
TaskHandle_t webServerTaskHandler;

Encoder enc(PIN_CLK, PIN_DT, ENC_NO_BUTTON, TYPE2);

char ** networks = new char * [0];
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
void drawBtAudioScreen(void);

void onUiStateChanged(void);

void showMainScreen(void);
void showMenuScreen(void);
void showWifiScren(void);
void showEnableWifiScreen(void);
void showCo2Scren(void);
void showEnableCo2Scren(void);
void showScanWifiScreen(void);
void showGeigerScren(void);
void showEnableGeigerScreen(void);
void showMeteoSensorScren(void);
void showBtAudioScreen(void);

uint8_t convertRealValueToPx(int item, int measuringCounter);
int getMeasurmentArraySize(int item);

void setup() {
  Serial.begin(UART_BAUNDRATE);
  Serial2.begin(Z19B_UART_BAUNDRATE);
  Serial.println("init");
  EEPROM.begin(EEPROM_INITIAL_SIZE);
  
  //audio.begin();
  //audio.I2S(PIN_I2S_BCLK, PIN_I2S_DOUT, PIN_I2S_LRC);

  pinMode(PIN_GEIGER_COUNTER, INPUT_PULLUP);
  pinMode(PIN_BUTTON_OK, INPUT_PULLUP);
  pinMode(PIN_BUTTON_UP, INPUT_PULLUP);
  pinMode(PIN_BUTTON_DOWN, INPUT_PULLUP);
  pinMode(PIN_ENABLE_GEIGER_COUNTER, OUTPUT);
  attachInterrupt(PIN_BUTTON_OK, buttonOkIsr, FALLING);
  attachInterrupt(PIN_DT, buttonUpIsr, CHANGE);
  attachInterrupt(PIN_CLK, buttonDownIsr, CHANGE);

  uiState.setUiState(UiState::MAIN);

  //oled.begin();
  oled.initR(INITR_144GREENTAB); // Init ST7735R chip, green tab
  oled.setRotation(1);
  delay(500);
  oled.fillScreen(BLACK);
  oled.setTextColor(GREEN);
  oled.setCursor(0, 0);
  oled.println("oled ok");

  co2Enabled = EEPROM.readBool(EEPROM_ADDRESS_CO2_ENABLED);
  wifiEnabled = EEPROM.readBool(EEPROM_ADDRESS_WIFI_ENABLED);
  geigerEnabled = EEPROM.readBool(EEPROM_ADDRESS_GEIGER_ENABLED);

  if (geigerEnabled) {
    attachInterrupt(PIN_GEIGER_COUNTER, geigerCounterIsr, FALLING);
  }

  digitalWrite(PIN_ENABLE_GEIGER_COUNTER, geigerEnabled);

  oled.printf("\nco2: %s\n", co2Enabled ? "enabled" : "disabled");
  oled.printf("\nwifi: %s\n", wifiEnabled ? "enabled" : "disabled");
  oled.printf("\ngeiger: %s\n", geigerEnabled ? "enabled" : "disabled");
  
  if (wifiEnabled) {
    oled.print("connecting to wifi");

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    uint8_t i = 0;
    while (WiFi.status() != WL_CONNECTED && i < WIFI_CONNECTION_TIMEOUT) {
      delay(1000);
      i++;
      oled.print(".");
    }

    if (WiFi.status() != WL_CONNECTED) {
      oled.setTextColor(RED);
      oled.println("\nconnection timeout");
      oled.setTextColor(GREEN);
      wifiEnabled = false;
    } else {
      oled.println("\nconnected");
      oled.println("ntp setting up..");
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      wifiEnabled = true;
    }
  }
  
  xTaskCreate(cpsUpdater, "cpsUpdater", 1000, NULL, 2, NULL );
  xTaskCreate(getSensorsData,  "getSensorsData", 2500, NULL, 4, NULL);
  xTaskCreate(ui, "ui", 3000, NULL, 5, NULL);

  if (wifiEnabled) {
    oled.println("start webServer");
    xTaskCreate(webServer, "webServer", 3000, NULL, 1, &webServerTaskHandler);
  }
  handler = xQueueCreate(1, sizeof networks);
}


void loop() {
  //enc.tick();
    if (enc.isLeft() && uiState.hasMenu()) {
      uiState.onEncoderEncrease();
    }
    if (enc.isRight() && uiState.hasMenu()) {
      uiState.onEncoderDecrease();
    }
}

void drawHeader() {
  oled.setTextSize(1);
  oled.setTextColor(WHITE, BLACK);
  oled.setCursor(0, 0);
  oled.setTextWrap(false);
  if (!wifiEnabled ) {
    oled.setTextColor(RED, BLACK);
    oled.print("[N/A] ");
  } else if (WiFi.status() != WL_CONNECTED) {
    oled.setTextColor(YELLOW, BLACK);
    oled.print("[---] ");
  } else {
    oled.setTextColor(WHITE, BLACK);
    oled.printf("[%d] ", WiFi.RSSI());
    oled.setTextColor(BLUE, BLACK);
    oled.printf("%02d:%02d ", timeinfo.tm_hour, timeinfo.tm_min);
  }
  
  oled.setTextColor(WHITE, BLACK);
  oled.printf("RAM:%dKB               \n", (ESP.getFreeHeap() / 1024));
  if (wifiEnabled && (WiFi.status() == WL_CONNECTED)) {
    oled.printf("ip: %s\n", WiFi.localIP().toString().c_str());
  } else {
    oled.printf("%21s","");  // fill address string
  }

  oled.drawLine(0, 17, 128, 17, WHITE);
  oled.setTextWrap(true);
}

void drawMeteoSensorScren(Data meteoData) {
  drawHeader();

  oled.setCursor(0, 19);
  oled.setTextColor(GREEN, BLACK);
  oled.setTextSize(1);

  drawMenu("[ Meteosensor data ]");

  for(
    uint8_t measuringCounter = 0;
    measuringCounter < getMeasurmentArraySize(uiState.getSelectedMenuItem());
    measuringCounter++
  ) {
  
    uint8_t convertedValue = convertRealValueToPx(uiState.getSelectedMenuItem(), measuringCounter);
    oled.fillRect(measuringCounter, 127 - convertedValue, 1, 
        -CHART_HEIGHT + (convertedValue <= CHART_HEIGHT ? convertedValue : CHART_HEIGHT), BLACK);
    oled.fillRect(measuringCounter, 127, 1,
       -(convertedValue <= CHART_HEIGHT ? convertedValue : CHART_HEIGHT), GREEN);
  }
}

uint8_t convertRealValueToPx(int item, int measuringCounter) {
  if (item == 2) {
    return pressureCycleArray.get(measuringCounter) / 20;
  } else if (item == 3) {
    return humidityCycleArray.get(measuringCounter) / 1.9;
  } else {
    return temperatureCycleArray.get(measuringCounter) / 1.5;
  }
}

int getMeasurmentArraySize(int item) {
  if (item == 2) {
    return pressureCycleArray.size();
  } else if (item == 3) {
    return humidityCycleArray.size();
  } else {
    return temperatureCycleArray.size();
  }
}

void drawEnableWifiScreen() {
  drawHeader();

  oled.setCursor(0, 19);
  oled.setTextColor(GREEN, BLACK);
  oled.setTextSize(1);

  drawMenu("[ Enable WiFi? ]");
}

void drawBtAudioScreen() {
  drawHeader();

  oled.setCursor(0, 19);
  oled.setTextColor(GREEN, BLACK);
  oled.setTextSize(1);

  drawMenu("[ Bluetooth speaker ]");
}

void drawWifiMenuScreen() {
  drawHeader();

  oled.setCursor(0, 19);
  oled.setTextColor(GREEN, BLACK);
  oled.setTextSize(1);

  drawMenu("[ WiFi ]");
}

void drawCo2Scren(uint16_t co2ppm) {
  drawHeader();
  
  oled.setCursor(0, 19);
  oled.setTextColor(GREEN, BLACK);
  oled.setTextSize(1);
  
  drawMenu("[ CO2 ]");
  
  oled.printf("cur: %-d ppm    ", co2ppm);
  
  for(
    uint8_t co2measuringCounter = 0;
    co2measuringCounter < co2DataCycleArray.size();
    co2measuringCounter++
  ) {
    // convert real value to line height. 5000(max ppm) / 56 (max chart line height)
    uint16_t co2RawValue = co2DataCycleArray.get(co2measuringCounter);
    uint8_t co2Value = co2RawValue / 89;
    oled.fillRect(co2measuringCounter, 127 - co2Value, 1, 
        -CHART_HEIGHT + (co2Value <= CHART_HEIGHT ? co2Value : CHART_HEIGHT), BLACK);
    oled.fillRect(co2measuringCounter, 127, 1, -(co2Value <= CHART_HEIGHT ? co2Value : CHART_HEIGHT),
        (co2RawValue >= CO2_RED_ALERT) ? RED : (co2RawValue >= CO2_YELLOW_ALERT) ? YELLOW : GREEN);
  }
}

void drawEnableCo2Scren() {
  drawHeader();
  
  oled.setCursor(0, 19);
  oled.setTextColor(GREEN, BLACK);
  oled.setTextSize(1);
  
  drawMenu("[ enable CO2? ]");
}

void drawGeigerScreen() {
  drawHeader();
  
  oled.setCursor(0, 19);
  oled.setTextColor(GREEN, BLACK);
  oled.setTextSize(1);
  
  drawMenu("[ Geiger counter ]");
  double microsiverts = double(cpm.sum() * (60 / cpm.capacity()) / CPS_TO_MECROSIVERTS_K);

  oled.printf("1min avg: %-2.2fuSv    ", microsiverts);
  for(
    uint8_t cpsCount = 0;
    cpsCount < cpm.size();
    cpsCount++
  ) {
    unsigned long cpsValue = cpm.get(cpsCount);
    oled.fillRect(cpsCount*2, 127 - cps, 2,
        -CHART_HEIGHT + (cpsValue <= CHART_HEIGHT ? cpsValue : CHART_HEIGHT), BLACK);
    oled.fillRect(cpsCount*2, 127, 2, 
        -(cpsValue <= CHART_HEIGHT ? cpsValue : CHART_HEIGHT), GREEN);
  }
}

void drawEnableGeigerScreen() {
  drawHeader();
  
  oled.setCursor(0, 19);
  oled.setTextColor(GREEN, BLACK);
  oled.setTextSize(1);
  
  drawMenu("[ enable geiger? ]");
}

void drawWifiScannerScreen() {
  drawHeader();
  
  oled.setCursor(0, 19);
  oled.setTextColor(GREEN, BLACK);
  oled.setTextSize(1);
  oled.println(" networks nearby");
  oled.println();

  for(
    uint8_t i = (uiState.getSelectedMenuItem() < ON_SCREEN_MENU_ITEMS) ? 0 : uiState.getSelectedMenuItem() - ON_SCREEN_MENU_ITEMS + 1; 
    (uiState.getSelectedMenuItem() < ON_SCREEN_MENU_ITEMS) ? (i < ON_SCREEN_MENU_ITEMS) && (i < uiState.getMenuItemsCounter()) : i <= uiState.getSelectedMenuItem(); 
    i++
  ) {
    oled.print((i == uiState.getSelectedMenuItem()) ?"[*]" : "[ ]");
    if (i == 0) {
      oled.printf(" %-17s\n", "<- back");
      continue;
    }
    oled.printf(" %-17s\n", networks[i-1]);
  }
  oled.printf("%-21s","");
}

void drawMenu(char title[]) {
  oled.println(title);
  oled.println();
  char ** menu = uiState.getMenu();
  for(
    uint8_t i = (uiState.getSelectedMenuItem() < ON_SCREEN_MENU_ITEMS) ? 0 : uiState.getSelectedMenuItem() - ON_SCREEN_MENU_ITEMS + 1; 
    (uiState.getSelectedMenuItem() < ON_SCREEN_MENU_ITEMS) ? (i < ON_SCREEN_MENU_ITEMS) && (i < uiState.getMenuItemsCounter()) : i <= uiState.getSelectedMenuItem(); 
    i++
  ) {
    oled.print((i == uiState.getSelectedMenuItem()) ?"[*]" : "[ ]");
    oled.printf(" %-17s\n", (menu[i]));
  }
  oled.printf("%-21s","");
}

void drawMenuScreen() {
  drawHeader();

  oled.setCursor(0, 19);
  oled.setTextColor(GREEN, BLACK);
  oled.setTextSize(1);

  drawMenu("[ main menu ]");
}

void drawMainScreen(Data meteodata) {
  drawHeader();
  oled.setTextWrap(false);
  oled.setTextSize(2);

  /** print temperature */
  oled.drawBitmap(2, 19, temp_bmp, 20, 20, BLUE);
  oled.setCursor(26, 22);
  oled.printf("%-2.1f C    ", meteodata.temperature);

  /** print humidity*/
  oled.drawBitmap(0, 40, hum_bmp, 20, 20, BLUE);
  oled.setCursor(26, 42);
  oled.printf("%-d %%    ", meteodata.humidity);
  
  /** print radiation details*/
  double microsiverts = double(cpm.sum() * (60 / cpm.capacity()) / CPS_TO_MECROSIVERTS_K);
  oled.setTextSize(2);
  if (microsiverts <= 0.30) {
    oled.setTextColor(GREEN, BLACK);
  } else if (microsiverts <= 0.52){
    oled.setTextColor(YELLOW, BLACK);
  } else {
    oled.setTextColor(RED, BLACK);
  }

  oled.drawBitmap(0, 60, rad_bmp, 24, 24, YELLOW);
  oled.setCursor(26, 62);
  oled.printf("%-2.2fuSv    ", microsiverts);

  /** print preassure*/
  oled.setTextColor(WHITE, BLACK);
  oled.drawBitmap(2, 84, pres_bmp, 20, 20, MAGENTA);
  oled.setCursor(26, 84);
  oled.printf("%-u mmHg    ", meteodata.preassure);

  /** print CO2 ppm*/
  int co2 = meteodata.co2;
  if (co2 < 1000) {
    oled.setTextColor(GREEN, BLACK);
  } else if (co2 < 2000) {
    oled.setTextColor(YELLOW, BLACK);
  } else {
    oled.setTextColor(RED, BLACK);
  }

  oled.drawBitmap(2, 104, co2_bmp, 20, 20, RED);
  oled.setCursor(26, 104);
  oled.printf("%-d ppm    ", co2);

  oled.setTextSize(1);
  oled.setTextColor(WHITE, BLACK);
  oled.setTextWrap(true);
  /*oled.printf(" %02d:%02d:%02d \n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  oled.println(&timeinfo, "  %a, %d.%m.%Y" );*/
}

void onUiStateChanged() {
    oled.fillRect(0, 18, 128, 128, BLACK);
    EEPROM.commit(); // TODO fix it
}

/**
 * FreeRTOS task
 * 
 * screen updater
*/
void ui(void * parameters) {
  Data meteoData;
  uint8_t shit = 0;
  meteodataQueueHandler = xQueueCreate(1, sizeof meteoData);
  buttonClickQueueHandler = xQueueCreate(1, 1);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  oled.fillScreen(BLACK);
  oled.setCursor(0, 0);

  uiState.setOnStateChangeListener(onUiStateChanged);
  
  for(;;) {
    if (uxQueueMessagesWaiting(meteodataQueueHandler) != 0) {
      if (xQueueReceive(meteodataQueueHandler, &meteoData, 0) == pdPASS) {
        if (DEBUG) {
          Serial.printf("received: \n co2: %d \n hum %d \n pres %u \n temp %2.1f \n============= \n", 
            meteoData.co2, meteoData.humidity, meteoData.preassure, meteoData.temperature
          );      
        }   
      }
    }
    if (uxQueueMessagesWaiting(buttonClickQueueHandler) != 0) {
      if (xQueueReceive(buttonClickQueueHandler, &shit, 0) == pdPASS) {
        uiState.onButtonPressed();
      }
    }
    
    
    switch (uiState.getCurrent()) {
      case UiState::MAIN:    
        drawMainScreen(meteoData);
        break;

      case UiState::MENU:
        drawMenuScreen();
        break;

      case UiState::WIFI: 
        drawWifiMenuScreen();
        break;

      case UiState::WIFI_ENABLE:
        drawEnableWifiScreen();
        break;

      case UiState::METEOSENSOR: 
        drawMeteoSensorScren(meteoData);
        break;
      
      case UiState::AUDIO:
        drawBtAudioScreen();
        break;

      case UiState::WIFI_SCAN:
        if (uxQueueMessagesWaiting(handler) != 0) {
          portBASE_TYPE xStatus;
          delete [] networks;
          xStatus = xQueueReceive(handler, &networks, 0);
          if (xStatus == pdPASS) {
            if (DEBUG) {
              Serial.println("received:");
              for (uint8_t i = 0; i < uiState.getMenuItemsCounter() - 1; i++) {
                Serial.println(networks[i]);
              }
            }   
          } /*else {
            menuItemsCounter = 1;
          }*/
        }
        drawWifiScannerScreen();
        break;

      case UiState::CO2:
        drawCo2Scren(meteoData.co2);
        break;

      case UiState::CO2_ENABLE:
        drawEnableCo2Scren();
        break;

      case UiState::GEIGER:
        drawGeigerScreen();
        break;

      case UiState::GEIGER_ENABLE:
        drawEnableGeigerScreen();
        break;

      default:
        break;
    }
    vTaskDelay(32 / portTICK_PERIOD_MS);
  }
}

void wifiSwitch(void * parameter) {
  if (*((bool*) parameter)) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);  
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    wifiEnabled = true;
    xTaskCreate(webServer, "webServer", 3000, NULL, 1, &webServerTaskHandler);
  } else {
    vTaskDelete(webServerTaskHandler);
    wifiEnabled = false;
    WiFi.mode(WIFI_OFF);
    
  }
  EEPROM.writeBool(EEPROM_ADDRESS_WIFI_ENABLED, wifiEnabled);
  EEPROM.commit();
  vTaskDelete(NULL);
}

void btAudioSwitch(void * parameter) {
  if (*((bool*) parameter)) {
    audio.begin();
    audio.I2S(PIN_I2S_BCLK, PIN_I2S_DOUT, PIN_I2S_LRC);
  } else {
    audio.end();
  }
  vTaskDelete(NULL);
}

/**
 * FreeRTOS task
 * 
 * web serser
*/
WebServer server(8080);

void webServer(void * parameter) {

  server.on("/test", []() {
    String response = "response";
    server.send(200, "text/plain", response);
  });

  server.begin();
  for(;;){ 
    server.handleClient();
  }
}

/**
 * FreeRTOS task
 * 
 * get data from bme280 and MH-Z19B sensors
*/
void getSensorsData(void * parameter) {
  ulong pollingCounter = 0;
  if (!bme.begin(BME280_ADDRESS_ALTERNATE)) {
      oled.println("BME280 error :(");
  } else {
      oled.println("BME280 ok");
  }

  static Data mData;

  mData.humidity = bme.readHumidity();
  mData.preassure = (int) (bme.readPressure() / 133.333);
  mData.temperature = bme.readTemperature();

  for(;;){ 
    if (wifiEnabled && (WiFi.status() == WL_CONNECTED)) {
      getLocalTime(&timeinfo);
    }
    #ifdef ENABLE_BME280_DATA_FILLTER
      uint8_t humidity = bme.readHumidity();
      if (abs(humidity - mData.humidity) < 10) {
        mData.humidity = humidity;
      }
      
      uint16_t pressure = (uint16_t) (bme.readPressure() / 133.333);
      if (abs(pressure - mData.preassure) < 100) {
        mData.preassure = pressure;
      }
      
      float temperature = bme.readTemperature();
      if (abs(temperature - mData.temperature) < 10) {
        mData.temperature = temperature;
      }
    #else
      mData.humidity = bme.readHumidity();
      mData.preassure = (uint16_t) (bme.readPressure() / 133.333);
      mData.temperature = bme.readTemperature();
    #endif
    
    mData.co2 = co2Enabled ? co2ppm() : 0; 
    pollingCounter++;
    // be careful with STORING_INTERVAL
    if ((pollingCounter * SENSOR_POLLING_INTERVAL) % CO2_STORING_INTERVAL == 0) {
      co2DataCycleArray.push(mData.co2);
    }
    if ((pollingCounter * SENSOR_POLLING_INTERVAL) % PRESSURE_STORING_INTERVAL == 0) {
      pressureCycleArray.push(mData.preassure);
    }
    if ((pollingCounter * SENSOR_POLLING_INTERVAL) % TEMPERATURE_STORING_INTERVAL == 0) {
      temperatureCycleArray.push(mData.temperature);
    }
    if ((pollingCounter * SENSOR_POLLING_INTERVAL) % HUMIDITY_STORING_INTERVAL == 0) {
      humidityCycleArray.push(mData.humidity);
    }
    xQueueSend(meteodataQueueHandler, &mData, 0);
    if (DEBUG) {
      Serial.printf("send: \n co2: %d \n hum %d \n pres %u \n temp %2.1f \n============= \n", 
        mData.co2, mData.humidity, mData.preassure, mData.temperature
      );
    }
    
    vTaskDelay(SENSOR_POLLING_INTERVAL / portTICK_PERIOD_MS);
  }
}

/**
 * FreeRTOS task
 * 
 * calc CPM and CPS values
*/
void cpsUpdater(void * parameter) {
  for(;;){ 
    noInterrupts();
    cpm.push(cps);
    cps = 0;
    interrupts();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

/**
 * FreeRTOS task
 * 
 * wifi scanner
*/
void scanWifi(void * parameter) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  int scanResult = WiFi.scanNetworks(false,true);
  char ** data = new char*[scanResult];
  for (int8_t networkCounter = 0; networkCounter < scanResult; networkCounter++) {
    int32_t rssi;
    uint8_t encryptionType;
    uint8_t* bssid;
    int32_t channel;
    String ssid;
    WiFi.getNetworkInfo(
      networkCounter,
      ssid,
      encryptionType,
      rssi,
      bssid,
      channel
    );
    if (DEBUG) {
      Serial.println("=====================");
      Serial.printf("%d %s\n", ssid.length(), ssid.c_str());
    }
    data[networkCounter] = new char[ssid.length()];
    strcpy(data[networkCounter], ssid.c_str());
  }
  xQueueSend(handler, &data, 0);
  if (wifiEnabled) {
    WiFi.begin(ssid, password);
  } else {
    WiFi.mode(WIFI_OFF);
  }
  
  uiState.setMenuItemCounter(scanResult + 1);
  vTaskDelete(NULL);
}


unsigned long lastButtonInterrupt = 0;

void IRAM_ATTR buttonOkIsr() {
  if (millis() - lastButtonInterrupt < BUTTONS_DEBOUNCE) return;
  uint8_t s = 0;
  xQueueSend(buttonClickQueueHandler, &s, 0);

  switch (uiState.getCurrent()) {
  case UiState::WIFI_ENABLE:
    switch(uiState.getSelectedMenuItem()) {

      case 1: 
        static bool stat_true = true;
        xTaskCreate(wifiSwitch, "wifiSwitch", 3000, (void*)&stat_true, 5, NULL);
        break;

      case 2: 
        static bool stat_false = false;
        xTaskCreate(wifiSwitch, "wifiSwitch", 3000, (void*)&stat_false, 5, NULL);
        break;
      default:
        break;
    }
      break;
    case UiState::CO2_ENABLE: {
      switch (uiState.getSelectedMenuItem()) {
        case 1:
          EEPROM.writeBool(EEPROM_ADDRESS_CO2_ENABLED, true);
          co2Enabled = true;
          break;
        case 2:
          EEPROM.writeBool(EEPROM_ADDRESS_CO2_ENABLED, false);
          co2Enabled = false;
          break;
        default:
          break;
      }
      break;
    }
    case UiState::GEIGER_ENABLE: {
      switch (uiState.getSelectedMenuItem()) {
          case 1:
            EEPROM.writeBool(EEPROM_ADDRESS_GEIGER_ENABLED, true);
            geigerEnabled = true;
            digitalWrite(PIN_ENABLE_GEIGER_COUNTER, HIGH);
            attachInterrupt(PIN_GEIGER_COUNTER, geigerCounterIsr, FALLING);
            break;
          case 2:
            EEPROM.writeBool(EEPROM_ADDRESS_GEIGER_ENABLED, false);
            geigerEnabled = false;
            digitalWrite(PIN_ENABLE_GEIGER_COUNTER, LOW);
            detachInterrupt(PIN_GEIGER_COUNTER);
            break;
          default:
            break;
      }
      break;
    }
    case UiState::WIFI: {
      switch(uiState.getSelectedMenuItem()){
        case 3:
          xTaskCreate(scanWifi, "scanWifi", 3000, NULL, 6, NULL);
          break;
        default: 
          break;
      }
      break;
    }
    case UiState::AUDIO: {
      switch (uiState.getSelectedMenuItem()) {
        case 1:
          static bool stat_true = true;
          xTaskCreate(btAudioSwitch, "btAudioSwitch", 10000, (void*)&stat_true, 5, NULL);
          break;
        case 2:
          static bool stat_false = false;
          xTaskCreate(btAudioSwitch, "btAudioSwitch", 10000, (void*)&stat_false, 5, NULL);
        break;
          default:
          break;
      }
      break;
    }
    default:
      break;
  }
  
  lastButtonInterrupt = millis();
}

void IRAM_ATTR buttonUpIsr() {
  enc.tick();
}

void IRAM_ATTR buttonDownIsr() {
  enc.tick();
}

unsigned long lastGeigerInterrupt = 0;

void IRAM_ATTR geigerCounterIsr() {
  if (micros() - lastGeigerInterrupt > GEIGER_DEBOUNCE_MICROSECONDS) {    
    if (DEBUG) {
      Serial.print(micros());
      Serial.println(" interrupt");
    }
    
    if (cps < ULONG_MAX / 60)	cps++;
    lastGeigerInterrupt = micros();
  }
}

int co2ppm() {
  static byte cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
  static byte response[9] = {0};

  Serial2.write(cmd, 9);
  Serial2.readBytes(response, 9);
  Serial2.flush();

  if (DEBUG) {
    for (int i = 0; i < 9; i++) {
      Serial.print(String(response[i], HEX));
      Serial.print("   ");
    }
    Serial.println();
  }
  return bytes2int(response[2], response[3]);
}