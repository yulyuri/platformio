#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// Include our header files
#include "config.h"
#include "commands.h"
#include "html.h"

// Create web server instance
WebServer server(WEB_SERVER_PORT);

// Preferences for persistent storage
Preferences preferences;

// Global state variables
bool isScanning = false;
int currentPower = DEFAULT_POWER;
String lastTagEPC = "No tags detected yet";
int tagCount = 0;
int byteCount = 0;
unsigned long lastTagTime = 0;

// Buffer for R200 data
byte rxBuffer[BUFFER_SIZE];
int bufferIndex = 0;

// Registration mode
bool registrationMode = false;
String registrationEPC = "";
int registrationConfirmCount = 0;
#define REGISTRATION_CONFIRM_THRESHOLD 5

// Structure to store complete tag information
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

// Reading history for CSV export - captures EVERY read
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

// Helper function to generate timestamp
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

// ========================================
// Tag Name Management
// ========================================

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

// ========================================
// Web Server Route Handlers
// ========================================

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
  
  // Add full tag table data
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
  // Return reading history as JSON
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
  startMultiplePolling();
  isScanning = true;
  server.send(200, "text/plain", "OK");
}

void handleStop() {
  stopMultiplePolling();
  isScanning = false;
  server.send(200, "text/plain", "OK");
}

void handlePower() {
  if (server.hasArg("value")) {
    int power = server.arg("value").toInt();
    currentPower = power;
    setPower(power);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing power value");
  }
}

void handleClear() {
  // Clear tag database AND history
  tagDatabaseCount = 0;
  tagCount = 0;
  historyCount = 0;
  systemStartTime = millis();
  lastTagEPC = "No tags detected yet";
  Serial.println("Tag database and history cleared!");
  server.send(200, "text/plain", "OK");
}

void handleRegisterStart() {
  registrationMode = true;
  registrationEPC = "";
  registrationConfirmCount = 0;
  
  if (!isScanning) {
    startMultiplePolling();
    isScanning = true;
  }
  
  Serial.println(">>> REGISTRATION MODE ACTIVATED <<<");
  server.send(200, "text/plain", "OK");
}

void handleRegisterCancel() {
  registrationMode = false;
  registrationEPC = "";
  registrationConfirmCount = 0;
  Serial.println("Registration cancelled");
  server.send(200, "text/plain", "OK");
}

void handleRegisterConfirm() {
  if (!server.hasArg("name") || !server.hasArg("epc")) {
    server.send(400, "text/plain", "Missing name or EPC");
    return;
  }
  
  String name = server.arg("name");
  String epc = server.arg("epc");
  
  saveTagName(epc, name);
  
  for (int i = 0; i < tagDatabaseCount; i++) {
    if (tagDatabase[i].epc == epc) {
      tagDatabase[i].friendlyName = name;
      Serial.println("Updated tag #" + String(i+1) + " with name: " + name);
      break;
    }
  }
  
  registrationMode = false;
  registrationEPC = "";
  registrationConfirmCount = 0;
  
  Serial.println("✓ Tag registered: " + epc + " as '" + name + "'");
  server.send(200, "text/plain", "OK");
}

// ========================================
// WiFi Setup
// ========================================

void setupWiFi() {
  Serial.println("\n--- WiFi Setup ---");
  Serial.print("Connecting to: ");
  Serial.println(WIFI_SSID);
  
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
  } else {
    Serial.println("✗ WiFi Connection Failed!");
    Serial.println("Please check SSID and password in config.h");
  }
}

// ========================================
// Web Server Setup
// ========================================

void setupWebServer() {
  Serial.println("\n--- Web Server Setup ---");
  
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
  
  server.begin();
  Serial.println("✓ Web server started on port " + String(WEB_SERVER_PORT));
  Serial.println("Access at: http://" + WiFi.localIP().toString());
}

// ========================================
// R200 Setup
// ========================================

void setupR200() {
  Serial.println("\n--- R200 RFID Reader Setup ---");
  
  Serial2.begin(R200_BAUD, SERIAL_8N1, R200_RX_PIN, R200_TX_PIN);
  
  delay(500);
  while (Serial2.available()) {
    Serial2.read();
  }
  
  Serial.println("Querying module info...");
  getHardwareVersion();
  delay(300);
  getSoftwareVersion();
  delay(300);
  
  Serial.println("Setting initial power...");
  setPower(currentPower);
  delay(300);
  
  getPower();
  delay(300);
  
  Serial.println("✓ R200 initialization complete");
}

// ========================================
// Arduino Setup
// ========================================

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

// ========================================
// Arduino Main Loop
// ========================================

void loop() {
  server.handleClient();
  
  while (Serial2.available()) {
    byte b = Serial2.read();
    byteCount++;
    
    if (!registrationMode) {
      if (b < 0x10) Serial.print("0");
      Serial.print(b, HEX);
      Serial.print(" ");
    }
    
    if (bufferIndex < BUFFER_SIZE) {
      rxBuffer[bufferIndex++] = b;
    } else {
      Serial.println("\n[Buffer overflow - resetting]");
      bufferIndex = 0;
    }
    
    if (b == 0xDD && bufferIndex > 2 && rxBuffer[0] == 0xAA) {
      if (!registrationMode) {
        Serial.print("\n<-- Packet (len=");
        Serial.print(bufferIndex);
        Serial.println(")");
      }
      
      if (bufferIndex >= 24 && rxBuffer[1] == 0x02 && rxBuffer[2] == 0x22) {
        
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
        
        String crc = "";
        if (rxBuffer[20] < 0x10) crc += "0";
        crc += String(rxBuffer[20], HEX);
        if (rxBuffer[21] < 0x10) crc += "0";
        crc += String(rxBuffer[21], HEX);
        crc.toUpperCase();
        
        // REGISTRATION MODE HANDLING
        if (registrationMode) {
          if (registrationEPC == "") {
            registrationEPC = epc;
            registrationConfirmCount = 1;
            Serial.println("\n>>> Tag detected: " + epc);
            Serial.println(">>> Hold steady... (1/5)");
          } else if (registrationEPC == epc) {
            registrationConfirmCount++;
            Serial.println(">>> Confirmation: " + String(registrationConfirmCount) + "/5");
            
            if (registrationConfirmCount >= REGISTRATION_CONFIRM_THRESHOLD) {
              Serial.println(">>> TAG READY FOR NAMING <<<");
            }
          } else {
            registrationEPC = epc;
            registrationConfirmCount = 1;
            Serial.println("\n>>> Different tag detected: " + epc);
            Serial.println(">>> Restarting... (1/5)");
          }
        } else {
          Serial.println("=== TAG DETECTED ===");
          Serial.print("EPC: ");
          Serial.println(epc);
          Serial.print("RSSI: ");
          Serial.print(rssi_dbm);
          Serial.println(" dBm");
        }
        
        // NORMAL MODE - Update database
        int tagIndex = -1;
        for (int i = 0; i < tagDatabaseCount; i++) {
          if (tagDatabase[i].epc == epc) {
            tagIndex = i;
            break;
          }
        }
        
        if (tagIndex == -1) {
          // NEW UNIQUE TAG
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
            
            if (!registrationMode) {
              Serial.print(">>> NEW UNIQUE TAG #");
              Serial.println(tagDatabaseCount);
            }
          }
        } else {
          // EXISTING TAG - Update
          tagDatabase[tagIndex].rssi = rssi_dbm;
          tagDatabase[tagIndex].readCount++;
          tagDatabase[tagIndex].lastSeen = millis();
          
          if (!registrationMode) {
            Serial.print("(Count: ");
            Serial.print(tagDatabase[tagIndex].readCount);
            Serial.println(")");
          }
        }
        
        // LOG EVERY SINGLE READ TO HISTORY
        if (historyCount < MAX_HISTORY) {
          readingHistory[historyCount].timestamp = millis();
          readingHistory[historyCount].epc = epc;
          readingHistory[historyCount].bottleName = (tagIndex >= 0) ? tagDatabase[tagIndex].friendlyName : "";
          readingHistory[historyCount].rssi = rssi_dbm;
          readingHistory[historyCount].datetime = getTimestamp();
          historyCount++;
        } else {
          if (!registrationMode) {
            Serial.println("[Warning: History buffer full - 5000 reads captured]");
          }
        }
        
        tagCount = tagDatabaseCount;
        lastTagEPC = epc;
        lastTagTime = millis();
        
        if (!registrationMode) {
          Serial.println("========================");
        }
      }
      
      bufferIndex = 0;
    }
    
    if (!registrationMode && byteCount % 20 == 0) {
      Serial.println();
    }
  }
  
  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck > 10000) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("\n[WiFi disconnected - reconnecting...]");
      WiFi.reconnect();
    }
    lastWiFiCheck = millis();
  }
}