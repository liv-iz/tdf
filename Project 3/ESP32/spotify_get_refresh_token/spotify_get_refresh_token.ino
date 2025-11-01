#include <Arduino.h>
#include <WiFi.h>
#include "SpotifyEsp32.h"
#include "secrets.h"

const char* SSID = WIFI_SSID;
const char* PASSWORD = WIFI_PASS;
const char* CLIENT_ID = SPOTIFY_CLIENT_ID;
const char* CLIENT_SECRET = SPOTIFY_CLIENT_SECRET;

// Create an instance of the Spotify class (optional: specify retry count)
Spotify sp(CLIENT_ID, CLIENT_SECRET);



void setup() {
 Serial.begin(115200);
 connect_to_wifi();
 // Optionally set custom scopes the available scopes are listed below
 // sp.set_scopes("user-read-playback-state user-modify-playback-state");
 sp.begin();

 while (!sp.is_auth()) {
     sp.handle_client(); // Required for receiving the authorization code
 }

 Serial.printf("Authenticated! Refresh token: %s\n", sp.get_user_tokens().refresh_token);

}



void loop() {

 // Your code here

}



void connect_to_wifi() {

 WiFi.begin(SSID, PASSWORD);

 Serial.print("Connecting to WiFi...");

 while (WiFi.status() != WL_CONNECTED) {

     delay(1000);

     Serial.print(".");

 }

 Serial.println("\nConnected to WiFi!");

}