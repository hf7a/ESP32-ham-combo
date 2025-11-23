/*
ESP32 Ham Combo
Copyright (c) 2025 Leszek (HF7A)
https://github.com/hf7a/ESP32-ham-combo

Licensed under CC BY-NC-SA 4.0.
Commercial use is prohibited.
*/

#include <Arduino.h>
#include "declarations.h"

namespace {
  // Maps a volume step (0-4) to the hardware attenuation levels.
  dac_cosine_atten_t getDacAttenuation(int volumeStep) {
    switch (volumeStep) {
      case 4: return DAC_COSINE_ATTEN_DB_0;   // 0 dB
      case 3: return DAC_COSINE_ATTEN_DB_6;   // -6 dB
      case 2: return DAC_COSINE_ATTEN_DB_12;  // -12 dB
      case 1: return DAC_COSINE_ATTEN_DB_18;  // -18 dB
      default: return DAC_COSINE_ATTEN_DB_0;
    }
  }
}

// --- UI Color Helper Functions ---

uint16_t getPropagationColor(PropagationCondition propValue) {
  switch (propValue) {
    case POOR: return TFT_RED;
    case FAIR: return TFT_YELLOW;
    case GOOD: return TFT_DARKGREEN;
    default:   return TFT_WHITE;
  }
}

uint16_t getSolarFluxColor(int sfi) {
  if (sfi >= 172) return TFT_GREEN;  // Good to Best
  if (sfi >= 124) return TFT_YELLOW; // Average
  if (sfi >= 83)  return TFT_ORANGE; // Low
  return TFT_RED;                    // Bad
}

uint16_t getAIndexColor(int aIndex) {
  if (aIndex >= 48) return TFT_RED;    // Bad
  if (aIndex >= 16) return TFT_ORANGE; // Poor
  if (aIndex >= 8)  return TFT_YELLOW; // Average
  return TFT_GREEN;                    // Best
}

uint16_t getKIndexColor(int kIndex) {
  if (kIndex >= 7) return TFT_RED;    // Bad
  if (kIndex >= 4) return TFT_ORANGE; // Poor
  if (kIndex >= 2) return TFT_YELLOW; // Average
  return TFT_GREEN;                   // Best
}

uint16_t getXRayColor(const char* xray) {
    if (strlen(xray) < 1) return TFT_GREEN;
    switch (xray[0]) {
        case 'X': return TFT_RED;
        case 'M': return TFT_ORANGE;
        case 'A':
        case 'B':
        case 'C':
        default:  return TFT_GREEN;
    }
}

uint16_t getGeomagFieldColor(const char* geomag_field) {
  if (strcasecmp(geomag_field, "QUIET") == 0 || strcasecmp(geomag_field, "VR QUIET") == 0) {
    return TFT_GREEN;
  }
  if (strcasecmp(geomag_field, "STORM") == 0) {
    return TFT_RED;
  }
  return TFT_YELLOW;
}

uint16_t getSignalNoiseColor(const char* signal_noise_level) {
  if (strncmp(signal_noise_level, "S0", 2) == 0 || strncmp(signal_noise_level, "S1", 2) == 0 || strncmp(signal_noise_level, "S2", 2) == 0) {
    return TFT_GREEN;
  }
  if (strncmp(signal_noise_level, "S3", 2) == 0 || strncmp(signal_noise_level, "S4", 2) == 0 || strncmp(signal_noise_level, "S5", 2) == 0 || strncmp(signal_noise_level, "S6", 2) == 0) {
    return TFT_YELLOW;
  }
  return TFT_RED;
}

uint16_t getVhfConditionsColor(const char* vhf_condition) {
  if (strcmp(vhf_condition, "Band Closed") == 0) {
    return TFT_MAROON;
  }
  return TFT_GREEN;
}

// --- Core UI & Hardware Functions ---

void setBrightness(int percent) {
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  uint32_t dutyCycle = map(percent, 0, 100, 0, 255);
  analogWrite(TFT_BL, dutyCycle);
}

void setupAudio(ApplicationState& state) {
  if (state.audio.cos_handle != nullptr) {
    dac_cosine_del_channel(state.audio.cos_handle);
    state.audio.cos_handle = nullptr;
  }
  if (state.audio.volumeStep == 0) { // Muted
    return;
  }
  dac_cosine_config_t cos_cfg;
  cos_cfg.chan_id = DAC_CHAN_1;
  cos_cfg.freq_hz = (uint32_t)state.audio.toneFrequency;
  cos_cfg.clk_src = DAC_COSINE_CLK_SRC_DEFAULT;
  cos_cfg.offset = 0;
  cos_cfg.phase = DAC_COSINE_PHASE_0;
  cos_cfg.atten = getDacAttenuation(state.audio.volumeStep);
  cos_cfg.flags.force_set_freq = false;
  
  esp_err_t result = dac_cosine_new_channel(&cos_cfg, &state.audio.cos_handle);
  if (result != ESP_OK) {
    Serial.printf("Failed to create DAC cosine channel: %s\n", esp_err_to_name(result));
    state.audio.cos_handle = nullptr;
  }
}

void playNewSpotSound(const ApplicationState& state) {
  if (state.audio.volumeStep == 0 || state.audio.cos_handle == nullptr) {
    return;
  }
  dac_cosine_start(state.audio.cos_handle);
  delay(state.audio.toneDurationMs);
  dac_cosine_stop(state.audio.cos_handle);
}

// --- Touch Handling Helper Functions ---

// Helper to check if a touch coordinate falls within a rectangular area.
bool isButtonTouched(uint16_t tx, uint16_t ty, int x, int y, int w, int h) {
    return (tx >= x && tx <= (x + w) && ty >= y && ty <= (y + h));
}

static void handleTouchSpotsScreen(ApplicationState& state, uint16_t t_x, uint16_t t_y) {
    if (isButtonTouched(t_x, t_y, CLOCK_BTN_X, BUTTON_Y, BUTTON_W, BUTTON_H)) {
        state.activeScreen = SCREEN_CLOCK;
        if (state.display.rememberLastScreen) { state.display.startupScreen = SCREEN_CLOCK; saveSettings(state); }
        state.lastSecond = -1;
        drawClockScreen(state);
        state.lastClockUpdateTime = millis();
    } else if (isButtonTouched(t_x, t_y, PROP_BTN_X, BUTTON_Y, BUTTON_W, BUTTON_H)) {
        state.activeScreen = SCREEN_PROPAGATION;
        if (state.display.rememberLastScreen) { state.display.startupScreen = SCREEN_PROPAGATION; saveSettings(state); }
        drawPropagationScreen(state);
    } else if (isButtonTouched(t_x, t_y, SETUP_BTN_X, BUTTON_Y, BUTTON_W, BUTTON_H)) {
        state.activeScreen = SCREEN_SETTINGS_MENU;
        drawSettingsMenuScreen(state);
    } else if (isButtonTouched(t_x, t_y, SLEEP_NOW_BTN_X, BUTTON_Y, BUTTON_W, BUTTON_H)) {
        enterDeepSleep(state);
    }
}

static void handleTouchSettingsMenu(ApplicationState& state, uint16_t t_x, uint16_t t_y) {
    if (isButtonTouched(t_x, t_y, SETUP_BTN_X, BUTTON_Y, BUTTON_W, BUTTON_H)) {
        determineAndDrawActiveScreen(state);
    } else {
        // Check menu items
        const int x = SETTINGS_MENU_BTN_X_MARGIN;
        const int w = tft.width() - 2 * x;
        
        if (isButtonTouched(t_x, t_y, x, SETTINGS_MENU_START_Y, w, SETTINGS_MENU_BTN_H)) {
            state.activeScreen = SCREEN_DISPLAY_SETTINGS;
            drawDisplaySettingsScreen(state);
        } else if (isButtonTouched(t_x, t_y, x, SETTINGS_MENU_START_Y + (SETTINGS_MENU_BTN_H + SETTINGS_MENU_GAP), w, SETTINGS_MENU_BTN_H)) {
            state.activeScreen = SCREEN_AUDIO_SETTINGS;
            drawAudioSettingsScreen(state);
        } else if (isButtonTouched(t_x, t_y, x, SETTINGS_MENU_START_Y + 2*(SETTINGS_MENU_BTN_H + SETTINGS_MENU_GAP), w, SETTINGS_MENU_BTN_H)) {
            state.activeScreen = SCREEN_SLEEP_SETTINGS;
            drawSleepSettingsScreen(state);
        } else if (isButtonTouched(t_x, t_y, x, SETTINGS_MENU_START_Y + 3*(SETTINGS_MENU_BTN_H + SETTINGS_MENU_GAP), w, SETTINGS_MENU_BTN_H)) {
            state.activeScreen = SCREEN_SYSTEM_SETTINGS;
            drawSystemSettingsScreen(state);
        }
    }
}

static void handleTouchDisplaySettings(ApplicationState& state, uint16_t t_x, uint16_t t_y) {
    if (isButtonTouched(t_x, t_y, SETUP_BTN_X, BUTTON_Y, BUTTON_W, BUTTON_H)) {
        state.activeScreen = SCREEN_SETTINGS_MENU;
        drawSettingsMenuScreen(state);
        return;
    }

    // Row 1: Clock Mode
    if (isButtonTouched(t_x, t_y, SETTINGS_CONTROL_X, SETTINGS_ROW1_Y, SETTINGS_BUTTON_W, SETTINGS_CONTROL_H)) {
        state.display.currentClockMode = (ClockDisplayMode)((state.display.currentClockMode + 1) % 3);
        state.lastSecond = -1;
        saveSettings(state);
        drawDisplaySettingsScreen(state);
    } 
    // Row 2: Spots View Mode
    else if (isButtonTouched(t_x, t_y, SETTINGS_CONTROL_X, SETTINGS_ROW2_Y, SETTINGS_BUTTON_W, SETTINGS_CONTROL_H)) {
        state.display.spotsViewMode = (state.display.spotsViewMode == SPOTS_ONLY) ? SPOTS_WITH_PROP : SPOTS_ONLY;
        saveSettings(state);
        drawDisplaySettingsScreen(state);
    } 
    // Row 3: Propagation View Mode
    else if (isButtonTouched(t_x, t_y, SETTINGS_CONTROL_X, SETTINGS_ROW3_Y, SETTINGS_BUTTON_W, SETTINGS_CONTROL_H)) {
        state.display.currentPropViewMode = (state.display.currentPropViewMode == VIEW_SIMPLE) ? VIEW_EXTENDED : VIEW_SIMPLE;
        saveSettings(state);
        drawDisplaySettingsScreen(state);
    } 
    // Row 4: Brightness (- / +)
    else if (t_y >= SETTINGS_ROW4_Y && t_y <= (SETTINGS_ROW4_Y + SETTINGS_CONTROL_H)) {
        int minus_x = SETTINGS_CONTROL_X;
        int plus_x = tft.width() - SETTINGS_TOUCH_W - 20;
        
        if (isButtonTouched(t_x, t_y, minus_x, SETTINGS_ROW4_Y, SETTINGS_TOUCH_W, SETTINGS_CONTROL_H)) {
            if (state.display.brightnessPercent > 10) {
                state.display.brightnessPercent -= 10;
                setBrightness(state.display.brightnessPercent);
                saveSettings(state);
                drawDisplaySettingsScreen(state);
            }
        } else if (isButtonTouched(t_x, t_y, plus_x, SETTINGS_ROW4_Y, SETTINGS_TOUCH_W, SETTINGS_CONTROL_H)) {
            if (state.display.brightnessPercent < 100) {
                state.display.brightnessPercent += 10;
                setBrightness(state.display.brightnessPercent);
                saveSettings(state);
                drawDisplaySettingsScreen(state);
            }
        }
    } 
    // Row 5: Color Inversion
    else if (isButtonTouched(t_x, t_y, SETTINGS_CONTROL_X, SETTINGS_ROW5_Y, SETTINGS_BUTTON_W, SETTINGS_CONTROL_H)) {
        state.display.colorInversion = !state.display.colorInversion;
        tft.invertDisplay(state.display.colorInversion);
        saveSettings(state);
        drawDisplaySettingsScreen(state);
    }
}

static void handleTouchAudioSettings(ApplicationState& state, uint16_t t_x, uint16_t t_y) {
    if (isButtonTouched(t_x, t_y, SETUP_BTN_X, BUTTON_Y, BUTTON_W, BUTTON_H)) {
        state.activeScreen = SCREEN_SETTINGS_MENU;
        drawSettingsMenuScreen(state);
        return;
    }

    int minus_x = SETTINGS_CONTROL_X;
    int plus_x = tft.width() - SETTINGS_TOUCH_W - 20;

    // Row 1: Volume
    if (t_y >= SETTINGS_ROW1_Y && t_y <= (SETTINGS_ROW1_Y + SETTINGS_CONTROL_H)) {
        if (isButtonTouched(t_x, t_y, minus_x, SETTINGS_ROW1_Y, SETTINGS_TOUCH_W, SETTINGS_CONTROL_H)) {
            if (state.audio.volumeStep > 0) {
                state.audio.volumeStep--;
                saveSettings(state);
                setupAudio(state);
                drawAudioSettingsScreen(state);
                playNewSpotSound(state);
            }
        } else if (isButtonTouched(t_x, t_y, plus_x, SETTINGS_ROW1_Y, SETTINGS_TOUCH_W, SETTINGS_CONTROL_H)) {
            if (state.audio.volumeStep < 4) {
                state.audio.volumeStep++;
                saveSettings(state);
                setupAudio(state);
                drawAudioSettingsScreen(state);
                playNewSpotSound(state);
            }
        }
    }
    // Row 2: Tone Frequency
    else if (t_y >= SETTINGS_ROW2_Y && t_y <= (SETTINGS_ROW2_Y + SETTINGS_CONTROL_H)) {
        if (isButtonTouched(t_x, t_y, minus_x, SETTINGS_ROW2_Y, SETTINGS_TOUCH_W, SETTINGS_CONTROL_H)) {
            if (state.audio.toneFrequency > 300) {
                state.audio.toneFrequency -= 100;
                saveSettings(state);
                setupAudio(state);
                drawAudioSettingsScreen(state);
                playNewSpotSound(state);
            }
        } else if (isButtonTouched(t_x, t_y, plus_x, SETTINGS_ROW2_Y, SETTINGS_TOUCH_W, SETTINGS_CONTROL_H)) {
            if (state.audio.toneFrequency < 1400) {
                state.audio.toneFrequency += 100;
                saveSettings(state);
                setupAudio(state);
                drawAudioSettingsScreen(state);
                playNewSpotSound(state);
            }
        }
    }
    // Row 3: Tone Duration
    else if (t_y >= SETTINGS_ROW3_Y && t_y <= (SETTINGS_ROW3_Y + SETTINGS_CONTROL_H)) {
        const int durationSteps[] = {50, 75, 100, 125};
        const int numSteps = sizeof(durationSteps) / sizeof(durationSteps[0]);
        int currentStep = -1;
        for (int i = 0; i < numSteps; i++) {
            if (state.audio.toneDurationMs <= durationSteps[i]) {
                currentStep = i;
                break;
            }
        }
        if (currentStep == -1) currentStep = numSteps - 1;

        bool changed = false;
        if (isButtonTouched(t_x, t_y, minus_x, SETTINGS_ROW3_Y, SETTINGS_TOUCH_W, SETTINGS_CONTROL_H)) {
            if (currentStep > 0) {
                state.audio.toneDurationMs = durationSteps[currentStep - 1];
                changed = true;
            }
        } else if (isButtonTouched(t_x, t_y, plus_x, SETTINGS_ROW3_Y, SETTINGS_TOUCH_W, SETTINGS_CONTROL_H)) {
            if (currentStep < numSteps - 1) {
                state.audio.toneDurationMs = durationSteps[currentStep + 1];
                changed = true;
            }
        }

        if (changed) {
            saveSettings(state);
            drawAudioSettingsScreen(state);
            playNewSpotSound(state);
        }
    }
}

static void handleTouchSystemSettings(ApplicationState& state, uint16_t t_x, uint16_t t_y) {
    if (isButtonTouched(t_x, t_y, SETUP_BTN_X, BUTTON_Y, BUTTON_W, BUTTON_H)) {
        state.activeScreen = SCREEN_SETTINGS_MENU;
        drawSettingsMenuScreen(state);
        return;
    }

    const int x = SETTINGS_MENU_BTN_X_MARGIN;
    const int w = tft.width() - 2 * x;

    if (isButtonTouched(t_x, t_y, x, SETTINGS_ROW1_Y, w, SETTINGS_CONTROL_H)) {
        state.activeScreen = SCREEN_INFO;
        drawInfoScreen(state);
    } else if (isButtonTouched(t_x, t_y, x, SETTINGS_ROW2_Y, w, SETTINGS_CONTROL_H)) {
        runTouchCalibration(state);
    } else if (isButtonTouched(t_x, t_y, x, SETTINGS_ROW3_Y, w, SETTINGS_CONTROL_H)) {
        state.display.rememberLastScreen = !state.display.rememberLastScreen;
        saveSettings(state);
        drawSystemSettingsScreen(state);
    } else if (isButtonTouched(t_x, t_y, x, SETTINGS_ROW4_Y, w, SETTINGS_CONTROL_H)) {
        state.activeScreen = SCREEN_UPDATES_INFO;
        drawUpdatesScreen(state);
    } else if (isButtonTouched(t_x, t_y, x, SETTINGS_ROW5_Y, w, SETTINGS_CONTROL_H)) {
        state.activeScreen = SCREEN_WIFI_RESET_CONFIRM;
        drawWifiResetConfirmScreen(state);
    }
}

static void handleTouchSleepSettings(ApplicationState& state, uint16_t t_x, uint16_t t_y) {
    if (isButtonTouched(t_x, t_y, SETUP_BTN_X, BUTTON_Y, BUTTON_W, BUTTON_H)) {
        state.activeScreen = SCREEN_SETTINGS_MENU;
        drawSettingsMenuScreen(state);
        return;
    }

    int minus_x = SETTINGS_CONTROL_X;
    int plus_x = tft.width() - SETTINGS_TOUCH_W - 20;

    // Row 1: Sleep Timeout
    if (t_y >= SETTINGS_ROW1_Y && t_y <= (SETTINGS_ROW1_Y + SETTINGS_CONTROL_H)) {
        if (isButtonTouched(t_x, t_y, minus_x, SETTINGS_ROW1_Y, SETTINGS_TOUCH_W, SETTINGS_CONTROL_H)) {
            if (state.power.sleepTimeoutMinutes > 0) {
                state.power.sleepTimeoutMinutes -= 60;
                if(state.power.sleepTimeoutMinutes < 0) state.power.sleepTimeoutMinutes = 0;
                saveSettings(state);
                drawSleepSettingsScreen(state);
            }
        } else if (isButtonTouched(t_x, t_y, plus_x, SETTINGS_ROW1_Y, SETTINGS_TOUCH_W, SETTINGS_CONTROL_H)) {
            if (state.power.sleepTimeoutMinutes < 720) {
                state.power.sleepTimeoutMinutes += 60;
                saveSettings(state);
                drawSleepSettingsScreen(state);
            }
        }
    }
    // Row 2: Schedule Toggle
    else if (isButtonTouched(t_x, t_y, SETTINGS_CONTROL_X, SETTINGS_ROW2_Y, SETTINGS_BUTTON_W, SETTINGS_CONTROL_H)) {
        state.power.scheduledSleepEnabled = !state.power.scheduledSleepEnabled;
        saveSettings(state);
        drawSleepSettingsScreen(state);
    }
    // Row 3 & 4: Schedule Times (only if enabled)
    else if (state.power.scheduledSleepEnabled) {
        if (t_y >= SETTINGS_ROW3_Y && t_y <= (SETTINGS_ROW3_Y + SETTINGS_CONTROL_H)) {
            if (isButtonTouched(t_x, t_y, minus_x, SETTINGS_ROW3_Y, SETTINGS_TOUCH_W, SETTINGS_CONTROL_H)) {
                state.power.scheduledSleepHour = (state.power.scheduledSleepHour + 23) % 24;
                saveSettings(state);
                drawSleepSettingsScreen(state);
            } else if (isButtonTouched(t_x, t_y, plus_x, SETTINGS_ROW3_Y, SETTINGS_TOUCH_W, SETTINGS_CONTROL_H)) {
                state.power.scheduledSleepHour = (state.power.scheduledSleepHour + 1) % 24;
                saveSettings(state);
                drawSleepSettingsScreen(state);
            }
        } else if (t_y >= SETTINGS_ROW4_Y && t_y <= (SETTINGS_ROW4_Y + SETTINGS_CONTROL_H)) {
            if (isButtonTouched(t_x, t_y, minus_x, SETTINGS_ROW4_Y, SETTINGS_TOUCH_W, SETTINGS_CONTROL_H)) {
                state.power.scheduledWakeHour = (state.power.scheduledWakeHour + 23) % 24;
                saveSettings(state);
                drawSleepSettingsScreen(state);
            } else if (isButtonTouched(t_x, t_y, plus_x, SETTINGS_ROW4_Y, SETTINGS_TOUCH_W, SETTINGS_CONTROL_H)) {
                state.power.scheduledWakeHour = (state.power.scheduledWakeHour + 1) % 24;
                saveSettings(state);
                drawSleepSettingsScreen(state);
            }
        }
    }
}

static void handleTouchGracePeriod(ApplicationState& state, uint16_t t_x, uint16_t t_y) {
    const int BTN_X = (tft.width() - GRACE_PERIOD_BTN_W) / 2;
    const int BTN_Y = tft.height() - GRACE_PERIOD_BTN_H - GRACE_PERIOD_BTN_Y_MARGIN;
    
    if (isButtonTouched(t_x, t_y, BTN_X, BTN_Y, GRACE_PERIOD_BTN_W, GRACE_PERIOD_BTN_H)) {
        state.power.lastInteractionTime = millis();
        state.power.scheduledSleepEnabled = false; // Disable schedule temporarily if user cancels
        saveSettings(state);
        state.activeScreen = SCREEN_SPOTS;
        drawSpotsScreen(state);
    }
}

static void handleTouchUpdatesScreen(ApplicationState& state, uint16_t t_x, uint16_t t_y) {
    const int updatesBtnY = tft.height() - SETTINGS_CONTROL_H - SETTINGS_V_GAP;
    const int updatesBtnX = SETTINGS_MENU_BTN_X_MARGIN;
    const int updatesBtnW = tft.width() - 2 * updatesBtnX;

    if (isButtonTouched(t_x, t_y, updatesBtnX, updatesBtnY, updatesBtnW, SETTINGS_CONTROL_H)) {
        state.checkForUpdates = !state.checkForUpdates;
        saveSettings(state);
        drawUpdatesScreen(state);
    } else {
        // Any other touch returns to system settings
        state.activeScreen = SCREEN_SYSTEM_SETTINGS;
        drawSystemSettingsScreen(state);
    }
}

static void handleTouchWifiResetConfirm(ApplicationState& state, uint16_t t_x, uint16_t t_y) {
    const int BTN_Y = tft.height() - CALIBRATION_BTN_H - CALIBRATION_BTN_Y_MARGIN;
    const int TOTAL_W = CALIBRATION_BTN_W * 2 + CALIBRATION_BTN_GAP;
    const int START_X = (tft.width() - TOTAL_W) / 2;
    const int CANCEL_BTN_X = START_X;
    const int CONFIRM_BTN_X = START_X + CALIBRATION_BTN_W + CALIBRATION_BTN_GAP;

    if (isButtonTouched(t_x, t_y, CANCEL_BTN_X, BTN_Y, CALIBRATION_BTN_W, CALIBRATION_BTN_H)) {
        state.activeScreen = SCREEN_SYSTEM_SETTINGS;
        drawSystemSettingsScreen(state);
    } else if (isButtonTouched(t_x, t_y, CONFIRM_BTN_X, BTN_Y, CALIBRATION_BTN_W, CALIBRATION_BTN_H)) {
        tft.fillScreen(TFT_RED);
        tft.setTextColor(TFT_WHITE, TFT_RED);
        tft.setTextDatum(MC_DATUM);
        tft.setFreeFont(&FreeSans9pt7b);
        tft.drawString("Wi-Fi settings cleared.", tft.width() / 2, tft.height() / 2 - 10);
        tft.drawString("Restarting...", tft.width() / 2, tft.height() / 2 + 10);
        clearWiFiSettings();
        delay(RESTART_DELAY_MS);
        ESP.restart();
    }
}

void handleTouch(ApplicationState& state) {
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    state.power.lastInteractionTime = millis();
    TS_Point p = touchscreen.getPoint();
    
    // Map raw coordinates to screen coordinates
    uint16_t t_x = map(p.x, state.calibration.topLeftX, state.calibration.bottomRightX, TOUCH_CALIBRATION_MARGIN, tft.width() - TOUCH_CALIBRATION_MARGIN);
    uint16_t t_y = map(p.y, state.calibration.topLeftY, state.calibration.bottomRightY, TOUCH_CALIBRATION_MARGIN, tft.height() - TOUCH_CALIBRATION_MARGIN);
    
    switch (state.activeScreen) {
      case SCREEN_SPOTS:
      case SCREEN_SPOTS_AND_PROP:
        handleTouchSpotsScreen(state, t_x, t_y);
        break;
      case SCREEN_SETTINGS_MENU:
        handleTouchSettingsMenu(state, t_x, t_y);
        break;
      case SCREEN_DISPLAY_SETTINGS:
        handleTouchDisplaySettings(state, t_x, t_y);
        break;
      case SCREEN_AUDIO_SETTINGS:
        handleTouchAudioSettings(state, t_x, t_y);
        break;
      case SCREEN_SYSTEM_SETTINGS:
        handleTouchSystemSettings(state, t_x, t_y);
        break;
      case SCREEN_SLEEP_SETTINGS:
        handleTouchSleepSettings(state, t_x, t_y);
        break;
      case SCREEN_SLEEP_GRACE_PERIOD:
        handleTouchGracePeriod(state, t_x, t_y);
        break;
      case SCREEN_UPDATES_INFO:
        handleTouchUpdatesScreen(state, t_x, t_y);
        break;
      case SCREEN_WIFI_RESET_CONFIRM:
        handleTouchWifiResetConfirm(state, t_x, t_y);
        break;
      case SCREEN_INFO:
        state.activeScreen = SCREEN_SYSTEM_SETTINGS;
        drawSystemSettingsScreen(state);
        break;
      case SCREEN_PROPAGATION:
      case SCREEN_CLOCK:
        state.lastSecond = -1;
        // Return to the appropriate spots screen
        if (state.display.spotsViewMode == SPOTS_WITH_PROP) {
          state.activeScreen = SCREEN_SPOTS_AND_PROP;
          if (state.display.rememberLastScreen) { state.display.startupScreen = SCREEN_SPOTS_AND_PROP; saveSettings(state); }
          drawSpotsAndPropScreen(state);
        } else {
          state.activeScreen = SCREEN_SPOTS;
          if (state.display.rememberLastScreen) { state.display.startupScreen = SCREEN_SPOTS; saveSettings(state); }
          drawSpotsScreen(state);
        }
        break;
    }
    // Simple debounce: wait for release
    while (touchscreen.touched()) {};
  }
}

void updateStartupStatus(const String& message, OperationStatus status, ApplicationState& state) {
  const int X_MARGIN = 20; 
  const int LINE_HEIGHT = 22;
  
  tft.setFreeFont(&FreeSans9pt7b);
  
  if (status == STATUS_IN_PROGRESS) {
    tft.setTextDatum(TL_DATUM); 
    tft.setTextColor(TFT_WHITE);
    tft.drawString(message + "...", X_MARGIN, state.startupScreenYPos);
    state.startupScreenYPos += LINE_HEIGHT;
  } else {
    int yToUpdate = state.startupScreenYPos - LINE_HEIGHT;
    const char* statusText; 
    uint16_t statusColor;
    
    if (status == STATUS_SUCCESS) { 
        statusText = "OK"; 
        statusColor = TFT_GREEN; 
    } else { 
        statusText = "Error"; 
        statusColor = TFT_RED; 
    }
    
    // Clear the line area before redrawing (to remove "..." if needed)
    tft.fillRect(0, yToUpdate, tft.width(), LINE_HEIGHT - 2, TFT_BLACK);
    
    tft.setTextDatum(TL_DATUM); 
    tft.setTextColor(TFT_WHITE);
    tft.drawString(message, X_MARGIN, yToUpdate);
    
    tft.setTextDatum(TR_DATUM); 
    tft.setTextColor(statusColor);
    tft.drawString(statusText, tft.width() - X_MARGIN, yToUpdate);
    
    tft.setTextDatum(TL_DATUM);
  }
}
