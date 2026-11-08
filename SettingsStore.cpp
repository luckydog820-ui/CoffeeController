#include "SettingsStore.h"

#include <Preferences.h>

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
  return settings;
}

RuntimeSettings SettingsStore::load() {
  RuntimeSettings settings;
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
  preferences.end();
  return true;
}
