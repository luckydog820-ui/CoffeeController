#include "PressureLoop.h"

#include <cmath>

#include "UserConfig.h"

namespace {

float clampFloat(float value, float low, float high) {
  if (value < low) {
    return low;
  }
  if (value > high) {
    return high;
  }
  return value;
}

}  // namespace

void PressureLoop::reset() {
  integral_ = 0.0f;
  previousMeasurement_ = NAN;
  filteredDerivative_ = 0.0f;
}

float PressureLoop::update(float targetBar, float measuredBar,
                           float dtSeconds, const PidConfig& config) {
  if (!std::isfinite(targetBar) || !std::isfinite(measuredBar) ||
      dtSeconds <= 0.0f || dtSeconds > 1.0f) {
    reset();
    return 0.0f;
  }

  const float error = targetBar - measuredBar;
  float derivative = 0.0f;
  if (std::isfinite(previousMeasurement_)) {
    derivative =
        -(measuredBar - previousMeasurement_) / dtSeconds;
  }
  previousMeasurement_ = measuredBar;
  filteredDerivative_ =
      filteredDerivative_ * 0.78f + derivative * 0.22f;

  const float proposedIntegral =
      integral_ + config.ki * error * dtSeconds;
  const float proportional = config.kp * error;
  const float derivativeTerm = config.kd * filteredDerivative_;
  float output = proportional + proposedIntegral + derivativeTerm;

  const bool atHigh = output >= user_config::PUMP_MAX_VOLTS;
  const bool atLow = output <= 0.0f;
  const bool pushingHigh = error > 0.0f;
  const bool pushingLow = error < 0.0f;
  if ((atHigh && pushingHigh) || (atLow && pushingLow)) {
    output = proportional + integral_ + derivativeTerm;
  } else {
    integral_ = proposedIntegral;
  }

  return clampFloat(output, 0.0f, user_config::PUMP_MAX_VOLTS);
}
