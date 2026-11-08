#ifndef COFFEE_CONTROLLER_PRESSURE_SENSOR_H_
#define COFFEE_CONTROLLER_PRESSURE_SENSOR_H_

#include <Arduino.h>

struct PressureSample {
  float bar = NAN;
  float sensorVoltage = 0.0f;
  float pinVoltage = 0.0f;
  bool valid = false;
};

class PressureSensor {
 public:
  void begin();
  PressureSample read();

 private:
  float filteredBar_ = NAN;
};

#endif  // COFFEE_CONTROLLER_PRESSURE_SENSOR_H_
