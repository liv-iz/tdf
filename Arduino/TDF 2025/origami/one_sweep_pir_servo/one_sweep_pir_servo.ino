#include <Servo.h>

Servo myservo;      // create servo object to control servo

int pirPin = 2;     // PIR sensor pin
int pirState = LOW; // PIR sensor's state
int servoPin = 9;   // pin attached to  servo motor

void setup() {
  myservo.attach(servoPin);  // attaches to servo object
  pinMode(pirPin, INPUT);    // set PIR sensor pin as input
  myservo.write(0);          // start servo at 0 degrees
}

void loop() {
  // Read  state of  PIR sensor
  pirState = digitalRead(pirPin);

  // Check if motion is detected
  if (pirState == HIGH) {
    // Sweep  servo from 0 to 120 degrees
    for (int pos = 0; pos <= 120; pos += 1) { // goes from 0 degrees to 120 degrees
      myservo.write(pos);              // tell servo to go to position in variable 'pos'
      delay(20);                       // waits 20ms for servo to reach position
    }
   // Hold  position at 120 degrees for one second
    delay(1000);

    // Sweep  servo back from 120 to 0 degrees
    for (int pos = 120; pos >= 0; pos -= 1) { // goes from 120 degrees to 0 degrees
      myservo.write(pos);              // tell servo to go to position in variable 'pos'
      delay(20);                       // waits 20ms for servo to reach  position
    }

    //wait before checking for motion again
    delay(1000);
  }
}
