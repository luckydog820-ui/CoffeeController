#include "SafetyMonitor.h"

#include <cmath>

#include "Config.h"

namespace {

bool elapsed(uint32_t now, uint32_t since, uint32_t threshold) {
  return since != 0 && now - since >= threshold;
}

}  // namespace

SafetyMonitor::SafetyMonitor() : mutex_(xSemaphoreCreateMutex()) {}

uint32_t SafetyMonitor::latchedMask() const {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  const uint32_t mask = latchedMask_;
  xSemaphoreGive(mutex_);
  return mask;
}

uint32_t SafetyMonitor::evaluate(const ControllerSnapshot& snapshot,
                                 const BrewProfile& profile, uint32_t nowMs,
                                 bool& newlyLatched) {
  uint32_t detected = kFaultNone;

  if (!snapshot.pressureSensorValid) {
    if (pressureInvalidSinceMs_ == 0) {
      pressureInvalidSinceMs_ = nowMs;
    }
    if (elapsed(nowMs, pressureInvalidSinceMs_,
                config::PRESSURE_INVALID_TRIP_MS)) {
      detected |= kFaultPressureSensor;
    }
  } else {
    pressureInvalidSinceMs_ = 0;
  }

  if (snapshot.pressureSensorValid &&
      snapshot.pressureBar > profile.maxPressureBar) {
    if (overpressureSinceMs_ == 0) {
      overpressureSinceMs_ = nowMs;
    }
    if (elapsed(nowMs, overpressureSinceMs_,
                config::OVERPRESSURE_TRIP_MS)) {
      detected |= kFaultOverpressure;
    }
  } else {
    overpressureSinceMs_ = 0;
  }

  if (std::isfinite(snapshot.boilerPvC) &&
      snapshot.boilerPvC > user_config::HARD_MAX_BOILER_C) {
    if (overtempSinceMs_ == 0) {
      overtempSinceMs_ = nowMs;
    }
    if (elapsed(nowMs, overtempSinceMs_, config::OVERTEMP_TRIP_MS)) {
      detected |= kFaultOvertemperature;
    }
  } else {
    overtempSinceMs_ = 0;
  }

  if (!snapshot.modbusOnline) {
    if (modbusLostSinceMs_ == 0) {
      modbusLostSinceMs_ = nowMs;
    }
    if (elapsed(nowMs, modbusLostSinceMs_,
                config::MODBUS_LOSS_TRIP_MS)) {
      detected |= kFaultModbus;
    }
  } else {
    modbusLostSinceMs_ = 0;
  }

  if (snapshot.pumpVoltage > 0.2f) {
    if (pumpRunningSinceMs_ == 0) {
      pumpRunningSinceMs_ = nowMs;
    }
    if (elapsed(nowMs, pumpRunningSinceMs_,
                config::PUMP_CONTINUOUS_TRIP_MS)) {
      detected |= kFaultPumpTimeout;
    }
  } else {
    pumpRunningSinceMs_ = 0;
  }

  xSemaphoreTake(mutex_, portMAX_DELAY);
  const uint32_t before = latchedMask_;
  latchedMask_ |= detected;
  newlyLatched = latchedMask_ != before;
  const uint32_t current = latchedMask_;
  xSemaphoreGive(mutex_);
  return current;
}

bool SafetyMonitor::canReset(
    const ControllerSnapshot& snapshot) const {
  return snapshot.pressureSensorValid &&
         std::isfinite(snapshot.pressureBar) &&
         snapshot.pressureBar < config::SAFE_RESET_MAX_PRESSURE_BAR &&
         std::isfinite(snapshot.boilerPvC) &&
         snapshot.boilerPvC < user_config::HARD_MAX_BOILER_C &&
         snapshot.modbusOnline &&
         snapshot.pumpVoltage < 0.1f && !snapshot.knobActive;
}

void SafetyMonitor::clearLatch() {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  latchedMask_ = kFaultNone;
  xSemaphoreGive(mutex_);
}
