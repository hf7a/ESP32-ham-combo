/*
ESP32 Ham Combo
Copyright (c) 2025 Leszek (HF7A)
https://github.com/hf7a/ESP32-ham-combo

Licensed under CC BY-NC-SA 4.0.
Commercial use is prohibited.
*/

#include "declarations.h"

namespace {
  const int SECOND_DOT_SIZE = 3;    // Size of the second dot (3x3 pixels)
  const int SECOND_DOT_MARGIN = 2;  // Margin from the screen edge

  // Calculates the (x, y) coordinates on the screen perimeter for a given second.
  // The dot travels clockwise starting from top-center.
  void calculatePerimeterPosition(int second, int& x, int& y) {
    int width = tft.width();
    int height = tft.height();
    int dot_offset = SECOND_DOT_MARGIN + (SECOND_DOT_SIZE / 2);

    // Calculate the effective perimeter length
    int w = width - 2 * dot_offset;
    int h = height - 2 * dot_offset;
    int perimeter = 2 * w + 2 * h;

    // Define path segments
    int path_top_right = w / 2;
    int path_right = h;
    int path_bottom = w;
    int path_left = h;
    // path_top_left is the remainder

    // Map second (0-59) to distance along perimeter
    long total_pos = map(second, 0, 60, 0, perimeter);

    if (total_pos < path_top_right) {
      // 1. Top edge (middle to right)
      x = (width / 2) + total_pos;
      y = dot_offset;
    } else if (total_pos < path_top_right + path_right) {
      // 2. Right edge (top to bottom)
      x = width - dot_offset - 1;
      y = dot_offset + (total_pos - path_top_right);
    } else if (total_pos < path_top_right + path_right + path_bottom) {
      // 3. Bottom edge (right to left)
      x = (width - dot_offset - 1) - (total_pos - (path_top_right + path_right));
      y = height - dot_offset - 1;
    } else if (total_pos < path_top_right + path_right + path_bottom + path_left) {
      // 4. Left edge (bottom to top)
      x = dot_offset;
      y = (height - dot_offset - 1) - (total_pos - (path_top_right + path_right + path_bottom));
    } else {
      // 5. Top edge (left to middle)
      x = dot_offset + (total_pos - (path_top_right + path_right + path_bottom + path_left));
      y = dot_offset;
    }
  }
}

void drawButtons(const ApplicationState& state) {
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(TFT_WHITE);

  // Main Screens (Spots / Prop)
  if (state.activeScreen == SCREEN_SPOTS || state.activeScreen == SCREEN_SPOTS_AND_PROP) {
    // Clock Button
    tft.fillRoundRect(CLOCK_BTN_X, BUTTON_Y, BUTTON_W, BUTTON_H, BUTTON_CORNER_RADIUS, COLOR_DARK_GREEN);
    tft.drawString("Clock", CLOCK_BTN_X + BUTTON_W / 2, BUTTON_Y + BUTTON_H / 2);
    
    // Propagation Button
    tft.fillRoundRect(PROP_BTN_X, BUTTON_Y, BUTTON_W, BUTTON_H, BUTTON_CORNER_RADIUS, COLOR_DARK_PURPLE);
    tft.drawString("Prop.", PROP_BTN_X + BUTTON_W / 2, BUTTON_Y + BUTTON_H / 2);

    // Setup / Update Button
    String setupButtonText = "Setup";
    uint16_t setupButtonColor = COLOR_DARK_BLUE;
    if (state.newVersionAvailable) {
      setupButtonText = "Update!";
      setupButtonColor = COLOR_DARK_GREEN;
    }
    tft.fillRoundRect(SETUP_BTN_X, BUTTON_Y, BUTTON_W, BUTTON_H, BUTTON_CORNER_RADIUS, setupButtonColor);
    tft.drawString(setupButtonText, SETUP_BTN_X + BUTTON_W / 2, BUTTON_Y + BUTTON_H / 2);

    // Sleep/Off Button
    tft.fillRoundRect(SLEEP_NOW_BTN_X, BUTTON_Y, BUTTON_W, BUTTON_H, BUTTON_CORNER_RADIUS, 0x3800); // Dark Red
    tft.drawString("Off", SLEEP_NOW_BTN_X + BUTTON_W / 2, BUTTON_Y + BUTTON_H / 2);
  } 
  // Sub-menus (Settings, Info, etc.)
  else if (state.activeScreen == SCREEN_SETTINGS_MENU ||
             state.activeScreen == SCREEN_DISPLAY_SETTINGS ||
             state.activeScreen == SCREEN_AUDIO_SETTINGS ||
             state.activeScreen == SCREEN_SLEEP_SETTINGS ||
             state.activeScreen == SCREEN_SYSTEM_SETTINGS ||
             state.activeScreen == SCREEN_UPDATES_INFO) {
    tft.fillRoundRect(SETUP_BTN_X, BUTTON_Y, BUTTON_W, BUTTON_H, BUTTON_CORNER_RADIUS, COLOR_DARK_BLUE);
    tft.drawString("Back", SETUP_BTN_X + BUTTON_W / 2, BUTTON_Y + BUTTON_H / 2);
  }
}

void drawClockScreen(ApplicationState& state) {
  struct tm timeinfo;
  char timeStr[6];
  time_t now;
  time(&now);

  // Wait for NTP sync (year > 2020)
  if (now < 1600000000) { return; } 

  // Full redraw on first run or mode switch
  if (state.lastSecond == -1) {
      tft.fillScreen(TFT_BLACK);
      state.lastUtcTimeStr[0] = '\0';
      state.lastLocalTimeStr[0] = '\0';
      state.lastSecondDotX = -1;
      state.lastSecondDotY = -1;
  }

  tft.setTextDatum(MC_DATUM);

  // --- Draw Time Text ---
  switch (state.display.currentClockMode) {
    case MODE_UTC:
      gmtime_r(&now, &timeinfo);
      snprintf(timeStr, sizeof(timeStr), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
      if (strcmp(timeStr, state.lastUtcTimeStr) != 0) {
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.setTextFont(8);
        tft.drawString(timeStr, tft.width() / 2, tft.height() / 2 - 10);
        tft.setFreeFont(&FreeSansBold12pt7b);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString("UTC", tft.width() / 2, tft.height() / 2 + 70);
        strlcpy(state.lastUtcTimeStr, timeStr, sizeof(state.lastUtcTimeStr));
      }
      break;

    case MODE_LOCAL:
      localtime_r(&now, &timeinfo);
      snprintf(timeStr, sizeof(timeStr), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
      if (strcmp(timeStr, state.lastLocalTimeStr) != 0) {
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.setTextFont(8);
        tft.drawString(timeStr, tft.width() / 2, tft.height() / 2 - 10);
        tft.setFreeFont(&FreeSansBold12pt7b);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString("Local", tft.width() / 2, tft.height() / 2 + 70);
        strlcpy(state.lastLocalTimeStr, timeStr, sizeof(state.lastLocalTimeStr));
      }
      break;

    case MODE_BOTH:
      // UTC Part
      gmtime_r(&now, &timeinfo);
      snprintf(timeStr, sizeof(timeStr), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
      if (strcmp(timeStr, state.lastUtcTimeStr) != 0) {
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.setTextFont(8);
        tft.drawString(timeStr, tft.width() / 2, (tft.height() / 4) - 15);
        tft.setFreeFont(&FreeSans9pt7b);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString("UTC", tft.width() / 2, (tft.height() / 4) + 45);
        strlcpy(state.lastUtcTimeStr, timeStr, sizeof(state.lastUtcTimeStr));
      }
      // Local Part
      localtime_r(&now, &timeinfo);
      snprintf(timeStr, sizeof(timeStr), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
      if (strcmp(timeStr, state.lastLocalTimeStr) != 0) {
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.setTextFont(8);
        tft.drawString(timeStr, tft.width() / 2, (tft.height() * 3 / 4) - 15);
        tft.setFreeFont(&FreeSans9pt7b);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString("Local", tft.width() / 2, (tft.height() * 3 / 4) + 45);
        strlcpy(state.lastLocalTimeStr, timeStr, sizeof(state.lastLocalTimeStr));
      }
      break;
  }

  // --- Draw Second Dot ---
  gmtime_r(&now, &timeinfo);
  int currentSecond = timeinfo.tm_sec;

  if (currentSecond != state.lastSecond) {
    // Erase previous dot
    if (state.lastSecondDotX != -1) {
      int eraseX = state.lastSecondDotX - (SECOND_DOT_SIZE / 2);
      int eraseY = state.lastSecondDotY - (SECOND_DOT_SIZE / 2);
      tft.fillRect(eraseX, eraseY, SECOND_DOT_SIZE, SECOND_DOT_SIZE, TFT_BLACK);
    }

    if (state.display.secondDotEnabled) {
      int newX, newY;
      calculatePerimeterPosition(currentSecond, newX, newY);
      int drawX = newX - (SECOND_DOT_SIZE / 2);
      int drawY = newY - (SECOND_DOT_SIZE / 2);
      tft.fillRect(drawX, drawY, SECOND_DOT_SIZE, SECOND_DOT_SIZE, SECOND_DOT_COLOR);

      state.lastSecondDotX = newX;
      state.lastSecondDotY = newY;
    } else {
      state.lastSecondDotX = -1;
      state.lastSecondDotY = -1;
    }
    state.lastSecond = currentSecond;
  }
}

void drawGracePeriodScreen(const ApplicationState& state) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans9pt7b);

    tft.setTextColor(TFT_WHITE);
    tft.drawString("Device will sleep soon.", tft.width() / 2, GRACE_PERIOD_TEXT_Y);

    tft.setFreeFont(&FreeSansBold12pt7b);
    tft.setTextColor(TFT_YELLOW);
    tft.drawString("60s to sleep", tft.width() / 2, tft.height() / 2 + GRACE_PERIOD_TIMER_TEXT_Y_OFFSET);

    const int BTN_X = (tft.width() - GRACE_PERIOD_BTN_W) / 2;
    const int BTN_Y = tft.height() - GRACE_PERIOD_BTN_H - GRACE_PERIOD_BTN_Y_MARGIN;
    
    tft.fillRoundRect(BTN_X, BTN_Y, GRACE_PERIOD_BTN_W, GRACE_PERIOD_BTN_H, BUTTON_CORNER_RADIUS, COLOR_DARK_GREEN);
    tft.setTextColor(TFT_WHITE);
    tft.setFreeFont(&FreeSans9pt7b);
    tft.drawString("Cancel Sleep", BTN_X + GRACE_PERIOD_BTN_W / 2, BTN_Y + GRACE_PERIOD_BTN_H / 2);
}

void drawPropagationFooter(const ApplicationState& state) {
  if (!state.propDataAvailable) {
    return;
  }

  tft.loadFont(nullptr); // Use default font for smaller size
  const int line_gap = 22;
  int yPos_day = PROP_FOOTER_Y;
  int yPos_night = yPos_day + line_gap;

  // Clear footer area
  tft.fillRect(0, yPos_day - 8, tft.width(), 2 * line_gap + 4, TFT_BLACK);

  // Column positions
  const int label_x = 32;
  const int col1_x = 75;
  const int col2_x = 135;
  const int col3_x = 195;
  const int col4_x = 255;

  tft.setTextDatum(MC_DATUM);

  // --- Draw Day Conditions ---
  tft.setTextColor(TFT_WHITE);
  tft.drawString("D:", label_x, yPos_day);

  tft.setTextColor(getPropagationColor(state.solarData.propagation[0]));
  tft.drawString("80-40", col1_x, yPos_day);
  tft.setTextColor(getPropagationColor(state.solarData.propagation[1]));
  tft.drawString("30-20", col2_x, yPos_day);
  tft.setTextColor(getPropagationColor(state.solarData.propagation[2]));
  tft.drawString("17-15", col3_x, yPos_day);
  tft.setTextColor(getPropagationColor(state.solarData.propagation[3]));
  tft.drawString("12-10", col4_x, yPos_day);

  // --- Draw Night Conditions ---
  tft.setTextColor(TFT_WHITE);
  tft.drawString("N:", label_x, yPos_night);

  tft.setTextColor(getPropagationColor(state.solarData.propagation[4]));
  tft.drawString("80-40", col1_x, yPos_night);
  tft.setTextColor(getPropagationColor(state.solarData.propagation[5]));
  tft.drawString("30-20", col2_x, yPos_night);
  tft.setTextColor(getPropagationColor(state.solarData.propagation[6]));
  tft.drawString("17-15", col3_x, yPos_night);
  tft.setTextColor(getPropagationColor(state.solarData.propagation[7]));
  tft.drawString("12-10", col4_x, yPos_night);
}
