 

void setup() {
  // Initialize the LED pin as an output
  pinMode(27, OUTPUT);
}

void loop() {
  digitalWrite(27, HIGH);  // Turn the LED on
  delay(500);                   // Wait for 0.5 second
  digitalWrite(27, LOW);   // Turn the LED off
  delay(500);                   // Wait for 0.5 second
}
