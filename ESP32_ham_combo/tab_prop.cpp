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

  // A custom, minimal, single-pass XML parser.
  // It uses a state machine to process the XML character by character.
  // This implementation avoids external heavy libraries.
  class SimpleXmlParser {
  public:
    void parse(const char* xml, ApplicationState& state) {
      _state = &state;
      const char* p = xml;

      // Reset all propagation data to UNKNOWN before parsing
      for (int i = 0; i < 8; ++i) _state->solarData.propagation[i] = UNKNOWN;
      memset(&_state->solarData.vhf, 0, sizeof(_state->solarData.vhf));

      while (*p) {
        if (*p == '<' && *(p + 1) != '/') { // Start of a new tag
          p++; // Skip '<'
          p = parseTag(p);
        } else {
          p++;
        }
      }
    }

  private:
    enum ParserState { NONE, IN_TAG, IN_CONTENT };
    ApplicationState* _state;

    // Extracts tag name and attributes
    const char* parseTag(const char* p) {
      char tagName[32] = {0};
      int i = 0;
      while (*p && *p != ' ' && *p != '>' && i < 31) {
        tagName[i++] = *p++;
      }
      tagName[i] = '\0';

      // --- Handle tags with attributes ---
      if (strcmp(tagName, "band") == 0) {
        return handleBandTag(p);
      }
      if (strcmp(tagName, "phenomenon") == 0) {
        return handlePhenomenonTag(p);
      }

      // --- Handle simple content tags ---
      while (*p && *p != '>') p++; // Skip to end of tag
      if (*p == '>') p++; // Skip '>'

      char content[64] = {0};
      i = 0;
      while (*p && *p != '<' && i < 63) {
        content[i++] = *p++;
      }
      content[i] = '\0';

      // Trim whitespace from content
      char* start = content;
      while (isspace(*start)) start++;
      char* end = start + strlen(start) - 1;
      while (end > start && isspace(*end)) *end-- = '\0';

      // Assign content to the correct state variable
      if (strcmp(tagName, "solarflux") == 0) _state->solarData.solarFlux = atoi(start);
      else if (strcmp(tagName, "aindex") == 0) _state->solarData.aIndex = atoi(start);
      else if (strcmp(tagName, "kindex") == 0) _state->solarData.kIndex = atoi(start);
      else if (strcmp(tagName, "sunspots") == 0) _state->solarData.sunspots = atoi(start);
      else if (strcmp(tagName, "xray") == 0) strlcpy(_state->solarData.xray, start, sizeof(_state->solarData.xray));
      else if (strcmp(tagName, "geomagfield") == 0) strlcpy(_state->solarData.geomagneticField, start, sizeof(_state->solarData.geomagneticField));
      else if (strcmp(tagName, "signalnoise") == 0) strlcpy(_state->solarData.signalNoiseLevel, start, sizeof(_state->solarData.signalNoiseLevel));

      return p;
    }

    const char* getAttrValue(const char* p, const char* attrName, char* buffer, size_t len) {
      const char* found = strstr(p, attrName);
      if (!found) return p;
      found += strlen(attrName) + 2; // Skip attrName and ="
      int i = 0;
      while (*found && *found != '"' && i < len - 1) {
        buffer[i++] = *found++;
      }
      buffer[i] = '\0';
      return found;
    }

    const char* handleBandTag(const char* p) {
      char name[16] = {0}, time[8] = {0};
      const char* endOfTag = strchr(p, '>');
      if (!endOfTag) return p;

      getAttrValue(p, "name", name, sizeof(name));
      getAttrValue(p, "time", time, sizeof(time));

      p = endOfTag + 1;
      char content[16] = {0};
      int i = 0;
      while (*p && *p != '<' && i < 15) content[i++] = *p++;
      content[i] = '\0';

      PropagationCondition condition = toConditionValue(content);
      
      // Map band/time to array index
      // 0-3: Day (80-40, 30-20, 17-15, 12-10)
      // 4-7: Night (same order)
      if (strcmp(time, "day") == 0) {
          if (strcmp(name, "80m-40m") == 0) _state->solarData.propagation[0] = condition;
          else if (strcmp(name, "30m-20m") == 0) _state->solarData.propagation[1] = condition;
          else if (strcmp(name, "17m-15m") == 0) _state->solarData.propagation[2] = condition;
          else if (strcmp(name, "12m-10m") == 0) _state->solarData.propagation[3] = condition;
      } else if (strcmp(time, "night") == 0) {
          if (strcmp(name, "80m-40m") == 0) _state->solarData.propagation[4] = condition;
          else if (strcmp(name, "30m-20m") == 0) _state->solarData.propagation[5] = condition;
          else if (strcmp(name, "17m-15m") == 0) _state->solarData.propagation[6] = condition;
          else if (strcmp(name, "12m-10m") == 0) _state->solarData.propagation[7] = condition;
      }

      return p;
    }

    const char* handlePhenomenonTag(const char* p) {
        char name[32] = {0}, location[32] = {0};
        const char* endOfTag = strchr(p, '>');
        if (!endOfTag) return p;

        getAttrValue(p, "name", name, sizeof(name));
        getAttrValue(p, "location", location, sizeof(location));

        p = endOfTag + 1;
        char content[32] = {0};
        int i = 0;
        while (*p && *p != '<' && i < 31) content[i++] = *p++;
        content[i] = '\0';

        if (strcmp(name, "vhf-aurora") == 0) {
            strlcpy(_state->solarData.vhf.aurora, content, sizeof(_state->solarData.vhf.aurora));
        } else if (strcmp(name, "E-Skip") == 0) {
            if (strcmp(location, "europe") == 0) strlcpy(_state->solarData.vhf.eSkipEurope2m, content, sizeof(_state->solarData.vhf.eSkipEurope2m));
            else if (strcmp(location, "europe_4m") == 0) strlcpy(_state->solarData.vhf.eSkipEurope4m, content, sizeof(_state->solarData.vhf.eSkipEurope4m));
            else if (strcmp(location, "europe_6m") == 0) strlcpy(_state->solarData.vhf.eSkipEurope6m, content, sizeof(_state->solarData.vhf.eSkipEurope6m));
        }
        return p;
    }
  };

  bool parsePropagationData(ApplicationState& state, const char* payload) {
    SimpleXmlParser parser;
    parser.parse(payload, state);
    return true;
  }

} // end of anonymous namespace

bool fetchPropagationData(ApplicationState& state) {
  if (!state.network.isWifiConnected) {
    state.propDataAvailable = false;
    return false;
  }

  Serial.println("Fetching propagation data...");
  httpClient.get(PROP_URL);

  int statusCode = httpClient.responseStatusCode();
  if (statusCode != 200) {
    Serial.printf("Failed to fetch data, status code: %d\n", statusCode);
    state.propDataAvailable = false;
    return false;
  }

  String response = httpClient.responseBody();

  if (parsePropagationData(state, response.c_str())) {
    Serial.println("Propagation data fetched and parsed successfully.");
    state.propDataAvailable = true;
    state.lastPropUpdateTime = millis();
    return true;
  } else {
    Serial.println("Failed to parse propagation data.");
    state.propDataAvailable = false;
    return false;
  }
}

void drawPropagationScreen(const ApplicationState& state) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(&FreeSans9pt7b);

  if (!state.network.isWifiConnected) {
    tft.setTextColor(TFT_RED);
    tft.drawString("WiFi Connection Lost", tft.width() / 2, tft.height() / 2 - 15);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Reconnecting...", tft.width() / 2, tft.height() / 2 + 15);
    return;
  }

  if (!state.propDataAvailable) {
    tft.setTextColor(TFT_YELLOW);
    tft.drawString("Failed to fetch data.", tft.width() / 2, tft.height() / 2 - 10);
    tft.drawString("Check connection.", tft.width() / 2, tft.height() / 2 + 10);
    return;
  }

  if (state.display.currentPropViewMode == VIEW_EXTENDED) {
    const char* bandNames[] = {"80-40", "30-20", "17-15", "12-10"};
    int verticalCenterlineX = tft.width() / 2;
    int leftHalfCenterX = verticalCenterlineX / 2;
    int rightHalfCenterX = verticalCenterlineX + (tft.width() - verticalCenterlineX) / 2;

    tft.setFreeFont(&FreeSansBold12pt7b);
    tft.setTextDatum(MC_DATUM);

    // --- Draw HF Bands (Top Section) ---
    const int topSectionHeight = PROP_H_LINE_Y;
    const int topVerticalGap = topSectionHeight / 4;
    const int topStartY = topVerticalGap / 2;

    for (int i = 0; i < 4; i++) {
      int yPos = topStartY + i * topVerticalGap;
      // Day
      tft.setTextColor(getPropagationColor(state.solarData.propagation[i]));
      tft.drawString(String(bandNames[i]) + " D", leftHalfCenterX, yPos);
      // Night
      tft.setTextColor(getPropagationColor(state.solarData.propagation[i + 4]));
      tft.drawString(String(bandNames[i]) + " N", rightHalfCenterX, yPos);
    }

    // Separator Lines
    tft.drawFastVLine(verticalCenterlineX, PROP_V_LINE_TOP_MARGIN, PROP_H_LINE_Y - PROP_V_LINE_BOTTOM_MARGIN, TFT_DARKGREY);
    tft.drawFastHLine(0, PROP_H_LINE_Y, tft.width(), TFT_DARKGREY);

    // --- Draw Solar Data (Bottom Section) ---
    const int bottomSectionTopY = PROP_H_LINE_Y + 1;
    const int bottomSectionHeight = tft.height() - bottomSectionTopY;
    const int bottomVerticalGap = bottomSectionHeight / 5;
    const int firstRowY = bottomSectionTopY + (bottomVerticalGap / 2);
    const int rowOffsetY = bottomVerticalGap;

    int col2X = verticalCenterlineX + PROP_COL2_X_OFFSET;

    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextDatum(TL_DATUM);

    // Row 1: A/K Index
    tft.setTextColor(TFT_WHITE);
    tft.drawString("A/K Index:", PROP_COL1_X, firstRowY);
    int currentX = PROP_COL1_X + PROP_VALUE_OFFSET_X;
    tft.setTextColor(getAIndexColor(state.solarData.aIndex));
    tft.drawString(String(state.solarData.aIndex), currentX, firstRowY);
    currentX += tft.textWidth(String(state.solarData.aIndex));
    tft.setTextColor(TFT_WHITE);
    tft.drawString(" / ", currentX, firstRowY);
    currentX += tft.textWidth(" / ");
    tft.setTextColor(getKIndexColor(state.solarData.kIndex));
    tft.drawString(String(state.solarData.kIndex), currentX, firstRowY);

    // Row 2: Solar Flux
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Solar Flux:", PROP_COL1_X, firstRowY + rowOffsetY);
    tft.setTextColor(getSolarFluxColor(state.solarData.solarFlux));
    tft.drawString(String(state.solarData.solarFlux), PROP_COL1_X + PROP_VALUE_OFFSET_X, firstRowY + rowOffsetY);

    // Row 3: Sunspots
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Sunspots:", PROP_COL1_X, firstRowY + 2 * rowOffsetY);
    uint16_t sunspotColor = TFT_RED;
    if (state.solarData.sunspots > SUNSPOTS_GOOD_THRESHOLD) sunspotColor = TFT_GREEN;
    else if (state.solarData.sunspots > SUNSPOTS_FAIR_THRESHOLD) sunspotColor = TFT_YELLOW;
    tft.setTextColor(sunspotColor);
    tft.drawString(String(state.solarData.sunspots), PROP_COL1_X + PROP_VALUE_OFFSET_X, firstRowY + 2 * rowOffsetY);

    // Row 4: X-Ray
    tft.setTextColor(TFT_WHITE);
    tft.drawString("X-Ray:", PROP_COL1_X, firstRowY + 3 * rowOffsetY);
    tft.setTextColor(getXRayColor(state.solarData.xray));
    tft.drawString(state.solarData.xray, PROP_COL1_X + PROP_VALUE_OFFSET_X, firstRowY + 3 * rowOffsetY);

    // Row 5: Geo Field
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Geo Field:", PROP_COL1_X, firstRowY + 4 * rowOffsetY);
    char geo[16];
    strlcpy(geo, state.solarData.geomagneticField, sizeof(geo));
    if (strcmp(geo, "UNSETTLD") == 0) strlcpy(geo, "UNSET.", sizeof(geo));
    tft.setTextColor(getGeomagFieldColor(state.solarData.geomagneticField));
    tft.drawString(geo, PROP_COL1_X + PROP_VALUE_OFFSET_X, firstRowY + 4 * rowOffsetY);

    // --- VHF Column ---
    int valueXRight = col2X + tft.textWidth("Aurora:") + PROP_LABEL_VALUE_GAP_X;

    auto drawVhfCondition = [&](const char* label, int rowIndex, const char* conditionValue) {
        tft.setTextColor(TFT_WHITE);
        tft.drawString(label, col2X, firstRowY + rowIndex * rowOffsetY);
        char condition[16];
        strlcpy(condition, conditionValue, sizeof(condition));
        if (strlen(condition) == 0) strlcpy(condition, "N/A", sizeof(condition));
        if (strcmp(condition, "Band Closed") == 0) strlcpy(condition, "Closed", sizeof(condition));
        tft.setTextColor(getVhfConditionsColor(conditionValue));
        tft.drawString(condition, valueXRight, firstRowY + rowIndex * rowOffsetY);
    };

    drawVhfCondition("6m:", 0, state.solarData.vhf.eSkipEurope6m);
    drawVhfCondition("4m:", 1, state.solarData.vhf.eSkipEurope4m);
    drawVhfCondition("2m:", 2, state.solarData.vhf.eSkipEurope2m);
    drawVhfCondition("Aurora:", 3, state.solarData.vhf.aurora);

    tft.setTextColor(TFT_WHITE);
    tft.drawString("SNL:", col2X, firstRowY + 4 * rowOffsetY);
    tft.setTextColor(getSignalNoiseColor(state.solarData.signalNoiseLevel));
    tft.drawString(state.solarData.signalNoiseLevel, valueXRight, firstRowY + 4 * rowOffsetY);

  } else { // Simple View
    const char* bandNames[] = {"80-40", "30-20", "17-15", "12-10"};
    int verticalCenterlineX = tft.width() / 2;
    int leftHalfCenterX = verticalCenterlineX / 2;
    int rightHalfCenterX = verticalCenterlineX + (tft.width() - verticalCenterlineX) / 2;

    tft.setFreeFont(&FreeSansBold18pt7b);
    tft.setTextDatum(MC_DATUM);

    int verticalGap = tft.height() / 4;
    int startY = verticalGap / 2;

    for (int i = 0; i < 4; i++) {
      int yPos = startY + i * verticalGap;
      tft.setTextColor(getPropagationColor(state.solarData.propagation[i]));
      tft.drawString(String(bandNames[i]) + " D", leftHalfCenterX, yPos);
      tft.setTextColor(getPropagationColor(state.solarData.propagation[i + 4]));
      tft.drawString(String(bandNames[i]) + " N", rightHalfCenterX, yPos);
    }

    tft.drawFastVLine(verticalCenterlineX, PROP_SIMPLE_V_LINE_TOP_MARGIN, tft.height() - PROP_SIMPLE_V_LINE_BOTTOM_MARGIN, TFT_DARKGREY);
  }
}

PropagationCondition toConditionValue(const char* val) {
  char lowerVal[16];
  strlcpy(lowerVal, val, sizeof(lowerVal));
  for (char *p = lowerVal; *p; ++p) *p = tolower(*p);

  if (strcmp(lowerVal, "good") == 0) return GOOD;
  if (strcmp(lowerVal, "fair") == 0) return FAIR;
  if (strcmp(lowerVal, "poor") == 0) return POOR;
  return UNKNOWN;
}
