#include "SettingsStore.h"

#include <Preferences.h>

#include <cstdio>
#include <cstring>

#include "Config.h"

namespace {

constexpr char kNamespace[] = "coffee_cfg";

float clampFloat(float value, float low, float high) {
  if (value < low) {
    return low;
  }
  if (value > high) {
    return high;
  }
  return value;
}

uint32_t clampU32(uint32_t value, uint32_t low, uint32_t high) {
  if (value < low) {
    return low;
  }
  if (value > high) {
    return high;
  }
  return value;
}

void copyText(char* destination, size_t destinationSize, const String& source) {
  if (destination == nullptr || destinationSize == 0) {
    return;
  }
  const size_t sourceLength = static_cast<size_t>(source.length());
  const size_t length =
      sourceLength < destinationSize - 1 ? sourceLength
                                         : destinationSize - 1;
  if (length > 0) {
    memcpy(destination, source.c_str(), length);
  }
  destination[length] = '\0';
}

void copyText(char* destination, size_t destinationSize, const char* source) {
  copyText(destination, destinationSize, String(source == nullptr ? "" : source));
}

void loadRecipeProfile(Preferences& preferences, const String& prefix,
                       BrewProfile& profile) {
  profile.preinfusionVoltage = preferences.getFloat(
      (prefix + "_pv").c_str(), profile.preinfusionVoltage);
  profile.preinfusionTimeMs = preferences.getUInt(
      (prefix + "_pms").c_str(), profile.preinfusionTimeMs);
  profile.extractionPressureBar = preferences.getFloat(
      (prefix + "_ep").c_str(), profile.extractionPressureBar);
  profile.extractionTimeMs = preferences.getUInt(
      (prefix + "_ems").c_str(), profile.extractionTimeMs);
  profile.pressureToleranceBar = preferences.getFloat(
      (prefix + "_pt").c_str(), profile.pressureToleranceBar);
  profile.rampTimeoutMs = preferences.getUInt(
      (prefix + "_rt").c_str(), profile.rampTimeoutMs);
  profile.maxPressureBar = preferences.getFloat(
      (prefix + "_mp").c_str(), profile.maxPressureBar);
}

void saveRecipeProfile(Preferences& preferences, const String& prefix,
                       const BrewProfile& profile) {
  preferences.putFloat((prefix + "_pv").c_str(),
                       profile.preinfusionVoltage);
  preferences.putUInt((prefix + "_pms").c_str(),
                      profile.preinfusionTimeMs);
  preferences.putFloat((prefix + "_ep").c_str(),
                       profile.extractionPressureBar);
  preferences.putUInt((prefix + "_ems").c_str(),
                      profile.extractionTimeMs);
  preferences.putFloat((prefix + "_pt").c_str(),
                       profile.pressureToleranceBar);
  preferences.putUInt((prefix + "_rt").c_str(),
                      profile.rampTimeoutMs);
  preferences.putFloat((prefix + "_mp").c_str(),
                       profile.maxPressureBar);
}

}  // namespace

RuntimeSettings SettingsStore::validated(RuntimeSettings settings) {
  settings.boilerSetpointC =
      clampFloat(settings.boilerSetpointC, 60.0f, 100.0f);

  settings.profile.preinfusionVoltage =
      clampFloat(settings.profile.preinfusionVoltage, 0.0f,
                 user_config::PUMP_MAX_VOLTS);
  settings.profile.preinfusionTimeMs =
      clampU32(settings.profile.preinfusionTimeMs, 0, 15000);
  settings.profile.extractionPressureBar =
      clampFloat(settings.profile.extractionPressureBar, 2.0f,
                 config::MAX_PROFILE_PRESSURE_BAR);
  settings.profile.extractionTimeMs =
      clampU32(settings.profile.extractionTimeMs, 3000, 60000);
  settings.profile.pressureToleranceBar =
      clampFloat(settings.profile.pressureToleranceBar, 0.1f, 1.5f);
  settings.profile.rampTimeoutMs =
      clampU32(settings.profile.rampTimeoutMs, 1000, 20000);
  settings.profile.maxPressureBar =
      clampFloat(settings.profile.maxPressureBar, 4.0f, 14.0f);
  if (settings.profile.maxPressureBar <=
      settings.profile.extractionPressureBar + 0.5f) {
    settings.profile.maxPressureBar =
        settings.profile.extractionPressureBar + 0.5f;
  }

  settings.pid.kp = clampFloat(settings.pid.kp, 0.0f, 5.0f);
  settings.pid.ki = clampFloat(settings.pid.ki, 0.0f, 5.0f);
  settings.pid.kd = clampFloat(settings.pid.kd, 0.0f, 2.0f);

  if (settings.network.accessPointSsid[0] == '\0') {
    copyText(settings.network.accessPointSsid,
             sizeof(settings.network.accessPointSsid),
             user_config::AP_SSID);
  }
  if (strlen(settings.network.accessPointPassword) < 8) {
    copyText(settings.network.accessPointPassword,
             sizeof(settings.network.accessPointPassword),
             user_config::AP_PASSWORD);
  }

  if (settings.recipes.count == 0 ||
      settings.recipes.count > kMaxRecipes) {
    settings.recipes.count = 1;
    settings.recipes.activeIndex = 0;
    copyText(settings.recipes.items[0].name,
             sizeof(settings.recipes.items[0].name), "我的配方");
    settings.recipes.items[0].profile = settings.profile;
  }
  if (settings.recipes.activeIndex >= settings.recipes.count) {
    settings.recipes.activeIndex = 0;
  }
  for (uint8_t index = 0; index < settings.recipes.count; ++index) {
    if (settings.recipes.items[index].name[0] == '\0') {
      char fallback[16];
      snprintf(fallback, sizeof(fallback), "配方 %u",
               static_cast<unsigned int>(index + 1));
      copyText(settings.recipes.items[index].name,
               sizeof(settings.recipes.items[index].name), fallback);
    }
  }
  settings.recipes.items[settings.recipes.activeIndex].profile =
      settings.profile;
  return settings;
}

RuntimeSettings SettingsStore::load() {
  RuntimeSettings settings;
  copyText(settings.network.accessPointSsid,
           sizeof(settings.network.accessPointSsid),
           user_config::AP_SSID);
  copyText(settings.network.accessPointPassword,
           sizeof(settings.network.accessPointPassword),
           user_config::AP_PASSWORD);
  Preferences preferences;
  if (!preferences.begin(kNamespace, true)) {
    return validated(settings);
  }

  settings.boilerSetpointC = preferences.getFloat(
      "boiler_sp", user_config::DEFAULT_BOILER_SETPOINT_C);
  settings.profile.preinfusionVoltage =
      preferences.getFloat("pre_v", settings.profile.preinfusionVoltage);
  settings.profile.preinfusionTimeMs =
      preferences.getUInt("pre_ms", settings.profile.preinfusionTimeMs);
  settings.profile.extractionPressureBar = preferences.getFloat(
      "ext_p", settings.profile.extractionPressureBar);
  settings.profile.extractionTimeMs =
      preferences.getUInt("ext_ms", settings.profile.extractionTimeMs);
  settings.profile.pressureToleranceBar = preferences.getFloat(
      "tol_p", settings.profile.pressureToleranceBar);
  settings.profile.rampTimeoutMs =
      preferences.getUInt("ramp_ms", settings.profile.rampTimeoutMs);
  settings.profile.maxPressureBar =
      preferences.getFloat("max_p", settings.profile.maxPressureBar);
  settings.pid.kp = preferences.getFloat("pid_p", settings.pid.kp);
  settings.pid.ki = preferences.getFloat("pid_i", settings.pid.ki);
  settings.pid.kd = preferences.getFloat("pid_d", settings.pid.kd);

  copyText(settings.network.stationSsid,
           sizeof(settings.network.stationSsid),
           preferences.getString("wifi_ssid", ""));
  copyText(settings.network.stationPassword,
           sizeof(settings.network.stationPassword),
           preferences.getString("wifi_pw", ""));
  copyText(settings.network.accessPointSsid,
           sizeof(settings.network.accessPointSsid),
           preferences.getString("ap_ssid", user_config::AP_SSID));
  copyText(settings.network.accessPointPassword,
           sizeof(settings.network.accessPointPassword),
           preferences.getString("ap_pw", user_config::AP_PASSWORD));

  const uint8_t savedCount = preferences.getUChar("r_count", 0);
  if (savedCount > 0 && savedCount <= kMaxRecipes) {
    settings.recipes.count = savedCount;
    for (uint8_t index = 0; index < savedCount; ++index) {
      const String prefix =
          String("r") + String(static_cast<unsigned int>(index));
      Recipe& recipe = settings.recipes.items[index];
      copyText(recipe.name, sizeof(recipe.name),
               preferences.getString((prefix + "_nm").c_str(), ""));
      loadRecipeProfile(preferences, prefix, recipe.profile);
    }
    settings.recipes.activeIndex =
        preferences.getUChar("r_active", 0);
    if (settings.recipes.activeIndex >= settings.recipes.count) {
      settings.recipes.activeIndex = 0;
    }
    settings.profile =
        settings.recipes.items[settings.recipes.activeIndex].profile;
  } else {
    settings.recipes.count = 1;
    settings.recipes.activeIndex = 0;
    copyText(settings.recipes.items[0].name,
             sizeof(settings.recipes.items[0].name), "我的配方");
    settings.recipes.items[0].profile = settings.profile;
  }
  preferences.end();
  return validated(settings);
}

bool SettingsStore::save(const RuntimeSettings& settings) {
  const RuntimeSettings safe = validated(settings);
  Preferences preferences;
  if (!preferences.begin(kNamespace, false)) {
    return false;
  }
  preferences.putFloat("boiler_sp", safe.boilerSetpointC);
  preferences.putFloat("pre_v", safe.profile.preinfusionVoltage);
  preferences.putUInt("pre_ms", safe.profile.preinfusionTimeMs);
  preferences.putFloat("ext_p", safe.profile.extractionPressureBar);
  preferences.putUInt("ext_ms", safe.profile.extractionTimeMs);
  preferences.putFloat("tol_p", safe.profile.pressureToleranceBar);
  preferences.putUInt("ramp_ms", safe.profile.rampTimeoutMs);
  preferences.putFloat("max_p", safe.profile.maxPressureBar);
  preferences.putFloat("pid_p", safe.pid.kp);
  preferences.putFloat("pid_i", safe.pid.ki);
  preferences.putFloat("pid_d", safe.pid.kd);
  preferences.putString("wifi_ssid", safe.network.stationSsid);
  preferences.putString("wifi_pw", safe.network.stationPassword);
  preferences.putString("ap_ssid", safe.network.accessPointSsid);
  preferences.putString("ap_pw", safe.network.accessPointPassword);
  preferences.putUChar("r_count", safe.recipes.count);
  preferences.putUChar("r_active", safe.recipes.activeIndex);
  for (uint8_t index = 0; index < safe.recipes.count; ++index) {
    const String prefix =
        String("r") + String(static_cast<unsigned int>(index));
    const Recipe& recipe = safe.recipes.items[index];
    preferences.putString((prefix + "_nm").c_str(), recipe.name);
    saveRecipeProfile(preferences, prefix, recipe.profile);
  }
  preferences.end();
  return true;
}
