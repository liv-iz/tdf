/*
 * ALERT UNIT - Driver Dashboard Alert System
 * ESP32 + Buzzer + LED + OLED Display
 * Receives ESP-NOW alerts from Sensor Unit
 */

#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===== HARDWARE PINS =====
#define BUZZER_PIN 25      // Piezo buzzer
#define LED_PIN 26         // Red LED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1      // Reset pin (or -1 if sharing Arduino reset)
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===== MESSAGE STRUCTURE (must match Sensor Unit) =====
typedef struct {
  char eventType[20];
  float pitch;
  float roll;
  float accelMagnitude;
  unsigned long timestamp;
} AlertMessage;

AlertMessage incomingAlert;

// ===== STATE VARIABLES =====
unsigned long alertStartTime = 0;
bool alertActive = false;
#define ALERT_DURATION_MS 3000  // How long to sound alarm

// ===== ESP-NOW RECEIVE CALLBACK =====
// For ESP32 Arduino Core 3.x
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingAlert, incomingData, sizeof(incomingAlert));
  
  Serial.println("\n========================================");
  Serial.println("!!! ALERT RECEIVED FROM CARGO !!!");
  Serial.println("========================================");
  Serial.print("Event Type: ");
  Serial.println(incomingAlert.eventType);
  Serial.print("Pitch: ");
  Serial.print(incomingAlert.pitch, 1);
  Serial.println("°");
  Serial.print("Roll: ");
  Serial.print(incomingAlert.roll, 1);
  Serial.println("°");
  Serial.print("Acceleration: ");
  Serial.print(incomingAlert.accelMagnitude, 1);
  Serial.println(" m/s²");
  Serial.println("========================================\n");

  // Trigger alert
  alertActive = true;
  alertStartTime = millis();
  activateAlerts();
}

// Wrapper for new API (Arduino Core 3.x)
void OnDataRecvWrapper(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  OnDataRecv(info->src_addr, incomingData, len);
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n========================================");
  Serial.println("ALERT UNIT - Driver Dashboard System");
  Serial.println("========================================\n");

  // Initialize hardware
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("ERROR: OLED not found!");
    while(1) delay(10);
  }
  Serial.println("✓ OLED initialized");

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("CARGO MONITOR");
  display.println("v1.0");
  display.println("");
  display.println("Waiting for");
  display.println("sensor unit...");
  display.display();

  // Initialize WiFi
  WiFi.mode(WIFI_STA);
  Serial.print("✓ MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("\nIMPORTANT: Use this MAC address");
  Serial.println("in your Sensor Unit code!\n");

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: ESP-NOW init failed!");
    while(1) delay(10);
  }
  Serial.println("✓ ESP-NOW initialized");

  // Register receive callback
  esp_now_register_recv_cb(OnDataRecvWrapper);

  Serial.println("\n========================================");
  Serial.println("SYSTEM READY - Listening for alerts...");
  Serial.println("========================================\n");
}

// ===== ACTIVATE ALL ALERTS =====
void activateAlerts() {
  // Turn on LED
  digitalWrite(LED_PIN, HIGH);
  
  // Sound buzzer (tone)
  tone(BUZZER_PIN, 2000); // 2kHz tone
  
  // Update OLED display
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  
  // Display event type
  display.setCursor(0, 0);
  display.println("! ALERT !");
  
  display.setTextSize(1);
  display.setCursor(0, 20);
  
  if (strcmp(incomingAlert.eventType, "TILT") == 0) {
    display.println("DANGEROUS TILT");
    display.println("");
    display.print("Pitch: ");
    display.print(incomingAlert.pitch, 0);
    display.println("deg");
    display.print("Roll:  ");
    display.print(incomingAlert.roll, 0);
    display.println("deg");
  } 
  else if (strcmp(incomingAlert.eventType, "JOLT") == 0) {
    display.println("SUDDEN JOLT/DROP");
    display.println("");
    display.print("Force: ");
    display.print(incomingAlert.accelMagnitude, 1);
    display.println("m/s2");
  }
  
  display.display();
}

// ===== DEACTIVATE ALERTS =====
void deactivateAlerts() {
  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);
  
  // Return to standby screen
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("CARGO MONITOR");
  display.println("");
  display.println("Status: OK");
  display.println("");
  display.println("Last alert:");
  display.print(incomingAlert.eventType);
  display.println(" detected");
  display.display();
  
  alertActive = false;
}

// ===== MAIN LOOP =====
void loop() {
  // Check if alert should be turned off
  if (alertActive && (millis() - alertStartTime > ALERT_DURATION_MS)) {
    deactivateAlerts();
    Serial.println("Alert cleared.\n");
  }
  
  // Optional: Flash LED during alert
  if (alertActive) {
    static unsigned long lastFlash = 0;
    if (millis() - lastFlash > 200) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      lastFlash = millis();
    }
  }
  
  delay(10);
}