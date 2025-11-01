// Include the ESP32Encoder library
#include <ESP32Encoder.h>

// --- Pin Definitions (Match your wiring) ---
const int ENA_PIN = 27;
const int IN1_PIN = 26;
const int IN2_PIN = 25;
const int ENCODER_A_PIN = 14;
const int ENCODER_B_PIN = 15;

// --- Control Variables ---
const long stop_count = 10000; // The target count to stop at

// Create an encoder object
ESP32Encoder encoder;

// Variables for the non-blocking timer
unsigned long previousMillis = 0;
const long interval = 1000; // Interval for printing (1000 ms = 1 second)

void setup() {
  // Start serial communication
  Serial.begin(115200);

  // Set motor control pins as outputs
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);

  // Configure the encoder
  encoder.attachHalfQuad(ENCODER_A_PIN, ENCODER_B_PIN);
  encoder.clearCount(); // Start the count at zero
  
  Serial.print("Starting motor. Will stop at count: ");
  Serial.println(stop_count);

  // Start the motor spinning forward at a medium speed
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  analogWrite(ENA_PIN, 150);
}

void loop() {
  // --- Primary Stop Logic (check on every loop iteration) ---
  // We use abs() so the motor stops at either 10000 or -10000
  if (abs(encoder.getCount()) >= stop_count) {
    
    // 1. Stop the motor
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);
    analogWrite(ENA_PIN, 0);

    // 2. Print a final message
    Serial.println("--------------------------------");
    Serial.println("Target count reached! Motor stopped.");
    Serial.print("Final count: ");
    Serial.println(encoder.getCount());
    Serial.println("--------------------------------");
    
    // 3. Halt the program indefinitely
    while(1) {
      delay(1000); 
    }
  }

  // --- Periodic Status Update (runs every second) ---
  // This part will stop running once the program is halted above.
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // Save the current time for the next comparison
    previousMillis = currentMillis;

    // Get the current count from the encoder and print it
    Serial.print("Current Count: ");
    Serial.println(encoder.getCount());
  }
}