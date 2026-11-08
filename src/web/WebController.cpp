#include "WebController.h"

#include <WiFi.h>

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "AppEvents.h"
#include "Config.h"
#include "WebApp.h"

namespace {

void appendJsonNumber(String& json, const char* key, float value,
                      uint8_t digits = 2) {
  json += '"';
  json += key;
  json += F("\":");
  if (std::isfinite(value)) {
    json += String(value, static_cast<unsigned int>(digits));
  } else {
    json += F("null");
  }
}

void appendBool(String& json, const char* key, bool value) {
  json += '"';
  json += key;
  json += F("\":");
  json += value ? F("true") : F("false");
}

void appendString(String& json, const char* key, const String& value) {
  json += '"';
  json += key;
  json += F("\":\"");
  for (size_t index = 0; index < value.length(); ++index) {
    const char character = value[index];
    if (character == '"' || character == '\\') {
      json += '\\';
      json += character;
    } else if (character == '\n') {
      json += F("\\n");
    } else if (character == '\r') {
      json += F("\\r");
    } else {
      json += character;
    }
  }
  json += '"';
}

void copyText(char* destination, size_t destinationSize, const String& source) {
  if (destination == nullptr || destinationSize == 0) {
    return;
  }
  size_t length = source.length();
  if (length >= destinationSize) {
    length = destinationSize - 1;
  }
  if (length > 0) {
    memcpy(destination, source.c_str(), length);
  }
  destination[length] = '\0';
}

const char* faultName(uint32_t mask) {
  if (mask & kFaultOverpressure) return "超压";
  if (mask & kFaultOvertemperature) return "超温";
  if (mask & kFaultPressureSensor) return "压力传感器异常";
  if (mask & kFaultModbus) return "E5CC 通信中断";
  if (mask & kFaultPumpTimeout) return "泵连续运行超时";
  return "";
}

bool findJsonValue(const String& body, const char* key, const char*& value,
                   size_t& remaining) {
  const String quotedKey = String('"') + key + '"';
  const int keyPosition = body.indexOf(quotedKey);
  if (keyPosition < 0) {
    return false;
  }
  const int colon = body.indexOf(':', keyPosition + quotedKey.length());
  if (colon < 0) {
    return false;
  }
  int start = colon + 1;
  while (start < static_cast<int>(body.length()) &&
         isspace(static_cast<unsigned char>(body[start]))) {
    ++start;
  }
  value = body.c_str() + start;
  remaining = body.length() - start;
  return true;
}

bool readJsonNumber(const String& body, const char* key, float& output) {
  const char* value = nullptr;
  size_t remaining = 0;
  if (!findJsonValue(body, key, value, remaining)) {
    return false;
  }
  char* end = nullptr;
  const double parsed = strtod(value, &end);
  if (end == value) {
    return false;
  }
  output = static_cast<float>(parsed);
  return std::isfinite(output);
}

bool readJsonString(const String& body, const char* key, String& output) {
  const char* value = nullptr;
  size_t remaining = 0;
  if (!findJsonValue(body, key, value, remaining) || remaining < 2 ||
      *value != '"') {
    return false;
  }

  output = "";
  for (size_t index = 1; index < remaining; ++index) {
    const char character = value[index];
    if (character == '"') {
      return true;
    }
    if (character != '\\') {
      output += character;
      continue;
    }
    if (++index >= remaining) {
      return false;
    }
    switch (value[index]) {
      case '"':
      case '\\':
      case '/':
        output += value[index];
        break;
      case 'n':
        output += '\n';
        break;
      case 'r':
        output += '\r';
        break;
      case 't':
        output += '\t';
        break;
      default:
        return false;
    }
  }
  return false;
}

bool isBrewing(const ControllerSnapshot& snapshot) {
  return snapshot.brewState == BrewState::kPreInfusion ||
         snapshot.brewState == BrewState::kPressureRamp ||
         snapshot.brewState == BrewState::kExtraction ||
         snapshot.brewState == BrewState::kStop;
}

}  // namespace

WebController::WebController(ControllerState& state,
                             SettingsStore& settingsStore)
    : state_(state), settingsStore_(settingsStore), server_(80) {}

void WebController::begin() {
  server_.on("/", HTTP_GET, [this]() {
    server_.sendHeader("Cache-Control", "no-store");
    server_.send_P(200, "text/html; charset=utf-8", kIndexHtml);
  });

  server_.on("/api/status", HTTP_GET, [this]() { sendStatus(); });
  server_.on("/api/settings", HTTP_GET, [this]() { sendSettings(); });
  server_.on("/api/settings", HTTP_POST, [this]() { saveSettings(); });
  server_.on("/api/recipes", HTTP_POST, [this]() { saveRecipe(); });
  server_.on("/api/network", HTTP_POST, [this]() { saveNetwork(); });
  server_.on("/api/brew", HTTP_POST, [this]() { commandBrew(); });
  server_.on("/api/fault/reset", HTTP_POST, [this]() { resetFault(); });
  server_.on("/api/health", HTTP_GET, [this]() {
    String health = F("{\"ok\":true,\"firmware\":\"");
    health += config::FIRMWARE_VERSION;
    health += F("\"}");
    server_.send(200, "application/json", health);
  });
  server_.onNotFound([this]() {
    server_.send(404, "application/json", F("{\"error\":\"not_found\"}"));
  });
  server_.begin();
}

void WebController::handle() { server_.handleClient(); }

void WebController::sendStatus() {
  const ControllerSnapshot snapshot = state_.snapshot();
  const ControllerSettingsSnapshot settings = state_.settings();
  String json;
  json.reserve(768);
  json = F("{");
  appendJsonNumber(json, "tempPv", snapshot.boilerPvC, 1);
  json += F(",");
  appendJsonNumber(json, "tempSp", snapshot.boilerSpC, 1);
  json += F(",");
  appendJsonNumber(json, "pressure", snapshot.pressureBar, 2);
  json += F(",");
  appendJsonNumber(json, "pumpVoltage", snapshot.pumpVoltage, 2);
  json += F(",");
  appendJsonNumber(json, "manualVoltage", snapshot.manualVoltage, 2);
  json += F(",");
  json += F("\"brewState\":\"");
  json += brewStateName(snapshot.brewState);
  json += F("\",");
  json += F("\"pumpSource\":\"");
  json += pumpSourceName(snapshot.pumpSource);
  json += F("\",");
  json += F("\"faultMask\":");
  json += String(snapshot.faultMask);
  json += F(",\"faultText\":\"");
  json += faultName(snapshot.faultMask);
  json += F("\",");
  json += F("\"brewElapsedMs\":");
  json += String(snapshot.brewElapsedMs);
  json += F(",\"stageElapsedMs\":");
  json += String(snapshot.stageElapsedMs);
  json += F(",");
  appendBool(json, "modbusOnline", snapshot.modbusOnline);
  json += F(",");
  appendBool(json, "pressureSensorValid", snapshot.pressureSensorValid);
  json += F(",");
  appendBool(json, "knobActive", snapshot.knobActive);
  json += F(",");
  appendBool(json, "boilerAtSetpoint", snapshot.boilerAtSetpoint);
  json += F(",\"uptimeMs\":");
  json += String(snapshot.uptimeMs);
  json += F(",\"heapFree\":");
  json += String(ESP.getFreeHeap());
  json += F(",\"revision\":");
  json += String(settings.revision);
  json += F(",\"activeRecipeIndex\":");
  json += String(settings.settings.recipes.activeIndex);
  json += F(",");
  appendString(
      json, "activeRecipeName",
      String(settings.settings.recipes.items[
                 settings.settings.recipes.activeIndex]
                 .name));
  json += F(",\"ip\":\"");
  const IPAddress webIp =
      WiFi.isConnected() ? WiFi.localIP() : WiFi.softAPIP();
  json += webIp.toString();
  json += F("\",\"rssi\":");
  json += String(WiFi.RSSI());
  json += F(",");
  appendBool(json, "accessPointActive",
             (WiFi.getMode() & WIFI_AP) == WIFI_AP);
  json += F(",");
  appendString(json, "accessPointSsid", user_config::AP_SSID);
  json += F(",");
  appendString(json, "accessPointIp", WiFi.softAPIP().toString());
  json += F(",");
  appendString(json, "stationIp",
               WiFi.isConnected() ? WiFi.localIP().toString() : String());
  json += F(",");
  appendString(json, "networkMode",
               WiFi.status() == WL_CONNECTED ? String("STA + AP")
                                               : String("AP"));
  json += F(",");
  appendString(json, "firmware", config::FIRMWARE_VERSION);
  json += F(",");
  appendString(json, "board", config::BOARD_MODEL);
  json += F("}");

  server_.sendHeader("Cache-Control", "no-store");
  server_.send(200, "application/json; charset=utf-8", json);
}

void WebController::sendSettings() {
  const RuntimeSettings settings = state_.settings().settings;
  String json;
  json.reserve(4096);
  json = F("{");
  appendJsonNumber(json, "boilerSp", settings.boilerSetpointC, 1);
  json += F(",");
  appendJsonNumber(json, "preVoltage", settings.profile.preinfusionVoltage, 2);
  json += F(",\"preTimeMs\":");
  json += String(settings.profile.preinfusionTimeMs);
  json += F(",");
  appendJsonNumber(json, "extractionPressure",
                   settings.profile.extractionPressureBar, 1);
  json += F(",\"extractionTimeMs\":");
  json += String(settings.profile.extractionTimeMs);
  json += F(",");
  appendJsonNumber(json, "pressureTolerance",
                   settings.profile.pressureToleranceBar, 1);
  json += F(",\"rampTimeoutMs\":");
  json += String(settings.profile.rampTimeoutMs);
  json += F(",");
  appendJsonNumber(json, "maxPressure", settings.profile.maxPressureBar, 1);
  json += F(",");
  appendJsonNumber(json, "pidKp", settings.pid.kp, 2);
  json += F(",");
  appendJsonNumber(json, "pidKi", settings.pid.ki, 2);
  json += F(",");
  appendJsonNumber(json, "pidKd", settings.pid.kd, 2);
  json += F(",\"recipes\":[");
  for (uint8_t index = 0; index < settings.recipes.count; ++index) {
    const Recipe& recipe = settings.recipes.items[index];
    if (index > 0) {
      json += ',';
    }
    json += '{';
    appendString(json, "name", String(recipe.name));
    json += ',';
    appendJsonNumber(json, "preVoltage",
                     recipe.profile.preinfusionVoltage, 2);
    json += F(",\"preTimeMs\":");
    json += String(recipe.profile.preinfusionTimeMs);
    json += ',';
    appendJsonNumber(json, "extractionPressure",
                     recipe.profile.extractionPressureBar, 1);
    json += F(",\"extractionTimeMs\":");
    json += String(recipe.profile.extractionTimeMs);
    json += ',';
    appendJsonNumber(json, "pressureTolerance",
                     recipe.profile.pressureToleranceBar, 1);
    json += F(",\"rampTimeoutMs\":");
    json += String(recipe.profile.rampTimeoutMs);
    json += ',';
    appendJsonNumber(json, "maxPressure",
                     recipe.profile.maxPressureBar, 1);
    json += '}';
  }
  json += F("],\"activeRecipeIndex\":");
  json += String(settings.recipes.activeIndex);
  json += ',';
  appendString(
      json, "activeRecipeName",
      String(settings.recipes.items[settings.recipes.activeIndex].name));
  json += ',';
  appendString(json, "wifiSsid", String(settings.network.stationSsid));
  json += ',';
  appendBool(json, "wifiPasswordConfigured",
             settings.network.stationPassword[0] != '\0');
  json += ',';
  appendString(json, "apSsid",
               String(settings.network.accessPointSsid));
  json += ',';
  appendBool(json, "apPasswordConfigured",
             settings.network.accessPointPassword[0] != '\0');
  json += '}';
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(200, "application/json; charset=utf-8", json);
}

bool WebController::readSettingsFromJson(const String& body,
                                         RuntimeSettings& settings) {
  float floatValue = 0.0f;
  if (readJsonNumber(body, "boilerSp", floatValue))
    settings.boilerSetpointC = floatValue;
  if (readJsonNumber(body, "preVoltage", floatValue))
    settings.profile.preinfusionVoltage = floatValue;
  if (readJsonNumber(body, "extractionPressure", floatValue))
    settings.profile.extractionPressureBar = floatValue;
  if (readJsonNumber(body, "pressureTolerance", floatValue))
    settings.profile.pressureToleranceBar = floatValue;
  if (readJsonNumber(body, "maxPressure", floatValue))
    settings.profile.maxPressureBar = floatValue;
  if (readJsonNumber(body, "pidKp", floatValue))
    settings.pid.kp = floatValue;
  if (readJsonNumber(body, "pidKi", floatValue))
    settings.pid.ki = floatValue;
  if (readJsonNumber(body, "pidKd", floatValue))
    settings.pid.kd = floatValue;

  if (readJsonNumber(body, "preTimeMs", floatValue))
    settings.profile.preinfusionTimeMs =
        static_cast<uint32_t>(lroundf(floatValue));
  if (readJsonNumber(body, "extractionTimeMs", floatValue))
    settings.profile.extractionTimeMs =
        static_cast<uint32_t>(lroundf(floatValue));
  if (readJsonNumber(body, "rampTimeoutMs", floatValue))
    settings.profile.rampTimeoutMs =
        static_cast<uint32_t>(lroundf(floatValue));

  return body.indexOf('{') >= 0 && body.indexOf('}') >= 0;
}

bool WebController::readRecipeFromJson(const String& body,
                                       BrewProfile& profile) {
  float floatValue = 0.0f;
  if (readJsonNumber(body, "preVoltage", floatValue))
    profile.preinfusionVoltage = floatValue;
  if (readJsonNumber(body, "extractionPressure", floatValue))
    profile.extractionPressureBar = floatValue;
  if (readJsonNumber(body, "pressureTolerance", floatValue))
    profile.pressureToleranceBar = floatValue;
  if (readJsonNumber(body, "maxPressure", floatValue))
    profile.maxPressureBar = floatValue;
  if (readJsonNumber(body, "preTimeMs", floatValue))
    profile.preinfusionTimeMs =
        static_cast<uint32_t>(lroundf(floatValue));
  if (readJsonNumber(body, "extractionTimeMs", floatValue))
    profile.extractionTimeMs =
        static_cast<uint32_t>(lroundf(floatValue));
  if (readJsonNumber(body, "rampTimeoutMs", floatValue))
    profile.rampTimeoutMs =
        static_cast<uint32_t>(lroundf(floatValue));
  return body.indexOf('{') >= 0 && body.indexOf('}') >= 0;
}

void WebController::saveSettings() {
  const ControllerSnapshot snapshot = state_.snapshot();
  if (isBrewing(snapshot) || snapshot.brewState == BrewState::kManual) {
    server_.send(409, "application/json; charset=utf-8",
                 F("{\"ok\":false,\"error\":\"stop_before_saving\"}"));
    return;
  }

  RuntimeSettings settings = state_.settings().settings;
  if (!readSettingsFromJson(server_.arg("plain"), settings)) {
    server_.send(400, "application/json; charset=utf-8",
                 F("{\"ok\":false,\"error\":\"invalid_json\"}"));
    return;
  }
  settings = SettingsStore::validated(settings);
  if (!settingsStore_.save(settings)) {
    server_.send(500, "application/json; charset=utf-8",
                 F("{\"ok\":false,\"error\":\"nvs_save_failed\"}"));
    return;
  }
  state_.applySettings(settings);
  postAppEvent(AppEventType::kSettingsChanged);
  server_.send(200, "application/json; charset=utf-8",
               F("{\"ok\":true}"));
}

void WebController::saveRecipe() {
  const ControllerSnapshot snapshot = state_.snapshot();
  if (isBrewing(snapshot) || snapshot.brewState == BrewState::kManual) {
    server_.send(409, "application/json; charset=utf-8",
                 F("{\"ok\":false,\"error\":\"stop_before_saving\"}"));
    return;
  }

  const String body = server_.arg("plain");
  String action;
  if (!readJsonString(body, "action", action)) {
    server_.send(400, "application/json; charset=utf-8",
                 F("{\"ok\":false,\"error\":\"missing_action\"}"));
    return;
  }

  RuntimeSettings settings = state_.settings().settings;

  if (action == "select") {
    float requestedIndex = -1.0f;
    if (!readJsonNumber(body, "index", requestedIndex) ||
        requestedIndex < 0 ||
        requestedIndex >= settings.recipes.count) {
      server_.send(400, "application/json; charset=utf-8",
                   F("{\"ok\":false,\"error\":\"invalid_recipe_index\"}"));
      return;
    }
    const uint8_t index = static_cast<uint8_t>(requestedIndex);
    settings.recipes.activeIndex = index;
    settings.profile = settings.recipes.items[index].profile;
  } else if (action == "delete") {
    float requestedIndex = -1.0f;
    if (!readJsonNumber(body, "index", requestedIndex) ||
        requestedIndex < 0 ||
        requestedIndex >= settings.recipes.count) {
      server_.send(400, "application/json; charset=utf-8",
                   F("{\"ok\":false,\"error\":\"invalid_recipe_index\"}"));
      return;
    }
    if (settings.recipes.count <= 1) {
      server_.send(409, "application/json; charset=utf-8",
                   F("{\"ok\":false,\"error\":\"last_recipe\"}"));
      return;
    }
    const uint8_t index = static_cast<uint8_t>(requestedIndex);
    for (uint8_t cursor = index;
         cursor + 1 < settings.recipes.count; ++cursor) {
      settings.recipes.items[cursor] =
          settings.recipes.items[cursor + 1];
    }
    --settings.recipes.count;
    if (settings.recipes.activeIndex == index) {
      settings.recipes.activeIndex =
          index < settings.recipes.count ? index
                                         : settings.recipes.count - 1;
    } else if (settings.recipes.activeIndex > index) {
      --settings.recipes.activeIndex;
    }
    settings.profile =
        settings.recipes.items[settings.recipes.activeIndex].profile;
  } else if (action == "save") {
    String name;
    float requestedIndex = -1.0f;
    if (!readJsonString(body, "name", name) || name.length() == 0) {
      server_.send(400, "application/json; charset=utf-8",
                   F("{\"ok\":false,\"error\":\"missing_recipe_name\"}"));
      return;
    }
    if (name.length() > kRecipeNameLength) {
      server_.send(400, "application/json; charset=utf-8",
                   F("{\"ok\":false,\"error\":\"recipe_name_too_long\"}"));
      return;
    }
    const bool hasIndex = readJsonNumber(body, "index", requestedIndex);
    const bool create =
        !hasIndex || requestedIndex < 0;
    if ((!create &&
         (requestedIndex < 0 ||
          requestedIndex >= settings.recipes.count)) ||
        (create && settings.recipes.count >= kMaxRecipes)) {
      server_.send(create ? 409 : 400, "application/json; charset=utf-8",
                   create
                       ? F("{\"ok\":false,\"error\":\"recipe_limit\"}")
                       : F("{\"ok\":false,\"error\":\"invalid_recipe_index\"}"));
      return;
    }

    const uint8_t target =
        create ? settings.recipes.count
               : static_cast<uint8_t>(requestedIndex);
    BrewProfile profile =
        create ? settings.profile : settings.recipes.items[target].profile;
    if (!readRecipeFromJson(body, profile)) {
      server_.send(400, "application/json; charset=utf-8",
                   F("{\"ok\":false,\"error\":\"invalid_json\"}"));
      return;
    }
    Recipe& recipe = settings.recipes.items[target];
    copyText(recipe.name, sizeof(recipe.name), name);
    RuntimeSettings profileSettings;
    profileSettings.profile = profile;
    recipe.profile = SettingsStore::validated(profileSettings).profile;
    if (create) {
      ++settings.recipes.count;
    }
    settings.recipes.activeIndex = target;
    settings.profile = recipe.profile;
  } else {
    server_.send(400, "application/json; charset=utf-8",
                 F("{\"ok\":false,\"error\":\"invalid_action\"}"));
    return;
  }

  settings = SettingsStore::validated(settings);
  if (!settingsStore_.save(settings)) {
    server_.send(500, "application/json; charset=utf-8",
                 F("{\"ok\":false,\"error\":\"nvs_save_failed\"}"));
    return;
  }
  state_.applySettings(settings);
  postAppEvent(AppEventType::kSettingsChanged);
  String json = F("{\"ok\":true,\"activeRecipeIndex\":");
  json += String(settings.recipes.activeIndex);
  json += F(",\"activeRecipeName\":");
  appendString(
      json, "value",
      String(settings.recipes.items[settings.recipes.activeIndex].name));
  json += '}';
  server_.send(200, "application/json; charset=utf-8", json);
}

void WebController::saveNetwork() {
  const ControllerSnapshot snapshot = state_.snapshot();
  if (isBrewing(snapshot) || snapshot.brewState == BrewState::kManual) {
    server_.send(409, "application/json; charset=utf-8",
                 F("{\"ok\":false,\"error\":\"stop_before_saving\"}"));
    return;
  }

  const String body = server_.arg("plain");
  RuntimeSettings settings = state_.settings().settings;
  String text;
  if (readJsonString(body, "wifiSsid", text)) {
    if (text.length() > kWifiSsidLength) {
      server_.send(400, "application/json; charset=utf-8",
                   F("{\"ok\":false,\"error\":\"invalid_wifi_ssid\"}"));
      return;
    }
    copyText(settings.network.stationSsid,
             sizeof(settings.network.stationSsid), text);
  }

  if (readJsonString(body, "wifiPassword", text) && text.length() > 0) {
    if (text.length() > kWifiPasswordLength) {
      server_.send(400, "application/json; charset=utf-8",
                   F("{\"ok\":false,\"error\":\"invalid_wifi_password\"}"));
      return;
    }
    copyText(settings.network.stationPassword,
             sizeof(settings.network.stationPassword), text);
  }

  if (readJsonString(body, "apSsid", text)) {
    if (text.length() == 0 || text.length() > kWifiSsidLength) {
      server_.send(400, "application/json; charset=utf-8",
                   F("{\"ok\":false,\"error\":\"invalid_ap_ssid\"}"));
      return;
    }
    copyText(settings.network.accessPointSsid,
             sizeof(settings.network.accessPointSsid), text);
  }

  if (readJsonString(body, "apPassword", text) && text.length() > 0) {
    if (text.length() < 8 || text.length() > kWifiPasswordLength) {
      server_.send(400, "application/json; charset=utf-8",
                   F("{\"ok\":false,\"error\":\"invalid_ap_password\"}"));
      return;
    }
    copyText(settings.network.accessPointPassword,
             sizeof(settings.network.accessPointPassword), text);
  }

  settings = SettingsStore::validated(settings);
  if (!settingsStore_.save(settings)) {
    server_.send(500, "application/json; charset=utf-8",
                 F("{\"ok\":false,\"error\":\"nvs_save_failed\"}"));
    return;
  }
  state_.applySettings(settings);
  postAppEvent(AppEventType::kSettingsChanged);
  server_.send(
      200, "application/json; charset=utf-8",
      F("{\"ok\":true,\"restartingNetwork\":true}"));
}

void WebController::commandBrew() {
  String action;
  if (!readJsonString(server_.arg("plain"), "action", action)) {
    server_.send(400, "application/json; charset=utf-8",
                 F("{\"ok\":false,\"error\":\"missing_action\"}"));
    return;
  }

  bool accepted = false;
  if (action == "start") {
    accepted = postAppEvent(AppEventType::kStartBrew);
  } else if (action == "stop") {
    accepted = postAppEvent(AppEventType::kStopBrew);
  } else if (action == "toggle") {
    accepted = postAppEvent(AppEventType::kToggleBrew);
  }
  server_.send(accepted ? 202 : 429, "application/json; charset=utf-8",
               accepted ? F("{\"ok\":true}")
                        : F("{\"ok\":false,\"error\":\"busy\"}"));
}

void WebController::resetFault() {
  const bool accepted = postAppEvent(AppEventType::kFaultReset);
  server_.send(accepted ? 202 : 429, "application/json; charset=utf-8",
               accepted ? F("{\"ok\":true}")
                        : F("{\"ok\":false,\"error\":\"busy\"}"));
}
