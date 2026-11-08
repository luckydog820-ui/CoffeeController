#include "PumpArbiter.h"

#include <cmath>

#include "Config.h"

namespace {

float clampFloat(float value, float low, float high) {
  if (!std::isfinite(value)) {
    return low;
  }
  if (value < low) {
    return low;
  }
  if (value > high) {
    return high;
  }
  return value;
}

}  // namespace

PumpDecision PumpArbiter::decide(uint32_t faultMask, bool knobActive,
                                 float manualVoltage, BrewState brewState,
                                 float autoVoltage) const {
  PumpDecision decision;
  if (faultMask != kFaultNone || brewState == BrewState::kFault) {
    decision.source = PumpSource::kSafety;
    return decision;
  }

  if (knobActive) {
    decision.voltage =
        clampFloat(manualVoltage, 0.0f, user_config::PUMP_MAX_VOLTS);
    decision.source = PumpSource::kManual;
    return decision;
  }

  if (brewState == BrewState::kPreInfusion ||
      brewState == BrewState::kPressureRamp ||
      brewState == BrewState::kExtraction) {
    decision.voltage =
        clampFloat(autoVoltage, 0.0f, user_config::PUMP_MAX_VOLTS);
    decision.source = PumpSource::kAuto;
  }
  return decision;
}
