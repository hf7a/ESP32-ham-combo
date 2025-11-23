/*
ESP32 Ham Combo
Copyright (c) 2025 Leszek (HF7A)
https://github.com/hf7a/ESP32-ham-combo

Licensed under CC BY-NC-SA 4.0.
Commercial use is prohibited.
*/

#include "declarations.h"

// --- Global Objects ---
TFT_eSPI tft = TFT_eSPI();
WiFiClient telnetClient;
Preferences preferences;
SPIClass touchscreenSPI = SPIClass(VSPI);
// Touchscreen pins are defined in User_Setup.h or constants.h
XPT2046_Touchscreen touchscreen(TOUCH_CS, TOUCH_IRQ);
WiFiClientSecure secureClient;
HttpClient httpClient(secureClient, PROP_HOST, HTTPS_PORT);
AsyncWebServer webServer(80);

// --- Global Application State ---
ApplicationState applicationState;

// --- Initialization State Machine ---
InitializationState initState = INIT_BEGIN;

// --- Helper Functions ---

// Determines which main screen to draw based on user settings and draws it.
void determineAndDrawActiveScreen(ApplicationState& state) {
  // Default to the standard spots screen
  ActiveScreen targetScreen = SCREEN_SPOTS;

  // If "Remember Last Screen" is enabled, use the saved startup screen
  if (state.display.rememberLastScreen) {
    targetScreen = state.display.startupScreen;
  }

  // Handle Spots Screens (Simple vs Extended)
  if (targetScreen == SCREEN_SPOTS || targetScreen == SCREEN_SPOTS_AND_PROP) {
    if (state.display.spotsViewMode == SPOTS_WITH_PROP) {
      state.activeScreen = SCREEN_SPOTS_AND_PROP;
      drawSpotsAndPropScreen(state);
    } else {
      state.activeScreen = SCREEN_SPOTS;
      drawSpotsScreen(state);
    }
    
    // Update saved state if needed
    if (state.display.rememberLastScreen) {
      state.display.startupScreen = state.activeScreen;
      saveSettings(state);
    }
    return;
  }

  // Handle other specific screens
  state.activeScreen = targetScreen;
  switch(targetScreen) {
    case SCREEN_CLOCK:
      state.lastSecond = -1; // Reset clock animation state
      drawClockScreen(state);
      break;
    case SCREEN_PROPAGATION:
      drawPropagationScreen(state);
      break;
    default: 
      // Fallback
      state.activeScreen = SCREEN_SPOTS;
      drawSpotsScreen(state);
      break;
  }

  // Update saved state if needed
  if (state.display.rememberLastScreen) {
    state.display.startupScreen = state.activeScreen;
    saveSettings(state);
  }
}

// Starts a SoftAP and a simple web server to configure WiFi credentials.
// This is a blocking function that never returns (device restarts after save).
void startConfigurationPortal() {
  tft.fillScreen(TFT_BLACK);
  tft.invertDisplay(true); // Ensure readability
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  int yPos = 40;
  tft.drawString("WiFi Configuration Mode", tft.width() / 2, yPos); yPos += 40;
  tft.drawString("Connect to this network:", tft.width() / 2, yPos); yPos += 25;
  tft.setTextColor(TFT_YELLOW);
  tft.drawString("ESP32-Ham-Combo-Setup", tft.width() / 2, yPos); yPos += 40;
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Then open 192.168.4.1", tft.width() / 2, yPos);

  WiFi.softAP("ESP32-Ham-Combo-Setup");
  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = R"rawliteral(
<!DOCTYPE html><html><head><title>Device Setup</title><meta name="viewport" content="width=device-width, initial-scale=1">
<style>body{font-family:Arial,sans-serif;background:#121212;color:#e0e0e0;display:flex;justify-content:center;align-items:center;min-height:100vh;margin:0;padding:10px 0;}
.container{background:#1e1e1e;padding:20px;border-radius:8px;border:1px solid #333;width:90%;max-width:400px;}
h1,h2{color:#009688;text-align:center;border-bottom:1px solid #333;padding-bottom:10px;margin-top:0;}
h2{font-size:1.1em;border:none;margin-bottom:10px;}
label{display:block;margin-top:10px;}
input[type=text],input[type=password]{width:100%;padding:8px;margin-top:5px;border-radius:4px;border:1px solid #555;background:#333;color:#fff;box-sizing:border-box;}
input[type=submit]{width:100%;background:#00796B;color:white;padding:10px;border:none;border-radius:4px;cursor:pointer;font-size:1em;margin-top:20px;}
</style></head><body><div class="container"><h1>Device Setup</h1><form action="/save" method="POST">
<h2>WiFi Configuration</h2>
<label for="ssid">SSID (Network Name):</label><input type="text" id="ssid" name="ssid">
<label for="pass">Password:</label><input type="password" id="pass" name="pass">
<hr style="border-color:#333;margin:20px 0;">
<h2>HamAlert Credentials (Optional)</h2>
<label for="ham_user">Login:</label><input type="text" id="ham_user" name="ham_user">
<label for="ham_pass">Password:</label><input type="password" id="ham_pass" name="ham_pass">
<input type="submit" value="Save & Connect"></form></div></body></html>)rawliteral";
    request->send(200, "text/html", html);
  });

  webServer.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
    // Save WiFi Creds
    preferences.begin("wifi-creds", false);
    preferences.putString("ssid", request->arg("ssid"));
    preferences.putString("password", request->arg("pass"));
    preferences.end();

    // Save HamAlert Creds if provided
    if (request->hasParam("ham_user") && request->arg("ham_user").length() > 0) {
      preferences.begin("app-settings", false);
      preferences.putString("telnetUser", request->arg("ham_user"));
      preferences.putString("telnetPass", request->arg("ham_pass"));
      preferences.end();
    }

    tft.fillScreen(TFT_GREEN);
    tft.setTextColor(TFT_BLACK, TFT_GREEN);
    tft.drawString("Settings Saved!", tft.width() / 2, tft.height() / 2 - 10);
    tft.drawString("Restarting...", tft.width() / 2, tft.height() / 2 + 10);
    delay(RESTART_DELAY_MS);
    ESP.restart();
  });

  webServer.begin();
  while(true) { 
    // Loop indefinitely in config mode to keep SoftAP alive
    delay(100); 
  }
}

void enterDeepSleep(const ApplicationState& state) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Entering sleep mode...", tft.width() / 2, tft.height() / 2);
  delay(1500);

  setBrightness(0);
  tft.writecommand(TFT_DISPOFF);

  // Wake up on touch screen press (IRQ pin)
  // GPIO_NUM_36 corresponds to the standard TOUCH_IRQ on many ESP32 TFT boards
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, 0); // 0 = low level trigger

  // Set a timed wakeup if a schedule is enabled
  if (state.power.scheduledSleepEnabled) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      long long secondsToWake;
      long long currentSecondsPastMidnight = timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60 + timeinfo.tm_sec;
      long long wakeSecondsPastMidnight = state.power.scheduledWakeHour * 3600;

      if (wakeSecondsPastMidnight > currentSecondsPastMidnight) {
        secondsToWake = wakeSecondsPastMidnight - currentSecondsPastMidnight;
      } else { // Wakeup is on the next day
        secondsToWake = (24 * 3600 - currentSecondsPastMidnight) + wakeSecondsPastMidnight;
      }
      Serial.printf("Timed wakeup scheduled in %lld seconds.\n", secondsToWake);
      esp_sleep_enable_timer_wakeup(secondsToWake * 1000000ULL);
    }
  }

  Serial.println("Entering deep sleep.");
  esp_deep_sleep_start();
}

bool isWithinScheduledSleepWindow(const ApplicationState& state) {
  if (!state.power.scheduledSleepEnabled) {
    return false;
  }
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    int currentHour = timeinfo.tm_hour;
    int sleepHour = state.power.scheduledSleepHour;
    int wakeHour = state.power.scheduledWakeHour;

    // Overnight schedule (e.g., 23:00 to 07:00)
    if (wakeHour < sleepHour) {
      return (currentHour >= sleepHour || currentHour < wakeHour);
    } else { 
      // Same-day schedule (e.g., 10:00 to 17:00)
      return (currentHour >= sleepHour && currentHour < wakeHour);
    }
  }
  return false;
}

bool shouldEnterSleep(const ApplicationState& state) {
  // 1. Check inactivity timeout
  if (state.power.sleepTimeoutMinutes > 0) {
    if (millis() - state.power.lastInteractionTime > (unsigned long)state.power.sleepTimeoutMinutes * 60 * 1000UL) {
      Serial.println("Sleep condition met: inactivity timeout.");
      return true;
    }
  }
  // 2. Check scheduled window
  if (isWithinScheduledSleepWindow(state)) {
    Serial.println("Sleep condition met: within scheduled window.");
    return true;
  }
  return false;
}

// --- Main Setup ---

void setup() {
  Serial.begin(115200);

  // Initialize Display
  tft.init();
  loadSettings(applicationState); // Load settings early to get rotation
  tft.setRotation(applicationState.display.screenRotation);
  tft.fillScreen(TFT_BLACK);

  // Check Wakeup Cause
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) Serial.println("Wakeup by touch (EXT0).");
  else if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) Serial.println("Wakeup by timer.");
  else Serial.println("Normal start (Power On).");

  // Initialize Touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, TOUCH_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(applicationState.display.screenRotation);

  // Draw Splash Screen
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  String titleBold = "ESP32 Ham Combo"; 
  String titleRegular = " by HF7A";
  
  tft.setFreeFont(&FreeSansBold9pt7b); 
  int titleBoldWidth = tft.textWidth(titleBold);
  tft.setFreeFont(&FreeSans9pt7b); 
  int titleRegularWidth = tft.textWidth(titleRegular);
  
  int totalWidth = titleBoldWidth + titleRegularWidth; 
  int startX = (tft.width() - totalWidth) / 2;
  
  tft.setFreeFont(&FreeSansBold9pt7b); 
  tft.drawString(titleBold, startX, 20);
  tft.setFreeFont(&FreeSans9pt7b); 
  tft.drawString(titleRegular, startX + titleBoldWidth, 20);
  
  tft.drawFastHLine(10, 45, tft.width() - 20, TFT_DARKGREY);
  applicationState.startupScreenYPos = 75;

  // Connect to WiFi
  preferences.begin("wifi-creds", true);
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");
  preferences.end();

  if (ssid.length() > 0) {
    WiFi.begin(ssid.c_str(), password.c_str());
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("Connecting to " + ssid, tft.width() / 2, applicationState.startupScreenYPos - 5);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < WIFI_CONNECT_ATTEMPTS) {
      delay(WIFI_CONNECT_DELAY_MS);
      Serial.print(".");
      attempts++;
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nCould not connect to WiFi. Starting Configuration Portal.");
    startConfigurationPortal();
  }

  Serial.println("\nWiFi Connected!");
  applicationState.network.isWifiConnected = true;
  
  // Clear "Connecting..." message
  tft.fillRect(0, applicationState.startupScreenYPos - 15, tft.width(), 40, TFT_BLACK);
  
  updateStartupStatus("Connecting to WiFi", STATUS_SUCCESS, applicationState);
  updateStartupStatus("IP: " + WiFi.localIP().toString(), STATUS_SUCCESS, applicationState);

  updateStartupStatus("Initializing hardware", STATUS_IN_PROGRESS, applicationState);
  updateStartupStatus("Initializing hardware", STATUS_SUCCESS, applicationState);

  updateStartupStatus("Loading settings", STATUS_IN_PROGRESS, applicationState);
  loadCalibrationData(applicationState);
  tft.invertDisplay(applicationState.display.colorInversion); 
  updateStartupStatus("Loading settings", STATUS_SUCCESS, applicationState);

  setupAudio(applicationState);
  setBrightness(applicationState.display.brightnessPercent);
  
  applicationState.power.lastInteractionTime = millis();
  applicationState.lastPeriodicCheckTime = millis();
  secureClient.setInsecure(); // Allow self-signed certs for HTTPS
}

// --- State Machines ---

void handleInitialization() {
  switch (initState) {
    case INIT_BEGIN:
      updateStartupStatus("Starting web server", STATUS_IN_PROGRESS, applicationState);
      setupWebServer(applicationState);
      updateStartupStatus("Starting web server", STATUS_SUCCESS, applicationState);
      initState = INIT_SYNC_TIME;
      break;

    case INIT_SYNC_TIME:
      updateStartupStatus("Syncing time (NTP)", STATUS_IN_PROGRESS, applicationState);

      char fullTimezoneString[128];
      strlcpy(fullTimezoneString, applicationState.network.timezone, sizeof(fullTimezoneString));

      // Append DST rules
      switch (applicationState.network.dstMode) {
        case 1: strlcat(fullTimezoneString, ",M3.5.0,M10.5.0/3", sizeof(fullTimezoneString)); break; // EU
        case 2: strlcat(fullTimezoneString, ",M3.2.0,M11.1.0", sizeof(fullTimezoneString)); break;   // NA
        case 3: strlcat(fullTimezoneString, applicationState.network.customDstRule, sizeof(fullTimezoneString)); break; // Custom
      }

      Serial.print("Using full timezone string: "); Serial.println(fullTimezoneString);
      configTzTime(fullTimezoneString, NTP_SERVER);
      delay(1000);
      updateStartupStatus("Syncing time (NTP)", STATUS_SUCCESS, applicationState);

      // Check for updates
      if (applicationState.checkForUpdates && (millis() - applicationState.lastUpdateCheckTime > UPDATE_CHECK_INTERVAL_MS || applicationState.lastUpdateCheckTime == 0)) {
        updateStartupStatus("Checking for updates", STATUS_IN_PROGRESS, applicationState);
        if (checkGithubForUpdate(applicationState)) {
          updateStartupStatus("New version found!", STATUS_SUCCESS, applicationState);
        } else {
          updateStartupStatus("Checking for updates", STATUS_SUCCESS, applicationState);
        }
      }

      initState = INIT_FETCH_PROPAGATION;
      break;

    case INIT_FETCH_PROPAGATION:
      updateStartupStatus("Fetching propagation data", STATUS_IN_PROGRESS, applicationState);
      if (fetchPropagationData(applicationState)) {
        updateStartupStatus("Fetching propagation data", STATUS_SUCCESS, applicationState);
      } else {
        updateStartupStatus("Fetching propagation data", STATUS_FAILURE, applicationState);
      }
      initState = INIT_CONNECT_TELNET;
      break;

    case INIT_CONNECT_TELNET:
      updateStartupStatus("Logging into HamAlert", STATUS_IN_PROGRESS, applicationState);
      if (connectToTelnet(applicationState, false)) {
        updateStartupStatus("Logging into HamAlert", STATUS_SUCCESS, applicationState);
        applicationState.network.lastReconnectTime = millis();
        applicationState.network.hamAlertConnected = true;
      } else {
        updateStartupStatus("Logging into HamAlert", STATUS_FAILURE, applicationState);
        applicationState.network.hamAlertConnected = false;
      }
      initState = INIT_FINALIZE;
      break;

    case INIT_FINALIZE:
      delay(1000);
      // Check if we woke up during a sleep schedule
      if (isWithinScheduledSleepWindow(applicationState)) {
          applicationState.activeScreen = SCREEN_SLEEP_GRACE_PERIOD;
          applicationState.power.gracePeriodStartTime = millis();
          drawGracePeriodScreen(applicationState);
      } else {
          if (applicationState.network.hamAlertConnected) playNewSpotSound(applicationState);
          delay(500);
          determineAndDrawActiveScreen(applicationState);
      }
      initState = INIT_RUNNING;
      break;

    case INIT_RUNNING:
      break;
  }
}

void handlePeriodicTasks() {
  // 1. Check WiFi Connection
  bool isCurrentlyConnected = (WiFi.status() == WL_CONNECTED);
  if (isCurrentlyConnected != applicationState.network.isWifiConnected) {
    applicationState.network.isWifiConnected = isCurrentlyConnected;
    if (!isCurrentlyConnected) {
      Serial.println("WiFi connection lost. Attempting to reconnect...");
      telnetClient.stop();
      applicationState.network.hamAlertConnected = false;
    } else {
      Serial.println("WiFi connection restored.");
    }
    determineAndDrawActiveScreen(applicationState);
  }

  if (applicationState.network.isWifiConnected) {
    // 2. Update Propagation Data
    if (millis() - applicationState.lastPropUpdateTime > PROPAGATION_UPDATE_INTERVAL_MS) {
      if (fetchPropagationData(applicationState)) {
        if (applicationState.activeScreen == SCREEN_PROPAGATION || applicationState.activeScreen == SCREEN_SPOTS_AND_PROP) {
          determineAndDrawActiveScreen(applicationState);
        }
      }
    }
    // 3. Check for Updates
    if (applicationState.checkForUpdates && millis() - applicationState.lastUpdateCheckTime > UPDATE_CHECK_INTERVAL_MS) {
      Serial.println("Periodic update check...");
      checkGithubForUpdate(applicationState);
    }
  }

  // 4. Check Telnet Connection
  if ((applicationState.activeScreen == SCREEN_SPOTS || applicationState.activeScreen == SCREEN_SPOTS_AND_PROP) && applicationState.network.isWifiConnected) {
    if (!telnetClient.connected() || (millis() - applicationState.network.lastReconnectTime >= TELNET_RECONNECT_INTERVAL_MS)) {
      telnetClient.stop();
      clearSpots(applicationState); 
      applicationState.network.hamAlertConnected = connectToTelnet(applicationState, true);
      if(applicationState.network.hamAlertConnected) {
        determineAndDrawActiveScreen(applicationState);
      }
      applicationState.network.lastReconnectTime = millis();
    }
  }
}

void handleRuntime() {
  // Handle Calibration Request from Web UI
  if (applicationState.calibrationRequested) {
    runTouchCalibration(applicationState);
    applicationState.calibrationRequested = false;
    return;
  }

  // Run Periodic Tasks
  if (millis() - applicationState.lastPeriodicCheckTime > PERIODIC_CHECK_INTERVAL_MS) {
    applicationState.lastPeriodicCheckTime = millis();
    handlePeriodicTasks();
  }

  // Check Sleep Conditions (unless already in grace period or settings)
  if (applicationState.activeScreen != SCREEN_SLEEP_GRACE_PERIOD && applicationState.activeScreen != SCREEN_SLEEP_SETTINGS) {
    if (shouldEnterSleep(applicationState)) {
      applicationState.activeScreen = SCREEN_SLEEP_GRACE_PERIOD;
      applicationState.power.gracePeriodStartTime = millis();
      drawGracePeriodScreen(applicationState);
      return;
    }
  }

  // Screen-Specific Logic
  switch (applicationState.activeScreen) {
    case SCREEN_SPOTS:
    case SCREEN_SPOTS_AND_PROP:
      if (applicationState.network.isWifiConnected && telnetClient.connected()) {
        readTelnetSpots(applicationState);
      }
      if (millis() - applicationState.lastDisplayUpdateTime >= SPOT_LIST_UPDATE_INTERVAL_MS) {
        updateSpotTimesOnly(applicationState);
      }
      break;

    case SCREEN_CLOCK:
      if (millis() - applicationState.lastClockUpdateTime >= 1000) {
        drawClockScreen(applicationState);
        applicationState.lastClockUpdateTime = millis();
      }
      break;

    case SCREEN_SLEEP_GRACE_PERIOD:
      {
        unsigned long elapsed = millis() - applicationState.power.gracePeriodStartTime;
        if (elapsed >= SLEEP_GRACE_PERIOD_MS) {
          enterDeepSleep(applicationState);
        } else {
          // Update countdown every second
          if ((elapsed / 1000) != ((elapsed - 100) / 1000)) {
             int secondsLeft = (SLEEP_GRACE_PERIOD_MS - elapsed) / 1000;
             tft.setTextDatum(MC_DATUM);
             tft.setFreeFont(&FreeSansBold12pt7b);
             tft.setTextColor(TFT_YELLOW, TFT_BLACK);
             tft.fillRect(0, tft.height() / 2 + GRACE_PERIOD_TIMER_Y_OFFSET, tft.width(), GRACE_PERIOD_TIMER_HEIGHT, TFT_BLACK);
             tft.drawString(String(secondsLeft) + "s to sleep", tft.width() / 2, tft.height() / 2 + GRACE_PERIOD_TIMER_TEXT_Y_OFFSET);
          }
        }
      }
      break;

    default:
      break;
  }
}

// --- Main Loop ---

void loop() {
  handleTouch(applicationState);

  if (initState != INIT_RUNNING) {
    handleInitialization();
  } else {
    handleRuntime();
  }
}
