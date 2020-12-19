#ifndef _CONFIG_H
#define _CONFIG_H

#define UART_BAUNDRATE 9600
#define Z19B_UART_BAUNDRATE 9600
#define GPS_UART_BAUDRATE 9600

#define DEBUG 0

#define PIN_SCLK 18
#define PIN_MOSI 23
#define PIN_MISO 19
#define PIN_DC   2
#define PIN_CS   15
#define RST_PIN  4
#define PIN_SDCARD_CS 5
#define PIN_GEIGER_COUNTER 14
#define PIN_ENABLE_GEIGER_COUNTER 13

#define PIN_BUTTON_OK 35
#define PIN_BUTTON_UP 34
#define PIN_BUTTON_DOWN 32

#define PIN_CLK 34
#define PIN_DT 32

#define PIN_GPS_RX 33
#define PIN_GPS_TX 12

#define PIN_I2S_DOUT 27
#define PIN_I2S_BCLK 26
#define PIN_I2S_LRC 25

#define BUTTONS_DEBOUNCE 200L
#define GEIGER_DEBOUNCE_MICROSECONDS 500L

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 128

// Color definitions
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	RED             0xF800
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF

#define ON_SCREEN_MENU_ITEMS 11

#define CHART_HEIGHT 56

#define EEPROM_INITIAL_SIZE 500

#define EEPROM_ADDRESS_WIFI_ENABLED 20
#define EEPROM_ADDRESS_CO2_ENABLED 21
#define EEPROM_ADDRESS_GEIGER_ENABLED 22

#define MAX_SSID_LENGTH 16
#define MAX_PASS_LENGTH 16

#define EEPROM_ARRDESS_WIFI_SSID 128
#define EEPROM_ARRDESS_WIFI_PASS EEPROM_ARRDESS_WIFI_SSID + MAX_SSID_LENGTH + 1

#define GEIGER_CYCLE_SIZE 60

#define CO2_CYCLE_SIZE 128
#define CO2_STORING_INTERVAL 300000 //5min, ~10h on 128 lines chart

#define PRESSURE_CYCLE_SIZE 128
#define PRESSURE_STORING_INTERVAL 300

#define TEMPERATURE_CYCLE_SIZE 128
#define TEMPERATURE_STORING_INTERVAL 300

#define HUMIDITY_CYCLE_SIZE 128
#define HUMIDITY_STORING_INTERVAL 300

#define CO2_YELLOW_ALERT 1000
#define CO2_RED_ALERT 2000

#define SENSOR_POLLING_INTERVAL 100

/** ignore incorrect data (dT > 10, dP > 100, dH > 10)*/
#define ENABLE_BME280_DATA_FILLTER 

const double CPS_TO_MECROSIVERTS_K = 151.0;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600 * 3;
const int   daylightOffset_sec = 3600;

const char* ssid     = "dd-wrt";
const char* password = "Elizabeth";
const char* bluetoothName = "ESP_speaker";

#define WIFI_CONNECTION_TIMEOUT 20

#endif