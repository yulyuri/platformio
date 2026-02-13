#ifndef CONFIG_H
#define CONFIG_H

// wifi
const char* WIFI_SSID = "amnet";
const char* WIFI_PASSWORD = "privateoxygen";

// R200 Serial Configuration - use pdf or github link  https://github.com/playfultechnology/arduino-rfid-R200
#define R200_RX_PIN 16
#define R200_TX_PIN 17
#define R200_BAUD 115200

// Web Server Configuration - use default
#define WEB_SERVER_PORT 80

// Default Settings
#define DEFAULT_POWER 3000  // 30.00 dBm (need to check this properly)
#define BUFFER_SIZE 256

#endif