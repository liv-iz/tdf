#include <ESP32Encoder.h>

// --- Pin Definitions (Corrected for Feather V2) ---
const int ENA_PIN = 27;
const int IN1_PIN = 26; // Feather A0
const int IN2_PIN = 25; // Feather A1
const int ENCODER_A_PIN = 14; // Feather A2
const int ENCODER_B_PIN = 15; // Feather A3

// --- Calibration Configuration ---
const float TOTAL_TRAVEL_MM = 60.0;
const int CALIBRATION_SPEED = 120; // Use a slow-medium speed (0-255) to be gentle on the mechanics
const int STALL_CHECK_DELAY_MS = 500; // How long to wait to confirm a stall (in milliseconds)

// --- Global Variables ---
ESP32Encoder encoder;
float ticks_per_mm = 0.0; // This is the value we want to find!

// Helper function to control the motor
void driveMotor(int direction, int speed) {
  if (direction == 1) { // Forward / Up
    digitalWrite(IN1_PIN, HIGH);
    digitalWrite(IN2_PIN, LOW);
  } else if (direction == -1) { // Backward / Down
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, HIGH);
  } else { // Stop
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);
  }
  analogWrite(ENA_PIN, speed);
}

// The main calibration routine
void runCalibration() {
  long lastEncoderValue = 0;
  long currentEncoderValue = 0;
  long totalTicks = 0;

  // --- Step 1: Drive to the first end (e.g., the top) until it stalls ---
  Serial.println("\nStep 1: Moving to the first physical end...");
  driveMotor(1, CALIBRATION_SPEED); // Start moving forward

  while (true) {
    lastEncoderValue = encoder.getCount();
    delay(STALL_CHECK_DELAY_MS);
    currentEncoderValue = encoder.getCount();

    // If the encoder value hasn't changed, we've hit the end and stalled
    if (currentEncoderValue == lastEncoderValue) {
      driveMotor(0, 0); // Stop the motor
      Serial.println("Stall detected. Reached the end.");
      break; // Exit the loop
    }
  }

  // --- Step 2: Reset the encoder at this known physical end ---
  Serial.println("Step 2: Resetting encoder count to zero at this end.");
  encoder.clearCount();
  delay(1000); // Small pause

  // --- Step 3: Drive to the other end (the bottom) until it stalls ---
  Serial.println("Step 3: Moving to the opposite end to measure travel...");
  driveMotor(-1, CALIBRATION_SPEED); // Start moving backward

  while (true) {
    lastEncoderValue = encoder.getCount();
    delay(STALL_CHECK_DELAY_MS);
    currentEncoderValue = encoder.getCount();
    
    // Print current count during travel so we can see it's working
    Serial.print("Encoder reading: ");
    Serial.println(abs(currentEncoderValue));

    if (currentEncoderValue == lastEncoderValue) {
      driveMotor(0, 0); // Stop the motor
      totalTicks = abs(currentEncoderValue); // Record the total ticks traveled
      Serial.print("Stall detected. Traveled a total of ");
      Serial.print(totalTicks);
      Serial.println(" ticks.");
      break; // Exit the loop
    }
  }
  delay(1000);

  // --- Step 4: Calculate and store the ticks_per_mm value ---
  if (totalTicks > 0) {
    ticks_per_mm = (float)totalTicks / TOTAL_TRAVEL_MM;
    Serial.println("\n--- CALIBRATION COMPLETE ---");
    Serial.print("Calculated Ticks per Millimeter: ");
    Serial.println(ticks_per_mm);
    Serial.println("--------------------------------");
  } else {
    Serial.println("\n--- CALIBRATION FAILED ---");
    Serial.println("Error: Zero ticks were measured. Check motor/encoder.");
    while(true); // Halt the program
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000); // Wait for Serial Monitor to connect
  Serial.println("--- Automated Calibration Routine ---");

  // Setup motor pins
  pinMode(ENA_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);

  // Setup encoder
  encoder.attachHalfQuad(ENCODER_A_PIN, ENCODER_B_PIN);
  encoder.clearCount();

  Serial.println("\nPlace the screw somewhere in the middle of its travel.");
  Serial.println("Calibration will begin in 5 seconds...");
  delay(5000);

  // Run the calibration process
  runCalibration();

  Serial.println("\nSystem is now calibrated. Moving to home position (0mm)...");
  // Drive back to the zero position (where we reset the counter)
  driveMotor(1, CALIBRATION_SPEED);
  while(encoder.getCount() < 0) {
    // Wait until it gets back to zero
  }
  driveMotor(0, 0); // Stop at home
  Serial.println("Homing complete. Ready for operation.");
}

void loop() {
  // This loop now demonstrates the calibration result.
  // It prints the current position in both raw ticks and calculated millimeters.
  long current_ticks = encoder.getCount();
  float current_mm = current_ticks / ticks_per_mm;

  Serial.print("Current Position: ");
  Serial.print(current_ticks);
  Serial.print(" ticks  |  ");
  Serial.print(current_mm, 2); // Print with 2 decimal places
  Serial.println(" mm");
  
  delay(500); // Update twice a second
}