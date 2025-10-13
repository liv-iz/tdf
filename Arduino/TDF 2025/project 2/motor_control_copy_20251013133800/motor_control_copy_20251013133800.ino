/*
 * Arduino code to control a motor's speed based on a single 0-255 value
 * sent from a p5.js sketch over Web Serial.
 *
 * - Receives an integer value (0-255) ending with a newline character.
 * - A value of 0 stops the motor.
 * - Any other value sets the motor's forward speed.

 Based on: https://www.youtube.com/watch?v=yThUrgBkZ2o, https://github.com/loopstick/ArduinoTutorial, and debugged using Gemini code assistance
*/


// Include the H-bridge library
#include <L298N.h>

const unsigned int ENA = 10; 
const unsigned int IN1 = 8;
const unsigned int IN2 = 9;

// Create motor instance using your pins
L298N motor(ENA, IN1, IN2);

void setup() {
  // Start serial communication at the same baud rate as p5.js
  Serial.begin(115200);
  Serial.println("Arduino is ready. Waiting for p5.js connection...");
}

void loop() {
  // Check if there is any incoming serial data
  if (Serial.available() > 0) {
    // Read incoming string until a newline character is received
    String receivedString = Serial.readStringUntil('\n');

    // Convert string to an integer
    int motorSpeed = receivedString.toInt();

    // For debugging: print the value received from p5.js
    Serial.print("Received Speed: ");
    Serial.println(motorSpeed);

    // --- Motor Control Logic ---
    // If the speed is very low, stop the motor to prevent humming.
    if (motorSpeed < 10) {
      motor.stop();
    } else {
      // Otherwise, set the motor to move forward at the received speed.
      // The L298N library handles setting the direction pins.
      motor.setSpeed(motorSpeed);
      motor.forward();
    }
  }
}
