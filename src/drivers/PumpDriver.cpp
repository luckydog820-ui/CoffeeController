#include "PumpDriver.h"

#include <esp_arduino_version.h>

#include "Config.h"

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

void writePumpEnable(bool enabled) {
  if (user_config::PIN_PUMP_ENABLE < 0) {
    return;
  }
  const bool level = user_config::PUMP_ENABLE_ACTIVE_HIGH ? enabled : !enabled;
  digitalWrite(user_config::PIN_PUMP_ENABLE, level ? HIGH : LOW);
}

}  // namespace

void PumpDriver::begin() {
  maxDuty_ =
      static_cast<uint16_t>((1UL << user_config::PUMP_PWM_RESOLUTION_BITS) - 1);

  if (user_config::PIN_PUMP_ENABLE >= 0) {
    pinMode(user_config::PIN_PUMP_ENABLE, OUTPUT);
    writePumpEnable(false);
  }

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  initialized_ = ledcAttach(user_config::PIN_PUMP_PWM,
                            user_config::PUMP_PWM_FREQUENCY_HZ,
                            user_config::PUMP_PWM_RESOLUTION_BITS);
#else
  constexpr uint8_t channel = 0;
  ledcSetup(channel, user_config::PUMP_PWM_FREQUENCY_HZ,
            user_config::PUMP_PWM_RESOLUTION_BITS);
  ledcAttachPin(user_config::PIN_PUMP_PWM, channel);
  initialized_ = true;
#endif
  setVoltage(0.0f);
}

void PumpDriver::setVoltage(float volts) {
  commandedVoltage_ =
      clampFloat(volts, 0.0f, user_config::PUMP_MAX_VOLTS);
  if (!initialized_) {
    return;
  }

  const float dutyFloat =
      commandedVoltage_ / config::PUMP_PWM_FULL_SCALE_VOLTS *
      static_cast<float>(maxDuty_);
  const auto duty =
      static_cast<uint16_t>(clampFloat(dutyFloat, 0.0f, maxDuty_));

  writePumpEnable(commandedVoltage_ > 0.01f);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(user_config::PIN_PUMP_PWM, duty);
#else
  ledcWrite(0, duty);
#endif
}

void PumpDriver::emergencyOff() {
  commandedVoltage_ = 0.0f;
  writePumpEnable(false);
  if (!initialized_) {
    return;
  }
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(user_config::PIN_PUMP_PWM, 0);
#else
  ledcWrite(0, 0);
#endif
}
