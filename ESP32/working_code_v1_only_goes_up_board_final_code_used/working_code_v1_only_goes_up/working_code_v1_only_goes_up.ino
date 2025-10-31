// =====================================================================
// LIBRARIES
// =====================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <ESP32Encoder.h>
#include "SpotifyEsp32Modified.h"
#include "secrets.h"

// =====================================================================
// SPOTIFY API SETUP
// =====================================================================
// NOTE: Do NOT disable PLAYER or TRACKS for get_current_playback()
const char* SSID = WIFI_SSID;
const char* PASSWORD = WIFI_PASS;
const char* CLIENT_ID = SPOTIFY_CLIENT_ID;
const char* CLIENT_SECRET = SPOTIFY_CLIENT_SECRET;
const char* REFRESH_TOKEN = SPOTIFY_REFRESH_TOKEN;

Spotify sp(CLIENT_ID, CLIENT_SECRET, REFRESH_TOKEN);
String lastTrackId = "";
const unsigned long SPOTIFY_GRACE_PERIOD_MS = 5000; // 5-second grace period
unsigned long lastSuccessfulSpotifyCheck = 0;

// =====================================================================
// MOTOR CONTROL SETUP
// =====================================================================
const int ENA_PIN = 27;
const int IN1_PIN = 26;
const int IN2_PIN = 25;
const int ENCODER_A_PIN = 14;
const int ENCODER_B_PIN = 15;

long max_count = 45000;
int MIN_PWM_START = 70;
float SPEED_DEADZONE = 10.0;
float Kp = 0.2, Ki = 0.2, Kd = 0.001;
float integral_clamp = 400.0;

ESP32Encoder encoder;
long last_encoder_count = 0;
long current_count = 0; // Made global for printing
float actual_speed = 0.0, target_speed = 0.0;
float integral_error = 0.0, last_error = 0.0;

// =====================================================================
// TIMING & SCHEDULING (Moved to global)
// =====================================================================
unsigned long last_motor_control_time = 0;
unsigned long last_serial_print_time = 0; // Timer for serial printing

const unsigned long dt = 50; // Motor control interval (50 ms)
const unsigned long SPOTIFY_CHECK_INTERVAL_MS = 3000; // Spotify check interval (3000 ms)
const unsigned long SERIAL_PRINT_INTERVAL_MS = 50; // Serial print interval (50 ms)


// =====================================================================
// SHARED STATE & DUAL-CORE SETUP
// =====================================================================
bool isMotorActive = false;
unsigned long current_song_duration_ms = 0;
unsigned long song_start_time_ms = 0;
long time_remaining_ms = 0; // Made global for printing

TaskHandle_t SpotifyTask; // Handle for the task on Core 0
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED; // Mutex for critical sections

// =====================================================================
// HELPER FUNCTIONS
// =====================================================================
void connect_to_wifi() {
    WiFi.begin(SSID, PASSWORD);
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");
}

void stopMotor() {
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);
    analogWrite(ENA_PIN, 0);
}

// =====================================================================
// LOGIC HANDLERS (Called from loop, no timing logic inside)
// =====================================================================

// --- Runs on Core 0: Handles all blocking network calls ---
void spotifyTask(void * pvParameters) {
    Serial.println("Spotify Task started on Core 0");
    unsigned long last_spotify_api_call = 0;

    for (;;) { // Infinite loop for this task
        // 1. This is crucial for token refreshes. Must be called often.
        sp.handle_client();

        unsigned long current_time = millis();

        // 2. Check if it's time to call the main playback API
        if (current_time - last_spotify_api_call >= SPOTIFY_CHECK_INTERVAL_MS) {
            last_spotify_api_call = current_time;
            
            Serial.println("--- Calling Spotify API ---"); // Debug print
            playback_info playback = sp.get_current_playback(); // This is the blocking call

            // 1. Handle SUCCESSFUL playback
            if (playback.http_status == 200 && playback.is_playing) {
                Serial.println("--- Spotify API Success (Playing) ---"); // Debug print
                
                // --- CRITICAL SECTION: Write shared variables ---
                portENTER_CRITICAL(&mux);
                lastSuccessfulSpotifyCheck = millis();
                if (lastTrackId != playback.track_id && !playback.track_id.isEmpty()) {
                    Serial.println("\n--- New Song Detected ---");
                    Serial.printf("Track: %s\n", playback.track_name.c_str());
                    Serial.printf("Duration: %lu ms\n", playback.duration_ms);

                    lastTrackId = playback.track_id;
                    current_song_duration_ms = playback.duration_ms;

                    // Reset motor variables
                    encoder.clearCount(); // Safe to call here
                    last_encoder_count = 0;
                    current_count = 0;
                    integral_error = 0.0;
                    last_error = 0.0;
                    song_start_time_ms = millis();
                    isMotorActive = true;
                }
                portEXIT_CRITICAL(&mux);
                // --- END CRITICAL SECTION ---
            } 
            // 2. Handle API ERRORS or PAUSED music
            else {
                if (playback.http_status != 200) {
                    Serial.println("--- Spotify API Error ---"); // Debug print
                } else {
                    Serial.println("--- Spotify API Success (Paused/Stopped) ---"); // Debug print
                }

                // --- CRITICAL SECTION: Write shared variables ---
                portENTER_CRITICAL(&mux);
                unsigned long timeSinceLastSuccess = millis() - lastSuccessfulSpotifyCheck;
                if (isMotorActive && (timeSinceLastSuccess > SPOTIFY_GRACE_PERIOD_MS)) {
                    Serial.println("--- Playback Stopped (Grace Period Expired). Halting motor. ---");
                    isMotorActive = false; // Core 1 will see this and call stopMotor()
                    lastTrackId = "NOT_PLAYING"; 
                }
                portEXIT_CRITICAL(&mux);
                // --- END CRITICAL SECTION ---
            }
        }

        // 3. Yield to other tasks on this core (if any)
        vTaskDelay(100 / portTICK_PERIOD_MS); // Run this check ~10x/sec
    }
}


// --- Runs on Core 1: High-frequency PID motor control loop ---
void handleMotor() {
    // --- CRITICAL SECTION: Read shared variables ---
    // Copy shared variables to local ones inside a mutex to ensure they
    // aren't being written by Core 0 at the exact same time.
    portENTER_CRITICAL(&mux);
    bool motor_active = isMotorActive;
    unsigned long song_dur_ms = current_song_duration_ms;
    unsigned long song_start_ms = song_start_time_ms;
    portEXIT_CRITICAL(&mux);
    // --- END CRITICAL SECTION ---

    if (!motor_active) {
        stopMotor(); // This is safe, it's on Core 1
        return;
    }

    current_count = -encoder.getCount(); // Update global variable
    if (abs(current_count) >= max_count) {
        Serial.println("\nMax count reached. Motor stopped for this song.");
        stopMotor();
        
        portENTER_CRITICAL(&mux);
        isMotorActive = false; // Tell Core 0 we stopped
        portEXIT_CRITICAL(&mux);
        return;
    }

    // --- Start PID Logic ---
    actual_speed = (float)(current_count - last_encoder_count) * 1000.0 / (float)dt;
    last_encoder_count = current_count;

    long elapsed_time_ms = millis() - song_start_ms;
    time_remaining_ms = song_dur_ms - elapsed_time_ms; // Update global variable
    long distance_remaining = max_count - abs(current_count);

    if (time_remaining_ms > 100 && distance_remaining > 0) {
        target_speed = (float)distance_remaining * 1000.0 / (float)time_remaining_ms;
    } else {
        target_speed = 0.0;
    }

    float error = target_speed - actual_speed;
    float p_term = Kp * error;
    integral_error = constrain(integral_error + (error * (dt / 1000.0)), -integral_clamp, integral_clamp);
    float i_term = Ki * integral_error;
    float derivative = (error - last_error) / (dt / 1000.0);
    float d_term = Kd * derivative;
    last_error = error;
    
    int pid_calc = constrain((int)(p_term + i_term + d_term), -255, 255);

    int final_pwm = pid_calc;
    if (abs(actual_speed) < SPEED_DEADZONE && abs(target_speed) > 0) { 
        if (pid_calc > 0 && pid_calc < MIN_PWM_START) final_pwm = MIN_PWM_START;
        else if (pid_calc < 0 && pid_calc > -MIN_PWM_START) final_pwm = -MIN_PWM_START;
    }

    if (target_speed < 0.1) {
        final_pwm = 0;
    }
    
    if (final_pwm > 0) {
        digitalWrite(IN1_PIN, HIGH);
        digitalWrite(IN2_PIN, LOW);
    } else {
        digitalWrite(IN1_PIN, LOW);
        digitalWrite(IN2_PIN, HIGH);
    }
    analogWrite(ENA_PIN, abs(final_pwm));
    // --- End PID Logic ---
}

// =====================================================================
// SETUP - Runs on Core 1
// =====================================================================
void setup() {
    Serial.begin(115200);
    
    // Motor setup
    pinMode(IN1_PIN, OUTPUT);
    pinMode(IN2_PIN, OUTPUT);
    pinMode(ENA_PIN, OUTPUT);
    encoder.attachHalfQuad(ENCODER_A_PIN, ENCODER_B_PIN);
    encoder.clearCount();
    Serial.println("Motor and Encoder Initialized.");
    
    // Network and API setup
    connect_to_wifi();
    sp.begin();
    while(!sp.is_auth()){
        sp.handle_client(); // Must handle client to get initial auth
    }
    Serial.printf("Spotify Authenticated! New refresh token (if changed): %s\n", sp.get_user_tokens().refresh_token);

    // Initialize timers
    unsigned long now = millis();
    last_motor_control_time = now;
    lastSuccessfulSpotifyCheck = now;
    last_serial_print_time = now;

    // Create the Spotify task and pin it to Core 0
    xTaskCreatePinnedToCore(
        spotifyTask,   /* Task function. */
        "SpotifyTask", /* name of task. */
        10000,         /* Stack size of task (increased for networking) */
        NULL,          /* parameter of the task */
        1,             /* priority of the task */
        &SpotifyTask,  /* Task handle to keep track of created task */
        0);            /* pin task to core 0 */
    
    Serial.println("Main loop starting on Core 1");
}

// =====================================================================
// MAIN LOOP - SCHEDULER - Runs on Core 1
// =====================================================================
void loop() {
    unsigned long current_time = millis();

    // --- Task 1: Handle Motor Control (High Frequency: every 50ms) ---
    if (current_time - last_motor_control_time >= dt) {
        last_motor_control_time += dt; // Use += to prevent drift
        handleMotor();
    }

    // --- Task 2: Handle Serial Plotter (High Frequency: every 50ms) ---
    if (current_time - last_serial_print_time >= SERIAL_PRINT_INTERVAL_MS) {
        last_serial_print_time = current_time;

        // --- CRITICAL SECTION: Read shared variables ---
        // Copy variables to local ones before printing
        portENTER_CRITICAL(&mux);
        float local_target = target_speed;
        float local_actual = actual_speed;
        long local_count = current_count;
        long local_time_left_ms = time_remaining_ms;
        portEXIT_CRITICAL(&mux);
        // --- END CRITICAL SECTION ---

        Serial.print("Tgt:"); Serial.print(local_target);
        Serial.print(", Act:"); Serial.print(local_actual);
        Serial.print(", Cnt:"); Serial.print(local_count);
        Serial.print(", TLeft(s):"); Serial.println(local_time_left_ms / 1000.0);
    }

    // --- Task 3 (Spotify) is now running on Core 0 ---
    // --- sp.handle_client() is also running on Core 0 ---
}

