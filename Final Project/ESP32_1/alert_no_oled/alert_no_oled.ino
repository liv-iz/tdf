/*
 * ALERT UNIT - BASIC (No OLED)
 * Debugging Version: LED + Buzzer Only
 * * HARDWARE CHANGE:
 * Move Buzzer from Pin 12 -> Pin 25 (Pin 12 causes boot failures!)
 */

#include <esp_now.h>
#include <WiFi.h>

// ===== HARDWARE PINS =====
#define PIN_LED 27      // Red LED
#define BUZZER_PIN 12   // CHANGED from 12 to 25 to prevent boot loops!

// ===== MESSAGE STRUCTURE =====
// Must match Sensor Unit exactly
typedef struct {
  char eventType[20];
  float pitch;
  float roll;
  float accelMagnitude;
  unsigned long timestamp;
} AlertMessage;

AlertMessage incomingAlert;

// ===== STATE VARIABLES =====
volatile bool newAlertReceived = false; // Flag to trigger loop
unsigned long alertStartTime = 0;
bool alertActive = false;
#define ALERT_DURATION_MS 3000

// ===== ESP-NOW CALLBACK (Keep this minimal!) =====
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  // 1. Copy data
  memcpy(&incomingAlert, incomingData, sizeof(incomingAlert));
  // 2. Set flag
  newAlertReceived = true;
}

// Wrapper for Arduino Core 3.x
void OnDataRecvWrapper(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  OnDataRecv(info->src_addr, incomingData, len);
}

// ===== SETUP =====
void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  delay(2000); // Give time to open Serial Monitor
  Serial.println("\n--- ALERT UNIT DEBUG MODE ---");

  // Initialize Pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  
  // Test Hardware on Boot (1 short beep)
  digitalWrite(PIN_LED, HIGH);
  tone(BUZZER_PIN, 1000, 200);
  delay(200);
  digitalWrite(PIN_LED, LOW);
  
  // Initialize WiFi
  WiFi.mode(WIFI_STA);
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register Callback
  esp_now_register_recv_cb(OnDataRecvWrapper);
  
  Serial.println("System Ready. Waiting for alerts...");
}

// ===== MAIN LOOP =====
void loop() {
  // 1. Check if flag was set by callback
  if (newAlertReceived) {
    newAlertReceived = false; // Reset flag
    
    Serial.println("\n! ALERT RECEIVED !");
    Serial.print("Type: ");
    Serial.println(incomingAlert.eventType);
    
    // Start Alert Logic
    alertActive = true;
    alertStartTime = millis();
    digitalWrite(PIN_LED, HIGH);
    tone(BUZZER_PIN, 2000); // Start Sound
  }

  // 2. Handle Alert Duration
  if (alertActive) {
    // Stop alert after 3 seconds
    if (millis() - alertStartTime > ALERT_DURATION_MS) {
      digitalWrite(PIN_LED, LOW);
      noTone(BUZZER_PIN);
      alertActive = false;
      Serial.println("Alert Ended.");
    }
    
    // Optional: Blink LED while active
    if ((millis() / 200) % 2 == 0) {
      digitalWrite(PIN_LED, HIGH);
    } else {
      digitalWrite(PIN_LED, LOW);
    }
  }

  // Small delay to prevent watchdog crashes
  delay(10);
}