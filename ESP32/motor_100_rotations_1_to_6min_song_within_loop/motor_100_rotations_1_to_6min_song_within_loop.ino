// Include the ESP32Encoder library
#include <ESP32Encoder.h>

// --- PIN DEFINITIONS (Match your wiring) ---
const int ENA_PIN = 27;
const int IN1_PIN = 26;
const int IN2_PIN = 25;
const int ENCODER_A_PIN = 14;
const int ENCODER_B_PIN = 15;

// --- CONTROL VARIABLES (ADJUST THESE) ---
long max_count = 45000; // 45000 counts = 100 rotations
// song_duration is now a variable that can be changed, not a constant.
unsigned long song_duration = 180000; // Default duration (e.g., 180000ms = 3 minutes)

// --- MOTOR/CONTROL CHARACTERISTICS (TUNE THESE) ---
int MIN_PWM_START = 70;      // The minimum PWM to overcome static friction
float SPEED_DEADZONE = 10.0; // Speeds below this (counts/sec) are considered "stopped"

// --- PID GAINS (TUNE THESE) ---
float Kp = 0.2;              // Proportional Gain
float Ki = 0.2;              // Integral Gain
float Kd = 0.001;            // Derivative Gain
float integral_clamp = 400.0; // Integral "anti-windup" clamp

// The time interval (in milliseconds) for the control loop
unsigned long dt = 50; // 50ms interval

// --- GLOBAL VARIABLES (Internal use) ---
ESP32Encoder encoder;
long last_encoder_count = 0;
unsigned long last_time = 0;
float actual_speed = 0.0;   // Calculated speed in counts per second
float target_speed = 0.0;   // Will be calculated dynamically in loop()

// PID internal variables
float integral_error = 0.0; // Accumulator for the integral term
float last_error = 0.0;     // For derivative calculation

// --- SETUP ---
void setup() {
  Serial.begin(115200);

  // Configure motor driver pins
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);

  // Configure the encoder
  encoder.attachHalfQuad(ENCODER_A_PIN, ENCODER_B_PIN);
  encoder.clearCount();

  // Initialize time for the control loop
  last_time = millis();
  
  Serial.println("System Initialized. Starting motor control loop...");
}

// --- PLACEHOLDER FUNCTION ---
// In a real application, this function would get the song duration from
// an external source like a Bluetooth command, a web interface, or a sensor.
unsigned long getUpdatedSongDuration() {
  // For this example, it just returns the global variable.
  // You could add logic here, for instance:
  // if (Serial.available() > 0) {
  //   unsigned long new_duration = Serial.parseInt();
  //   if (new_duration > 0) {
  //     song_duration = new_duration * 1000; // convert seconds to ms
  //     Serial.print("New duration set: ");
  //     Serial.println(song_duration);
  //   }
  // }
  return song_duration;
}


// --- MAIN LOOP ---
void loop() {
  // --- 1. STOPPING CONDITION (Checked constantly) ---
  if (abs(encoder.getCount()) >= max_count) {
    // Stop the motor
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);
    analogWrite(ENA_PIN, 0);

    Serial.println("\nMax count reached. Motor stopped.");

    // Halt the program
    while (1) {
      delay(10);
    }
  }

  // --- 2. CONTROL LOGIC (Run on a fixed interval for consistency) ---
  if (millis() - last_time >= dt) {

    // --- A. Calculate Actual Speed ---
    long current_count = -encoder.getCount(); // Invert count as needed for your wiring
    actual_speed = (float)(current_count - last_encoder_count) * 1000.0 / (float)dt;
    last_encoder_count = current_count;

    // --- B. DYNAMIC TARGET SPEED CALCULATION ---
    // 1. Get the current total song duration.
    unsigned long current_song_duration = getUpdatedSongDuration();

    // 2. Calculate remaining distance and time.
    long distance_remaining = max_count - abs(current_count);
    long time_remaining_ms = current_song_duration - millis();

    // 3. Calculate the new target speed needed to finish on time. 🎯
    if (time_remaining_ms > 0 && distance_remaining > 0) {
      // Speed (counts/sec) = distance (counts) / time (sec)
      target_speed = (float)distance_remaining / ((float)time_remaining_ms / 1000.0);
    } else {
      // If time has run out or we've already arrived, the target is 0.
      target_speed = 0.0;
    }

    // --- C. PID Control Logic ---
    float error = target_speed - actual_speed;

    // P Term (Proportional)
    float p_term = Kp * error;

    // I Term (Integral) with anti-windup
    integral_error += error * (dt / 1000.0); // Scale by time in seconds
    integral_error = constrain(integral_error, -integral_clamp, integral_clamp);
    float i_term = Ki * integral_error;

    // D Term (Derivative)
    float derivative = (error - last_error) / (dt / 1000.0);
    float d_term = Kd * derivative;
    last_error = error;
    
    // Total PID Output
    int pid_calc = (int)(p_term + i_term + d_term);
    pid_calc = constrain(pid_calc, -255, 255);

    // --- D. Drive the Motor (with robust startup logic) ---
    int final_pwm = pid_calc;

    // Only apply a "startup kick" if the motor is stopped but needs to move.
    if (abs(actual_speed) < SPEED_DEADZONE) {
      if (pid_calc > 0 && pid_calc < MIN_PWM_START) {
        final_pwm = MIN_PWM_START;
      } else if (pid_calc < 0 && pid_calc > -MIN_PWM_START) {
        final_pwm = -MIN_PWM_START;
      }
    }
    
    // Set motor direction and speed
    if (final_pwm > 0) {
      digitalWrite(IN1_PIN, HIGH);
      digitalWrite(IN2_PIN, LOW);
    } else {
      digitalWrite(IN1_PIN, LOW);
      digitalWrite(IN2_PIN, HIGH);
    }
    analogWrite(ENA_PIN, abs(final_pwm));

    // --- E. Debugging Output ---
    Serial.print("Tgt:");
    Serial.print(target_speed, 1);
    Serial.print(", Act:");
    Serial.print(actual_speed, 1);
    Serial.print(", Err:");
    Serial.print(error, 1);
    Serial.print(", PWM:");
    Serial.print(final_pwm);
    Serial.print(", TimeLeft(ms):");
    Serial.println(time_remaining_ms);
    
    // Update last_time for the next control cycle
    last_time += dt;
  }
}