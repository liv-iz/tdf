#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;

#define INTERVAL_MS_PRINT 1000

struct {
  struct {
    float x, y, z;
  } accelerometer, gyroscope;
  float temperature;
} normalized;

struct angle {
  float x, y, z;
};

angle position = {0, 0, 0};

unsigned long lastPrintMillis = 0;
unsigned long lastSampleMicros = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println("MPU6050 Pitch/Roll Test!");

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }

  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_1000_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_184_HZ);

  Serial.println("Sensor configured!");
  Serial.println("Calculating orientation...\n");
  delay(100);
}

angle calculateAccelerometerAngles() {
  angle acc;
  acc.x = atan(normalized.accelerometer.y / sqrt(sq(normalized.accelerometer.x) + sq(normalized.accelerometer.z)));
  acc.y = atan(-1 * normalized.accelerometer.x / sqrt(sq(normalized.accelerometer.y) + sq(normalized.accelerometer.z)));
  acc.z = 0;
  return acc;
}

angle calculateGyroscopeAngles(unsigned long sampleMicros) {
  angle gyro;
  gyro.x = normalized.gyroscope.x * sampleMicros / 1000000.0;
  gyro.y = normalized.gyroscope.y * sampleMicros / 1000000.0;
  gyro.z = normalized.gyroscope.z * sampleMicros / 1000000.0;
  return gyro;
}

void detectPitch(angle gyro, angle acc) {
  position.x = 0.98 * (position.x + degrees(gyro.x)) + 0.02 * degrees(acc.x);
}

void detectRoll(angle gyro, angle acc) {
  position.y = 0.98 * (position.y + degrees(gyro.y)) + 0.02 * degrees(acc.y);
}

float getPitch() {
  return position.x;
}

float getRoll() {
  return position.y;
}

bool readSample() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  unsigned long sampleMicros = (lastSampleMicros > 0) ? micros() - lastSampleMicros : 0;
  lastSampleMicros = micros();

  // Store normalized values (already in correct units from Adafruit library)
  normalized.accelerometer.x = a.acceleration.x;
  normalized.accelerometer.y = a.acceleration.y;
  normalized.accelerometer.z = a.acceleration.z;

  normalized.gyroscope.x = g.gyro.x;
  normalized.gyroscope.y = g.gyro.y;
  normalized.gyroscope.z = g.gyro.z;

  normalized.temperature = temp.temperature;

  // Calculate angles
  angle accelerometer = calculateAccelerometerAngles();
  angle gyroscope = calculateGyroscopeAngles(sampleMicros);

  // Update position using complementary filter
  detectPitch(gyroscope, accelerometer);
  detectRoll(gyroscope, accelerometer);

  return true;
}

void loop() {
  unsigned long currentMillis = millis();

  readSample();

  if (currentMillis - lastPrintMillis > INTERVAL_MS_PRINT) {
    Serial.print("TEMP:\t");
    Serial.print(normalized.temperature, 2);
    Serial.print("°C");
    Serial.println();

    Serial.print("Pitch:\t");
    Serial.print(getPitch(), 2);
    Serial.print("°");
    Serial.println();

    Serial.print("Roll:\t");
    Serial.print(getRoll(), 2);
    Serial.print("°");
    Serial.println();

    Serial.println();

    lastPrintMillis = currentMillis;
  }
}