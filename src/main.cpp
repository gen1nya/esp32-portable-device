#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "Adafruit_Sensor.h"
#include "Adafruit_BME280.h"
#include <libraries/CircularBuffer.h>
#include <libraries/MovingAverage.h>

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

//Data data;
struct tm timeinfo;

Adafruit_SSD1351 oled = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, CS_PIN, DC_PIN, RST_PIN);
Adafruit_BME280 bme;

UiState uiState = UiState::MAIN; 
UiState prevUiState = UiState::MAIN; // for ui state changes detecting
uint8_t menuItemsCounter = 0;
uint8_t selectedMenuItem = 0;
char** menuItems;

bool co2Enabled = false;
bool geigerEnabled = false;
bool wifiEnabled = false;

QueueHandle_t handler;
QueueHandle_t meteodataQueueHandler;
TaskHandle_t webServerTaskHandler;

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

void showMainScreen(void);
void showMenuScreen(void);
void showWifiScren(void);
void showEnableWifiScreen(void);
void showCo2Scren(void);
void showEnableCo2Scren(void);
void showScanWifiScreen(void);
void showGeigerScren(void);
void showEnableGeigerScreen(void);

void setup() {
  Serial.begin(UART_BAUNDRATE);
  Serial2.begin(Z19B_UART_BAUNDRATE);
  Serial.println("init");
  EEPROM.begin(EEPROM_INITIAL_SIZE);
  
  pinMode(PIN_GEIGER_COUNTER, INPUT_PULLUP);
  pinMode(PIN_BUTTON_OK, INPUT_PULLUP);
  pinMode(PIN_BUTTON_UP, INPUT_PULLUP);
  pinMode(PIN_BUTTON_DOWN, INPUT_PULLUP);
  pinMode(PIN_ENABLE_GEIGER_COUNTER, OUTPUT);
  attachInterrupt(PIN_BUTTON_OK, buttonOkIsr, FALLING);
  attachInterrupt(PIN_BUTTON_UP, buttonUpIsr, FALLING);
  attachInterrupt(PIN_BUTTON_DOWN, buttonDownIsr, FALLING);

  oled.begin();
  oled.setRotation(0);
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

void showMainScreen() {
  uiState = UiState::MAIN;
  //delete[] menuItems; // NPE
  selectedMenuItem = 0;
  menuItemsCounter = 0;
}

void showMenuScreen() {
  uiState = UiState::MENU;
  delete[] menuItems;
  menuItems = new char*[4] {" <- back"," Wifi", " Geiger", " Co2"};
  selectedMenuItem = 0;
  menuItemsCounter = 4;
}

void showWifiScren() {
  uiState = UiState::WIFI;
  delete[] menuItems;
  menuItems = new char*[4] {" <- back"," on/off", " Connect", " Scanner"};
  selectedMenuItem = 0;
  menuItemsCounter = 4;
}

void showEnableWifiScreen() {
  uiState = UiState::WIFI_ENABLE;
  delete[] menuItems;
  menuItems = new char*[3] {" <- back"," Enable", " Disable"};
  menuItemsCounter = 3;
  selectedMenuItem = wifiEnabled ? 2 : 1;
}

void showScanWifiScreen() {
  menuItemsCounter = 0;
  selectedMenuItem = 0;
  xTaskCreate(scanWifi, "scanWifi", 3000, NULL, 6, NULL);
  uiState = UiState::WIFI_SCAN;
}

void showCo2Scren() {
  uiState = UiState::CO2;
  delete[] menuItems;
  menuItems = new char*[2] {" <- back"," on/off"};
  selectedMenuItem = 0;
  menuItemsCounter = 2;
}

void showEnableCo2Scren() {
  uiState = UiState::CO2_ENABLE;
  delete[] menuItems;
  menuItems = new char*[3] {" <- back"," Enable", " Disable"};
  menuItemsCounter = 3;
  selectedMenuItem = co2Enabled ? 2 : 1;
}

void showGeigerScren() {
  uiState = UiState::GEIGER;
  delete[] menuItems;
  menuItems = new char*[2] {" <- back"," on/off"};
  menuItemsCounter = 2;
  selectedMenuItem = 0;
}

void showEnableGeigerScreen() {
  uiState = UiState::GEIGER_ENABLE;
  delete[] menuItems;
  menuItems = new char*[3] {" <- back"," Enable", " Disable"};
  menuItemsCounter = 3;
  selectedMenuItem = geigerEnabled ? 2 : 1;
}


void loop() {
 
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

void drawEnableWifiScreen() {
  drawHeader();

  oled.setCursor(0, 19);
  oled.setTextColor(GREEN, BLACK);
  oled.setTextSize(1);

  drawMenu("[ Enable WiFi? ]");
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
    oled.fillRect(co2measuringCounter, 127, 1, 
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
    oled.fillRect(cpsCount*2, 127, 2,
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
    uint8_t i = (selectedMenuItem < ON_SCREEN_MENU_ITEMS) ? 0 : selectedMenuItem - ON_SCREEN_MENU_ITEMS + 1; 
    (selectedMenuItem < ON_SCREEN_MENU_ITEMS) ? (i < ON_SCREEN_MENU_ITEMS) && (i < menuItemsCounter) : i <= selectedMenuItem; 
    i++
  ) {
    oled.print((i == selectedMenuItem) ?"[*]" : "[ ]");
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

  for(
    uint8_t i = (selectedMenuItem < ON_SCREEN_MENU_ITEMS) ? 0 : selectedMenuItem - ON_SCREEN_MENU_ITEMS + 1; 
    (selectedMenuItem < ON_SCREEN_MENU_ITEMS) ? (i < ON_SCREEN_MENU_ITEMS) && (i < menuItemsCounter) : i <= selectedMenuItem; 
    i++
  ) {
    oled.print((i == selectedMenuItem) ?"[*]" : "[ ]");
    oled.printf(" %-17s\n", (menuItems[i]));
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

/**
 * FreeRTOS task
 * 
 * screen updater
*/
void ui(void * parameters) {
  Data meteoData;
  meteodataQueueHandler = xQueueCreate(1, sizeof meteoData);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  oled.fillScreen(BLACK);
  oled.setCursor(0, 0);
  
  for(;;) {
    if (prevUiState != uiState) {
      oled.fillRect(0, 18, 128, 128, BLACK);
      prevUiState = uiState;
      EEPROM.commit(); // TODO fix it
    }
    
    switch (uiState) {
      case UiState::MAIN:
        
        if (uxQueueMessagesWaiting(meteodataQueueHandler) != 0) {
          portBASE_TYPE xStatus;
          xStatus = xQueueReceive(meteodataQueueHandler, &meteoData, 0);
          if (xStatus == pdPASS) {
            if (DEBUG) {
              Serial.printf("received: \n co2: %d \n hum %d \n pres %u \n temp %2.1f \n============= \n", 
                meteoData.co2, meteoData.humidity, meteoData.preassure, meteoData.temperature
              );      
            }   
          }
        }
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
      
      case UiState::WIFI_SCAN:
        if (uxQueueMessagesWaiting(handler) != 0) {
          portBASE_TYPE xStatus;
          delete [] networks;
          xStatus = xQueueReceive(handler, &networks, 0);
          if (xStatus == pdPASS) {
            if (DEBUG) {
              Serial.println("received:");
              for (uint8_t i = 0; i < menuItemsCounter - 1; i++) {
                Serial.println(networks[i]);
              }
            }   
          } else {
            menuItemsCounter = 1;
          }
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
  for(;;){ 
    if (wifiEnabled && (WiFi.status() == WL_CONNECTED)) {
      getLocalTime(&timeinfo);
    }
    mData.humidity = bme.readHumidity();
    mData.preassure = (int) (bme.readPressure() / 133.333);
    mData.temperature = bme.readTemperature();
    
    mData.co2 = co2Enabled ? co2ppm() : 0; 
    pollingCounter++;
    // be careful
    if ((pollingCounter * SENSOR_POLLING_INTERVAL) % CO2_STORING_INTERVAL == 0) {
      co2DataCycleArray.push(mData.co2);
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
  
  menuItemsCounter = scanResult + 1;
  vTaskDelete(NULL);
}


unsigned long lastButtonInterrupt = 0;

void IRAM_ATTR buttonOkIsr() {
  if (millis() - lastButtonInterrupt < BUTTONS_DEBOUNCE) return;
  prevUiState = uiState;
  switch (uiState) {
    case UiState::MENU: {
      switch (selectedMenuItem) {
        case 0:
          showMainScreen();
          break;
        case 1:
          showWifiScren();
          break;
        case 2: 
          showGeigerScren();
          break;
        case 3: 
          showCo2Scren();
          break;
        default:
          break;
      }
      break;
    }
    case UiState::MAIN: {
      showMenuScreen();
      break;
    }
    case UiState::WIFI: {
      switch (selectedMenuItem) {
        case 0:
          showMenuScreen();
          break;
        case 1: 
          showEnableWifiScreen();
          break;
        case 3:
          showScanWifiScreen();
        default:
          break;
      }
      break;
    }
    case UiState::WIFI_ENABLE: {
      switch (selectedMenuItem) {
        case 0:
          break;
        case 1:
          static bool stat_true = true;
          xTaskCreate(wifiSwitch, "wifiSwitch", 3000, (void*)&stat_true, 5, NULL);
          break;
        case 2:
          static bool stat_false = false;
          xTaskCreate(wifiSwitch, "wifiSwitch", 3000, (void*)&stat_false, 5, NULL);
        default:
          break;
      }
      showWifiScren();
      break;
    }
    case UiState::WIFI_SCAN: {
      switch (selectedMenuItem) {
        case 0: {
          showWifiScren();
          break;
        }
        default:
          break;
      }
      break;
    }
    case UiState::CO2: {
      switch (selectedMenuItem) {
      case 0:
        showMenuScreen();
        break;
      case 1: 
        showEnableCo2Scren();
        break;
      }
      break;
    }
    case UiState::CO2_ENABLE: {
      switch (selectedMenuItem) {
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
      showCo2Scren();
      break;
    }
    case UiState::GEIGER: {
      switch (selectedMenuItem) {
      case 0:
        showMenuScreen();
        break;
      case 1: 
        showEnableGeigerScreen();
        break;
      }
      break;
    }
    case UiState::GEIGER_ENABLE: {
      switch (selectedMenuItem) {
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
      showGeigerScren();
      break;
    }
  }
  lastButtonInterrupt = millis();
}

void IRAM_ATTR buttonUpIsr() {
  if (millis() - lastButtonInterrupt < BUTTONS_DEBOUNCE) return;
  if (uiState.hasMenu()) {
    if (selectedMenuItem < menuItemsCounter - 1) selectedMenuItem++;
  }
  lastButtonInterrupt = millis();
}

void IRAM_ATTR buttonDownIsr() {
  if (millis() - lastButtonInterrupt < BUTTONS_DEBOUNCE) return;
  if (selectedMenuItem > 0 ) selectedMenuItem--;
  lastButtonInterrupt = millis();  
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