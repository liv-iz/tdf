#include <Servo.h>

const int pirPin = 9;
const int servoPin = 8;

Servo myservo;

int motion;

void setup() {
  myservo.attach(servoPin);

  pinMode(pirPin, INPUT);

  Serial.begin(9600);
  Serial.println("PIR Test Servo");

  // Servo at the 0 position
  myservo.write(0);
}

void loop() {
  // Read the state of the PIR sensor
  motion = digitalRead(pirPin);

  if (motion == HIGH) {
    // If motion detected, move the servo to 90
    myservo.write(90);
    Serial.println("Motion detected! - Servo moved to 90 degrees");
    // Wait for a second to hold the position
    delay(10);
  } else {
    // If no motion is detected, move the servo back to the 0-degree position
    myservo.write(0);
    Serial.println("No motion. - Servo at 0 degrees");
  }

  // A small delay to prevent rapid, unnecessary readings
  delay(10);
}
