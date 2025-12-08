/*
   ESP32 Buzzer Test (for Arduino Core 3.0.0 and newer)
   
   This sketch uses the NEW ledcAttach and ledcWriteTone functions
   to make a buzzer on GPIO 12 beep on and off.
*/

// --- Define Your Pin ---
#define BUZZER_PIN 12

// --- Buzzer Tone Setup ---
const int TONE_FREQUENCY = 700; // 1kHz tone
const int PWM_RESOLUTION = 8;    // 8-bit resolution (0-255)

void setup() {
  Serial.begin(115200);
  Serial.println("Buzzer Test starting");

  // 1. Attach the pin to the LEDC peripheral
  // This new function replaces both ledcSetup and ledcAttachPin.
  // It automatically finds and assigns a free PWM channel.
  // Syntax: ledcAttach(pin, frequency, resolution)
  ledcAttach(BUZZER_PIN, TONE_FREQUENCY, PWM_RESOLUTION);
}

void loop() {
  // Turn buzzer ON
  Serial.println("Beep ON");
  // The new ledcWriteTone function is perfect for buzzers.
  // It sets a 50% duty cycle at the frequency you already set.
  // Syntax: ledcWriteTone(pin)
  ledcWriteTone(BUZZER_PIN, TONE_FREQUENCY);
  delay(500); // Keep it on for 0.5 seconds

  // Turn buzzer OFF
  Serial.println("Beep OFF");
  // To turn it off, you can use ledcWrite with a duty cycle of 0.
  // Syntax: ledcWrite(pin, duty_cycle)
  ledcWrite(BUZZER_PIN, 0); 
  delay(1000); // Keep it off for 1 second
}