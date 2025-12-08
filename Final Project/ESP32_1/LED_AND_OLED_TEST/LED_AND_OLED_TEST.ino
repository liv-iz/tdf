/*
  ESP32 Code for:
  - 1.3" I2C OLED (SDA: 21, SCL: 22, RST: 14)
  - LED (Pin 23)
  - Buzzer (Pin 19)
*/

// Libraries for the OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h> 
#include <Adafruit_SSD1306.h>

// --- Define Your Pins ---

// OLED I2C Pins
#define OLED_SDA 23
#define OLED_SCL 22
#define OLED_RST 14 // Your new RST pin

// Other Component Pins
#define LED_PIN 27

// --- OLED Display Setup ---
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL)
// This is the important change: We add OLED_RST to the constructor.
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// --- Buzzer PWM Setup (for ESP32) ---
// We use the ESP32's LEDC (LED Control) peripheral for the buzzer tone
const int BUZZER_CHANNEL = 0;     // Use PWM Channel 0
const int BUZZER_RESOLUTION = 8;  // 8-bit resolution (0-255)
const int TONE_FREQUENCY = 1000;  // 1kHz tone

// --- Setup Function ---
void setup() {
  Serial.begin(115200);

  // 1. Configure LED Pin
  pinMode(LED_PIN, OUTPUT);

  // 3. Configure and start I2C for the OLED
  // On ESP32, it's best to specify the SDA and SCL pins in Wire.begin()
  Wire.begin(OLED_SDA, OLED_SCL);

  // 4. Initialize the OLED Display
  // The '0x3C' is the common I2C address for 128x64 displays.
  // If this fails, try 0x3D.
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  // Show initial display content
  display.clearDisplay();
  display.setTextSize(2); // Draw 2x size text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Hello!");
  display.display();
  delay(2000); // Pause for 2 seconds
}

// --- Main Loop ---
void loop() {
  // --- ON STATE ---
  Serial.println("Components ON");

  // LED On
  digitalWrite(LED_PIN, HIGH);

  // Buzzer On (50% duty cycle)
  ledcWrite(BUZZER_CHANNEL, 128); // 128 is 50% of 255 (8-bit resolution)

  // Screen Update
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("LOOK AT ME");
  display.display();

  delay(1000); // Keep them on for 1 second

  // --- OFF STATE ---
  Serial.println("Components OFF");

  // LED Off
  digitalWrite(LED_PIN, LOW);


  // Screen Update
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("I AM WORKING");
  display.display();

  delay(1000); // Keep them off for 1 second
}