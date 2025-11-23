/*
ESP32 Ham Combo
Copyright (c) 2025 Leszek (HF7A)
https://github.com/hf7a/ESP32-ham-combo

Licensed under CC BY-NC-SA 4.0.
Commercial use is prohibited.
*/

#ifndef CONSTANTS_H
#define CONSTANTS_H

// --- Firmware Metadata ---
#define FW_VERSION "1.0.0"
#define FW_DATE "2025-11-23"
#define GITHUB_REPO "hf7a/ESP32-ham-combo"
#define GITHUB_API_HOST "api.github.com"
#define PROJECT_URL "github.com/" GITHUB_REPO

// --- Network Configuration ---
const char* const TELNET_HOST = "hamalert.org";
const int TELNET_PORT = 7300;
const char* const NTP_SERVER = "pool.ntp.org";
const char* const PROP_HOST = "www.hamqsl.com";
const int HTTPS_PORT = 443;
const char* const PROP_URL = "/solarxml.php";

// --- Defaults ---
const char* const DEFAULT_TELNET_USERNAME = "N0CALL";
const char* const DEFAULT_TELNET_PASSWORD = "";
const char* const DEFAULT_TIMEZONE = "CET-1CEST,M3.5.0,M10.5.0/3";

// --- Timings (ms) ---
const unsigned long PERIODIC_CHECK_INTERVAL_MS = 5000UL;
const unsigned long TELNET_RECONNECT_INTERVAL_MS = 60 * 60 * 1000UL;
const unsigned long TELNET_LOGIN_TIMEOUT_MS = 5000UL;
const unsigned long SPOT_LIST_UPDATE_INTERVAL_MS = 30 * 1000UL;
const unsigned long PROPAGATION_UPDATE_INTERVAL_MS = 30 * 60 * 1000UL;
const unsigned long SLEEP_GRACE_PERIOD_MS = 60 * 1000UL;
const unsigned long UPDATE_CHECK_INTERVAL_MS = 24 * 60 * 60 * 1000UL;
const unsigned long RESTART_DELAY_MS = 2000UL;
const unsigned long WIFI_CONNECT_DELAY_MS = 500UL;
const unsigned long CALIBRATION_SAVE_DELAY_MS = 1500UL;

// --- Hardware Pins ---
#define AUDIO_OUT_PIN 26
#define AMP_ENABLE_PIN 21

// Touchscreen SPI pins (VSPI) - Required for manual initialization in .ino
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
// Note: TOUCH_CS and TOUCH_IRQ are usually defined in User_Setup.h,
// but if not, ensure they match your wiring (e.g., 33 and 36).

// --- Data Thresholds ---
#define SUNSPOTS_GOOD_THRESHOLD 100
#define SUNSPOTS_FAIR_THRESHOLD 50

// --- UI Colors ---
#define COLOR_DARK_GREEN 0x0100
#define COLOR_DARK_PURPLE 0x1806
#define COLOR_DARK_BLUE 0x0004
#define SECOND_DOT_COLOR TFT_ORANGE

// --- Main UI Layout ---
#define BUTTON_W 70
#define BUTTON_H 30
#define BUTTON_GAP 10
#define BUTTON_CORNER_RADIUS 5
#define BUTTON_Y_MARGIN 10

// --- Spots Screen Layout ---
#define SPOT_LINE_HEIGHT 30
#define SPOT_COL_TIME_X 40
#define SPOT_COL_CALL_X 60
#define SPOT_COL_MODE_X 190
#define SPOT_COL_FREQ_X_MARGIN 10
#define SPOT_COL_TIME_WIDTH 45

// --- Propagation Screen Layout ---
#define PROP_H_LINE_Y 122
#define PROP_V_LINE_TOP_MARGIN 10
#define PROP_V_LINE_BOTTOM_MARGIN 15
#define PROP_COL1_X 5
#define PROP_COL2_X_OFFSET 25
#define PROP_VALUE_OFFSET_X 90
#define PROP_LABEL_VALUE_GAP_X 5
#define PROP_SIMPLE_V_LINE_TOP_MARGIN 10
#define PROP_SIMPLE_V_LINE_BOTTOM_MARGIN 20

// --- Settings Screens Layout ---
#define SETTINGS_V_GAP 8
#define SETTINGS_CONTROL_H 30
#define SETTINGS_TOUCH_W 40
#define SETTINGS_LABEL_X 5
#define SETTINGS_CONTROL_X 130
#define SETTINGS_ROW1_Y 15
#define SETTINGS_ROW2_Y (SETTINGS_ROW1_Y + SETTINGS_CONTROL_H + SETTINGS_V_GAP)
#define SETTINGS_ROW3_Y (SETTINGS_ROW2_Y + SETTINGS_CONTROL_H + SETTINGS_V_GAP)
#define SETTINGS_ROW4_Y (SETTINGS_ROW3_Y + SETTINGS_CONTROL_H + SETTINGS_V_GAP)
#define SETTINGS_ROW5_Y (SETTINGS_ROW4_Y + SETTINGS_CONTROL_H + SETTINGS_V_GAP)
#define SETTINGS_ROW6_Y (SETTINGS_ROW5_Y + SETTINGS_CONTROL_H + SETTINGS_V_GAP)

// Menu Buttons
#define SETTINGS_MENU_START_Y 10
#define SETTINGS_MENU_BTN_H 35
#define SETTINGS_MENU_GAP 8
#define SETTINGS_MENU_BTN_X_MARGIN 20
#define SETTINGS_MENU_ARROW_X_MARGIN 20
#define SETTINGS_MENU_LABEL_X_MARGIN 20
#define SETTINGS_MENU_STATUS_X_MARGIN 30

// --- Info Screen Layout ---
#define INFO_SCREEN_START_Y 20
#define INFO_SCREEN_LABEL_X 15
#define INFO_SCREEN_VALUE_X 130
#define INFO_SCREEN_LINE_GAP 20
#define INFO_SCREEN_SEPARATOR_GAP_BEFORE 5
#define INFO_SCREEN_SEPARATOR_GAP_AFTER 15

// --- Calibration Screen ---
#define TOUCH_CALIBRATION_MARGIN 20
#define CALIBRATION_BTN_W 120
#define CALIBRATION_BTN_H 40
#define CALIBRATION_BTN_GAP 20
#define CALIBRATION_BTN_Y_MARGIN 20

// --- Grace Period Screen ---
#define GRACE_PERIOD_TEXT_Y 40
#define GRACE_PERIOD_BTN_W 200
#define GRACE_PERIOD_BTN_H 40
#define GRACE_PERIOD_BTN_Y_MARGIN 30
#define GRACE_PERIOD_TIMER_Y_OFFSET 10
#define GRACE_PERIOD_TIMER_HEIGHT 30
#define GRACE_PERIOD_TIMER_TEXT_Y_OFFSET 25

// --- Buffers ---
#define WIFI_CONNECT_ATTEMPTS 20
#define TELNET_LINE_BUFFER_SIZE 256
#define UPTIME_BUFFER_SIZE 20

#endif // CONSTANTS_H