/*
  ESP32 Ham Combo
  Copyright (c) 2025 Leszek (HF7A)
  https://github.com/hf7a/ESP32-ham-combo
  
  Licensed under CC BY-NC-SA 4.0.
  Commercial use is prohibited.
*/

#include "declarations.h"

namespace {
  // Helper to generate <option> tags for DST rule components (month, week, day).
  String generateRuleOptions(const char* const names[], int count, int selectedValue) {
    String options = "";
    for (int i = 0; i < count; ++i) {
      options += "<option value=\"" + String(i + 1) + "\"" + (selectedValue == (i + 1) ? " selected" : "") + ">" + names[i] + "</option>";
    }
    return options;
  }

  String generateDayOptions(const char* const names[], int count, int selectedValue) {
    String options = "";
    for (int i = 0; i < count; ++i) {
      options += "<option value=\"" + String(i) + "\"" + (selectedValue == i ? " selected" : "") + ">" + names[i] + "</option>";
    }
    return options;
  }
}

void setupWebServer(ApplicationState& state) {

  webServer.on("/", HTTP_ANY, [&state](AsyncWebServerRequest *request){
    // --- Handle Form Submission (POST) ---
    if (request->method() == HTTP_POST) {
      ApplicationState newState = state; // Create a copy to modify

      // Network Credentials
      if (request->hasParam("user", true)) strlcpy(newState.network.telnetUsername, request->getParam("user", true)->value().c_str(), sizeof(newState.network.telnetUsername));
      if (request->hasParam("pass", true)) strlcpy(newState.network.telnetPassword, request->getParam("pass", true)->value().c_str(), sizeof(newState.network.telnetPassword));
      
      // Display Settings
      if (request->hasParam("brightness", true)) newState.display.brightnessPercent = request->getParam("brightness", true)->value().toInt();
      if (request->hasParam("clockMode", true)) newState.display.currentClockMode = (ClockDisplayMode)request->getParam("clockMode", true)->value().toInt();
      if (request->hasParam("propMode", true)) newState.display.currentPropViewMode = (PropagationViewMode)request->getParam("propMode", true)->value().toInt();
      if (request->hasParam("rotation", true)) newState.display.screenRotation = request->getParam("rotation", true)->value().toInt();
      newState.display.colorInversion = request->hasParam("inversion", true);
      newState.display.secondDotEnabled = request->hasParam("secondDot", true);
      newState.display.rememberLastScreen = request->hasParam("rememberScreen", true);

      // Audio Settings
      if (request->hasParam("volume", true)) {
        int volPercent = request->getParam("volume", true)->value().toInt();
        // Map 0-100% slider to 0-4 step
        if (volPercent == 0) newState.audio.volumeStep = 0;
        else if (volPercent <= 25) newState.audio.volumeStep = 1;
        else if (volPercent <= 50) newState.audio.volumeStep = 2;
        else if (volPercent <= 75) newState.audio.volumeStep = 3;
        else newState.audio.volumeStep = 4;
      }
      if (request->hasParam("tone", true)) newState.audio.toneFrequency = request->getParam("tone", true)->value().toInt();
      if (request->hasParam("toneDuration", true)) newState.audio.toneDurationMs = request->getParam("toneDuration", true)->value().toInt();

      // Power Settings
      if (request->hasParam("sleepTimeout", true)) newState.power.sleepTimeoutMinutes = request->getParam("sleepTimeout", true)->value().toInt();
      newState.power.scheduledSleepEnabled = request->hasParam("schedSleepOn", true);
      if (request->hasParam("schedSleepH", true)) newState.power.scheduledSleepHour = request->getParam("schedSleepH", true)->value().toInt();
      if (request->hasParam("schedWakeH", true)) newState.power.scheduledWakeHour = request->getParam("schedWakeH", true)->value().toInt();

      // Timezone and DST settings
      if (request->hasParam("timezone", true)) strlcpy(newState.network.timezone, request->getParam("timezone", true)->value().c_str(), sizeof(newState.network.timezone));
      if (request->hasParam("dstMode", true)) newState.network.dstMode = request->getParam("dstMode", true)->value().toInt();

      if (newState.network.dstMode == 3) { // Custom rule
        int sm = request->getParam("start_m", true)->value().toInt();
        int sw = request->getParam("start_w", true)->value().toInt();
        int sd = request->getParam("start_d", true)->value().toInt();
        int em = request->getParam("end_m", true)->value().toInt();
        int ew = request->getParam("end_w", true)->value().toInt();
        int ed = request->getParam("end_d", true)->value().toInt();
        snprintf(newState.network.customDstRule, sizeof(newState.network.customDstRule), ",M%d.%d.%d,M%d.%d.%d", sm, sw, sd, em, ew, ed);
      }

      // System Settings
      newState.checkForUpdates = request->hasParam("checkUpdates", true);

      saveSettings(newState);

      // Send Restart Page
      String responseHtml = R"rawliteral(
<!DOCTYPE html><html><head><title>Restarting...</title><style>body{font-family:Arial,sans-serif;background-color:#121212;color:#e0e0e0;margin:20px;text-align:center;}.container{max-width:500px;margin:auto;background:#1e1e1e;padding:20px;border-radius:8px;border:1px solid #333;}h1{color:#009688;}</style></head>
<body><div class="container"><h1>Settings Saved!</h1><p>The device is restarting.</p><p>This page will refresh automatically in 5 seconds.</p></div>
<script>setTimeout(function(){fetch('/restart');},1000);setTimeout(function(){window.location.href='/';},5000);</script></body></html>)rawliteral";
      request->send(200, "text/html", responseHtml);
      return;
    }

    // --- Serve Configuration Form (GET) ---
    String html = R"rawliteral(
<!DOCTYPE html><html><head><title>ESP Ham Combo - Configuration</title><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body{font-family:Arial,sans-serif;background-color:#121212;color:#e0e0e0;margin:0;padding:5px;font-size:14px;}
.container{max-width:450px;margin:auto;background:#1e1e1e;padding:10px;border-radius:8px;border:1px solid #333;}
h1{color:#009688;text-align:center;font-size:1.4em;margin-top:0;margin-bottom:10px;}
fieldset{border:1px solid #444;border-radius:5px;margin-bottom:8px;padding:8px 12px;}
legend{font-weight:bold;color:#00bcd4;padding:0 8px;font-size:1.1em;}
.form-grid{display:grid;grid-template-columns:150px 1fr;gap:8px;align-items:center;}
.form-grid label{grid-column:1;text-align:right;padding-right:8px;font-weight:bold;color:#ccc;}
.form-grid .control{grid-column:2;}
input[type=text],input[type=password],input[type=number],select{width:100%;padding:5px;background-color:#333;color:#fff;border:1px solid #555;border-radius:4px;box-sizing:border-box;font-size:1em;}
input[type=range]{width:100%;vertical-align:middle;}
input[type=checkbox]{width:16px;height:16px;vertical-align:middle;}
input[type=submit]{background-color:#00796B;color:white;padding:10px;border:none;border-radius:4px;cursor:pointer;font-size:1.1em;width:100%;margin-top:8px;}
button{background-color:#004D40;color:#80CBC4;border:1px solid #00796B;padding:8px;border-radius:4px;cursor:pointer;font-size:1em;width:100%;margin-top:8px;}
.range-container{display:flex;align-items:center;}
.range-value{font-weight:bold;color:#00bcd4;margin-left:8px;white-space:nowrap;min-width:50px;text-align:left;}
.radio-group label{display:inline-block;margin-right:10px;font-weight:normal;}
.radio-group input[type=radio]{margin-right:4px;vertical-align:middle;}
.custom-dst-grid{display:grid;grid-template-columns:auto 1fr 1fr 1fr;gap:5px;align-items:center;margin-top:5px;}
.custom-dst-grid label{text-align:left;font-weight:normal;}
</style>
</head><body><div class="container"><h1>{TITLE}</h1><form action="/" method="POST">
<fieldset><legend>HamAlert Credentials</legend><div class="form-grid">
<label for="user">Login:</label><input class="control" type="text" id="user" name="user" value="{USER}">
<label for="pass">Password:</label><input class="control" type="password" id="pass" name="pass" value="{PASS}">
</div></fieldset>
<fieldset><legend>Display & Sound</legend><div class="form-grid">
<label for="brightness">Brightness:</label><div class="control range-container"><input type="range" id="brightness" name="brightness" min="10" max="100" step="10" value="{BRIGHTNESS}" oninput="this.nextElementSibling.innerText=this.value+'%'"><span class="range-value"></span></div>
<label for="volume">Volume:</label><div class="control range-container"><input type="range" id="volume" name="volume" min="0" max="100" step="25" value="{VOLUME}" oninput="updateVolumeLabel(this)"><span class="range-value"></span></div>
<label for="tone">Tone Freq:</label><div class="control range-container"><input type="range" id="tone" name="tone" min="300" max="1400" step="100" value="{TONE}" oninput="this.nextElementSibling.innerText=this.value+' Hz'"><span class="range-value"></span></div>
<label for="toneDuration">Tone Duration:</label><div class="control range-container"><input type="range" id="toneDuration" name="toneDuration" min="50" max="125" step="25" value="{TONE_DURATION}" oninput="this.nextElementSibling.innerText=this.value+' ms'"><span class="range-value"></span></div>
<label>Clock Mode:</label><div class="control radio-group"><label><input type="radio" name="clockMode" value="0" {CM_UTC_CHECKED}>UTC</label><label><input type="radio" name="clockMode" value="1" {CM_LOCAL_CHECKED}>Local</label><label><input type="radio" name="clockMode" value="2" {CM_BOTH_CHECKED}>Both</label></div>
<label>Propagation View:</label><div class="control radio-group"><label><input type="radio" name="propMode" value="0" {PM_SIMPLE_CHECKED}>Simple</label><label><input type="radio" name="propMode" value="1" {PM_EXTENDED_CHECKED}>Extended</label></div>
<label for="rotation">Screen Rotation:</label><select class="control" id="rotation" name="rotation">{ROTATION_OPTIONS}</select>
<label for="inversion">Invert Colors:</label><input class="control" type="checkbox" id="inversion" name="inversion" {INVERSION_CHECKED}>
<label for="secondDot">Second Dot:</label><input class="control" type="checkbox" id="secondDot" name="secondDot" {SECOND_DOT_CHECKED}>
<label for="rememberScreen">Remember Screen:</label><input class="control" type="checkbox" id="rememberScreen" name="rememberScreen" {REMEMBER_SCREEN_CHECKED}>
</div></fieldset>
<fieldset><legend>Power Management</legend><div class="form-grid">
<label for="sleepTimeout">Inactivity Sleep:</label><select class="control" id="sleepTimeout" name="sleepTimeout">{TIMEOUT_OPTIONS}</select>
<label for="schedSleepOn">Sleep Schedule:</label><input class="control" type="checkbox" id="schedSleepOn" name="schedSleepOn" {SCHED_ON}>
<div id="schedule-times" style="display:none;grid-column:1/-1;"><div class="form-grid">
<label for="schedSleepH">Sleep Time (H):</label><input class="control" type="number" id="schedSleepH" name="schedSleepH" min="0" max="23" value="{SLEEP_H}">
<label for="schedWakeH">Wake Time (H):</label><input class="control" type="number" id="schedWakeH" name="schedWakeH" min="0" max="23" value="{WAKE_H}">
</div></div></div></fieldset>
<fieldset><legend>Regional Settings</legend><div class="form-grid">
<label for="timezone">Base Timezone:</label><select class="control" id="timezone" name="timezone">{TIMEZONE_OPTIONS}</select>
<label for="dstMode">Summer Time:</label><select class="control" id="dstMode" name="dstMode">{DST_MODE_OPTIONS}</select>
</div><div id="custom-dst-rules" style="display:none;grid-column:1/-1;">
<div class="custom-dst-grid"><label>Starts:</label><select name="start_m">{MONTH_OPTIONS_START}</select><select name="start_w">{WEEK_OPTIONS_START}</select><select name="start_d">{DAY_OPTIONS_START}</select></div>
<div class="custom-dst-grid"><label>Ends:</label><select name="end_m">{MONTH_OPTIONS_END}</select><select name="end_w">{WEEK_OPTIONS_END}</select><select name="end_d">{DAY_OPTIONS_END}</select></div>
</div></fieldset>
<fieldset><legend>System</legend><div class="form-grid">
<label for="checkUpdates">Check for Updates:</label><input class="control" type="checkbox" id="checkUpdates" name="checkUpdates" {CHECK_UPDATES_CHECKED}>
</div></fieldset>
<input type="submit" value="Save Settings & Restart"></form>
<button type="button" id="startCalBtn">Start Touch Calibration</button>
</div>
<script>
const volumeLabels = ['Muted', '-18 dB', '-12 dB', '-6 dB', '0 dB'];
function updateVolumeLabel(slider) { slider.nextElementSibling.innerText = volumeLabels[slider.value / 25]; }
document.addEventListener('DOMContentLoaded',function(){
['brightness','tone','toneDuration'].forEach(id=>document.getElementById(id).dispatchEvent(new Event('input')));
updateVolumeLabel(document.getElementById('volume'));
const schedCheckbox=document.getElementById('schedSleepOn'),schedTimesDiv=document.getElementById('schedule-times');
function toggleScheduleTimes(){schedTimesDiv.style.display=schedCheckbox.checked?'block':'none';}
schedCheckbox.addEventListener('change',toggleScheduleTimes);toggleScheduleTimes();
const dstModeSelect=document.getElementById('dstMode'),customRulesDiv=document.getElementById('custom-dst-rules');
function toggleCustomDst(){customRulesDiv.style.display=(dstModeSelect.value==='3')?'block':'none';}
dstModeSelect.addEventListener('change',toggleCustomDst);toggleCustomDst();
document.getElementById('startCalBtn').addEventListener('click',function(){fetch('/start_calibration').then(res=>res.text()).then(text=>alert(text));});
});
</script></body></html>
)rawliteral";

    // --- Generate Dynamic Options ---
    
    auto generateRotationOptions = [](int selectedRotation) {
        String options = "";
        const char* names[] = {"Rotate 90 deg.", "Landscape (Flipped)", "Rotate 270 deg.", "Landscape (Default)"};
        const int values[] = {0, 1, 2, 3};
        for (int i = 0; i < 4; ++i) {
            options += "<option value=\"" + String(values[i]) + "\"" + (selectedRotation == values[i] ? " selected" : "") + ">" + names[i] + "</option>";
        }
        return options;
    };

    auto generateTimezoneOptions = [](const char* selectedTz) {
        String options = "";
        const char* tzNames[] = {"UTC-12", "UTC-11", "UTC-10", "UTC-9", "UTC-8 (PST)", "UTC-7 (MST)", "UTC-6 (CST)", "UTC-5 (EST)", "UTC-4", "UTC-3", "UTC-2", "UTC-1", "UTC", "UTC+1 (CET)", "UTC+2 (EET)", "UTC+3", "UTC+4", "UTC+5", "UTC+6", "UTC+7", "UTC+8", "UTC+9", "UTC+10", "UTC+11", "UTC+12"};
        const char* tzValues[] = {"<+12>-12", "<+11>-11", "<+10>-10", "<+09>-9", "PST8PDT", "MST7MDT", "CST6CDT", "EST5EDT", "<+04>-4", "<+03>-3", "<+02>-2", "<+01>-1", "UTC0", "CET-1CEST", "EET-2EEST", "<+03>-3", "<+04>-4", "<+05>-5", "<+06>-6", "<+07>-7", "<+08>-8", "<+09>-9", "<+10>-10", "<+11>-11", "<+12>-12"};
        for (int i = 0; i < 25; ++i) {
            options += "<option value=\"" + String(tzValues[i]) + "\"" + (strcmp(selectedTz, tzValues[i]) == 0 ? " selected" : "") + ">" + tzNames[i] + "</option>";
        }
        return options;
    };

    auto generateTimeoutOptions = [](int selectedMinutes) {
        String options = "";
        options += String("<option value=\"0\"") + (selectedMinutes == 0 ? " selected" : "") + ">Off</option>";
        for (int h = 1; h <= 12; ++h) {
            int minutes = h * 60;
            options += String("<option value=\"") + minutes + "\"" + (selectedMinutes == minutes ? " selected" : "") + ">" + h + " h</option>";
        }
        return options;
    };

    String dstModeOptions = "";
    const char* dstNames[] = {"Disabled", "European Union Rules", "North America Rules", "Custom..."};
    for (int i = 0; i < 4; ++i) {
        dstModeOptions += "<option value=\"" + String(i) + "\"" + (state.network.dstMode == i ? " selected" : "") + ">" + dstNames[i] + "</option>";
    }

    // Parse custom DST rule for pre-filling
    int sm=0, sw=0, sd=0, em=0, ew=0, ed=0;
    if (state.network.dstMode == 3 && strlen(state.network.customDstRule) > 0) {
        sscanf(state.network.customDstRule, ",M%d.%d.%d,M%d.%d.%d", &sm, &sw, &sd, &em, &ew, &ed);
    }

    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    const char* weeks[] = {"1st", "2nd", "3rd", "4th", "Last"};
    const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

    // Convert volume step to percentage for UI
    int volumePercentForWeb = state.audio.volumeStep * 25;

    // --- Replace Placeholders ---
    html.replace("{TITLE}", "ESP32 Ham Combo v" + String(FW_VERSION) + " (" + String(FW_DATE) + ")");
    html.replace("{USER}", String(state.network.telnetUsername));
    html.replace("{PASS}", String(state.network.telnetPassword));
    html.replace("{BRIGHTNESS}", String(state.display.brightnessPercent));
    html.replace("{VOLUME}", String(volumePercentForWeb));
    html.replace("{TONE}", String(state.audio.toneFrequency));
    html.replace("{TONE_DURATION}", String(state.audio.toneDurationMs));
    html.replace("{CM_UTC_CHECKED}", state.display.currentClockMode == MODE_UTC ? "checked" : "");
    html.replace("{CM_LOCAL_CHECKED}", state.display.currentClockMode == MODE_LOCAL ? "checked" : "");
    html.replace("{CM_BOTH_CHECKED}", state.display.currentClockMode == MODE_BOTH ? "checked" : "");
    html.replace("{PM_SIMPLE_CHECKED}", state.display.currentPropViewMode == VIEW_SIMPLE ? "checked" : "");
    html.replace("{PM_EXTENDED_CHECKED}", state.display.currentPropViewMode == VIEW_EXTENDED ? "checked" : "");
    html.replace("{ROTATION_OPTIONS}", generateRotationOptions(state.display.screenRotation));
    html.replace("{INVERSION_CHECKED}", state.display.colorInversion ? "checked" : "");
    html.replace("{SECOND_DOT_CHECKED}", state.display.secondDotEnabled ? "checked" : "");
    html.replace("{REMEMBER_SCREEN_CHECKED}", state.display.rememberLastScreen ? "checked" : "");
    html.replace("{TIMEOUT_OPTIONS}", generateTimeoutOptions(state.power.sleepTimeoutMinutes));
    html.replace("{SCHED_ON}", state.power.scheduledSleepEnabled ? "checked" : "");
    html.replace("{SLEEP_H}", String(state.power.scheduledSleepHour));
    html.replace("{WAKE_H}", String(state.power.scheduledWakeHour));
    html.replace("{TIMEZONE_OPTIONS}", generateTimezoneOptions(state.network.timezone));
    html.replace("{DST_MODE_OPTIONS}", dstModeOptions);
    html.replace("{MONTH_OPTIONS_START}", generateRuleOptions(months, 12, sm));
    html.replace("{WEEK_OPTIONS_START}", generateRuleOptions(weeks, 5, sw));
    html.replace("{DAY_OPTIONS_START}", generateDayOptions(days, 7, sd));
    html.replace("{MONTH_OPTIONS_END}", generateRuleOptions(months, 12, em));
    html.replace("{WEEK_OPTIONS_END}", generateRuleOptions(weeks, 5, ew));
    html.replace("{DAY_OPTIONS_END}", generateDayOptions(days, 7, ed));
    html.replace("{CHECK_UPDATES_CHECKED}", state.checkForUpdates ? "checked" : "");

    request->send(200, "text/html", html);
  });

  webServer.on("/start_calibration", HTTP_GET, [&state](AsyncWebServerRequest *request){
    state.calibrationRequested = true;
    request->send(200, "text/plain", "Calibration process started. Please follow the instructions on the device screen.");
  });

  webServer.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Restarting...");
    delay(200);
    ESP.restart();
  });

  webServer.begin();
}