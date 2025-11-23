/*
  ESP32 Ham Combo
  Copyright (c) 2025 Leszek (HF7A)
  https://github.com/hf7a/ESP32-ham-combo
  
  Licensed under CC BY-NC-SA 4.0.
  Commercial use is prohibited.
*/

#include "declarations.h"

// --- Local Helper Functions for Settings Screens ---
namespace {
  // Helper function to convert volume step to a displayable dB string.
  String getVolumeDbString(int volumeStep) {
    const char* dbLevels[] = {"Muted", "-18 dB", "-12 dB", "-6 dB", "0 dB"};
    if (volumeStep >= 0 && volumeStep < 5) {
      return dbLevels[volumeStep];
    }
    return "Error";
  }

  void drawSliderControl(const String& label, int y, const String& valueText, int min_val, int max_val, int current_val) {
    int minus_btn_x = SETTINGS_CONTROL_X;
    int plus_btn_x = tft.width() - SETTINGS_TOUCH_W - 20;

    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextDatum(CL_DATUM);
    tft.setTextColor(TFT_WHITE);
    tft.drawString(label, SETTINGS_LABEL_X, y + SETTINGS_CONTROL_H / 2);

    // Draw Minus Button
    uint16_t minus_color = (current_val == -1 || current_val > min_val) ? COLOR_DARK_BLUE : TFT_DARKGREY;
    tft.fillRoundRect(minus_btn_x, y, SETTINGS_TOUCH_W, SETTINGS_CONTROL_H, BUTTON_CORNER_RADIUS, minus_color);

    tft.setFreeFont(&FreeSansBold12pt7b);
    tft.setTextDatum(CC_DATUM);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("-", minus_btn_x + SETTINGS_TOUCH_W / 2, y + SETTINGS_CONTROL_H / 2);

    // Draw Value Text
    tft.setFreeFont(&FreeSans9pt7b);
    tft.drawString(valueText, (minus_btn_x + SETTINGS_TOUCH_W + plus_btn_x) / 2, y + SETTINGS_CONTROL_H / 2);

    // Draw Plus Button
    uint16_t plus_color = (current_val == -1 || current_val < max_val) ? COLOR_DARK_BLUE : TFT_DARKGREY;
    tft.fillRoundRect(plus_btn_x, y, SETTINGS_TOUCH_W, SETTINGS_CONTROL_H, BUTTON_CORNER_RADIUS, plus_color);

    tft.setFreeFont(&FreeSansBold12pt7b);
    tft.drawString("+", plus_btn_x + SETTINGS_TOUCH_W / 2, y + SETTINGS_CONTROL_H / 2);
  }

  void drawToggleControl(const String& label, int y, bool enabled) {
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextDatum(CL_DATUM);
    tft.setTextColor(TFT_WHITE);
    tft.drawString(label, SETTINGS_LABEL_X, y + SETTINGS_CONTROL_H / 2);

    tft.setTextDatum(MC_DATUM);
    uint16_t color = enabled ? COLOR_DARK_GREEN : TFT_MAROON;
    String text = enabled ? "Enabled" : "Disabled";
    
    tft.fillRoundRect(SETTINGS_CONTROL_X, y, SETTINGS_BUTTON_W, SETTINGS_CONTROL_H, BUTTON_CORNER_RADIUS, color);
    tft.setTextColor(TFT_WHITE);
    tft.drawString(text, SETTINGS_CONTROL_X + SETTINGS_BUTTON_W / 2, y + SETTINGS_CONTROL_H / 2);
  }

  void drawMenuButton(const String& label, int y, int h, uint16_t color) {
    const int x = SETTINGS_MENU_BTN_X_MARGIN;
    const int w = tft.width() - 2 * x;

    tft.fillRoundRect(x, y, w, h, BUTTON_CORNER_RADIUS, color);
    tft.setTextColor(TFT_WHITE);
    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.setTextDatum(MC_DATUM);

    tft.drawString(">", x + w - SETTINGS_MENU_ARROW_X_MARGIN, y + h / 2);
    tft.setTextDatum(ML_DATUM);
    tft.drawString(label, x + SETTINGS_MENU_LABEL_X_MARGIN, y + h / 2);
  }

  void drawOnOffButton(const String& label, int y, int h, bool isOn) {
    const int x = SETTINGS_MENU_BTN_X_MARGIN;
    const int w = tft.width() - 2 * x;
    uint16_t statusColor = isOn ? TFT_GREEN : TFT_RED;
    String statusText = isOn ? "ON" : "OFF";

    tft.fillRoundRect(x, y, w, h, BUTTON_CORNER_RADIUS, COLOR_DARK_BLUE);
    tft.setFreeFont(&FreeSansBold9pt7b);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(statusColor);
    tft.drawString(statusText, x + w - SETTINGS_MENU_STATUS_X_MARGIN, y + h / 2);

    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(TFT_WHITE);
    tft.drawString(label, x + SETTINGS_MENU_LABEL_X_MARGIN, y + h / 2);
  }
}

// --- Settings Screens Drawing Functions ---

void drawSettingsMenuScreen(const ApplicationState& state) {
  tft.fillScreen(TFT_BLACK);
  
  drawMenuButton("Display Settings", SETTINGS_MENU_START_Y, SETTINGS_MENU_BTN_H, COLOR_DARK_BLUE);
  drawMenuButton("Audio Settings", SETTINGS_MENU_START_Y + (SETTINGS_MENU_BTN_H + SETTINGS_MENU_GAP), SETTINGS_MENU_BTN_H, COLOR_DARK_GREEN);
  drawMenuButton("Power Management", SETTINGS_MENU_START_Y + 2 * (SETTINGS_MENU_BTN_H + SETTINGS_MENU_GAP), SETTINGS_MENU_BTN_H, TFT_DARKCYAN);
  
  String systemButtonText = "System & Info";
  uint16_t systemButtonColor = COLOR_DARK_PURPLE;
  if (state.newVersionAvailable) {
    systemButtonText = "New Update Available!";
    systemButtonColor = COLOR_DARK_GREEN;
  }
  drawMenuButton(systemButtonText, SETTINGS_MENU_START_Y + 3 * (SETTINGS_MENU_BTN_H + SETTINGS_MENU_GAP), SETTINGS_MENU_BTN_H, systemButtonColor);
  
  drawButtons(state);
}

void drawDisplaySettingsScreen(const ApplicationState& state) {
  tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(&FreeSans9pt7b);
  
  // Clock Mode Button
  String clockButtonText;
  switch (state.display.currentClockMode) {
    case MODE_UTC: clockButtonText = "UTC"; break;
    case MODE_LOCAL: clockButtonText = "Local"; break;
    case MODE_BOTH: clockButtonText = "UTC + Local"; break;
  }
  tft.setTextDatum(CL_DATUM); tft.setTextColor(TFT_WHITE);
  tft.drawString("Clock:", SETTINGS_LABEL_X, SETTINGS_ROW1_Y + SETTINGS_CONTROL_H / 2);
  tft.setTextDatum(MC_DATUM);
  tft.fillRoundRect(SETTINGS_CONTROL_X, SETTINGS_ROW1_Y, SETTINGS_BUTTON_W, SETTINGS_CONTROL_H, BUTTON_CORNER_RADIUS, COLOR_DARK_BLUE);
  tft.setTextColor(TFT_WHITE);
  tft.drawString(clockButtonText, SETTINGS_CONTROL_X + SETTINGS_BUTTON_W / 2, SETTINGS_ROW1_Y + SETTINGS_CONTROL_H / 2);
  
  // Spots View Mode Button
  String spotsViewText = (state.display.spotsViewMode == SPOTS_ONLY) ? "6 Spots" : "5 Spots + Prop.";
  tft.setTextDatum(CL_DATUM); tft.setTextColor(TFT_WHITE);
  tft.drawString("Spots View:", SETTINGS_LABEL_X, SETTINGS_ROW2_Y + SETTINGS_CONTROL_H / 2);
  tft.setTextDatum(MC_DATUM);
  tft.fillRoundRect(SETTINGS_CONTROL_X, SETTINGS_ROW2_Y, SETTINGS_BUTTON_W, SETTINGS_CONTROL_H, BUTTON_CORNER_RADIUS, COLOR_DARK_BLUE);
  tft.setTextColor(TFT_WHITE);
  tft.drawString(spotsViewText, SETTINGS_CONTROL_X + SETTINGS_BUTTON_W / 2, SETTINGS_ROW2_Y + SETTINGS_CONTROL_H / 2);
  
  // Propagation View Mode Button
  String propButtonText = (state.display.currentPropViewMode == VIEW_SIMPLE) ? "Simple" : "Extended";
  tft.setTextDatum(CL_DATUM); tft.setTextColor(TFT_WHITE);
  tft.drawString("Propagation:", SETTINGS_LABEL_X, SETTINGS_ROW3_Y + SETTINGS_CONTROL_H / 2);
  tft.setTextDatum(MC_DATUM);
  tft.fillRoundRect(SETTINGS_CONTROL_X, SETTINGS_ROW3_Y, SETTINGS_BUTTON_W, SETTINGS_CONTROL_H, BUTTON_CORNER_RADIUS, COLOR_DARK_BLUE);
  tft.setTextColor(TFT_WHITE);
  tft.drawString(propButtonText, SETTINGS_CONTROL_X + SETTINGS_BUTTON_W / 2, SETTINGS_ROW3_Y + SETTINGS_CONTROL_H / 2);
  
  // Sliders and Toggles
  drawSliderControl("Brightness:", SETTINGS_ROW4_Y, String(state.display.brightnessPercent) + "%", 10, 100, state.display.brightnessPercent);
  drawToggleControl("Invert Colors:", SETTINGS_ROW5_Y, state.display.colorInversion);
  
  drawButtons(state);
}

void drawAudioSettingsScreen(const ApplicationState& state) {
  tft.fillScreen(TFT_BLACK);
  drawSliderControl("Volume:", SETTINGS_ROW1_Y, getVolumeDbString(state.audio.volumeStep), 0, 4, state.audio.volumeStep);
  drawSliderControl("Tone Freq:", SETTINGS_ROW2_Y, String(state.audio.toneFrequency) + " Hz", 300, 1400, state.audio.toneFrequency);
  drawSliderControl("Tone Duration:", SETTINGS_ROW3_Y, String(state.audio.toneDurationMs) + " ms", 50, 125, state.audio.toneDurationMs);
  drawButtons(state);
}

void drawSystemSettingsScreen(const ApplicationState& state) {
  tft.fillScreen(TFT_BLACK);
  
  drawMenuButton("Device Info", SETTINGS_ROW1_Y, SETTINGS_CONTROL_H, COLOR_DARK_BLUE);
  drawMenuButton("Calibrate Touch", SETTINGS_ROW2_Y, SETTINGS_CONTROL_H, COLOR_DARK_BLUE);
  drawOnOffButton("Remember Screen:", SETTINGS_ROW3_Y, SETTINGS_CONTROL_H, state.display.rememberLastScreen);
  
  String updatesButtonText = "Updates & License";
  uint16_t updatesButtonColor = COLOR_DARK_BLUE;
  if (state.newVersionAvailable) {
    updatesButtonText = "New Update Available!";
    updatesButtonColor = COLOR_DARK_GREEN;
  }
  drawMenuButton(updatesButtonText, SETTINGS_ROW4_Y, SETTINGS_CONTROL_H, updatesButtonColor);
  
  drawMenuButton("Reset Wi-Fi Settings", SETTINGS_ROW5_Y, SETTINGS_CONTROL_H, TFT_MAROON);
  
  drawButtons(state);
}

void drawSleepSettingsScreen(const ApplicationState& state) {
    tft.fillScreen(TFT_BLACK);
    
    String inactivityText = (state.power.sleepTimeoutMinutes == 0) ? "Off" : String(state.power.sleepTimeoutMinutes / 60) + " h";
    drawSliderControl("Inactivity:", SETTINGS_ROW1_Y, inactivityText, 0, 720, state.power.sleepTimeoutMinutes);
    
    drawToggleControl("Schedule:", SETTINGS_ROW2_Y, state.power.scheduledSleepEnabled);
    
    if (state.power.scheduledSleepEnabled) {
        drawSliderControl("Sleep at:", SETTINGS_ROW3_Y, String(state.power.scheduledSleepHour) + ":00", 0, 23, state.power.scheduledSleepHour);
        drawSliderControl("Wake at:", SETTINGS_ROW4_Y, String(state.power.scheduledWakeHour) + ":00", 0, 23, state.power.scheduledWakeHour);
    }
    
    drawButtons(state);
}

void drawInfoScreen(const ApplicationState& state) {
  tft.fillScreen(TFT_BLACK); 
  tft.setTextDatum(TL_DATUM); 
  tft.setFreeFont(&FreeSans9pt7b);
  
  int yPos = INFO_SCREEN_START_Y;
  
  // Hardware Info
  tft.setTextColor(TFT_WHITE); tft.drawString("Chip:", INFO_SCREEN_LABEL_X, yPos); 
  tft.setTextColor(TFT_CYAN); tft.drawString(ESP.getChipModel(), INFO_SCREEN_VALUE_X, yPos); 
  yPos += INFO_SCREEN_LINE_GAP;
  
  String cpuInfo = String(ESP.getCpuFreqMHz()) + " MHz"; 
  tft.setTextColor(TFT_WHITE); tft.drawString("CPU:", INFO_SCREEN_LABEL_X, yPos); 
  tft.setTextColor(TFT_CYAN); tft.drawString(cpuInfo, INFO_SCREEN_VALUE_X, yPos); 
  yPos += INFO_SCREEN_LINE_GAP;
  
  String memInfo = String(ESP.getFreeHeap() / 1024) + " kB"; 
  tft.setTextColor(TFT_WHITE); tft.drawString("Free RAM:", INFO_SCREEN_LABEL_X, yPos); 
  tft.setTextColor(TFT_CYAN); tft.drawString(memInfo, INFO_SCREEN_VALUE_X, yPos); 
  yPos += INFO_SCREEN_LINE_GAP;
  
  // Separator
  yPos += INFO_SCREEN_SEPARATOR_GAP_BEFORE; 
  tft.drawFastHLine(10, yPos, tft.width() - 20, TFT_DARKGREY); 
  yPos += INFO_SCREEN_SEPARATOR_GAP_AFTER;
  
  // Network Info
  if (state.network.isWifiConnected) {
    tft.setTextColor(TFT_WHITE); tft.drawString("SSID:", INFO_SCREEN_LABEL_X, yPos); 
    tft.setTextColor(TFT_CYAN); tft.drawString(WiFi.SSID(), INFO_SCREEN_VALUE_X, yPos); 
    yPos += INFO_SCREEN_LINE_GAP;
    
    tft.setTextColor(TFT_WHITE); tft.drawString("IP:", INFO_SCREEN_LABEL_X, yPos); 
    tft.setTextColor(TFT_CYAN); tft.drawString(WiFi.localIP().toString(), INFO_SCREEN_VALUE_X, yPos); 
    yPos += INFO_SCREEN_LINE_GAP;
    
    String rssiInfo = String(WiFi.RSSI()) + " dBm"; 
    tft.setTextColor(TFT_WHITE); tft.drawString("Signal:", INFO_SCREEN_LABEL_X, yPos); 
    tft.setTextColor(TFT_CYAN); tft.drawString(rssiInfo, INFO_SCREEN_VALUE_X, yPos); 
    yPos += INFO_SCREEN_LINE_GAP;
    
    tft.setTextColor(TFT_WHITE); tft.drawString("HamAlert:", INFO_SCREEN_LABEL_X, yPos);
    if (telnetClient.connected()) { 
        tft.setTextColor(TFT_GREEN); tft.drawString("Connected", INFO_SCREEN_VALUE_X, yPos); 
    } else { 
        tft.setTextColor(TFT_RED); tft.drawString("Disconnected", INFO_SCREEN_VALUE_X, yPos); 
    } 
    yPos += INFO_SCREEN_LINE_GAP;
  } else {
    tft.setTextColor(TFT_WHITE); tft.drawString("WiFi Status:", INFO_SCREEN_LABEL_X, yPos); 
    tft.setTextColor(TFT_RED); tft.drawString("Disconnected", INFO_SCREEN_VALUE_X, yPos); 
    yPos += INFO_SCREEN_LINE_GAP;
  }
  
  // Uptime
  unsigned long seconds = millis() / 1000;
  int days = seconds / 86400; 
  int hours = (seconds % 86400) / 3600; 
  int minutes = (seconds % 3600) / 60;
  char uptimeBuffer[UPTIME_BUFFER_SIZE]; 
  snprintf(uptimeBuffer, sizeof(uptimeBuffer), "%dd %02dh %02dm", days, hours, minutes);
  
  tft.setTextColor(TFT_WHITE); tft.drawString("Uptime:", INFO_SCREEN_LABEL_X, yPos); 
  tft.setTextColor(TFT_CYAN); tft.drawString(uptimeBuffer, INFO_SCREEN_VALUE_X, yPos);
}

void drawUpdatesScreen(const ApplicationState& state) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setFreeFont(&FreeSans9pt7b);
  
  int yPos = 20;
  int xLabel = 15;
  
  if (state.newVersionAvailable) {
    tft.setTextColor(TFT_GREEN);
    tft.drawString("New Version Available:", xLabel, yPos);
    tft.setTextColor(TFT_YELLOW);
    tft.drawString(state.newVersionTag, xLabel + 180, yPos);
  } else {
    tft.setTextColor(TFT_GREENYELLOW);
    tft.drawString("Your software is up to date.", xLabel, yPos);
  }
  
  yPos += 25;
  tft.drawFastHLine(10, yPos, tft.width() - 20, TFT_DARKGREY);
  yPos += 15;
  
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Get updates at:", xLabel, yPos);
  yPos += 20;
  tft.setTextColor(TFT_CYAN);
  tft.drawString(PROJECT_URL, xLabel, yPos);
  
  yPos += 30;
  int xValue = 100;
  tft.setTextColor(TFT_WHITE); tft.drawString("Version:", xLabel, yPos); 
  tft.setTextColor(TFT_CYAN); tft.drawString(FW_VERSION, xValue, yPos); 
  yPos += 20;

  tft.setTextColor(TFT_WHITE); tft.drawString("Date:", xLabel, yPos); 
  tft.setTextColor(TFT_CYAN); tft.drawString(FW_DATE, xValue, yPos); 
  yPos += 20;
  
  tft.setTextColor(TFT_WHITE); tft.drawString("Author:", xLabel, yPos); 
  tft.setTextColor(TFT_CYAN); tft.drawString("Leszek HF7A", xValue, yPos); 
  yPos += 20;
  
  tft.setTextColor(TFT_WHITE); tft.drawString("License:", xLabel, yPos); 
  tft.setTextColor(TFT_CYAN); tft.drawString("CC BY-NC-SA 4.0", xValue, yPos);
  
  const int updatesBtnY = tft.height() - SETTINGS_CONTROL_H - SETTINGS_V_GAP;
  drawOnOffButton("Check for Updates:", updatesBtnY, SETTINGS_CONTROL_H, state.checkForUpdates);
}

void drawWifiResetConfirmScreen(const ApplicationState& state) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(&FreeSans9pt7b);

  int yPos = 40;
  tft.setTextColor(TFT_YELLOW);
  tft.drawString("This will clear all saved", tft.width() / 2, yPos); yPos += 20;
  tft.drawString("Wi-Fi networks.", tft.width() / 2, yPos); yPos += 30;

  tft.setTextColor(TFT_WHITE);
  tft.drawString("The device will restart and", tft.width() / 2, yPos); yPos += 20;
  tft.drawString("enter configuration mode.", tft.width() / 2, yPos); yPos += 30;

  tft.setTextColor(TFT_RED);
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.drawString("Are you sure?", tft.width() / 2, yPos);

  const int BTN_Y = tft.height() - CALIBRATION_BTN_H - CALIBRATION_BTN_Y_MARGIN;
  const int TOTAL_W = CALIBRATION_BTN_W * 2 + CALIBRATION_BTN_GAP;
  const int START_X = (tft.width() - TOTAL_W) / 2;

  const int CANCEL_BTN_X = START_X;
  const int CONFIRM_BTN_X = START_X + CALIBRATION_BTN_W + CALIBRATION_BTN_GAP;

  tft.setFreeFont(&FreeSans9pt7b);
  tft.fillRoundRect(CANCEL_BTN_X, BTN_Y, CALIBRATION_BTN_W, CALIBRATION_BTN_H, BUTTON_CORNER_RADIUS, COLOR_DARK_GREEN);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Cancel", CANCEL_BTN_X + CALIBRATION_BTN_W / 2, BTN_Y + CALIBRATION_BTN_H / 2);

  tft.fillRoundRect(CONFIRM_BTN_X, BTN_Y, CALIBRATION_BTN_W, CALIBRATION_BTN_H, BUTTON_CORNER_RADIUS, TFT_MAROON);
  tft.drawString("Confirm Reset", CONFIRM_BTN_X + CALIBRATION_BTN_W / 2, BTN_Y + CALIBRATION_BTN_H / 2);
}