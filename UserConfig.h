#ifndef COFFEE_CONTROLLER_USER_CONFIG_H_
#define COFFEE_CONTROLLER_USER_CONFIG_H_

#include <Arduino.h>

// 用户硬件配置：实际接线前请逐项核对，尤其是 ADC 耐压和泵输入电压。
namespace user_config {

constexpr char WIFI_SSID[] = "";
constexpr char WIFI_PASSWORD[] = "";
constexpr char AP_SSID[] = "CoffeePilot";
constexpr char AP_PASSWORD[] = "CoffeePilot-1234";
constexpr char MDNS_HOST[] = "coffee";

static_assert(sizeof(AP_PASSWORD) - 1 >= 8,
              "ESP32 SoftAP password must contain at least 8 characters");

// ESP32-S3-N16R8 示例引脚，可按自己的控制板修改。
// N16R8 表示 16 MB Flash + 8 MB PSRAM；PSRAM 默认不参与本工程的数据缓冲，
// 但 Arduino IDE 中仍应按实物选择 16 MB Flash 和 OPI PSRAM。
constexpr int8_t PIN_RS485_RX = 18;
constexpr int8_t PIN_RS485_TX = 17;
// -1 表示使用自动收发方向的 RS485 模块；手动方向模块应填入 DE/RE 引脚。
constexpr int8_t PIN_RS485_DE = -1;
constexpr int8_t PIN_PUMP_PWM = 5;
constexpr int8_t PIN_PUMP_ENABLE = -1;
constexpr int8_t PIN_PRESSURE_ADC = 4;
constexpr int8_t PIN_KNOB_ADC = 2;
constexpr int8_t PIN_KNOB_ACTIVE = 1;
constexpr int8_t PIN_BREW_BUTTON = 6;
constexpr int8_t PIN_EXTRACTION_VALVE = 7;
constexpr int8_t PIN_STATUS_LED = 48;

constexpr bool PUMP_ENABLE_ACTIVE_HIGH = true;
constexpr bool EXTRACTION_VALVE_ACTIVE_HIGH = true;
// 默认按“开关接地即手动有效”的低电平方案，避免上电悬空时误启动。
constexpr bool KNOB_ACTIVE_LEVEL = false;
constexpr bool BUTTON_ACTIVE_LOW = true;
constexpr bool STATUS_LED_ACTIVE_HIGH = true;

// E5CC 通信参数。务必用面板或串口工具确认后再联机。
constexpr uint8_t E5CC_SLAVE_ID = 1;
constexpr uint32_t E5CC_BAUD = 9600;
constexpr uint16_t E5CC_PV_REGISTER = 0x2000;
constexpr uint16_t E5CC_SP_REGISTER = 0x2103;
constexpr float E5CC_TEMPERATURE_SCALE = 0.1f;

// 压力变送器 0.4-2.0 V 对应 0-1.6 MPa（0-16 bar），直接进入 ADC，无分压。
constexpr float PRESSURE_DIVIDER_TOP_OHMS = 0.0f;
constexpr float PRESSURE_DIVIDER_BOTTOM_OHMS = 1.0f;
constexpr float PRESSURE_SENSOR_MIN_VOLTAGE = 0.4f;
constexpr float PRESSURE_SENSOR_MAX_VOLTAGE = 2.0f;
constexpr float PRESSURE_RANGE_MIN_BAR = 0.0f;
constexpr float PRESSURE_RANGE_MAX_BAR = 16.0f;

// PWM 经 RC 滤波后的理论满量程，以及泵允许的最高输入电压。
constexpr float PUMP_PWM_SUPPLY_VOLTS = 3.3f;
constexpr float PUMP_MAX_VOLTS = 3.0f;
constexpr uint32_t PUMP_PWM_FREQUENCY_HZ = 20000;
constexpr uint8_t PUMP_PWM_RESOLUTION_BITS = 10;

constexpr float DEFAULT_BOILER_SETPOINT_C = 93.0f;
constexpr float BOILER_READY_TOLERANCE_C = 2.0f;
constexpr float HARD_MAX_BOILER_C = 105.0f;

}  // namespace user_config

#endif  // COFFEE_CONTROLLER_USER_CONFIG_H_
