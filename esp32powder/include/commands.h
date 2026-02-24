#ifndef COMMANDS_H
#define COMMANDS_H

#include <Arduino.h>

// Calculate checksum for R200 commands - USE ADDITION NOT XOR!
byte calculateChecksum(byte* data, int len) {
  byte checksum = 0;
  for (int i = 1; i < len; i++) {  // Start from 1 (skip 0xAA)
    checksum += data[i];  // ADDITION!
  }
  return checksum;
}

// command to R200 via Serial2
void sendR200Command(byte* cmd, int len) {
  Serial.print("TX: ");
  for (int i = 0; i < len; i++) {
    if (cmd[i] < 0x10) Serial.print("0");
    Serial.print(cmd[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  Serial2.write(cmd, len);
  delay(50);
}
//The below commands are taken from the user commands note 6.4demo command
//{0xAA, 0x00, 0xB7, 0x00, 0x00, 0xB7, 0xDD,},            //22. Acquire transmitting power 
//{0xAA, 0x00, 0xB6, 0x00, 0x02, 0x07, 0xD0, 0x8F, 0xDD,}, //23. Set the transmitting power 


void setPower(int power) {
  byte cmd[9];
  cmd[0] = 0xAA;
  cmd[1] = 0x00;
  cmd[2] = 0xB6;
  cmd[3] = 0x00;
  cmd[4] = 0x02;
  cmd[5] = (power >> 8) & 0xFF;
  cmd[6] = power & 0xFF;
  cmd[7] = calculateChecksum(cmd, 7);  // Sum of bytes 1-6
  cmd[8] = 0xDD;
  
  Serial2.write(cmd, sizeof(cmd));
  Serial.print("Power set to ");
  Serial.print(power / 100.0);
  Serial.println(" dBm");
}

// Get current power setting
void getPower() {
  byte cmd[] = {0xAA, 0x00, 0xB7, 0x00, 0x00, 0xB7, 0xDD};
  sendR200Command(cmd, sizeof(cmd));
  Serial.println("Requesting power level...");
}
    //   {0xAA, 0x00, 0x22, 0x00, 0x00, 0x22, 0xDD,},             //3. Single polling instruction 
    //   {0xAA, 0x00, 0x27, 0x00, 0x03, 0x22, 0x27, 0x10, 0x83, 0xDD,}, //4. Multiple polling instructions 
// Start continuous multi-tag polling
void startMultiplePolling() {
  byte cmd[] = {0xAA, 0x00, 0x27, 0x00, 0x03, 0x22, 0x27, 0x10, 0x83, 0xDD};
  sendR200Command(cmd, sizeof(cmd));
  Serial.println(">>> Scanning STARTED <<<");
}

// Stop multi-tag polling
void stopMultiplePolling() {
  byte cmd[] = {0xAA, 0x00, 0x28, 0x00, 0x00, 0x28, 0xDD};
  sendR200Command(cmd, sizeof(cmd));
  Serial.println(">>> Scanning STOPPED <<<");
}

// Single tag poll (read once)
void singlePoll() {
  byte cmd[] = {0xAA, 0x00, 0x22, 0x00, 0x00, 0x22, 0xDD};
  sendR200Command(cmd, sizeof(cmd));
  Serial.println("Single poll triggered");
}


// {0xAA, 0x00, 0x03, 0x00, 0x01, 0x00, 0x04, 0xDD,},       //0. Hardware version 
//       {0xAA, 0x00, 0x03, 0x00, 0x01, 0x01, 0x05, 0xDD,},       //1. Software version 

// Get hardware version
void getHardwareVersion() {
  byte cmd[] = {0xAA, 0x00, 0x03, 0x00, 0x01, 0x00, 0x04, 0xDD};
  sendR200Command(cmd, sizeof(cmd));
  Serial.println("Requesting hardware version...");
}

// Get software version
void getSoftwareVersion() {
  byte cmd[] = {0xAA, 0x00, 0x03, 0x00, 0x01, 0x01, 0x05, 0xDD};
  sendR200Command(cmd, sizeof(cmd));
  Serial.println("Requesting software version...");
}



// Write EPC to tag
bool writeEPC(String epcHex) {
  if (epcHex.length() != 24) {
    Serial.println("Error: EPC must be exactly 24 hex characters (12 bytes)");
    return false;
  }
  
  byte epcBytes[12];
  for (int i = 0; i < 12; i++) {
    String byteStr = epcHex.substring(i * 2, i * 2 + 2);
    epcBytes[i] = strtol(byteStr.c_str(), NULL, 16);
  }
  
  byte cmd[28];
  cmd[0] = 0xAA;
  cmd[1] = 0x00;
  cmd[2] = 0x49;
  cmd[3] = 0x00;
  cmd[4] = 0x15;  // Length = 21 bytes
  
  // Access password
  cmd[5] = 0x00;
  cmd[6] = 0x00;
  cmd[7] = 0x00;
  cmd[8] = 0x00;
  
  // Memory bank: 0x01 = EPC
  cmd[9] = 0x01;
  
  // Starting address: Word 2
  cmd[10] = 0x00;
  cmd[11] = 0x00;
  cmd[12] = 0x02;
  
  // Word count: 6 words
  cmd[13] = 0x06;
  
  // EPC data
  for (int i = 0; i < 12; i++) {
    cmd[14 + i] = epcBytes[i];
  }
  
  // Calculate checksum - FIXED! Use ADDITION not XOR
  byte checksum = 0;
  for (int i = 1; i < 26; i++) {
    checksum += cmd[i];  // ADDITION!
  }
  cmd[26] = checksum;
  
  cmd[27] = 0xDD;
  
  // Send
  Serial.print("TX: ");
  for (int i = 0; i < 28; i++) {
    if (cmd[i] < 0x10) Serial.print("0");
    Serial.print(cmd[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  Serial2.write(cmd, 28);
  Serial.println("Writing EPC with SUM checksum: " + epcHex);
  
  return true;
}
#endif