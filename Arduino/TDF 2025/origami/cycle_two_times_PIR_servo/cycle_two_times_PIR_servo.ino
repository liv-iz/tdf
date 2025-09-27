/*
  Controlling a servo with a PIR motion sensor
  Inspired by: https://arduinointro.com/articles/projects/maximizing-home-security-how-to-utilize-pir-sensors-with-servo-motors
  and used https://forum.arduino.cc/t/how-to-repeat-a-set-of-instructions-a-fixed-number-of-times/619549/2 & https://docs.arduino.cc/tutorials/generic/basic-servo-control/
*/

#include <Servo.h>

Servo myservo;      // create servo object to control a servo

int pirPin = 2;     // PIR sensor input pin
int pirState = LOW; // PIR sensor initial state
int servoPin = 9;   // Servo motor control pin

void setup() {
  myservo.attach(servoPin);  // attach pin to servo object
  pinMode(pirPin, INPUT);    // set PIR sensor pin as an input
  myservo.write(0);          // start servo at 0 degrees
}

void loop() {
  // Read state of sensor
  pirState = digitalRead(pirPin);

  // Check if motion is detected
  if (pirState == HIGH) {
    // Perform cycle twice
    for (int i = 0; i < 2; i++) {
      // Sweep the servo from 0 to 120 degrees
      for (int pos = 0; pos <= 120; pos += 1) { // goes from 0 degrees to 120 degrees in steps of 1 degree
        myservo.write(pos);              // tell servo to go to position in variable 'pos'
        delay(15);                       // waits 15ms for the servo to reach the position
      }

      // Hold the position at 120 degrees for one second
      delay(1000);

      // Sweep the servo back from 120 to 0 degrees
      for (int pos = 120; pos >= 0; pos -= 1) { // goes from 120 degrees to 0 degrees
        myservo.write(pos);              // tell servo to go to position in variable 'pos'
        delay(15);                       // waits 15ms for the servo to reach the position
      }
    }

    // Wait for 1 second before checking for motion again to help prevent multiple triggers
    delay(1000);
  }
}

