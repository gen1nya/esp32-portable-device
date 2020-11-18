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

volatile unsigned long cps = 0L;
MovingAverage<volatile unsigned long, CYCLE_SIZE> cpm;

Data data;
struct tm timeinfo;

Adafruit_SSD1351 oled = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, CS_PIN, DC_PIN, RST_PIN);
Adafruit_BME280 bme;

UiState uiState = UiState::MAIN; 
UiState prevUiState = UiState::MAIN; // for ui state changes detecting
uint8_t menuItemsCounter = 0;
uint8_t selectedMenuItem = 0;
char** menuItems;

bool co2Enabled = false;

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

// ui methods
void drawHeader(void);
void drawMenu(char title[]);
void drawMainScreen(void);
void drawMenuScreen(void);
void drawWifiMenuScreen(void);
void drawEnableWifiScreen(void);
void drawCo2Scren(void);
void drawEnableCo2Scren(void);

void showMainScreen(void);
void showMenuScreen(void);
void showWifiScren(void);
void showEnableWifiScreen(void);
void showCo2Scren(void);
void showEnableCo2Scren(void);

void setup() {
  Serial.begin(UART_BAUNDRATE);
  Serial2.begin(Z19B_UART_BAUNDRATE);
  Serial.println("init");
  EEPROM.begin(EEPROM_INITIAL_SIZE);
  
  pinMode(PIN_GEIGER_COUNTER, INPUT_PULLUP);
  pinMode(PIN_BUTTON_OK, INPUT_PULLUP);
  pinMode(PIN_BUTTON_UP, INPUT_PULLUP);
  pinMode(PIN_BUTTON_DOWN, INPUT_PULLUP);
  attachInterrupt(PIN_GEIGER_COUNTER, geigerCounterIsr, FALLING);
  attachInterrupt(PIN_BUTTON_OK, buttonOkIsr, FALLING);
  attachInterrupt(PIN_BUTTON_UP, buttonUpIsr, FALLING);
  attachInterrupt(PIN_BUTTON_DOWN, buttonDownIsr, FALLING);

  oled.begin();
  oled.fillScreen(BLACK);
  oled.setTextColor(GREEN);
  oled.setCursor(0, 0);
  oled.println("oled ok");
  oled.print("connecting to wifi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  co2Enabled = EEPROM.readBool(EEPROM_ADDRESS_CO2_ENABLED);
  
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i < WIFI_CONNECTION_TIMEOUT) {
    delay(1000);
    i++;
    oled.print(".");
  }

  oled.printf("\nco2: %s\n", co2Enabled ? "enabled" : "disabled");

  if (WiFi.status() != WL_CONNECTED) {
    oled.setTextColor(RED);
    oled.println("\nconnection timeout");
    oled.setTextColor(GREEN);
  } else {
    oled.println("\nconnected");
    oled.println("push ntp client settings");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  }
  
  xTaskCreate(cpsUpdater, "cpsUpdater", 1000, NULL, 2, NULL );
  xTaskCreate(getSensorsData,  "getSensorsData", 2500, NULL, 4, NULL);
  xTaskCreate(ui, "ui", 3000, NULL, 5, NULL);

  if (WiFi.status() == WL_CONNECTED) {
    xTaskCreate(webServer, "webServer", 3000, NULL, 1, NULL);
  }

  /** writeStringToEEPROM("asdfghjk", 50);
  int length = EEPROM.read(50);
  Serial.printf("eeprom string length %d \n", length)
  Serial.print("read from eeprom: ");
  Serial.println(readStringFromEEPROM(50)); */
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
  selectedMenuItem = 1;
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

void loop() { }

void drawHeader() {
  oled.setTextSize(1);
  oled.setTextColor(WHITE, BLACK);
  oled.setCursor(0, 0);
  if (WiFi.status() != WL_CONNECTED) {
    oled.setTextColor(RED, BLACK);
    oled.print("[NA] ");
  } else {
    oled.setTextColor(WHITE, BLACK);
    oled.printf("[%d] ", WiFi.RSSI());
  }
  oled.setTextColor(BLUE, BLACK);
  oled.printf("%02d:%02d ", timeinfo.tm_hour, timeinfo.tm_min);
  
  oled.setTextColor(WHITE, BLACK);
  oled.printf("RAM:%dKB\n", (ESP.getFreeHeap() / 1024));

  if (WiFi.status() == WL_CONNECTED) oled.printf("ip: %s\n", WiFi.localIP().toString().c_str());

  oled.drawLine(0, 17, 128, 17, WHITE);
}

void drawEnableWifiScreen() {
  drawHeader();

  oled.setCursor(0, 19);
  oled.setTextColor(GREEN, BLACK);
  oled.setTextSize(1);

  oled.println("[ Enable WiFi? ]");
  oled.println();
  for(uint8_t i = 0; i < menuItemsCounter; i++) {
    if (i == selectedMenuItem) {
      oled.print("[*]");
    } else { 
      oled.print("[ ]");
    }
    oled.println(menuItems[i]);
  }
}

void drawWifiMenuScreen() {
  drawHeader();

  oled.setCursor(0, 19);
  oled.setTextColor(GREEN, BLACK);
  oled.setTextSize(1);

  oled.println("[ WiFi ]");
  oled.println();
  for(uint8_t i = 0; i < menuItemsCounter; i++) {
    if (i == selectedMenuItem) {
      oled.print("[*]");
    } else { 
      oled.print("[ ]");
    }
    oled.println(menuItems[i]);
  }
}

void drawCo2Scren() {
  drawHeader();
  
  oled.setCursor(0, 19);
  oled.setTextColor(GREEN, BLACK);
  oled.setTextSize(1);
  
  drawMenu("[ CO2 ]");
}

void drawEnableCo2Scren() {
  drawHeader();
  
  oled.setCursor(0, 19);
  oled.setTextColor(GREEN, BLACK);
  oled.setTextSize(1);
  
  drawMenu("[ enable CO2? ]");
}

void drawMenu(char title[]) {
  oled.println(title);
  oled.println();
  for(uint8_t i = 0; i < menuItemsCounter; i++) {
    if (i == selectedMenuItem) {
      oled.print("[*]");
    } else { 
      oled.print("[ ]");
    }
    oled.println(menuItems[i]);
  }
}

void drawMenuScreen() {
  drawHeader();

  oled.setCursor(0, 19);
  oled.setTextColor(GREEN, BLACK);
  oled.setTextSize(1);

  drawMenu("[ main menu ]");
}

void drawMainScreen() {
  drawHeader();

  oled.setTextSize(2);

  /** print temperature */
  oled.drawBitmap(2, 19, temp_bmp, 20, 20, BLUE);
  oled.setCursor(26, 22);
  oled.printf("%2.1f C ", data.temperature);

  /** print humidity*/
  oled.drawBitmap(0, 40, hum_bmp, 20, 20, BLUE);
  oled.setCursor(26, 42);
  oled.printf("%2d %% ", data.humidity);
  
  /** print radiation details*/
  double microsiverts = double(cpm.sum() * (60 / cpm.capacity()) / 151.0);
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
  oled.printf("%2.2fuSv", microsiverts);

  /** print preassure*/
  oled.setTextColor(WHITE, BLACK);
  oled.drawBitmap(2, 84, pres_bmp, 20, 20, MAGENTA);
  oled.setCursor(26, 84);
  oled.printf("%3u mmHg", data.preassure);

  /** print CO2 ppm*/
  int co2 = data.co2;
  if (co2 < 1000) {
    oled.setTextColor(GREEN, BLACK);
  } else if (co2 < 2000) {
    oled.setTextColor(YELLOW, BLACK);
  } else {
    oled.setTextColor(RED, BLACK);
  }

  oled.drawBitmap(2, 104, co2_bmp, 20, 20, RED);
  oled.setCursor(26, 104);
  oled.printf("%4d ppm", co2);

  oled.setTextSize(1);
  oled.setTextColor(WHITE, BLACK);
  
  /*oled.printf(" %02d:%02d:%02d \n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  oled.println(&timeinfo, "  %a, %d.%m.%Y" );*/
}

/**
 * FreeRTOS task
 * 
 * screen updater
*/
void ui(void * parameters) {
  vTaskDelay(2000 / portTICK_PERIOD_MS);
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
        drawMainScreen();
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

      case UiState::CO2:
        drawCo2Scren();
        break;

      case UiState::CO2_ENABLE:
        drawEnableCo2Scren();
        break;

      default:
        break;
    }
    vTaskDelay(32 / portTICK_PERIOD_MS);
  }
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
  oled.println("http started!");
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
  
  if (!bme.begin(BME280_ADDRESS_ALTERNATE)) {
      oled.println("BME280 error :(");
  } else {
      oled.println("BME280 ok");
  }

  for(;;){ 
    getLocalTime(&timeinfo);
    data.humidity = bme.readHumidity();
    data.preassure = (int) (bme.readPressure() / 133.333);
    data.temperature = bme.readTemperature();
    
    data.co2 = co2Enabled ? co2ppm() : 0; 
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

/**
 * FreeRTOS task
 * 
 * calc CPM and CPS values
*/
void cpsUpdater(void * parameter){
  for(;;){ 
    noInterrupts();
    cpm.push(cps);
    cps = 0;
    interrupts();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
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
        case 2: 
          break;
        case 3: 
          showCo2Scren();
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
        default:
          break;
      }
      break;
    }
    case UiState::WIFI_ENABLE: {
      showWifiScren();
      switch (selectedMenuItem) {
        case 1:
          break;
        case 2:
          // TODO add anything)
          break;
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
    Serial.print(micros());
    Serial.println(" interrupt");
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