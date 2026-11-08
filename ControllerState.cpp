#include "ControllerState.h"

#include <cmath>

#include "Config.h"

ControllerState::ControllerState() : mutex_(xSemaphoreCreateMutex()) {
  snapshot_.brewState = BrewState::kBoot;
  snapshot_.boilerSpC = settings_.boilerSetpointC;
}

ControllerSnapshot ControllerState::snapshot() const {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  ControllerSnapshot copy = snapshot_;
  copy.uptimeMs = millis();
  copy.brewState = snapshot_.brewState;
  copy.boilerAtSetpoint =
      std::isfinite(copy.boilerPvC) &&
      copy.boilerPvC >=
          copy.boilerSpC - user_config::BOILER_READY_TOLERANCE_C;

  if (copy.stateEnteredMs != 0) {
    copy.stageElapsedMs = copy.uptimeMs - copy.stateEnteredMs;
  }
  if (brewTimingActive_ && brewStartedMs_ != 0) {
    copy.brewElapsedMs = copy.uptimeMs - brewStartedMs_;
  }
  xSemaphoreGive(mutex_);
  return copy;
}

ControllerSettingsSnapshot ControllerState::settings() const {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  ControllerSettingsSnapshot copy;
  copy.settings = settings_;
  copy.revision = settingsRevision_;
  xSemaphoreGive(mutex_);
  return copy;
}

void ControllerState::applySettings(const RuntimeSettings& settings) {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  settings_ = settings;
  snapshot_.boilerSpC = settings_.boilerSetpointC;
  ++settingsRevision_;
  xSemaphoreGive(mutex_);
}

void ControllerState::updateBoilerPv(float celsius) {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  snapshot_.boilerPvC = celsius;
  xSemaphoreGive(mutex_);
}

void ControllerState::setModbusOnline(bool online) {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  snapshot_.modbusOnline = online;
  xSemaphoreGive(mutex_);
}

void ControllerState::updatePressure(float bar, bool valid) {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  snapshot_.pressureBar = bar;
  snapshot_.pressureSensorValid = valid;
  xSemaphoreGive(mutex_);
}

void ControllerState::updateKnob(bool active, float requestedVoltage) {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  snapshot_.knobActive = active;
  snapshot_.manualVoltage = requestedVoltage;
  xSemaphoreGive(mutex_);
}

void ControllerState::updatePump(float commandedVoltage, PumpSource source) {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  snapshot_.pumpVoltage = commandedVoltage;
  snapshot_.pumpSource = source;
  xSemaphoreGive(mutex_);
}

void ControllerState::updateHydraulics(bool extractionValveOpen,
                                       bool pressureReliefActive) {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  snapshot_.extractionValveOpen = extractionValveOpen;
  snapshot_.pressureReliefActive = pressureReliefActive;
  xSemaphoreGive(mutex_);
}

void ControllerState::setFaultMask(uint32_t mask) {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  snapshot_.faultMask = mask;
  xSemaphoreGive(mutex_);
}

void ControllerState::enterBrewState(BrewState state, uint32_t nowMs) {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  const BrewState previous = snapshot_.brewState;
  snapshot_.brewState = state;
  snapshot_.stateEnteredMs = nowMs;
  snapshot_.stageElapsedMs = 0;

  const bool enteringBrew =
      state == BrewState::kPreInfusion &&
      previous != BrewState::kPreInfusion;
  if (enteringBrew) {
    brewStartedMs_ = nowMs;
    brewTimingActive_ = true;
  }
  if (state == BrewState::kIdle || state == BrewState::kReady ||
      state == BrewState::kFault) {
    brewTimingActive_ = false;
    brewStartedMs_ = 0;
    snapshot_.brewElapsedMs = 0;
  }
  xSemaphoreGive(mutex_);
}
