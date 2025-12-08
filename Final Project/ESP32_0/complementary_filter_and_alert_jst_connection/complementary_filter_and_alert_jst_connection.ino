/*
 * SENSOR UNIT - Cargo Stability Monitor
 * ESP32 + MPU6050 IMU
 * Detects dangerous tilts and sends ESP-NOW alerts to Alert Unit
 */

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>

Adafruit_MPU6050 mpu;

// ===== ALERT UNIT MAC ADDRESS =====
// IMPORTANT: Replace with your Alert Unit's MAC address
// Example: If MAC is 14:2B:2F:AE:BD:C4, use:
// uint8_t alertUnitAddress[] = {0x14, 0x2B, 0x2F, 0xAE, 0xBD, 0xC4};
uint8_t alertUnitAddress[] = {0x14, 0x2B, 0x2F, 0xAE, 0xBD, 0xC4};

// ===== DANGER THRESHOLDS =====
#define DANGER_TILT_ANGLE 25.0    // degrees - adjust based on testing
#define DANGER_ACCELERATION 15.0  // m/s^2 - for sudden jolts/drops
#define ALERT_COOLDOWN_MS 2000    // minimum time between alerts

// ===== DATA STRUCTURES =====
struct angle {
  float x, y, z;
};

struct {
  struct {
    float x, y, z;
  } accelerometer, gyroscope;
  float temperature;
} normalized;

angle position = {0, 0, 0};

// ESP-NOW message structure
typedef struct {
  char eventType[20];  // "TILT", "JOLT", "DROP"
  float pitch;
  float roll;
  float accelMagnitude;
  unsigned long timestamp;
} AlertMessage;

AlertMessage outgoingAlert;

// ===== STATE VARIABLES =====
unsigned long lastSampleMicros = 0;
unsigned long lastAlertMillis = 0;
bool systemReady = false;

// ===== ESP-NOW CALLBACK =====
// For ESP32 Arduino Core 3.x
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("✓ Alert sent successfully!");
  } else {
    Serial.println("✗ Alert send failed!");
  }
}

// Wrapper for new API (Arduino Core 3.x)
void OnDataSentWrapper(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  OnDataSent(nullptr, status);
}

// ===== SETUP =====
void setup() {
  Serial.begin(9600);
  delay(1000);
  
  Serial.println("\n========================================");
  Serial.println("SENSOR UNIT - Cargo Stability Monitor");
  Serial.println("========================================\n");

  // Initialize I2C for STEMMA QT connector on Feather ESP32 V2
  // SDA = GPIO 22, SCL = GPIO 20
  Wire.begin(22, 20);
  Serial.println("✓ I2C initialized (STEMMA QT)");

  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("ERROR: Failed to find MPU6050!");
    Serial.println("Check wiring and restart.");
    while (1) delay(10);
  }
  Serial.println("✓ MPU6050 initialized");

  // Configure IMU
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_1000_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_184_HZ);
  Serial.println("✓ IMU configured");

  // Initialize WiFi in STA mode (required for ESP-NOW)
  WiFi.mode(WIFI_STA);
  Serial.print("✓ MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: ESP-NOW init failed!");
    while (1) delay(10);
  }
  Serial.println("✓ ESP-NOW initialized");

  // Register send callback
  esp_now_register_send_cb(OnDataSentWrapper);

  // Register Alert Unit as peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, alertUnitAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("ERROR: Failed to add Alert Unit peer!");
    Serial.println("Check Alert Unit MAC address!");
    while (1) delay(10);
  }
  Serial.println("✓ Alert Unit paired");

  Serial.println("\n--- THRESHOLDS ---");
  Serial.print("Tilt Angle: >");
  Serial.print(DANGER_TILT_ANGLE);
  Serial.println("°");
  Serial.print("Acceleration: >");
  Serial.print(DANGER_ACCELERATION);
  Serial.println(" m/s²");
  
  Serial.println("\n========================================");
  Serial.println("SYSTEM READY - Monitoring cargo...");
  Serial.println("========================================\n");
  
  systemReady = true;
  delay(100);
}

// ===== ANGLE CALCULATIONS =====
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

void updateOrientation(angle gyro, angle acc) {
  // Complementary filter: 98% gyro + 2% accel
  position.x = 0.98 * (position.x + degrees(gyro.x)) + 0.02 * degrees(acc.x);
  position.y = 0.98 * (position.y + degrees(gyro.y)) + 0.02 * degrees(acc.y);
}

float getPitch() {
  return position.x;
}

float getRoll() {
  return position.y;
}

// ===== READ SENSOR DATA =====
void readSample() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  unsigned long sampleMicros = (lastSampleMicros > 0) ? micros() - lastSampleMicros : 0;
  lastSampleMicros = micros();

  // Store normalized values
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

  // Update position
  updateOrientation(gyroscope, accelerometer);
}

// ===== DANGER DETECTION =====
void checkForDanger() {
  float pitch = getPitch();
  float roll = getRoll();
  
  // Calculate total acceleration magnitude
  float accelMag = sqrt(sq(normalized.accelerometer.x) + 
                        sq(normalized.accelerometer.y) + 
                        sq(normalized.accelerometer.z));

  // Check cooldown period
  unsigned long currentMillis = millis();
  if (currentMillis - lastAlertMillis < ALERT_COOLDOWN_MS) {
    return; // Still in cooldown
  }

  bool dangerDetected = false;
  char eventType[20] = "";

  // Check for dangerous tilt
  if (abs(pitch) > DANGER_TILT_ANGLE || abs(roll) > DANGER_TILT_ANGLE) {
    dangerDetected = true;
    strcpy(eventType, "TILT");
    
    Serial.println("\n!!! DANGER: EXCESSIVE TILT !!!");
    Serial.print("Pitch: ");
    Serial.print(pitch, 1);
    Serial.print("° | Roll: ");
    Serial.print(roll, 1);
    Serial.println("°");
  }
  
  // Check for sudden jolt/drop
  else if (accelMag > DANGER_ACCELERATION) {
    dangerDetected = true;
    strcpy(eventType, "JOLT");
    
    Serial.println("\n!!! DANGER: SUDDEN JOLT/DROP !!!");
    Serial.print("Acceleration: ");
    Serial.print(accelMag, 1);
    Serial.println(" m/s²");
  }

  // Send alert if danger detected
  if (dangerDetected) {
    strcpy(outgoingAlert.eventType, eventType);
    outgoingAlert.pitch = pitch;
    outgoingAlert.roll = roll;
    outgoingAlert.accelMagnitude = accelMag;
    outgoingAlert.timestamp = currentMillis;

    esp_err_t result = esp_now_send(alertUnitAddress, 
                                     (uint8_t *) &outgoingAlert, 
                                     sizeof(outgoingAlert));

    if (result == ESP_OK) {
      Serial.println("→ Sending alert to driver...");
    } else {
      Serial.println("✗ Error sending alert!");
    }

    lastAlertMillis = currentMillis;
  }
}

// ===== MAIN LOOP =====
void loop() {
  if (!systemReady) return;

  readSample();
  checkForDanger();

  // Optional: Print status every 2 seconds
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 2000) {
    Serial.print("Status - Pitch: ");
    Serial.print(getPitch(), 1);
    Serial.print("° | Roll: ");
    Serial.print(getRoll(), 1);
    Serial.print("° | Temp: ");
    Serial.print(normalized.temperature, 1);
    Serial.println("°C");
    lastPrint = millis();
  }

  delay(50); // 20Hz sampling rate
}