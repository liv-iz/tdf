/*
    An example of how to authenticate with Spotify and print track information.

    - On new song: prints track_name and song_duration.
    - Every second: prints song_progress in milliseconds.
    - Optimized to call the API only once per second.
*/

// --- OPTIMIZATION FLAGS ---
// NOTE: Do NOT disable PLAYER or TRACKS, as get_current_playback() depends on them.
#define DISABLE_ALBUM
#define DISABLE_ARTIST
#define DISABLE_AUDIOBOOKS
#define DISABLE_CATEGORIES
#define DISABLE_CHAPTERS
#define DISABLE_EPISODES
#define DISABLE_MARKETS
#define DISABLE_PLAYLISTS
#define DISABLE_SEARCH
#define DISABLE_SHOWS

// --- LIBRARIES ---
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "SpotifyEsp32Modified.h"
#include "secrets.h"

// --- SECRETS & SETUP ---
const char* SSID = WIFI_SSID;
const char* PASSWORD = WIFI_PASS;
const char* CLIENT_ID = SPOTIFY_CLIENT_ID;
const char* CLIENT_SECRET = SPOTIFY_CLIENT_SECRET;
const char* REFRESH_TOKEN = SPOTIFY_REFRESH_TOKEN;

Spotify sp(CLIENT_ID, CLIENT_SECRET, REFRESH_TOKEN);

void connect_to_wifi(){
    WiFi.begin(SSID, PASSWORD);
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.printf("\nConnected to WiFi\n");
}

void setup() {
    Serial.begin(115200);
    connect_to_wifi();
    
    // This is the initial authentication flow.
    // If you have a valid refresh token in secrets.h, you can use the faster
    // sp.get_access_token() method instead.
    sp.begin();
    while(!sp.is_auth()){
        sp.handle_client();
    }
    Serial.printf("Authenticated! New refresh token (if changed): %s\n", sp.get_user_tokens().refresh_token);
}

// --- THIS IS THE NEW, EFFICIENT LOOP FUNCTION ---
void loop() {
    // Timer to control how often we check Spotify
    static unsigned long lastCheckTime = 0;
    const unsigned long checkInterval = 1000; // Check every 1 second

    // Variable to track the song
    static String lastTrackId = "";

    // Only run the code inside this block once per interval
    if (millis() - lastCheckTime >= checkInterval) {
        lastCheckTime = millis(); // Reset the timer

        // --- Make a single, efficient API call ---
        // This one call gets all the info we need.
        playback_info playback = sp.get_current_playback();

        // 1. Handle case where nothing is playing or an error occurred
        if (playback.http_status != 200 || !playback.is_playing) {
            if (lastTrackId != "NOT_PLAYING") {
                Serial.println("--- Playback Stopped ---");
                lastTrackId = "NOT_PLAYING"; // Set state to prevent repeat messages
            }
            return; // Stop here if nothing is playing.
        }

        // 2. Handle when a new song starts
        // We use the data from the 'playback' struct we already fetched.
        if (lastTrackId != playback.track_id && !playback.track_id.isEmpty()) {
            lastTrackId = playback.track_id;
            
            Serial.println("\n--- New Song Detected ---");
            Serial.printf("track_name: %s\n", playback.track_name.c_str());
            Serial.printf("song_duration: %ld\n", playback.duration_ms);
        }

        // 3. Always get and print the progress from the same 'playback' struct
        if (playback.progress_ms != -1) {
             Serial.printf("song_progress: %ld\n", playback.progress_ms);
        }
    }
}
