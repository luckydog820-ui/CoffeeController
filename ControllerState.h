#ifndef COFFEE_CONTROLLER_CONTROLLER_STATE_H_
#define COFFEE_CONTROLLER_CONTROLLER_STATE_H_

#include "AppTypes.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class ControllerState {
 public:
  ControllerState();

  ControllerSnapshot snapshot() const;
  ControllerSettingsSnapshot settings() const;

  void applySettings(const RuntimeSettings& settings);
  void updateBoilerPv(float celsius);
  void setModbusOnline(bool online);
  void updatePressure(float bar, bool valid);
  void updateKnob(bool active, float requestedVoltage);
  void updatePump(float commandedVoltage, PumpSource source);
  void updateHydraulics(bool extractionValveOpen, bool pressureReliefActive);
  void setFaultMask(uint32_t mask);
  void enterBrewState(BrewState state, uint32_t nowMs);

 private:
  mutable SemaphoreHandle_t mutex_;
  ControllerSnapshot snapshot_;
  RuntimeSettings settings_;
  uint32_t settingsRevision_ = 0;
  uint32_t brewStartedMs_ = 0;
  bool brewTimingActive_ = false;
};

#endif  // COFFEE_CONTROLLER_CONTROLLER_STATE_H_
