#ifndef COFFEE_CONTROLLER_CONFIG_H_
#define COFFEE_CONTROLLER_CONFIG_H_

#include "UserConfig.h"

namespace config {

constexpr char FIRMWARE_VERSION[] = "0.4.0";
constexpr char DEVICE_NAME[] = "CoffeePilot";
constexpr char BOARD_MODEL[] = "ESP32-S3-N16R8";

constexpr uint32_t CONTROL_PERIOD_MS = 50;
constexpr uint32_t PID_PERIOD_MS = 100;
constexpr uint32_t SAFETY_PERIOD_MS = 50;
constexpr uint32_t INPUT_PERIOD_MS = 20;
constexpr uint32_t MODBUS_POLL_MS = 500;

constexpr uint32_t PRESSURE_INVALID_TRIP_MS = 2000;
constexpr uint32_t OVERPRESSURE_TRIP_MS = 250;
constexpr uint32_t OVERTEMP_TRIP_MS = 2000;
constexpr uint32_t MODBUS_LOSS_TRIP_MS = 5000;
constexpr uint32_t PUMP_CONTINUOUS_TRIP_MS = 90000;
constexpr uint32_t STOP_SETTLE_MS = 1200;
constexpr uint32_t BUTTON_LONG_PRESS_MS = 3000;
constexpr uint32_t INPUT_DEBOUNCE_MS = 30;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 30000;

constexpr float MIN_PRESSURE_BAR = 0.0f;
constexpr float MAX_PROFILE_PRESSURE_BAR = 12.0f;
constexpr float SENSOR_STALE_PRESSURE_BAR = 0.0f;
constexpr float SAFE_RESET_MAX_PRESSURE_BAR = 1.0f;
constexpr float SAFE_RESET_MIN_BOILER_C = 20.0f;
constexpr float MANUAL_KNOB_FULL_SCALE_VOLTS = 3.3f;

constexpr float PRESSURE_DIVIDER_RATIO =
    user_config::PRESSURE_DIVIDER_BOTTOM_OHMS /
    (user_config::PRESSURE_DIVIDER_TOP_OHMS +
     user_config::PRESSURE_DIVIDER_BOTTOM_OHMS);

// 验证两点：用万用表确认 PWM+RC 直流输出，再按实际泵修正。
constexpr float PUMP_PWM_FULL_SCALE_VOLTS = user_config::PUMP_PWM_SUPPLY_VOLTS;

}  // namespace config

#endif  // COFFEE_CONTROLLER_CONFIG_H_
