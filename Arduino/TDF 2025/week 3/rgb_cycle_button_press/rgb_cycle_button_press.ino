// Define the pins for the RGB LED and the button
const int redPin = 11;
const int greenPin = 10;
const int bluePin = 9;
const int buttonPin = 2;

// Variables to store state
int buttonCount = 0;
int lastButtonState = LOW; // Variable to track the last known button state

void setup() {
  // Set the LED pins as outputs
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  // Set the button pin as an input
  pinMode(buttonPin, INPUT);
  
}

void loop() {
  // Read the current state of the button
  int currentButtonState = digitalRead(buttonPin);

  // Check if the button was just pressed (transitioned from LOW to HIGH)
  if (currentButtonState == HIGH & lastButtonState == LOW) {
    
    // Increase counter on press
    buttonCount++;

    // Cycle the light based on the new count
    if (buttonCount == 1) {
      // Turn red on
      digitalWrite(redPin, HIGH);
      digitalWrite(greenPin, LOW);
      digitalWrite(bluePin, LOW);
    } else if (buttonCount == 2) {
      // Turn green on
      digitalWrite(redPin, LOW);
      digitalWrite(greenPin, HIGH);
      digitalWrite(bluePin, LOW);
    } else if (buttonCount == 3) {
      // Turn blue on
      digitalWrite(redPin, LOW);
      digitalWrite(greenPin, LOW);
      digitalWrite(bluePin, HIGH);
    } else {
      digitalWrite(redPin, LOW);
      digitalWrite(greenPin, LOW);
      digitalWrite(bluePin, LOW);
      buttonCount = 0; // Reset
    }
  }
  lastButtonState = currentButtonState;
}