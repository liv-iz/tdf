// =====================================================================
// LIBRARIES
// =====================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <ESP32Encoder.h>
#include "SpotifyEsp32Modified.h"
#include "secrets.h"
#include "esp_task_wdt.h" // For watchdog control

// =====================================================================
// SPOTIFY API SETUP
// =====================================================================
#define DISABLE_ALBUM
#define DISABLE_ARTIST
#define DISABLE_AUDIOBOOKS
#define DISABLE_CATEGORIES
#define DISABLE_CHAPTERS
#define DISABLE_EPISODES
#define DISABLE_GENRES
#define DISABLE_MARKETS
#define DISABLE_PLAYLISTS
#define DISABLE_SEARCH
#define DISABLE_SHOWS
#define DISABLE_USER

const char* SSID = WIFI_SSID;
const char* PASSWORD = WIFI_PASS;
const char* CLIENT_ID = SPOTIFY_CLIENT_ID;
const char* CLIENT_SECRET = SPOTIFY_CLIENT_SECRET;
const char* REFRESH_TOKEN = SPOTIFY_REFRESH_TOKEN;

Spotify sp(CLIENT_ID, CLIENT_SECRET, REFRESH_TOKEN);
String lastTrackId = "";
const unsigned long SPOTIFY_GRACE_PERIOD_MS = 5000;
unsigned long lastSuccessfulSpotifyCheck = 0;

// =====================================================================
// MOTOR CONTROL SETUP
// =====================================================================
const int ENA_PIN = 27;
const int IN1_PIN = 26;
const int IN2_PIN = 25;
const int ENCODER_A_PIN = 14;
const int ENCODER_B_PIN = 15;

// PWM Configuration for ESP32 Core 3.0+
const int PWM_FREQ = 5000;
const int PWM_CHANNEL = 0;
const int PWM_RESOLUTION = 8;

long max_count = 45000;
int MIN_PWM_START = 70;
float SPEED_DEADZONE = 10.0;
float Kp = 0.2, Ki = 0.2, Kd = 0.001;
float integral_clamp = 400.0;

ESP32Encoder encoder;
long last_encoder_count = 0;
long current_count = 0;
float actual_speed = 0.0, target_speed = 0.0;
float integral_error = 0.0, last_error = 0.0;

// =====================================================================
// TIMING & SCHEDULING
// =====================================================================
unsigned long last_motor_control_time = 0;
unsigned long last_serial_print_time = 0;
const unsigned long MOTOR_CONTROL_INTERVAL_MS = 50;
const unsigned long SPOTIFY_CHECK_INTERVAL_MS = 3000;
const unsigned long SERIAL_PRINT_INTERVAL_MS = 50;

// =====================================================================
// SHARED STATE & DUAL-CORE SETUP
// =====================================================================
volatile bool isMotorActive = false;
volatile bool shouldResetMotor = false;
volatile unsigned long current_song_duration_ms = 0;
volatile unsigned long song_start_time_ms = 0;

TaskHandle_t SpotifyTask;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
long time_remaining_ms = 0;

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
    ledcWrite(ENA_PIN, 0);
}

void resetMotorState() {
    encoder.clearCount();
    last_encoder_count = 0;
    current_count = 0;
    integral_error = 0.0;
    last_error = 0.0;
    actual_speed = 0.0;
    target_speed = 0.0;
    Serial.println("\n--- Motor State Reset ---");
}

// =====================================================================
// SPOTIFY TASK - Runs on Core 0
// =====================================================================
void spotifyTask(void * pvParameters) {
    uint32_t stack_guard = 0xDEADBEEF;
    Serial.println("Spotify Task started on Core 0");
    unsigned long last_spotify_api_call = 0;

    for (;;) {
        // Check for stack overflow
        if (stack_guard != 0xDEADBEEF) {
            Serial.println("STACK OVERFLOW DETECTED!");
            while(1);
        }

        // Feed the idle task
        vTaskDelay(1);

        sp.handle_client();

        unsigned long current_time = millis();
        if (current_time - last_spotify_api_call >= SPOTIFY_CHECK_INTERVAL_MS) {
            last_spotify_api_call = current_time;
            Serial.println("--- Calling Spotify API ---");

            unsigned long start = millis();
            playback_info playback;
            bool success = false;
            while (millis() - start < 2000) { // Timeout after 2s
                playback = sp.get_current_playback();
                if (playback.http_status != 0) {
                    success = true;
                    break;
                }
                delay(10);
            }

            if (!success) {
                Serial.println("--- Spotify API Timeout ---");
                continue;
            }

            Serial.printf("HTTP Status: %d, Is Playing: %d, Track ID: %s\n",
                playback.http_status, playback.is_playing, playback.track_id.c_str());

            if (playback.http_status == 200 && playback.is_playing) {
                if (playback.track_id.isEmpty() || playback.track_name.isEmpty()) {
                    Serial.println("Empty track ID or name!");
                    continue;
                }

                portENTER_CRITICAL(&mux);
                lastSuccessfulSpotifyCheck = millis();

                if (lastTrackId != playback.track_id) {
                    Serial.println("\n--- New Song Detected ---");
                    Serial.printf("Track: %s\n", playback.track_name.c_str());
                    Serial.printf("Duration: %lu ms\n", playback.duration_ms);
                    lastTrackId = playback.track_id;
                    current_song_duration_ms = playback.duration_ms;
                    song_start_time_ms = millis();
                    shouldResetMotor = true;
                    isMotorActive = true;
                }
                portEXIT_CRITICAL(&mux);
            }
            else {
                if (playback.http_status != 200) {
                    Serial.println("--- Spotify API Error ---");
                } else {
                    Serial.println("--- Spotify API Success (Paused/Stopped) ---");
                }
                portENTER_CRITICAL(&mux);
                unsigned long timeSinceLastSuccess = millis() - lastSuccessfulSpotifyCheck;
                if (isMotorActive && (timeSinceLastSuccess > SPOTIFY_GRACE_PERIOD_MS)) {
                    Serial.println("--- Playback Stopped (Grace Period Expired). Halting motor. ---");
                    isMotorActive = false;
                    lastTrackId = "NOT_PLAYING";
                }
                portEXIT_CRITICAL(&mux);
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

// =====================================================================
// MOTOR CONTROL - Runs on Core 1
// =====================================================================
void handleMotor() {
    portENTER_CRITICAL(&mux);
    bool motor_active = isMotorActive;
    bool should_reset = shouldResetMotor;
    unsigned long song_dur_ms = current_song_duration_ms;
    unsigned long song_start_ms = song_start_time_ms;
    portEXIT_CRITICAL(&mux);

    if (should_reset) {
        resetMotorState();
        portENTER_CRITICAL(&mux);
        shouldResetMotor = false;
        portEXIT_CRITICAL(&mux);
    }

    if (!motor_active) {
        stopMotor();
        return;
    }

    current_count = -encoder.getCount();

    if (abs(current_count) >= max_count) {
        Serial.println("\nMax count reached. Motor stopped for this song.");
        stopMotor();

        portENTER_CRITICAL(&mux);
        isMotorActive = false;
        portEXIT_CRITICAL(&mux);
        return;
    }

    actual_speed = (float)(current_count - last_encoder_count) * 1000.0 / (float)MOTOR_CONTROL_INTERVAL_MS;
    last_encoder_count = current_count;
    long elapsed_time_ms = millis() - song_start_ms;
    time_remaining_ms = song_dur_ms - elapsed_time_ms;
    long distance_remaining = max_count - abs(current_count);

    if (time_remaining_ms > 100 && distance_remaining > 0) {
        target_speed = (float)distance_remaining * 1000.0 / (float)time_remaining_ms;
    } else {
        target_speed = 0.0;
    }

    float error = target_speed - actual_speed;
    float p_term = Kp * error;

    integral_error = constrain(
        integral_error + (error * (MOTOR_CONTROL_INTERVAL_MS / 1000.0)),
        -integral_clamp,
        integral_clamp
    );
    float i_term = Ki * integral_error;

    float derivative = (error - last_error) / (MOTOR_CONTROL_INTERVAL_MS / 1000.0);
    float d_term = Kd * derivative;
    last_error = error;

    int pid_calc = constrain((int)(p_term + i_term + d_term), -255, 255);
    int final_pwm = pid_calc;

    if (abs(actual_speed) < SPEED_DEADZONE && abs(target_speed) > 0) {
        if (pid_calc > 0 && pid_calc < MIN_PWM_START) {
            final_pwm = MIN_PWM_START;
        } else if (pid_calc < 0 && pid_calc > -MIN_PWM_START) {
            final_pwm = -MIN_PWM_START;
        }
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

    ledcWrite(ENA_PIN, abs(final_pwm));
}

// =====================================================================
// SETUP - Runs on Core 1
// =====================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    // Disable the watchdog for all tasks
    esp_task_wdt_deinit();

    pinMode(IN1_PIN, OUTPUT);
    pinMode(IN2_PIN, OUTPUT);
    pinMode(ENA_PIN, OUTPUT);

    // PWM setup using ESP32 Core 3.0+ API
    ledcAttachChannel(ENA_PIN, PWM_FREQ, PWM_RESOLUTION, PWM_CHANNEL);

    encoder.attachHalfQuad(ENCODER_A_PIN, ENCODER_B_PIN);
    encoder.clearCount();
    Serial.println("Motor and Encoder Initialized.");

    connect_to_wifi();
    sp.begin();
    while(!sp.is_auth()){
        sp.handle_client();
    }
    Serial.printf("Spotify Authenticated! New refresh token: %s\n", sp.get_user_tokens().refresh_token);

    unsigned long now = millis();
    last_motor_control_time = now;
    lastSuccessfulSpotifyCheck = now;
    last_serial_print_time = now;

    xTaskCreatePinnedToCore(
        spotifyTask,
        "SpotifyTask",
        32000, // Increased stack size
        NULL,
        3, // Increased priority
        &SpotifyTask,
        0
    );

    Serial.println("Main loop starting on Core 1");
}

// =====================================================================
// MAIN LOOP - Runs on Core 1
// =====================================================================
void loop() {
    unsigned long current_time = millis();
    if (current_time - last_motor_control_time >= MOTOR_CONTROL_INTERVAL_MS) {
        last_motor_control_time = current_time;
        handleMotor();
    }
    if (current_time - last_serial_print_time >= SERIAL_PRINT_INTERVAL_MS) {
        last_serial_print_time = current_time;
        Serial.print("Tgt:"); Serial.print(target_speed);
        Serial.print(", Act:"); Serial.print(actual_speed);
        Serial.print(", Cnt:"); Serial.print(current_count);
        Serial.print(", TLeft(s):"); Serial.println(time_remaining_ms / 1000.0);
    }
}
