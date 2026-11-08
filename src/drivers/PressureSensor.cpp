#include "PressureSensor.h"

#include <algorithm>
#include <cmath>

#include "Config.h"

namespace {

float medianOfNine(uint32_t values[9]) {
  std::sort(values, values + 9);
  return static_cast<float>(values[4]) / 1000.0f;
}

}  // namespace

void PressureSensor::begin() {
  pinMode(user_config::PIN_PRESSURE_ADC, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(user_config::PIN_PRESSURE_ADC, ADC_11db);
}

PressureSample PressureSensor::read() {
  uint32_t samplesMillivolts[9] = {};
  for (uint32_t& sample : samplesMillivolts) {
    sample = analogReadMilliVolts(user_config::PIN_PRESSURE_ADC);
    delayMicroseconds(250);
  }

  PressureSample sample;
  sample.pinVoltage = medianOfNine(samplesMillivolts);
  if (config::PRESSURE_DIVIDER_RATIO <= 0.01f) {
    return sample;
  }
  sample.sensorVoltage =
      sample.pinVoltage / config::PRESSURE_DIVIDER_RATIO;

  if (sample.sensorVoltage <
          user_config::PRESSURE_SENSOR_MIN_VOLTAGE - 0.15f ||
      sample.sensorVoltage >
          user_config::PRESSURE_SENSOR_MAX_VOLTAGE + 0.15f ||
      sample.pinVoltage > 3.25f) {
    filteredBar_ = NAN;
    return sample;
  }

  const float ratio =
      (sample.sensorVoltage - user_config::PRESSURE_SENSOR_MIN_VOLTAGE) /
      (user_config::PRESSURE_SENSOR_MAX_VOLTAGE -
       user_config::PRESSURE_SENSOR_MIN_VOLTAGE);
  const float rawBar =
      user_config::PRESSURE_RANGE_MIN_BAR +
      ratio * (user_config::PRESSURE_RANGE_MAX_BAR -
               user_config::PRESSURE_RANGE_MIN_BAR);

  if (!std::isfinite(filteredBar_)) {
    filteredBar_ = rawBar;
  } else {
    filteredBar_ = filteredBar_ * 0.82f + rawBar * 0.18f;
  }
  sample.bar = filteredBar_;
  sample.valid = true;
  return sample;
}
