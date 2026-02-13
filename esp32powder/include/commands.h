#ifndef COMMANDS_H
#define COMMANDS_H

#include <Arduino.h>

// Calc checksum for R200 protocol
byte calculateChecksum(byte* data, int startPos, int endPos) {
  byte sum = 0;
  for (int i = startPos; i <= endPos; i++) {
    sum += data[i];
  }
  return sum;
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



// Set RF transmit power
// powerValue format: dBm * 100 (e.g., 3000 = 30.00 dBm)
void setPower(int powerValue) {
  byte highByte = (powerValue >> 8) & 0xFF;
  byte lowByte = powerValue & 0xFF;
  
  byte cmd[] = {0xAA, 0x00, 0xB6, 0x00, 0x02, highByte, lowByte, 0x00, 0xDD};
  cmd[7] = calculateChecksum(cmd, 1, 6);
  sendR200Command(cmd, sizeof(cmd));
  
  Serial.print("Power set to ");
  Serial.print(powerValue / 100.0);
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

#endif