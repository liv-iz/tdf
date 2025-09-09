// --- PIN DEFINITIONS ---
// NOTE: LEDs have been moved to PWM pins (marked with ~ on the Arduino)
const int ldrPin = A0;
const int redLedPin = 9;    // PWM pin for "pitch black"
const int yellowLedPin = 10; // PWM pin for "ambient"
const int greenLedPin = 11;  // PWM pin for "super bright"

// --- CALIBRATION THRESHOLDS ---
// You MUST change these values to match your room's lighting!
// More light = lower LDR value.
const int brightThreshold = 150; // Below this value, it's "super bright"
const int darkThreshold = 550;   // Above this value, it's "pitch black"

void setup() {
  // Set all LED pins as outputs
  pinMode(redLedPin, OUTPUT);
  pinMode(yellowLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  
  // Start serial communication to help you find your threshold values
  Serial.begin(9600); 
}

void loop() {
  // Read the value from the LDR sensor
  int ldrValue = analogRead(ldrPin);
  
  // Print the current light value to help you calibrate
  Serial.print("LDR Value: ");
  Serial.println(ldrValue); 
  
  // Logic to turn on the correct LED with variable brightness
  if (ldrValue < brightThreshold) { // Super bright conditions
    // The GREEN LED gets brighter as the light increases (ldrValue drops)
    int brightness = map(ldrValue, brightThreshold, 0, 0, 255);
    analogWrite(redLedPin, brightness);
    analogWrite(yellowLedPin, 0); // Turn other LEDs off
    analogWrite(greenLedPin, 0);
  } 
  else if (ldrValue > darkThreshold) { // Pitch black conditions
    // The RED LED gets brighter as it gets darker (ldrValue increases)
    int brightness = map(ldrValue, darkThreshold, 1023, 0, 255);
    analogWrite(greenLedPin, brightness);
    analogWrite(redLedPin, 0); // Turn other LEDs off
    analogWrite(yellowLedPin, 0);
  } 
  else { // Ambient light (in between bright and dark)
    // The YELLOW LED fades in and out within the ambient range
    int brightness = map(ldrValue, brightThreshold, darkThreshold, 0, 255);
    analogWrite(yellowLedPin, brightness);
    analogWrite(greenLedPin, 0); // Turn other LEDs off
    analogWrite(redLedPin, 0);
  }
  
  delay(10); // A smaller delay for smoother transitions
}