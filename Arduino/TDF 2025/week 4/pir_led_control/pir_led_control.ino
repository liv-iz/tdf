//  Code and set up from: https://www.youtube.com/watch?v=3gj_68ywod4

const int pirPin = 9;
const int ledPin = 8;

// // Variable to store the current state of the PIR sensor
int motion;

void setup() {
  // LED pin output
  pinMode(ledPin, OUTPUT);

  //PIR sensor pin as input
  pinMode(pirPin, INPUT);

  Serial.begin(9600);
  Serial.println("PIR Motion Sensor Test");
}

void loop() {
  // Read the state of the PIR sensor
  motion = digitalRead(pirPin);

  // Check if motion is detected
  if (motion) {
    // If motion is detected, turn the LED on
    digitalWrite(ledPin, HIGH);
    Serial.println("Motion detected! - LED ON");
  } else {
    // If no motion is detected, turn the LED off
    digitalWrite(ledPin, LOW);
    Serial.println("No motion. - LED OFF");
  }

  // A small delay to prevent rapid, unnecessary readings
  delay(100);
}
