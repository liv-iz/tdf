/*
 * Ultrasonic sensor + servo in-class example code for TDF 9/16/25 
 *
 * modified from Ultrasonic Simple example from Erick Simoes' Ultrasonic library
 * and Arduino Servo library Sweep example
 *
 */

// ultrasonic library include and sensor pin definitions =======================
#include <Ultrasonic.h>

Ultrasonic ultrasonic(12, 13);  // define the ultrasonic sensor pins (Trigger, Echo)
int distance;
//==========================================================

// copied from servo sweep example =======================
#include <Servo.h>  

Servo myservo;  // create servo object to control a servo

int pos = 0;    // variable to store the servo position
//==========================================================

void setup() {
  Serial.begin(9600);
  myservo.attach(9);  // attaches the servo on pin 9 to the servo object
}

void loop() {
  distance = ultrasonic.read();
  
  Serial.print("Distance in CM: ");
  Serial.print(distance);
  //delay(1000);
    delay(100);

  // in class, we found the minimum and maximum distances that the sensor was reading
  // min distance = 1
  // max distance = 90

  pos = map(distance, 0, 90, 0, 180);  // remap the distance value to position
                                       // map(value to map, from min, from max, to min, to max)

      Serial.print("\t servo position: ");  // print helpful text
      Serial.println(pos);                  // print position 

    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15 ms for the servo to reach the position
  
  // want the servo motion to be more smooth?
  //  check out the Smoothing example: https://docs.arduino.cc/built-in-examples/analog/Smoothing/
  
}//END LOOP=====================================================================================