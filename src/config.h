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

#define CYCLE_SIZE      60

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600 * 3;
const int   daylightOffset_sec = 3600;

const char* ssid     = "dd-wrt";
const char* password = "Elizabeth";

#define WIFI_CONNECTION_TIMEOUT 20
#endif