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
#define REGISTRATION_CONFIRM_THRESHOLD 5  // Need 5 consistent reads

// Structure to store complete tag information
struct TagInfo {
  String epc;
  String pc;
  String crc;
  int rssi;
  int readCount;
  int antenna;
  unsigned long lastSeen;
  String friendlyName;  // User-assigned name
};

#define MAX_UNIQUE_TAGS 50
TagInfo tagDatabase[MAX_UNIQUE_TAGS];
int tagDatabaseCount = 0;

// ========================================
// Tag Name Management
// ========================================

void saveTagName(String epc, String name) {
  preferences.begin("rfid", false);  // Read-write
  
  // Use a simple key based on EPC
  String key = "name_" + epc.substring(0, 8);  // Use first 8 chars as key
  preferences.putString(key.c_str(), epc + "|" + name);
  
  preferences.end();
  
  Serial.println("Saved: " + epc + " -> " + name);
}

String getTagName(String epc) {
  preferences.begin("rfid", true);  // Read-only
  
  String key = "name_" + epc.substring(0, 8);
  String stored = preferences.getString(key.c_str(), "");
  
  preferences.end();
  
  // Parse stored format: "EPC|NAME"
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
  // Clear tag database (session only, names are kept)
  tagDatabaseCount = 0;
  tagCount = 0;
  lastTagEPC = "No tags detected yet";
  Serial.println("Tag database cleared!");
  server.send(200, "text/plain", "OK");
}

void handleRegisterStart() {
  // Start registration mode
  registrationMode = true;
  registrationEPC = "";
  registrationConfirmCount = 0;
  
  // Start scanning if not already
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
  
  // Save to flash
  saveTagName(epc, name);
  
  // Update database if tag is currently visible
  for (int i = 0; i < tagDatabaseCount; i++) {
    if (tagDatabase[i].epc == epc) {
      tagDatabase[i].friendlyName = name;
      Serial.println("Updated tag #" + String(i+1) + " with name: " + name);
      break;
    }
  }
  
  // Exit registration mode
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
  
  // Initialize Serial2 for R200 communication
  Serial2.begin(R200_BAUD, SERIAL_8N1, R200_RX_PIN, R200_TX_PIN);
  
  // Clear any pending data
  delay(500);
  while (Serial2.available()) {
    Serial2.read();
  }
  
  // Get module information
  Serial.println("Querying module info...");
  getHardwareVersion();
  delay(300);
  getSoftwareVersion();
  delay(300);
  
  // Set initial power
  Serial.println("Setting initial power...");
  setPower(currentPower);
  delay(300);
  
  // Verify power setting
  getPower();
  delay(300);
  
  Serial.println("✓ R200 initialization complete");
}

// ========================================
// Arduino Setup
// ========================================

void setup() {
  // Initialize serial monitor
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n");
  Serial.println("====================================");
  Serial.println("   ESP32 RFID POWDER TRACKING");
  Serial.println("====================================");
  
  // Setup WiFi
  setupWiFi();
  
  // Setup web server
  if (WiFi.status() == WL_CONNECTED) {
    setupWebServer();
  }
  
  // Setup R200 RFID reader
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
  // Handle web server requests
  server.handleClient();
  
  // Process R200 data
  while (Serial2.available()) {
    byte b = Serial2.read();
    byteCount++;
    
    // Print raw data for debugging (only when not in registration mode to reduce spam)
    if (!registrationMode) {
      if (b < 0x10) Serial.print("0");
      Serial.print(b, HEX);
      Serial.print(" ");
    }
    
    // Add to buffer safely
    if (bufferIndex < BUFFER_SIZE) {
      rxBuffer[bufferIndex++] = b;
    } else {
      // Buffer overflow - reset
      Serial.println("\n[Buffer overflow - resetting]");
      bufferIndex = 0;
    }
    
    // Check for complete packet (0xAA ... 0xDD)
    if (b == 0xDD && bufferIndex > 2 && rxBuffer[0] == 0xAA) {
      if (!registrationMode) {
        Serial.print("\n<-- Packet (len=");
        Serial.print(bufferIndex);
        Serial.println(")");
      }
      
      // Check if TAG packet: Byte[1]=0x02, Byte[2]=0x22
      if (bufferIndex >= 24 && rxBuffer[1] == 0x02 && rxBuffer[2] == 0x22) {
        
        // Extract RSSI
        byte rssi_raw = rxBuffer[5];
        int rssi_dbm = (int)rssi_raw - 256;
        
        // Extract PC (Protocol Control) - 2 bytes
        String pc = "";
        if (rxBuffer[6] < 0x10) pc += "0";
        pc += String(rxBuffer[6], HEX);
        if (rxBuffer[7] < 0x10) pc += "0";
        pc += String(rxBuffer[7], HEX);
        pc.toUpperCase();
        
        // Extract EPC (12 bytes, positions 8-19)
        String epc = "";
        for (int i = 8; i < 20; i++) {
          if (rxBuffer[i] < 0x10) epc += "0";
          epc += String(rxBuffer[i], HEX);
        }
        epc.toUpperCase();
        
        // Extract CRC (2 bytes)
        String crc = "";
        if (rxBuffer[20] < 0x10) crc += "0";
        crc += String(rxBuffer[20], HEX);
        if (rxBuffer[21] < 0x10) crc += "0";
        crc += String(rxBuffer[21], HEX);
        crc.toUpperCase();
        
        // REGISTRATION MODE HANDLING
        if (registrationMode) {
          if (registrationEPC == "") {
            // First detection
            registrationEPC = epc;
            registrationConfirmCount = 1;
            Serial.println("\n>>> Tag detected: " + epc);
            Serial.println(">>> Hold steady... (1/5)");
          } else if (registrationEPC == epc) {
            // Same tag detected again
            registrationConfirmCount++;
            Serial.println(">>> Confirmation: " + String(registrationConfirmCount) + "/5");
            
            if (registrationConfirmCount >= REGISTRATION_CONFIRM_THRESHOLD) {
              Serial.println(">>> TAG READY FOR NAMING <<<");
            }
          } else {
            // Different tag detected - reset
            registrationEPC = epc;
            registrationConfirmCount = 1;
            Serial.println("\n>>> Different tag detected: " + epc);
            Serial.println(">>> Restarting... (1/5)");
          }
        } else {
          // Normal mode - show tag detection
          Serial.println("=== TAG DETECTED ===");
          Serial.print("EPC: ");
          Serial.println(epc);
          Serial.print("RSSI: ");
          Serial.print(rssi_dbm);
          Serial.println(" dBm");
        }
        
        // NORMAL MODE HANDLING (always update database)
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
            tagDatabase[tagDatabaseCount].friendlyName = getTagName(epc);  // Load saved name
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
        
        tagCount = tagDatabaseCount;
        lastTagEPC = epc;
        lastTagTime = millis();
        
        if (!registrationMode) {
          Serial.println("========================");
        }
      }
      
      bufferIndex = 0;
    }
    
    // Newline every 20 bytes for readability (only when not registering)
    if (!registrationMode && byteCount % 20 == 0) {
      Serial.println();
    }
  }
  
  // WiFi keepalive check
  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck > 10000) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("\n[WiFi disconnected - reconnecting...]");
      WiFi.reconnect();
    }
    lastWiFiCheck = millis();
  }
}