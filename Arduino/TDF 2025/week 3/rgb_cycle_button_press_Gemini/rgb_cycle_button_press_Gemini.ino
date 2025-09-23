/*
  RGB LED State Cycler

  Each time a button is pressed, an RGB LED cycles through
  different colors (Red, Green, Blue) and then turns off.

  The circuit:
  - RGB LED:
    - Red anode to Arduino Pin 11
    - Green anode to Arduino Pin 10
    - Blue anode to Arduino Pin 9
    - Common Cathode to GND (through appropriate resistors, e.g., 220 Ohm each)
  - Pushbutton:
    - One leg to Arduino Pin 2
    - Other leg to +5V
  - 10K Ohm pulldown resistor:
    - From Arduino Pin 2 to GND
*/

// Define the pins for the RGB LED and the button
const int redPin = 11;
const int greenPin = 10;
const int bluePin = 9;
const int buttonPin = 2;

// Variables to store the state
int colorState = 0;         // Tracks the current color (0=Off, 1=Red, 2=Green, 3=Blue)
int lastButtonState = LOW;  // Tracks the previous state of the button

void setup() {
  // Set the LED pins as outputs
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  // Set the button pin as an input
  pinMode(buttonPin, INPUT);
  
  // Start with the LED off
  setColor(0); 
}

void loop() {
  // Read the current state of the button
  int currentButtonState = digitalRead(buttonPin);

  // Check if the button state has changed from LOW to HIGH (i.e., it was just pressed)
  if (currentButtonState == HIGH && lastButtonState == LOW) {
    // Advance to the next state
    colorState++;

    // If we've gone past the last state (Blue), loop back to the first state (Off)
    if (colorState > 3) {
      colorState = 0;
    }
    
    // Set the LED to the new color based on the current state
    setColor(colorState);
    
    // A small delay to handle "debouncing" - mechanical bouncing of the button contacts
    delay(200); 
  }

  // Save the current button state for the next loop iteration
  lastButtonState = currentButtonState;
}

// Function to set the RGB LED color based on a state number
void setColor(int state) {
  switch (state) {
    case 0: // Off
      digitalWrite(redPin, LOW);
      digitalWrite(greenPin, LOW);
      digitalWrite(bluePin, LOW);
      break;
    case 1: // Red
      digitalWrite(redPin, HIGH);
      digitalWrite(greenPin, LOW);
      digitalWrite(bluePin, LOW);
      break;
    case 2: // Green
      digitalWrite(redPin, LOW);
      digitalWrite(greenPin, HIGH);
      digitalWrite(bluePin, LOW);
      break;
    case 3: // Blue
      digitalWrite(redPin, LOW);
      digitalWrite(greenPin, LOW);
      digitalWrite(bluePin, HIGH);
      break;
  }
}