#ifndef COFFEE_CONTROLLER_SAFETY_MONITOR_H_
#define COFFEE_CONTROLLER_SAFETY_MONITOR_H_

#include "AppTypes.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class SafetyMonitor {
 public:
  SafetyMonitor();

  uint32_t evaluate(const ControllerSnapshot& snapshot,
                    const BrewProfile& profile, uint32_t nowMs,
                    bool& newlyLatched);
  bool canReset(const ControllerSnapshot& snapshot) const;
  uint32_t latchedMask() const;
  void clearLatch();

 private:
  mutable SemaphoreHandle_t mutex_;
  uint32_t latchedMask_ = kFaultNone;
  uint32_t pressureInvalidSinceMs_ = 0;
  uint32_t overpressureSinceMs_ = 0;
  uint32_t overtempSinceMs_ = 0;
  uint32_t modbusLostSinceMs_ = 0;
  uint32_t pumpRunningSinceMs_ = 0;
};

#endif  // COFFEE_CONTROLLER_SAFETY_MONITOR_H_
