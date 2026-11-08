#ifndef COFFEE_CONTROLLER_PRESSURE_LOOP_H_
#define COFFEE_CONTROLLER_PRESSURE_LOOP_H_

#include "AppTypes.h"

class PressureLoop {
 public:
  void reset();
  float update(float targetBar, float measuredBar, float dtSeconds,
               const PidConfig& config);

 private:
  float integral_ = 0.0f;
  float previousMeasurement_ = NAN;
  float filteredDerivative_ = 0.0f;
};

#endif  // COFFEE_CONTROLLER_PRESSURE_LOOP_H_
