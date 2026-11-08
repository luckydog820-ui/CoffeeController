#ifndef COFFEE_CONTROLLER_PUMP_DRIVER_H_
#define COFFEE_CONTROLLER_PUMP_DRIVER_H_

#include <Arduino.h>

class PumpDriver {
 public:
  void begin();
  void setVoltage(float volts);
  void emergencyOff();
  float commandedVoltage() const { return commandedVoltage_; }

 private:
  float commandedVoltage_ = 0.0f;
  uint16_t maxDuty_ = 0;
  bool initialized_ = false;
};

#endif  // COFFEE_CONTROLLER_PUMP_DRIVER_H_
