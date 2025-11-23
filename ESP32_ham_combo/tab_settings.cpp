/*
ESP32 Ham Combo
Copyright (c) 2025 Leszek (HF7A)
https://github.com/hf7a/ESP32-ham-combo

Licensed under CC BY-NC-SA 4.0.
Commercial use is prohibited.
*/

#include "declarations.h"

void saveSettings(const ApplicationState& state) {
  preferences.begin("app-settings", false); // Read-write mode

  // Display
  preferences.putInt("clockMode", state.display.currentClockMode);
  preferences.putInt("brightness", state.display.brightnessPercent);
  preferences.putInt("propViewMode", state.display.currentPropViewMode);
  preferences.putInt("spotsViewMode", state.display.spotsViewMode);
  preferences.putBool("inversion", state.display.colorInversion);
  preferences.putBool("secondDot", state.display.secondDotEnabled);
  preferences.putInt("rotation", state.display.screenRotation);
  preferences.putBool("rememberScreen", state.display.rememberLastScreen);
  preferences.putInt("startupScreen", state.display.startupScreen);

  // Audio
  preferences.putInt("volumeStep", state.audio.volumeStep);
  preferences.putInt("toneFreq", state.audio.toneFrequency);
  preferences.putInt("toneDur", state.audio.toneDurationMs);

  // Network & Credentials
  preferences.putString("telnetUser", state.network.telnetUsername);
  preferences.putString("telnetPass", state.network.telnetPassword);
  preferences.putString("timezone", state.network.timezone);
  preferences.putInt("dstMode", state.network.dstMode);
  preferences.putString("customDst", state.network.customDstRule);

  // Power
  preferences.putInt("sleepTimeout", state.power.sleepTimeoutMinutes);
  preferences.putBool("schedSleepOn", state.power.scheduledSleepEnabled);
  preferences.putInt("schedSleepH", state.power.scheduledSleepHour);
  preferences.putInt("schedWakeH", state.power.scheduledWakeHour);

  // System
  preferences.putBool("checkUpdates", state.checkForUpdates);
  preferences.putULong("lastUpdateCheck", state.lastUpdateCheckTime);

  preferences.end();
  Serial.println("Settings saved to flash memory.");
}

void loadSettings(ApplicationState& state) {
  preferences.begin("app-settings", true); // Read-only mode

  // Display
  state.display.currentClockMode = (ClockDisplayMode)preferences.getInt("clockMode", MODE_UTC);
  state.display.brightnessPercent = preferences.getInt("brightness", 80);
  state.display.currentPropViewMode = (PropagationViewMode)preferences.getInt("propViewMode", VIEW_EXTENDED);
  state.display.spotsViewMode = (SpotsViewMode)preferences.getInt("spotsViewMode", SPOTS_WITH_PROP);
  state.display.colorInversion = preferences.getBool("inversion", true);
  state.display.secondDotEnabled = preferences.getBool("secondDot", true);
  state.display.screenRotation = preferences.getInt("rotation", 3);
  state.display.rememberLastScreen = preferences.getBool("rememberScreen", false);
  state.display.startupScreen = (ActiveScreen)preferences.getInt("startupScreen", SCREEN_SPOTS);

  // Audio
  state.audio.volumeStep = preferences.getInt("volumeStep", 1); // Default to -18dB (quiet)
  state.audio.toneFrequency = preferences.getInt("toneFreq", 500);
  state.audio.toneDurationMs = preferences.getInt("toneDur", 50);

  // Network & Credentials
  String user = preferences.getString("telnetUser", DEFAULT_TELNET_USERNAME);
  strlcpy(state.network.telnetUsername, user.c_str(), sizeof(state.network.telnetUsername));

  String pass = preferences.getString("telnetPass", DEFAULT_TELNET_PASSWORD);
  strlcpy(state.network.telnetPassword, pass.c_str(), sizeof(state.network.telnetPassword));

  String tz = preferences.getString("timezone", DEFAULT_TIMEZONE);
  strlcpy(state.network.timezone, tz.c_str(), sizeof(state.network.timezone));

  state.network.dstMode = preferences.getInt("dstMode", 1); // Default to EU rules
  String customDst = preferences.getString("customDst", ",M3.5.0,M10.5.0/3");
  strlcpy(state.network.customDstRule, customDst.c_str(), sizeof(state.network.customDstRule));

  // Power
  state.power.sleepTimeoutMinutes = preferences.getInt("sleepTimeout", 0);
  state.power.scheduledSleepEnabled = preferences.getBool("schedSleepOn", false);
  state.power.scheduledSleepHour = preferences.getInt("schedSleepH", 23);
  state.power.scheduledWakeHour = preferences.getInt("schedWakeH", 7);

  // System
  state.checkForUpdates = preferences.getBool("checkUpdates", true);
  state.lastUpdateCheckTime = preferences.getULong("lastUpdateCheck", 0);

  preferences.end();
  Serial.println("Settings loaded from flash memory.");
}

void clearWiFiSettings() {
  preferences.begin("wifi-creds", false);
  preferences.clear();
  preferences.end();
  Serial.println("Wi-Fi settings have been cleared.");
}
