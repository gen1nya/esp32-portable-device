#ifndef _CONFIG_H
#define _CONFIG_H

#define UART_BAUNDRATE 9600
#define Z19B_UART_BAUNDRATE 9600

#define DEBUG 0

#define SCLK_PIN 18
#define MOSI_PIN 23
#define DC_PIN   2
#define CS_PIN   15
#define RST_PIN  4
#define PIN_GEIGER_COUNTER 14
#define PIN_ENABLE_GEIGER_COUNTER 5

#define PIN_BUTTON_OK 35
#define PIN_BUTTON_UP 34
#define PIN_BUTTON_DOWN 32

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
#define CO2_STORING_INTERVAL 3000 //5min, ~10h on chart

#define SENSOR_POLLING_INTERVAL 100

const double CPS_TO_MECROSIVERTS_K = 151.0;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600 * 3;
const int   daylightOffset_sec = 3600;

const char* ssid     = "dd-wrt";
const char* password = "Elizabeth";

#define WIFI_CONNECTION_TIMEOUT 20

#endif