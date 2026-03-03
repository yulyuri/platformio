#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>

#include "config.h"
#include "commands.h"
#include "html.h"

WebServer server(WEB_SERVER_PORT);
Preferences preferences;

// Global state
bool isScanning = false;
int currentPower = DEFAULT_POWER;
String lastTagEPC = "No tags detected yet";
int tagCount = 0;
unsigned long lastTagTime = 0;

// Buffer
byte rxBuffer[BUFFER_SIZE];
int bufferIndex = 0;

// Registration mode
bool registrationMode = false;
String registrationEPC = "";
int registrationConfirmCount = 0;
#define REGISTRATION_CONFIRM_THRESHOLD 5

// Programming mode
bool programmingMode = false;
String programmingEPC = "";
int programmingConfirmCount = 0;
bool programmingWriteComplete = false;
int verifyAttempts = 0;

// Tag database
struct TagInfo {
  String epc;
  String pc;
  String crc;
  int rssi;
  int readCount;
  int antenna;
  unsigned long lastSeen;
  String friendlyName;
};

#define MAX_UNIQUE_TAGS 50
TagInfo tagDatabase[MAX_UNIQUE_TAGS];
int tagDatabaseCount = 0;

// Reading history
struct ReadingHistory {
  unsigned long timestamp;
  String epc;
  String bottleName;
  int rssi;
  String datetime;
};

#define MAX_HISTORY 1000
ReadingHistory readingHistory[MAX_HISTORY];
int historyCount = 0;
unsigned long systemStartTime = 0;

// Timestamp helper
String getTimestamp() {
  unsigned long elapsed = millis() - systemStartTime;
  unsigned long seconds = elapsed / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  seconds = seconds % 60;
  minutes = minutes % 60;
  
  char timestamp[20];
  sprintf(timestamp, "%02lu:%02lu:%02lu", hours, minutes, seconds);
  return String(timestamp);
}

// Tag name management
void saveTagName(String epc, String name) {
  preferences.begin("rfid", false);
  String key = "name_" + epc.substring(0, 8);
  preferences.putString(key.c_str(), epc + "|" + name);
  preferences.end();
  Serial.println("Saved: " + epc + " -> " + name);
}

String getTagName(String epc) {
  preferences.begin("rfid", true);
  String key = "name_" + epc.substring(0, 8);
  String stored = preferences.getString(key.c_str(), "");
  preferences.end();
  
  if (stored.length() > 0 && stored.startsWith(epc)) {
    int separator = stored.indexOf('|');
    if (separator > 0) {
      return stored.substring(separator + 1);
    }
  }
  return "";
}

// Process tag packet
void processTagPacket() {
  byte rssi_raw = rxBuffer[5];
  int rssi_dbm = (int)rssi_raw - 256;
  
  String pc = "";
  if (rxBuffer[6] < 0x10) pc += "0";
  pc += String(rxBuffer[6], HEX);
  if (rxBuffer[7] < 0x10) pc += "0";
  pc += String(rxBuffer[7], HEX);
  pc.toUpperCase();
  
  String epc = "";
  for (int i = 8; i < 20; i++) {
    if (rxBuffer[i] < 0x10) epc += "0";
    epc += String(rxBuffer[i], HEX);
  }
  epc.toUpperCase();
  
  // CRC (might not exist in 20-byte packets)
  String crc = "";
  if (bufferIndex >= 24) {
    if (rxBuffer[20] < 0x10) crc += "0";
    crc += String(rxBuffer[20], HEX);
    if (rxBuffer[21] < 0x10) crc += "0";
    crc += String(rxBuffer[21], HEX);
    crc.toUpperCase();
  } else {
    crc = "N/A";
  }
  
  // Registration mode
  if (registrationMode) {
    if (registrationEPC == "") {
      registrationEPC = epc;
      registrationConfirmCount = 1;
    } else if (registrationEPC == epc) {
      registrationConfirmCount++;
      if (registrationConfirmCount >= REGISTRATION_CONFIRM_THRESHOLD) {
        Serial.println(">>> READY FOR NAMING <<<");
      }
    } else {
      registrationEPC = epc;
      registrationConfirmCount = 1;
    }
    return;
  }
  
  // Programming mode
  if (programmingMode) {
    bool isBlank = true;
    for (int i = 8; i < 20; i++) {
      if (rxBuffer[i] != 0x00) {
        isBlank = false;
        break;
      }
    }
    
    if (isBlank && !programmingWriteComplete) {
      programmingConfirmCount++;
      if (programmingConfirmCount >= 3) {
        Serial.println(">>> WRITING <<<");
        stopMultiplePolling();
        delay(500);
        
        while (Serial2.available()) Serial2.read();
        bufferIndex = 0;
        
        writeEPC(programmingEPC);
        delay(1000);
        
        while (Serial2.available()) Serial2.read();
        bufferIndex = 0;
        
        programmingWriteComplete = true;
        programmingConfirmCount = 0;
        startMultiplePolling();
        delay(300);
      }
    } else if (!isBlank && programmingWriteComplete) {
      if (epc == programmingEPC) {
        Serial.println(">>> SUCCESS! <<<");
        programmingMode = false;
        programmingWriteComplete = false;
        programmingConfirmCount = 0;
        verifyAttempts = 0;
      } else {
        verifyAttempts++;
        if (verifyAttempts > 8) {
          Serial.println(">>> FAILED <<<");
          programmingMode = false;
          programmingWriteComplete = false;
          verifyAttempts = 0;
        }
      }
    }
    return;
  }
  
  // Normal mode - update database
  int tagIndex = -1;
  for (int i = 0; i < tagDatabaseCount; i++) {
    if (tagDatabase[i].epc == epc) {
      tagIndex = i;
      break;
    }
  }
  
  if (tagIndex == -1) {
    if (tagDatabaseCount < MAX_UNIQUE_TAGS) {
      tagDatabase[tagDatabaseCount].epc = epc;
      tagDatabase[tagDatabaseCount].pc = pc;
      tagDatabase[tagDatabaseCount].crc = crc;
      tagDatabase[tagDatabaseCount].rssi = rssi_dbm;
      tagDatabase[tagDatabaseCount].readCount = 1;
      tagDatabase[tagDatabaseCount].antenna = 1;
      tagDatabase[tagDatabaseCount].lastSeen = millis();
      tagDatabase[tagDatabaseCount].friendlyName = getTagName(epc);
      tagDatabaseCount++;
    }
  } else {
    tagDatabase[tagIndex].rssi = rssi_dbm;
    tagDatabase[tagIndex].readCount++;
    tagDatabase[tagIndex].lastSeen = millis();
  }
  
  if (historyCount < MAX_HISTORY) {
    readingHistory[historyCount].timestamp = millis();
    readingHistory[historyCount].epc = epc;
    readingHistory[historyCount].bottleName = (tagIndex >= 0) ? tagDatabase[tagIndex].friendlyName : "";
    readingHistory[historyCount].rssi = rssi_dbm;
    readingHistory[historyCount].datetime = getTimestamp();
    historyCount++;
  }
  
  tagCount = tagDatabaseCount;
  lastTagEPC = epc;
  lastTagTime = millis();
}

// Web handlers
void handleRoot() {
  server.send(200, "text/html", HTML_PAGE);
}

void handleStatus() {
  String json = "{";
  json += "\"scanning\":" + String(isScanning ? "true" : "false") + ",";
  json += "\"tagCount\":" + String(tagCount) + ",";
  json += "\"power\":" + String(currentPower) + ",";
  json += "\"lastTag\":\"" + lastTagEPC + "\",";
  json += "\"registrationMode\":" + String(registrationMode ? "true" : "false") + ",";
  json += "\"registrationEPC\":\"" + registrationEPC + "\",";
  json += "\"registrationProgress\":" + String(registrationConfirmCount) + ",";
  json += "\"historyCount\":" + String(historyCount) + ",";
  json += "\"programmingMode\":" + String(programmingMode ? "true" : "false") + ",";
  json += "\"programmingEPC\":\"" + programmingEPC + "\",";
  json += "\"programmingProgress\":" + String(programmingConfirmCount) + ",";
  json += "\"programmingComplete\":" + String(programmingWriteComplete ? "true" : "false") + ",";
  
  json += "\"tags\":[";
  for (int i = 0; i < tagDatabaseCount; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"no\":" + String(i + 1) + ",";
    json += "\"pc\":\"" + tagDatabase[i].pc + "\",";
    json += "\"epc\":\"" + tagDatabase[i].epc + "\",";
    json += "\"crc\":\"" + tagDatabase[i].crc + "\",";
    json += "\"rssi\":" + String(tagDatabase[i].rssi) + ",";
    json += "\"cnt\":" + String(tagDatabase[i].readCount) + ",";
    json += "\"ant\":" + String(tagDatabase[i].antenna) + ",";
    json += "\"name\":\"" + tagDatabase[i].friendlyName + "\"";
    json += "}";
  }
  json += "]";
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleHistory() {
  String json = "{\"count\":" + String(historyCount) + ",\"readings\":[";
  for (int i = 0; i < historyCount; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"time\":\"" + readingHistory[i].datetime + "\",";
    json += "\"epc\":\"" + readingHistory[i].epc + "\",";
    json += "\"name\":\"" + readingHistory[i].bottleName + "\",";
    json += "\"rssi\":" + String(readingHistory[i].rssi);
    json += "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleStart() {
  isScanning = true;
  startMultiplePolling();
  server.send(200, "text/plain", "OK");
}

void handleStop() {
  isScanning = false;
  stopMultiplePolling();
  delay(100);
  while (Serial2.available()) Serial2.read();
  bufferIndex = 0;
  server.send(200, "text/plain", "OK");
}

void handlePower() {
  if (server.hasArg("value")) {
    int power = server.arg("value").toInt();
    currentPower = power;
    setPower(power);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing power");
  }
}

void handleClear() {
  tagDatabaseCount = 0;
  tagCount = 0;
  historyCount = 0;
  systemStartTime = millis();
  lastTagEPC = "No tags detected yet";
  Serial.println("Cleared!");
  server.send(200, "text/plain", "OK");
}

void handleRegisterStart() {
  registrationMode = true;
  registrationEPC = "";
  registrationConfirmCount = 0;
  
  if (!isScanning) {
    isScanning = true;
    startMultiplePolling();
  }
  
  Serial.println(">>> REGISTRATION MODE <<<");
  server.send(200, "text/plain", "OK");
}

void handleRegisterCancel() {
  registrationMode = false;
  registrationEPC = "";
  registrationConfirmCount = 0;
  server.send(200, "text/plain", "OK");
}

void handleRegisterConfirm() {
  if (!server.hasArg("name") || !server.hasArg("epc")) {
    server.send(400, "text/plain", "Missing data");
    return;
  }
  
  String name = server.arg("name");
  String epc = server.arg("epc");
  
  saveTagName(epc, name);
  
  for (int i = 0; i < tagDatabaseCount; i++) {
    if (tagDatabase[i].epc == epc) {
      tagDatabase[i].friendlyName = name;
      break;
    }
  }
  
  registrationMode = false;
  registrationEPC = "";
  registrationConfirmCount = 0;
  
  Serial.println("✓ Registered: " + name);
  server.send(200, "text/plain", "OK");
}

void handleProgramStart() {
  if (!server.hasArg("epc")) {
    server.send(400, "text/plain", "Missing EPC");
    return;
  }
  
  programmingEPC = server.arg("epc");
  programmingEPC.toUpperCase();
  programmingMode = true;
  programmingConfirmCount = 0;
  programmingWriteComplete = false;
  verifyAttempts = 0;
  
  if (!isScanning) {
    isScanning = true;
    startMultiplePolling();
  }
  
  Serial.println(">>> PROGRAMMING MODE <<<");
  server.send(200, "text/plain", "OK");
}

void handleProgramCancel() {
  programmingMode = false;
  programmingEPC = "";
  programmingConfirmCount = 0;
  programmingWriteComplete = false;
  verifyAttempts = 0;
  server.send(200, "text/plain", "OK");
}

// ========================================
// WiFi Setup - HARDCODED HERE
// ========================================
void setupWiFi() {
  Serial.println("\n--- WiFi Setup ---");
  Serial.print("Connecting to: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  // DON'T use static IP - let DHCP assign it!
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✓ WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    
    // Setup mDNS - Access via http://esprfid.local
    if (MDNS.begin("esprfid")) {
      Serial.println("✓ mDNS started!");
      Serial.println("Access at: http://esprfid.local");
      MDNS.addService("http", "tcp", 80);
    } else {
      Serial.println("⚠ mDNS failed to start");
    }
  } else {
    Serial.println("✗ WiFi Connection Failed!");
  }
  // // Static IP configuration - 192.168.183.134 (WORK NETWORK)
  // IPAddress local_IP(192, 168, 183, 134);
  // IPAddress gateway(192, 168, 183, 1);
  // IPAddress subnet(255, 255, 255, 0);
  // IPAddress primaryDNS(8, 8, 8, 8);
  
  // if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
  //   Serial.println("⚠ Static IP configuration failed!");
  // } else {
  //   Serial.println("✓ Static IP configured: 192.168.183.134");
  // }
  
  // WiFi.mode(WIFI_STA);
  // WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // int attempts = 0;
  // while (WiFi.status() != WL_CONNECTED && attempts < 40) {
  //   delay(500);
  //   Serial.print(".");
  //   attempts++;
  // }
  // Serial.println();
  
  // if (WiFi.status() == WL_CONNECTED) {
  //   Serial.println("✓ WiFi Connected!");
  //   Serial.print("IP Address: ");
  //   Serial.println(WiFi.localIP());
  //   Serial.print("Signal Strength: ");
  //   Serial.print(WiFi.RSSI());
  //   Serial.println(" dBm");
  // } else {
  //   Serial.println("✗ WiFi Connection Failed!");
  //   Serial.println("Please check SSID and password in config.h");
  // }
}

void setupWebServer() {
  Serial.println("\n--- Web Server ---");
  
  server.on("/", handleRoot);
  server.on("/api/status", handleStatus);
  server.on("/api/history", handleHistory);
  server.on("/api/start", handleStart);
  server.on("/api/stop", handleStop);
  server.on("/api/power", handlePower);
  server.on("/api/clear", handleClear);
  server.on("/api/register/start", handleRegisterStart);
  server.on("/api/register/cancel", handleRegisterCancel);
  server.on("/api/register/confirm", handleRegisterConfirm);
  server.on("/api/program/start", handleProgramStart);
  server.on("/api/program/cancel", handleProgramCancel);
  
  server.begin();
  Serial.println("✓ Server started on port " + String(WEB_SERVER_PORT));
  Serial.println("Access at: http://" + WiFi.localIP().toString());
}

void setupR200() {
  Serial.println("\n--- R200 Setup ---");
  
  Serial2.begin(R200_BAUD, SERIAL_8N1, R200_RX_PIN, R200_TX_PIN);
  delay(500);
  while (Serial2.available()) Serial2.read();
  
  getHardwareVersion();
  delay(300);
  getSoftwareVersion();
  delay(300);
  setPower(currentPower);
  delay(300);
  
  Serial.println("✓ R200 ready");
}

void setup() {
  Serial.begin(115200);
  systemStartTime = millis();
  delay(2000);
  
  Serial.println("\n\n");
  Serial.println("====================================");
  Serial.println("   ESP32 RFID POWDER TRACKING");
  Serial.println("====================================");
  
  setupWiFi();
  
  if (WiFi.status() == WL_CONNECTED) {
    setupWebServer();
  }
  
  setupR200();
  
  Serial.println("\n====================================");
  Serial.println("✓ System Ready!");
  Serial.println("====================================");
  Serial.println("Open browser to: http://" + WiFi.localIP().toString());
  Serial.println("====================================\n");
}

void loop() {
  server.handleClient();
  
  // Process incoming data
  while (Serial2.available()) {
    byte b = Serial2.read();
    
    if (bufferIndex < BUFFER_SIZE) {
      rxBuffer[bufferIndex++] = b;
    } else {
      bufferIndex = 0;
      break;
    }
    
    if (b == 0xDD && bufferIndex > 2 && rxBuffer[0] == 0xAA) {
      
      // Silently skip error packets
      if (bufferIndex == 8 && rxBuffer[1] == 0x01 && rxBuffer[2] == 0xFF) {
        bufferIndex = 0;
        continue;
      }
      
      // Process tag packets (20 or 24 bytes)
      if ((bufferIndex == 20 || bufferIndex >= 24) && rxBuffer[1] == 0x02 && rxBuffer[2] == 0x22) {
        processTagPacket();
      }
      
      bufferIndex = 0;
    }
  }
  
  // Emergency drain
  if (Serial2.available() > 300) {
    while (Serial2.available()) Serial2.read();
    bufferIndex = 0;
  }
  
  // WiFi watchdog
  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck > 10000) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("\n[WiFi disconnected - reconnecting...]");
      WiFi.reconnect();
    }
    lastWiFiCheck = millis();
  }
}