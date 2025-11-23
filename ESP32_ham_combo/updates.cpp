/*
ESP32 Ham Combo
Copyright (c) 2025 Leszek (HF7A)
https://github.com/hf7a/ESP32-ham-combo

Licensed under CC BY-NC-SA 4.0.
Commercial use is prohibited.
*/

#include "declarations.h"
#include <ArduinoJson.h>

// Checks the latest release on GitHub to see if a new version is available.
// Returns true if a new version is found.
bool checkGithubForUpdate(ApplicationState& state) {
  if (!state.checkForUpdates || !state.network.isWifiConnected) {
    return false;
  }

  WiFiClientSecure client;
  // GitHub API uses a certificate that might not be in the ESP32's root store.
  // We skip verification for this simple check.
  client.setInsecure(); 

  Serial.println("Connecting to GitHub API...");
  if (!client.connect(GITHUB_API_HOST, HTTPS_PORT)) {
    Serial.println("Connection to GitHub API failed.");
    return false;
  }

  // Construct the request URL for the latest release API endpoint.
  String url = String("/repos/") + GITHUB_REPO + "/releases/latest";

  // Send the HTTP GET request.
  // User-Agent is required by GitHub API.
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + GITHUB_API_HOST + "\r\n" +
               "User-Agent: ESP32-ham-combo\r\n" +
               "Connection: close\r\n\r\n");

  // Skip HTTP headers to get to the JSON body.
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }

  // Parse the JSON response.
  // Adjust size if the API response grows significantly.
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, client);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    client.stop();
    return false;
  }

  // Extract the 'tag_name' (version number) from the JSON.
  const char* latest_tag = doc["tag_name"];

  if (latest_tag) {
    Serial.print("Latest GitHub release tag: ");
    Serial.println(latest_tag);
    Serial.print("Current firmware version: ");
    Serial.println(FW_VERSION);

    // Compare the fetched tag with the current firmware version.
    if (strncmp(latest_tag, FW_VERSION, strlen(FW_VERSION)) != 0) {
      Serial.println("New version is available!");
      state.newVersionAvailable = true;
      strlcpy(state.newVersionTag, latest_tag, sizeof(state.newVersionTag));
    } else {
      Serial.println("Firmware is up to date.");
      state.newVersionAvailable = false;
      state.newVersionTag[0] = '\0';
    }
  } else {
    Serial.println("Could not find 'tag_name' in GitHub API response.");
    state.newVersionAvailable = false;
  }

  client.stop();
  
  // Update state and save to memory so we don't check too often
  state.lastUpdateCheckTime = millis(); 
  saveSettings(state); 
  
  return state.newVersionAvailable;
}
