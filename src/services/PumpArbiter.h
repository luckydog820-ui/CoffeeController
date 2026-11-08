#ifndef COFFEE_CONTROLLER_PUMP_ARBITER_H_
#define COFFEE_CONTROLLER_PUMP_ARBITER_H_

#include "AppTypes.h"

struct PumpDecision {
  float voltage = 0.0f;
  PumpSource source = PumpSource::kIdle;
};

class PumpArbiter {
 public:
  PumpDecision decide(uint32_t faultMask, bool knobActive,
                      float manualVoltage, BrewState brewState,
                      float autoVoltage) const;
};

#endif  // COFFEE_CONTROLLER_PUMP_ARBITER_H_
