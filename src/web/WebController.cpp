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
  const char* closingQuote = strchr(value + 1, '"');
  if (closingQuote == nullptr) {
    return false;
  }
  output = String(value + 1).substring(
      0, static_cast<unsigned int>(closingQuote - (value + 1)));
  return true;
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
  json.reserve(384);
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
  json += F("}");
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
