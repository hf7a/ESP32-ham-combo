/*
ESP32 Ham Combo
Copyright (c) 2025 Leszek (HF7A)
https://github.com/hf7a/ESP32-ham-combo

Licensed under CC BY-NC-SA 4.0.
Commercial use is prohibited.
*/

#include "declarations.h"

// Waits until the touchscreen is no longer being pressed.
static void waitForTouchRelease() {
  while (touchscreen.touched()) {
    delay(20);
  }
}

// Draws a crosshair at the specified coordinates.
static void drawCrosshair(int x, int y, uint16_t color) {
  tft.drawFastHLine(x - 10, y, 21, color);
  tft.drawFastVLine(x, y - 10, 21, color);
}

// Displays instructions and waits for the user to touch a calibration point.
static void getCalibrationPoint(int x, int y, const char* text, uint16_t &rawX, uint16_t &rawY) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(&FreeSans9pt7b);

  tft.drawString("Touch the center of the crosshair", tft.width() / 2, tft.height() / 2 - 40);
  tft.drawString(text, tft.width() / 2, tft.height() / 2 - 15);

  drawCrosshair(x, y, TFT_CYAN);

  TS_Point touchPoint;
  bool touched = false;

  while (!touched) {
    if (touchscreen.touched()) {
      delay(50); // Debounce
      touchPoint = touchscreen.getPoint();
      // A z-value greater than 100 indicates a firm press
      if (touchPoint.z > 100) {
        rawX = touchPoint.x;
        rawY = touchPoint.y;
        touched = true;
      }
    }
    delay(10);
  }

  waitForTouchRelease();
}

void runTouchCalibration(ApplicationState& state) {
  delay(500); // Wait for the user to lift their finger after pressing the menu button

  uint16_t newTopLeftX, newTopLeftY;
  uint16_t newBottomRightX, newBottomRightY;

  // 1. Get Top-Left Point
  getCalibrationPoint(TOUCH_CALIBRATION_MARGIN, TOUCH_CALIBRATION_MARGIN,
                      "(top left corner)",
                      newTopLeftX, newTopLeftY);

  // 2. Get Bottom-Right Point
  getCalibrationPoint(tft.width() - TOUCH_CALIBRATION_MARGIN, tft.height() - TOUCH_CALIBRATION_MARGIN,
                      "(bottom right corner)",
                      newBottomRightX, newBottomRightY);

  // 3. Show Confirmation Screen
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setFreeFont(&FreeSans9pt7b);

  int yPos = 20;
  tft.drawString("Save new calibration?", 10, yPos); yPos += 30;
  tft.setTextColor(TFT_YELLOW);
  tft.drawString("If issues occur after restart,", 10, yPos); yPos += 20;
  tft.drawString("boot the device with the BOOT", 10, yPos); yPos += 20;
  tft.drawString("button pressed.", 10, yPos);

  const int BTN_Y = tft.height() - CALIBRATION_BTN_H - CALIBRATION_BTN_Y_MARGIN;
  const int TOTAL_W = CALIBRATION_BTN_W * 2 + CALIBRATION_BTN_GAP;
  const int START_X = (tft.width() - TOTAL_W) / 2;

  const int CANCEL_BTN_X = START_X;
  const int SAVE_BTN_X = START_X + CALIBRATION_BTN_W + CALIBRATION_BTN_GAP;

  // Draw Cancel Button
  tft.fillRoundRect(CANCEL_BTN_X, BTN_Y, CALIBRATION_BTN_W, CALIBRATION_BTN_H, BUTTON_CORNER_RADIUS, COLOR_DARK_BLUE);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Cancel", CANCEL_BTN_X + CALIBRATION_BTN_W / 2, BTN_Y + CALIBRATION_BTN_H / 2);

  // Draw Save Button
  tft.fillRoundRect(SAVE_BTN_X, BTN_Y, CALIBRATION_BTN_W, CALIBRATION_BTN_H, BUTTON_CORNER_RADIUS, COLOR_DARK_GREEN);
  tft.drawString("Save", SAVE_BTN_X + CALIBRATION_BTN_W / 2, BTN_Y + CALIBRATION_BTN_H / 2);

  while (true) {
    if (touchscreen.touched()) {
      TS_Point p = touchscreen.getPoint();
      waitForTouchRelease();

      // Map raw touch coordinates using the *old* calibration to detect button presses.
      uint16_t t_x = map(p.x, state.calibration.topLeftX, state.calibration.bottomRightX, TOUCH_CALIBRATION_MARGIN, tft.width() - TOUCH_CALIBRATION_MARGIN);
      uint16_t t_y = map(p.y, state.calibration.topLeftY, state.calibration.bottomRightY, TOUCH_CALIBRATION_MARGIN, tft.height() - TOUCH_CALIBRATION_MARGIN);

      // Check Cancel
      if (isButtonTouched(t_x, t_y, CANCEL_BTN_X, BTN_Y, CALIBRATION_BTN_W, CALIBRATION_BTN_H)) {
        drawSystemSettingsScreen(state);
        return;
      }

      // Check Save
      if (isButtonTouched(t_x, t_y, SAVE_BTN_X, BTN_Y, CALIBRATION_BTN_W, CALIBRATION_BTN_H)) {
        preferences.begin("calibration", false);
        preferences.putUShort("tl_x", newTopLeftX);
        preferences.putUShort("tl_y", newTopLeftY);
        preferences.putUShort("br_x", newBottomRightX);
        preferences.putUShort("br_y", newBottomRightY);
        preferences.putBool("calibrated", true);
        preferences.end();

        // Update RAM state
        state.calibration.topLeftX = newTopLeftX;
        state.calibration.topLeftY = newTopLeftY;
        state.calibration.bottomRightX = newBottomRightX;
        state.calibration.bottomRightY = newBottomRightY;
        state.calibration.calibrated = true;

        tft.fillScreen(TFT_GREEN);
        tft.setTextColor(TFT_BLACK, TFT_GREEN);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("Calibration Saved!", tft.width() / 2, tft.height() / 2);
        delay(CALIBRATION_SAVE_DELAY_MS);

        drawSystemSettingsScreen(state);
        return;
      }
    }
    delay(50);
  }
}

bool loadCalibrationData(ApplicationState& state) {
  preferences.begin("calibration", true); // Read-only
  state.calibration.calibrated = preferences.getBool("calibrated", false);

  if (state.calibration.calibrated) {
    state.calibration.topLeftX = preferences.getUShort("tl_x", 200);
    state.calibration.topLeftY = preferences.getUShort("tl_y", 240);
    state.calibration.bottomRightX = preferences.getUShort("br_x", 3700);
    state.calibration.bottomRightY = preferences.getUShort("br_y", 3800);
    Serial.println("Calibration data loaded.");
  } else {
    Serial.println("No calibration data found. Using defaults.");
  }

  preferences.end();
  return true;
}
