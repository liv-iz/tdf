// Define the pins for your colored LEDs
const int ldrPin = A0;
const int redLedPin = 2;    // For "pitch black"
const int yellowLedPin = 3; // For "ambient"
const int greenLedPin = 4;  // For "super bright"


// Use the Serial Monitor to find the best numbers.
// More light = lower LDR value.
const int brightThreshold = 250; // above this value, it's bright
const int darkThreshold = 150;   // below this value, it's pitch black

void setup() {
  // Set all LED pins as outputs
  pinMode(redLedPin, OUTPUT);
  pinMode(yellowLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  
  // Start serial communication so you can find your threshold values
  Serial.begin(9600); 
}

void loop() {
  // Read the value from the LDR sensor
  int ldrValue = analogRead(ldrPin);
  
  // Print the current light value to help you calibrate
  Serial.print("LDR Value: ");
  Serial.println(ldrValue); 
  
  // Logic to turn on the correct LED
  if (ldrValue > brightThreshold) { // Super bright conditions
    digitalWrite(greenLedPin, HIGH);
    digitalWrite(yellowLedPin, LOW);
    digitalWrite(redLedPin, LOW);
  } 
  else if (ldrValue < darkThreshold) { // Pitch black conditions
    digitalWrite(greenLedPin, LOW);
    digitalWrite(yellowLedPin, LOW);
    digitalWrite(redLedPin, HIGH);
  } 
  else { // Ambient light (in between bright and dark)
    digitalWrite(greenLedPin, LOW);
    digitalWrite(yellowLedPin, HIGH);
    digitalWrite(redLedPin, LOW);
  }
  
  delay(100); // A small delay for stability
}