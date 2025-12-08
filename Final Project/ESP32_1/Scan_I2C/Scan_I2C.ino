/*
   I2C Scanner for ESP32
   This sketch will scan the I2C bus and report any devices it finds.
*/

#include <Wire.h>

// --- Define Your I2C Pins ---
#define OLED_SDA 22
#define OLED_SCL 20

void setup() {
  // IMPORTANT: Set your Serial Monitor to 115200 baud
  Serial.begin(115200); 

  // Wait for serial to connect
  while (!Serial); 
  Serial.println("\nI2C Scanner");

  // Start I2C on the correct pins
  Wire.begin(OLED_SDA, OLED_SCL);
}

void loop() {
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 127; address++) {
    // Wire.endTransmission() returns
    // 0: success
    // 1: data too long to fit in transmit buffer
    // 2: received NACK on transmit of address
    // 3: received NACK on transmit of data
    // 4: other error
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("Scan complete\n");

  delay(5000); // Wait 5 seconds and scan again
}