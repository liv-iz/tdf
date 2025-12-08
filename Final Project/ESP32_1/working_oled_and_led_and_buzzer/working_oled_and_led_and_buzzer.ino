/*
  Combined Code for:
  - 1.3" I2C OLED (SDA: 22, SCL: 20, RST: 14)
  - LED (Pin 27)
  - Buzzer (Pin 19) - Using Arduino Core 3.0.0+ Syntax
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- PIN DEFINITIONS ---
#define OLED_SDA 22
#define OLED_SCL 20
#define OLED_RST 14
#define LED_PIN  27
// I kept Pin 19 from your original file, but you can change this to 12 if you moved the wire.
#define BUZZER_PIN 12 

// --- OLED CONFIGURATION ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// --- BUZZER SETTINGS ---
const int TONE_FREQUENCY = 1000; // 1kHz tone
const int PWM_RESOLUTION = 8;    // 8-bit resolution (0-255)

void setup() {
  Serial.begin(115200);

  // 1. Configure LED Pin
  pinMode(LED_PIN, OUTPUT);

  // 2. Configure Buzzer (New Core 3.0.0 Syntax)
  // This replaces ledcSetup and ledcAttachPin
  // Syntax: ledcAttach(pin, frequency, resolution)
  if(!ledcAttach(BUZZER_PIN, TONE_FREQUENCY, PWM_RESOLUTION)) {
     Serial.println("Buzzer attach failed!");
  }

  // 3. Configure and start I2C for the OLED
  // We specify the pins here because they differ from the default ESP32 I2C pins
  Wire.begin(OLED_SDA, OLED_SCL);

  // 4. Initialize the OLED Display
  // Using 0x3D as requested in your original code [cite: 6]
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Loop forever
  }

  // --- Show Initial Display ---
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Hello!");
  display.display();
  delay(2000);
}

void loop() {
  // --- ON STATE ---
  Serial.println("Components ON");

  // 1. LED On
  digitalWrite(LED_PIN, HIGH);

  // 2. Buzzer On (New Core 3.0.0 Syntax)
  // Sets 50% duty cycle automatically
  ledcWriteTone(BUZZER_PIN, TONE_FREQUENCY);

  // 3. Screen Update
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("LOOK AT ME");
  display.display();
  
  delay(1000); // Keep ON for 1 second

  // --- OFF STATE ---
  Serial.println("Components OFF");

  // 1. LED Off
  digitalWrite(LED_PIN, LOW);

  // 2. Buzzer Off
  // To turn off, write duty cycle 0
  ledcWrite(BUZZER_PIN, 0);

  // 3. Screen Update
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("I AM WORKING");
  display.display();

  delay(1000); // Keep OFF for 1 second
}