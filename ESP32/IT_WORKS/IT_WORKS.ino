// Include the ESP32Encoder library
#include <ESP32Encoder.h>

// --- PIN DEFINITIONS (Match your wiring) ---
const int ENA_PIN = 27;
const int IN1_PIN = 26;
const int IN2_PIN = 25;
const int ENCODER_A_PIN = 14;
const int ENCODER_B_PIN = 15;

// --- CONTROL VARIABLES (ADJUST THESE) ---
long max_count = 45000; //45000 counts = 100 rotations   // The motor will stop after reaching this total absolute count
unsigned long song_duration = 360000; // Total duration of the song in milliseconds (e.g., 180000ms = 3 minutes)

// --- MOTOR/CONTROL CHARACTERISTICS (TUNE THESE) ---
int MIN_PWM_START = 70; // The minimum PWM to overcome static friction
float SPEED_DEADZONE = 10.0; // Speeds below this (counts/sec) are considered "stopped"

// --- PID GAINS (TUNE THESE) ---
// We are starting with very conservative, stable gains.
float Kp = 0.2; // Proportional Gain: Responds to current error.
float Ki = 0.2; // Integral Gain: Responds to accumulated error.
float Kd = 0.001; // Derivative Gain: Responds to rate of change (DAMPING)

// Integral "anti-windup" clamp.
float integral_clamp = 400.0;

// The time interval (in milliseconds) for the control loop
unsigned long dt = 50; // 50ms is a good, stable interval

// --- GLOBAL VARIABLES (Internal use) ---
ESP32Encoder encoder;
long last_encoder_count = 0;
unsigned long last_time = 0;
float actual_speed = 0.0;     // Calculated speed in counts per second
float target_speed = 0.0;     // Will be calculated in setup()

// PID internal variables
float integral_error = 0.0;   // Accumulator for the integral term
float last_error = 0.0;       // For derivative calculation

void setup() {
  Serial.begin(115200);

  // --- Calculate Target Speed ---
  if (song_duration > 0) {
    target_speed = (float)max_count / (song_duration / 1000.0);
  } else {
    target_speed = 0.0; // Avoid division by zero
  }

  Serial.print("Max Count: ");
  Serial.println(max_count);
  Serial.print("Song Duration (ms): ");
  Serial.println(song_duration);
  Serial.print("Calculated Target Speed (counts/sec): ");
  Serial.println(target_speed);
  // ---

  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);

  // Configure the encoder
  encoder.attachHalfQuad(ENCODER_A_PIN, ENCODER_B_PIN);
  encoder.clearCount();

  // Initialize time for speed calculation
  last_time = millis();
}

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
    long current_count = -encoder.getCount(); // Invert count as per your wiring
    actual_speed = (float)(current_count - last_encoder_count) * 1000.0 / (float)dt;
    last_encoder_count = current_count;

    // --- B. PID Control Logic ---
    float error = target_speed - actual_speed;

    // --- P Term (Proportional) ---
    float p_term = Kp * error;

    // --- I Term (Integral) ---
    integral_error += error * (dt / 1000.0); // Scale by time in seconds
    integral_error = constrain(integral_error, -integral_clamp, integral_clamp);
    float i_term = Ki * integral_error;

    // --- D Term (Derivative) ---
    float derivative = (error - last_error) / (dt / 1000.0);
    float d_term = Kd * derivative;
    last_error = error;
    
    // --- Total PID Output ---
    int pid_calc = (int)(p_term + i_term + d_term);
    
    // Constrain the PID calculation
    pid_calc = constrain(pid_calc, -255, 255);

    // --- C. Drive the Motor (NEW ROBUST LOGIC) ---
    int final_pwm = pid_calc;

    // Only apply "startup" boost if the motor is effectively stopped
    // and the controller is trying to move it.
    if (abs(actual_speed) < SPEED_DEADZONE) {
      if (pid_calc > 0 && pid_calc < MIN_PWM_START) {
        final_pwm = MIN_PWM_START;
      } else if (pid_calc < 0 && pid_calc > -MIN_PWM_START) {
        final_pwm = -MIN_PWM_START;
      }
    }
    
    // This new logic allows the controller to send values *below* MIN_PWM_START
    // (e.g., 50) as long as the motor is already spinning.
    // This lets the PID controller *actually control the speed* instead of fighting our logic.
    
    // Set motor direction and speed
    if (final_pwm > 0) {
      digitalWrite(IN1_PIN, HIGH);
      digitalWrite(IN2_PIN, LOW);
    } else {
      digitalWrite(IN1_PIN, LOW);
      digitalWrite(IN2_PIN, HIGH);
    }
    analogWrite(ENA_PIN, abs(final_pwm));

    // --- D. Debugging Output ---
    Serial.print("Tgt:");
    Serial.print(target_speed);
    Serial.print(", Act:");
    Serial.print(actual_speed, 1);
    Serial.print(", P:");
    Serial.print(p_term, 1);
    Serial.print(", I:");
    Serial.print(i_term, 1);
    Serial.print(", D:");
    Serial.print(d_term, 1);
    Serial.print(", PWM:");
    Serial.println(final_pwm);
    
    // Update last_time for the next loop
    last_time += dt;
  }
}

