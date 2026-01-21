#include <Arduino.h>

// Pin definitions
#define SW1_RESET     21   // Power latch
#define SW1_SENSE     13   // HIGH = not pressed, LOW = pressed
#define SW1_ALT_SENSE 14   // Like what it says, alt sensing circuit
#define LED1          1    // Power off warning LED
#define LED2          2    // Heartbeat LED
#define LED3          38   // Activity/status LED

// Timing constants
#define LATCH_DELAY      500    // 500ms to hold SW1/SW2 for latch to engage
#define LONG_PRESS_TIME  2000   // 2000ms to hold SW1/SW2 for it to shutdown
#define HEARTBEAT_TIME   3000   // Heartbeat on LED2 every 3000ms
#define DEBOUNCE_TIME    50     // 50ms debounce

// State variables
unsigned long buttonPressStart = 0;
bool buttonIsPressed = false;
bool shutdownInitiated = false;
unsigned long lastHeartbeat = 0;

void setup() {
  Serial.begin(115200);
  
  // Configure sensing input 
  pinMode(SW1_SENSE, INPUT);
  pinMode(SW1_ALT_SENSE, INPUT);
  
  // Configure LEDs
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  
  // All LEDs on to show first boot  
  digitalWrite(LED1, HIGH);
  digitalWrite(LED2, HIGH);
  digitalWrite(LED3, HIGH);
  
  Serial.println("\nBooting");
  Serial.println("Wait 500ms before latching");
  
  delay(LATCH_DELAY);
  
  // Latch
  pinMode(SW1_RESET, OUTPUT);
  digitalWrite(SW1_RESET, HIGH);  
  
  Serial.println("Latched!");
  
  // Startup LED sequence
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW);
  delay(100);
  
  // Blink LED3 3 times to confirm ready
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED3, HIGH);
    delay(100);
    digitalWrite(LED3, LOW);
    delay(100);
  }
  
  lastHeartbeat = millis(); //amount of time that have passed since program started 
}

void shutdown() {
  Serial.println("\n power off");
  shutdownInitiated = true;
  
  // Flash LED1 rapidly as warning
  for (int i = 0; i < 10; i++) {
    digitalWrite(LED1, HIGH);
    delay(50);
    digitalWrite(LED1, LOW);
    delay(50);
  }
  
  // All LEDs on before shutdown
  digitalWrite(LED1, HIGH);
  digitalWrite(LED2, HIGH);
  digitalWrite(LED3, HIGH);
  
  Serial.println("Releasing power latch in 1 s");
  delay(1000);
  
  Serial.println("GN");
  delay(100);  // Let serial finish
  
  // Release the latch - power will cut off
  digitalWrite(SW1_RESET, LOW);  // unlatch
  
  // If we're still here, button is still pressed
  // Blink LED1 to indicate waiting for button release
  while (true) {
    digitalWrite(LED1, !digitalRead(LED1));
    delay(200);
  }
}

void loop() {
  unsigned long currentTime = millis();
  
  // Read button state (LOW = pressed in your circuit)
  bool senseState = digitalRead(SW1_SENSE);
  bool altSenseState = digitalRead(SW1_ALT_SENSE);
  
  // Use primary sense or alt sense (whichever shows button pressed)
  bool currentlyPressed = (senseState == LOW) || (altSenseState == LOW);
  
  // Detect button press start
  if (currentlyPressed && !buttonIsPressed) {
    buttonPressStart = currentTime;
    buttonIsPressed = true;
    
    Serial.println("Button pressed");
    
    // LED3 flash ack
    digitalWrite(LED3, HIGH);
    delay(50);
    digitalWrite(LED3, LOW);
  }
  
  // Check for long press while button is held
  if (currentlyPressed && buttonIsPressed) {
    unsigned long pressDuration = currentTime - buttonPressStart;
    
    // 1 second mark
    if (pressDuration > 1000 && pressDuration < 1100) {
      Serial.println("Keep holding to offf");
      digitalWrite(LED1, HIGH);  // Warning LED on
    }
    
    // Long press threshold reached 
    if (pressDuration >= LONG_PRESS_TIME) {
      shutdown();  
    }
  }
  
  // Button released before threshold
  if (!currentlyPressed && buttonIsPressed) {
    buttonIsPressed = false;
    digitalWrite(LED1, LOW);  // Turn off warning LED
    
    unsigned long pressDuration = currentTime - buttonPressStart;
    Serial.print("Button released after");
    Serial.print(pressDuration);
    Serial.println("ms");
  }
  
  // Heartbeat - LED2 blinks every 3 seconds
  if (currentTime - lastHeartbeat >= HEARTBEAT_TIME) {
    lastHeartbeat = currentTime;
    
    
    digitalWrite(LED2, HIGH);
    Serial.println("Heartbeat");
    delay(100);
    digitalWrite(LED2, LOW);
  }
  
  
  delay(10);
}