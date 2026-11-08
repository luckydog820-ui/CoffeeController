#ifndef COFFEE_CONTROLLER_APP_TYPES_H_
#define COFFEE_CONTROLLER_APP_TYPES_H_

#include <Arduino.h>

enum class BrewState : uint8_t {
  kBoot,
  kIdle,
  kReady,
  kPreInfusion,
  kPressureRamp,
  kExtraction,
  kStop,
  kManual,
  kFault,
};

enum FaultBits : uint32_t {
  kFaultNone = 0,
  kFaultModbus = 1UL << 0,
  kFaultOvertemperature = 1UL << 1,
  kFaultOverpressure = 1UL << 2,
  kFaultPressureSensor = 1UL << 3,
  kFaultPumpTimeout = 1UL << 4,
};

enum class PumpSource : uint8_t {
  kIdle,
  kAuto,
  kManual,
  kSafety,
};

constexpr uint8_t kMaxRecipes = 8;
constexpr uint8_t kRecipeNameLength = 32;
constexpr uint8_t kWifiSsidLength = 32;
constexpr uint8_t kWifiPasswordLength = 63;

struct BrewProfile {
  float preinfusionVoltage = 1.2f;
  uint32_t preinfusionTimeMs = 4000;
  float extractionPressureBar = 9.0f;
  uint32_t extractionTimeMs = 25000;
  float pressureToleranceBar = 0.3f;
  uint32_t rampTimeoutMs = 8000;
  float maxPressureBar = 11.5f;
};

struct PidConfig {
  float kp = 0.9f;
  float ki = 0.6f;
  float kd = 0.02f;
};

struct Recipe {
  char name[kRecipeNameLength + 1] = {};
  BrewProfile profile;
};

struct RecipeLibrary {
  Recipe items[kMaxRecipes];
  uint8_t count = 0;
  uint8_t activeIndex = 0;
};

struct NetworkConfig {
  char stationSsid[kWifiSsidLength + 1] = {};
  char stationPassword[kWifiPasswordLength + 1] = {};
  char accessPointSsid[kWifiSsidLength + 1] = {};
  char accessPointPassword[kWifiPasswordLength + 1] = {};
};

enum class E5ccParity : uint8_t {
  kNone = 0,
  kEven = 1,
  kOdd = 2,
};

struct E5ccConfig {
  uint8_t slaveId = 1;
  uint32_t baud = 9600;
  E5ccParity parity = E5ccParity::kEven;
  uint16_t pvRegister = 0x2000;
  uint16_t spRegister = 0x2103;
  float temperatureScale = 0.1f;
};

struct RuntimeSettings {
  BrewProfile profile;
  PidConfig pid;
  float boilerSetpointC = 93.0f;
  float pressureReliefBar = 11.0f;
  E5ccConfig e5cc;
  RecipeLibrary recipes;
  NetworkConfig network;
};

struct ControllerSnapshot {
  float boilerPvC = NAN;
  float boilerSpC = 93.0f;
  float pressureBar = NAN;
  float pumpVoltage = 0.0f;
  float manualVoltage = 0.0f;
  BrewState brewState = BrewState::kBoot;
  PumpSource pumpSource = PumpSource::kIdle;
  uint32_t faultMask = kFaultNone;
  uint32_t brewElapsedMs = 0;
  uint32_t stageElapsedMs = 0;
  uint32_t stateEnteredMs = 0;
  uint32_t uptimeMs = 0;
  bool modbusOnline = false;
  bool pressureSensorValid = false;
  bool knobActive = false;
  bool boilerAtSetpoint = false;
  bool extractionValveOpen = false;
  bool pressureReliefActive = false;
};

struct ControllerSettingsSnapshot {
  RuntimeSettings settings;
  uint32_t revision = 0;
};

enum class AppEventType : uint8_t {
  kToggleBrew,
  kStartBrew,
  kStopBrew,
  kManualOverride,
  kManualReleased,
  kFaultReset,
  kSafetyTrip,
  kSettingsChanged,
};

struct AppEvent {
  AppEventType type = AppEventType::kStopBrew;
  float value = 0.0f;
};

inline const char* brewStateName(BrewState state) {
  switch (state) {
    case BrewState::kBoot:
      return "BOOT";
    case BrewState::kIdle:
      return "IDLE";
    case BrewState::kReady:
      return "READY";
    case BrewState::kPreInfusion:
      return "PRE_INFUSION";
    case BrewState::kPressureRamp:
      return "PRESSURE_RAMP";
    case BrewState::kExtraction:
      return "EXTRACTION";
    case BrewState::kStop:
      return "STOP";
    case BrewState::kManual:
      return "MANUAL";
    case BrewState::kFault:
      return "FAULT";
  }
  return "UNKNOWN";
}

inline const char* pumpSourceName(PumpSource source) {
  switch (source) {
    case PumpSource::kAuto:
      return "AUTO";
    case PumpSource::kManual:
      return "MANUAL";
    case PumpSource::kSafety:
      return "SAFETY";
    case PumpSource::kIdle:
    default:
      return "IDLE";
  }
}

#endif  // COFFEE_CONTROLLER_APP_TYPES_H_
