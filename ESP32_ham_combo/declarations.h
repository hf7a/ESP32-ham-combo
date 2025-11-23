/*
ESP32 Ham Combo
Copyright (c) 2025 Leszek (HF7A)
https://github.com/hf7a/ESP32-ham-combo

Licensed under CC BY-NC-SA 4.0.
Commercial use is prohibited.
*/

#ifndef DECLARATIONS_H
#define DECLARATIONS_H

#include <WiFi.h>
#include <WiFiClient.h>
#include "time.h"
#include <SPI.h>
#include "TFT_eSPI.h"
#include <XPT2046_Touchscreen.h>
#include <WiFiClientSecure.h>
#include <ArduinoHttpClient.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include "driver/dac_cosine.h"
#include "constants.h"

// --- External Object Declarations ---
extern TFT_eSPI tft;
extern WiFiClient telnetClient;
extern Preferences preferences;
extern SPIClass touchscreenSPI;
extern XPT2046_Touchscreen touchscreen;
extern WiFiClientSecure secureClient;
extern HttpClient httpClient;
extern AsyncWebServer webServer;

// --- Dynamic UI Layout Definitions ---
// These macros depend on the 'tft' object and must be defined after it is declared.
#define BUTTON_Y (tft.height() - BUTTON_H - BUTTON_Y_MARGIN)

// Main screen buttons
#define SETUP_BTN_X (tft.width() - BUTTON_W - 10)
#define PROP_BTN_X (SETUP_BTN_X - BUTTON_W - BUTTON_GAP)
#define CLOCK_BTN_X (PROP_BTN_X - BUTTON_W - BUTTON_GAP)
#define SLEEP_NOW_BTN_X (CLOCK_BTN_X - BUTTON_W - BUTTON_GAP)

// Footer and Settings dynamic widths
#define PROP_FOOTER_Y (BUTTON_Y - 40)
#define SETTINGS_BUTTON_W (tft.width() - SETTINGS_CONTROL_X - 20)

// --- Enumerations ---

enum ActiveScreen {
SCREEN_SPOTS,
SCREEN_SETTINGS_MENU,
SCREEN_DISPLAY_SETTINGS,
SCREEN_AUDIO_SETTINGS,
SCREEN_SLEEP_SETTINGS,
SCREEN_SYSTEM_SETTINGS,
SCREEN_CLOCK,
SCREEN_PROPAGATION,
SCREEN_INFO,
SCREEN_SLEEP_GRACE_PERIOD,
SCREEN_UPDATES_INFO,
SCREEN_SPOTS_AND_PROP,
SCREEN_WIFI_RESET_CONFIRM
};

enum OperationStatus {
STATUS_IN_PROGRESS,
STATUS_SUCCESS,
STATUS_FAILURE
};

enum ClockDisplayMode {
MODE_UTC,
MODE_LOCAL,
MODE_BOTH
};

enum PropagationViewMode {
VIEW_SIMPLE,
VIEW_EXTENDED
};

enum SpotsViewMode {
SPOTS_ONLY,
SPOTS_WITH_PROP
};

enum PropagationCondition {
POOR,
FAIR,
GOOD,
UNKNOWN
};

enum InitializationState {
INIT_BEGIN,
INIT_SYNC_TIME,
INIT_FETCH_PROPAGATION,
INIT_CONNECT_TELNET,
INIT_FINALIZE,
INIT_RUNNING
};

// --- Data Structures ---

struct TouchCalibration {
bool calibrated = false;
uint16_t topLeftX = 200;
uint16_t topLeftY = 240;
uint16_t bottomRightX = 3700;
uint16_t bottomRightY = 3800;
};

struct DxSpot {
char call[12];
char freq[10];
char spotter[12];
int spotHour;
int spotMinute;
char mode[5];
};

struct VhfPropagationData {
char aurora[16];
char eSkipEurope2m[16];
char eSkipEurope4m[16];
char eSkipEurope6m[16];
};

struct SolarPropagationData {
int solarFlux;
int aIndex;
int kIndex;
char xray[8];
char geomagneticField[16];
char signalNoiseLevel[8];
int sunspots;
PropagationCondition propagation[8];
VhfPropagationData vhf;
};

struct DisplayState {
int brightnessPercent = 80;
ClockDisplayMode currentClockMode = MODE_UTC;
PropagationViewMode currentPropViewMode = VIEW_EXTENDED;
SpotsViewMode spotsViewMode = SPOTS_ONLY;
bool colorInversion = true;
bool rememberLastScreen = false;
ActiveScreen startupScreen = SCREEN_SPOTS;
bool secondDotEnabled = true;
int screenRotation = 3;
};

struct AudioState {
int volumeStep = 3;
int toneFrequency = 880;
int toneDurationMs = 100;
dac_cosine_handle_t cos_handle = nullptr;
};

struct PowerState {
int sleepTimeoutMinutes = 0;
bool scheduledSleepEnabled = false;
int scheduledSleepHour = 23;
int scheduledWakeHour = 7;
unsigned long lastInteractionTime = 0;
unsigned long gracePeriodStartTime = 0;
};

struct NetworkState {
char telnetUsername[32];
char telnetPassword[32];
char timezone[64];
int dstMode = 1; // 0=Off, 1=EU, 2=NA, 3=Custom
char customDstRule[64];
bool hamAlertConnected = false;
unsigned long lastReconnectTime = 0;
bool isWifiConnected = true;
};

struct ApplicationState {
ActiveScreen activeScreen = SCREEN_SPOTS;
int startupScreenYPos = 0;
bool calibrationRequested = false;

DisplayState display;
AudioState audio;
PowerState power;
NetworkState network;

bool checkForUpdates = true;
bool newVersionAvailable = false;
char newVersionTag[16];
unsigned long lastUpdateCheckTime = 0;

static const int MAX_SPOTS = 6;
DxSpot spots[MAX_SPOTS];
int spotCount = 0;
int latestSpotIndex = -1;

SolarPropagationData solarData;
bool propDataAvailable = false;

TouchCalibration calibration;

unsigned long lastDisplayUpdateTime = 0;
unsigned long lastClockUpdateTime = 0;
unsigned long lastPropUpdateTime = 0;
unsigned long lastPeriodicCheckTime = 0;

char lastUtcTimeStr[6] = "";
char lastLocalTimeStr[6] = "";
int lastSecond = -1;
int lastSecondDotX = -1;
int lastSecondDotY = -1;
};

// --- Function Prototypes ---

// main .ino file
void startConfigurationPortal();
void enterDeepSleep(const ApplicationState& state);
bool isWithinScheduledSleepWindow(const ApplicationState& state);
bool shouldEnterSleep(const ApplicationState& state);
void determineAndDrawActiveScreen(ApplicationState& state);

// calibration.cpp
void runTouchCalibration(ApplicationState& state);
bool loadCalibrationData(ApplicationState& state);

// tab_prop.cpp
bool fetchPropagationData(ApplicationState& state);
void drawPropagationScreen(const ApplicationState& state);
PropagationCondition toConditionValue(const char* val);

// tab_settings.cpp
void saveSettings(const ApplicationState& state);
void loadSettings(ApplicationState& state);
void clearWiFiSettings();

// tab_spots.cpp
bool connectToTelnet(ApplicationState& state, bool silentMode);
void readTelnetSpots(ApplicationState& state);
void parseSpot(const char* line, ApplicationState& state);
void addSpot(const DxSpot& newSpot, ApplicationState& state);
void clearSpots(ApplicationState& state);
void getModeFromLine(const char* line, float freq_khz, char* mode_buffer, size_t buffer_size);
uint16_t getModeColor(const char* mode);
String formatElapsedMinutes(long elapsedSeconds);
void drawSpotsScreen(ApplicationState& state);
void drawSpotsAndPropScreen(ApplicationState& state);
void updateSpotTimesOnly(ApplicationState& state);

// ui_core.cpp
void setBrightness(int percent);
void setupAudio(ApplicationState& state);
void playNewSpotSound(const ApplicationState& state);
void handleTouch(ApplicationState& state);
void updateStartupStatus(const String& message, OperationStatus status, ApplicationState& state);
bool isButtonTouched(uint16_t tx, uint16_t ty, int x, int y, int w, int h);
uint16_t getPropagationColor(PropagationCondition propValue);
uint16_t getSolarFluxColor(int sfi);
uint16_t getAIndexColor(int aIndex);
uint16_t getKIndexColor(int kIndex);
uint16_t getXRayColor(const char* xray);
uint16_t getGeomagFieldColor(const char* geomag_field);
uint16_t getSignalNoiseColor(const char* signal_noise_level);
uint16_t getVhfConditionsColor(const char* vhf_condition);

// ui_screens_main.cpp
void drawButtons(const ApplicationState& state);
void drawClockScreen(ApplicationState& state);
void drawGracePeriodScreen(const ApplicationState& state);
void drawPropagationFooter(const ApplicationState& state);

// ui_screens_settings.cpp
void drawSettingsMenuScreen(const ApplicationState& state);
void drawDisplaySettingsScreen(const ApplicationState& state);
void drawAudioSettingsScreen(const ApplicationState& state);
void drawSleepSettingsScreen(const ApplicationState& state);
void drawSystemSettingsScreen(const ApplicationState& state);
void drawInfoScreen(const ApplicationState& state);
void drawUpdatesScreen(const ApplicationState& state);
void drawWifiResetConfirmScreen(const ApplicationState& state);

// webserver.cpp
void setupWebServer(ApplicationState& state);

// updates.cpp
bool checkGithubForUpdate(ApplicationState& state);

#endif // DECLARATIONS_H