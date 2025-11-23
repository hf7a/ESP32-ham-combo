/*
ESP32 Ham Combo
Copyright (c) 2025 Leszek (HF7A)
https://github.com/hf7a/ESP32-ham-combo

Licensed under CC BY-NC-SA 4.0.
Commercial use is prohibited.
*/

#include "declarations.h"
#include <ctype.h>

namespace {
  // State flag to track if the "Waiting for time sync..." message is on screen.
  // This prevents updateSpotTimesOnly from drawing over it.
  bool isDisplayingTimeSyncMessage = false;

  // Calculates the vertical starting position for the spot list based on the current view mode.
  int calculateSpotsStartY(const ApplicationState& state) {
    int spotsToDisplay = (state.display.spotsViewMode == SPOTS_ONLY) ? 6 : 5;
    const int BLOCK_HEIGHT = spotsToDisplay * SPOT_LINE_HEIGHT;
    int availableHeight = BUTTON_Y;

    // If the prop footer is visible, reduce the available height for spots.
    if (state.display.spotsViewMode == SPOTS_WITH_PROP) {
      availableHeight = PROP_FOOTER_Y - 5; // Give 5px margin above footer
    }

    return (availableHeight - BLOCK_HEIGHT) / 2;
  }

  // Internal function to draw just the list of spots. Reused by both screen variants.
  static void drawSpotsList(ApplicationState& state) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    gmtime_r(&now, &timeinfo);

    // Check if time is synchronized (year > 2020)
    if (timeinfo.tm_year < (2020 - 1900)) {
      tft.setTextColor(TFT_CYAN);
      tft.setTextDatum(MC_DATUM);
      tft.drawString("Waiting for time sync...", tft.width() / 2, tft.height() / 2);
      isDisplayingTimeSyncMessage = true;
      return;
    }

    isDisplayingTimeSyncMessage = false;
    long currentTimeInSeconds = timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60 + timeinfo.tm_sec;

    const int START_Y = calculateSpotsStartY(state);
    const int COL_FREQ_X = tft.width() - SPOT_COL_FREQ_X_MARGIN;
    int spotsToDisplay = (state.display.spotsViewMode == SPOTS_ONLY) ? 6 : 5;
    int spotsAvailable = (state.spotCount > spotsToDisplay) ? spotsToDisplay : state.spotCount;

    for (int i = 0; i < spotsAvailable; i++) {
      // Calculate index in the circular buffer (newest first)
      int displayIndex = (state.latestSpotIndex - i + ApplicationState::MAX_SPOTS) % ApplicationState::MAX_SPOTS;
      int yPos = START_Y + (i * SPOT_LINE_HEIGHT) + 5;

      long spotTimeInSeconds = state.spots[displayIndex].spotHour * 3600 + state.spots[displayIndex].spotMinute * 60;
      long elapsedSeconds = currentTimeInSeconds - spotTimeInSeconds;
      if (elapsedSeconds < 0) elapsedSeconds += 86400; // Handle spots from the previous day

      // Draw Time (Elapsed)
      tft.setTextDatum(TR_DATUM);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString(formatElapsedMinutes(elapsedSeconds), SPOT_COL_TIME_X, yPos);

      // Draw Callsign
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.drawString(state.spots[displayIndex].call, SPOT_COL_CALL_X, yPos);

      // Draw Mode
      tft.setTextColor(getModeColor(state.spots[displayIndex].mode), TFT_BLACK);
      tft.drawString(state.spots[displayIndex].mode, SPOT_COL_MODE_X, yPos);

      // Draw Frequency
      tft.setTextDatum(TR_DATUM);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString(state.spots[displayIndex].freq, COL_FREQ_X, yPos);
    }
  }
}

bool connectToTelnet(ApplicationState& state, bool silentMode) {
  if (!state.network.isWifiConnected) {
    return false;
  }

  if (telnetClient.connect(TELNET_HOST, TELNET_PORT)) {
    // HamAlert usually sends "login: "
    telnetClient.readStringUntil(':'); 
    telnetClient.println(state.network.telnetUsername);
    
    // Then "password: "
    telnetClient.readStringUntil(':'); 
    telnetClient.println(state.network.telnetPassword);

    unsigned long startTime = millis();
    while (millis() - startTime < TELNET_LOGIN_TIMEOUT_MS) {
      if (telnetClient.available()) {
        String line = telnetClient.readStringUntil('\n');
        if (!silentMode) {
          Serial.print("HamAlert RSP: ");
          Serial.println(line);
        }

        if (line.indexOf("Hello ") != -1) {
          if (!silentMode) Serial.println("Login successful.");
          // Request the max number of spots immediately to fill the screen
          String command = "sh/dx " + String(ApplicationState::MAX_SPOTS);
          telnetClient.println(command);
          return true;
        }
        if (line.indexOf("Login failed") != -1) {
          if (!silentMode) Serial.println("Login failed message detected.");
          return false;
        }
      }
    }

    if (!silentMode) Serial.println("Login timed out. No confirmation from HamAlert server.");
    return false;

  } else {
    if (!silentMode) Serial.println("Could not connect to HamAlert server.");
    return false;
  }
}

void readTelnetSpots(ApplicationState& state) {
  static char lineBuffer[TELNET_LINE_BUFFER_SIZE];
  static int bufferPos = 0;
  bool newSpotReceived = false; 

  // Process all available characters from the telnet buffer
  while (telnetClient.available()) {
    char c = telnetClient.read();
    if (c == '\n' || c == '\r') {
      if (bufferPos > 0) {
        lineBuffer[bufferPos] = '\0'; // Null-terminate

        // Trim trailing whitespace
        int i = bufferPos - 1;
        while (i >= 0 && isspace(lineBuffer[i])) {
          lineBuffer[i--] = '\0';
        }

        if (strlen(lineBuffer) > 0) {
          parseSpot(lineBuffer, state);
          newSpotReceived = true;
        }
        bufferPos = 0; // Reset for next line
      }
    } else if (bufferPos < sizeof(lineBuffer) - 1) {
      lineBuffer[bufferPos++] = c;
    }
  }

  // Redraw only if new data arrived to avoid flickering
  if (newSpotReceived) {
    determineAndDrawActiveScreen(state);
  }
}

void parseSpot(const char* line, ApplicationState& state) {
  // Expected format: "DX de SPOTTER:  FREQ  CALL  TEXT  TIMEZ"
  // Example: "DX de SP7ABC:  14074.0  K1ABC  FT8 -10dB  1234Z"
  
  const char* de_ptr = strstr(line, "DX de ");
  if (!de_ptr) return;

  const char* spotter_start = de_ptr + 6;
  const char* colon_ptr = strchr(spotter_start, ':');
  if (!colon_ptr) return;

  const char* freq_start = colon_ptr + 1;
  while (*freq_start && isspace(*freq_start)) freq_start++;
  const char* freq_end = strchr(freq_start, ' ');
  if (!freq_end) return;

  const char* call_start = freq_end + 1;
  while (*call_start && isspace(*call_start)) call_start++;
  const char* call_end = strchr(call_start, ' ');
  if (!call_end) return;

  const char* time_ptr = strrchr(line, ' ');
  if (!time_ptr || strlen(time_ptr + 1) < 4) return;
  const char* time_start = time_ptr + 1;

  DxSpot newSpot;

  // Copy Spotter
  size_t len = colon_ptr - spotter_start;
  if (len >= sizeof(newSpot.spotter)) len = sizeof(newSpot.spotter) - 1;
  strncpy(newSpot.spotter, spotter_start, len);
  newSpot.spotter[len] = '\0';

  // Copy Frequency
  len = freq_end - freq_start;
  if (len >= sizeof(newSpot.freq)) len = sizeof(newSpot.freq) - 1;
  strncpy(newSpot.freq, freq_start, len);
  newSpot.freq[len] = '\0';
  float freqKHz = atof(newSpot.freq);

  // Copy Callsign
  len = call_end - call_start;
  if (len >= sizeof(newSpot.call)) len = sizeof(newSpot.call) - 1;
  strncpy(newSpot.call, call_start, len);
  newSpot.call[len] = '\0';

  // Parse Time (HHMM)
  char hour_str[3] = { time_start[0], time_start[1], '\0' };
  char min_str[3] = { time_start[2], time_start[3], '\0' };
  newSpot.spotHour = atoi(hour_str);
  newSpot.spotMinute = atoi(min_str);

  // Determine Mode
  getModeFromLine(line, freqKHz, newSpot.mode, sizeof(newSpot.mode));

  addSpot(newSpot, state);
}

void addSpot(const DxSpot& newSpot, ApplicationState& state) {
  state.latestSpotIndex = (state.latestSpotIndex + 1) % ApplicationState::MAX_SPOTS;
  state.spots[state.latestSpotIndex] = newSpot;
  if (state.spotCount < ApplicationState::MAX_SPOTS) {
    state.spotCount++;
  }
  playNewSpotSound(state);
}

void clearSpots(ApplicationState& state) {
  state.spotCount = 0;
  state.latestSpotIndex = -1;
  Serial.println("Spot list cleared.");
}

void getModeFromLine(const char* line, float freq_khz, char* mode_buffer, size_t buffer_size) {
    // Case-insensitive check
    char upperLine[TELNET_LINE_BUFFER_SIZE];
    strlcpy(upperLine, line, sizeof(upperLine));
    for (char *p = upperLine; *p; ++p) *p = toupper(*p);

    if (strstr(upperLine, "FT8")) { strlcpy(mode_buffer, "FT8", buffer_size); }
    else if (strstr(upperLine, "FT4")) { strlcpy(mode_buffer, "FT4", buffer_size); }
    else if (strstr(upperLine, "SSB")) { strlcpy(mode_buffer, "SSB", buffer_size); }
    else if (strstr(upperLine, "USB")) { strlcpy(mode_buffer, "SSB", buffer_size); }
    else if (strstr(upperLine, "LSB")) { strlcpy(mode_buffer, "SSB", buffer_size); }
    else if (strstr(upperLine, "CW"))  { strlcpy(mode_buffer, "CW", buffer_size); }
    else if (strstr(upperLine, "WPM")) { strlcpy(mode_buffer, "CW", buffer_size); }
    // Fallback: Band plan check for SSB
    else if ((freq_khz >= 1840 && freq_khz <= 2000) ||     // 160m
             (freq_khz >= 3600 && freq_khz <= 3800) ||     // 80m
             (freq_khz >= 5330 && freq_khz <= 5410) ||     // 60m
             (freq_khz >= 7050 && freq_khz <= 7200) ||     // 40m
             (freq_khz >= 14100 && freq_khz <= 14350) ||   // 20m
             (freq_khz >= 18110 && freq_khz <= 18168) ||   // 17m
             (freq_khz >= 21150 && freq_khz <= 21450) ||   // 15m
             (freq_khz >= 24920 && freq_khz <= 24990) ||   // 12m
             (freq_khz >= 28300 && freq_khz <= 29000) ||   // 10m
             (freq_khz >= 50100 && freq_khz <= 51000)) {   // 6m
        strlcpy(mode_buffer, "SSB", buffer_size);
    } else {
        strlcpy(mode_buffer, "-", buffer_size);
    }
}

uint16_t getModeColor(const char* mode) {
  if (strcmp(mode, "SSB") == 0) return TFT_GREEN;
  if (strcmp(mode, "FT8") == 0 || strcmp(mode, "FT4") == 0) return TFT_YELLOW;
  if (strcmp(mode, "CW") == 0) return TFT_ORANGE;
  return TFT_GREEN;
}

String formatElapsedMinutes(long elapsedSeconds) {
  long minutes = elapsedSeconds / 60;
  return String(minutes) + "m";
}

void drawSpotsScreen(ApplicationState& state) {
  tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(&FreeSans9pt7b);

  if (!state.network.isWifiConnected) {
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_RED);
    tft.drawString("WiFi Connection Lost", tft.width() / 2, tft.height() / 2 - 15);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Reconnecting...", tft.width() / 2, tft.height() / 2 + 15);
    drawButtons(state);
    isDisplayingTimeSyncMessage = false;
    return;
  }

  if (!state.network.hamAlertConnected) {
    tft.setTextDatum(MC_DATUM);
    int yPos = tft.height() / 2 - 40;
    tft.setTextColor(TFT_RED);
    tft.drawString("HamAlert Login Failed", tft.width() / 2, yPos);
    yPos += 40;
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Configure credentials at:", tft.width() / 2, yPos);
    yPos += 25;
    tft.setTextColor(TFT_YELLOW);
    tft.drawString("http://" + WiFi.localIP().toString(), tft.width() / 2, yPos);
    drawButtons(state);
    isDisplayingTimeSyncMessage = false;
    return;
  }

  drawSpotsList(state);
  drawButtons(state);
  state.lastDisplayUpdateTime = millis();
}

void drawSpotsAndPropScreen(ApplicationState& state) {
  // Draw the base spots screen
  drawSpotsScreen(state);

  // If no errors, draw the footer
  if (state.network.isWifiConnected && state.network.hamAlertConnected) {
    drawPropagationFooter(state);
  }
}

void updateSpotTimesOnly(ApplicationState& state) {
  if (state.activeScreen != SCREEN_SPOTS && state.activeScreen != SCREEN_SPOTS_AND_PROP) return;
  if (!state.network.isWifiConnected || !state.network.hamAlertConnected) return;

  time_t now;
  struct tm timeinfo;
  time(&now);
  gmtime_r(&now, &timeinfo);

  bool isTimeSynced = (timeinfo.tm_year >= (2020 - 1900));

  if (isDisplayingTimeSyncMessage) {
    if (isTimeSynced) {
      // Time just synced, full redraw needed
      determineAndDrawActiveScreen(state);
    }
    return;
  }

  if (!isTimeSynced) return;

  long currentTimeInSeconds = timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60 + timeinfo.tm_sec;

  const int START_Y = calculateSpotsStartY(state);
  int spotsToDisplay = (state.display.spotsViewMode == SPOTS_ONLY) ? 6 : 5;
  int spotsAvailable = (state.spotCount > spotsToDisplay) ? spotsToDisplay : state.spotCount;

  tft.setTextDatum(TR_DATUM);
  tft.setFreeFont(&FreeSans9pt7b);

  for (int i = 0; i < spotsAvailable; i++) {
    int displayIndex = (state.latestSpotIndex - i + ApplicationState::MAX_SPOTS) % ApplicationState::MAX_SPOTS;
    int yPos = START_Y + (i * SPOT_LINE_HEIGHT) + 5;

    long spotTimeInSeconds = state.spots[displayIndex].spotHour * 3600 + state.spots[displayIndex].spotMinute * 60;
    long elapsedSeconds = currentTimeInSeconds - spotTimeInSeconds;
    if (elapsedSeconds < 0) elapsedSeconds += 86400;

    String timeStr = formatElapsedMinutes(elapsedSeconds);

    // Clear and redraw only the time area
    tft.fillRect(0, yPos, SPOT_COL_TIME_WIDTH + 5, 20, TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(timeStr, SPOT_COL_TIME_X, yPos);
  }

  state.lastDisplayUpdateTime = millis();
}
