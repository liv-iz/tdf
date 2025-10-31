#include <ESP32Servo.h>
Servo s;
void setup() {
  s.setPeriodHertz(50);
  s.attach(13, 600, 2400);
}
void moveTo(float a0, float a1, unsigned long dur) {
  unsigned long t0 = millis();
  while (millis() - t0 < dur) {
    float t = float(millis() - t0) / dur; // 0→1
    float eased = 0.5 - 0.5 * cos(PI * t); // smooth ease-in/out
    float a = a0 + (a1 - a0) * eased;
    s.write(a);
    delay(20); // keep 50 Hz pulse
  }
}
void loop() {
  moveTo(0, 180, 1000);   //  1800s sweep up
  moveTo(180, 0, 1000);   // 1.5 s sweep down
  delay(500);
}