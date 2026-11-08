#ifndef COFFEE_CONTROLLER_ESPRESSO_STATE_MACHINE_H_
#define COFFEE_CONTROLLER_ESPRESSO_STATE_MACHINE_H_

#include "AppTypes.h"
#include "ControllerState.h"

class EspressoStateMachine {
 public:
  explicit EspressoStateMachine(ControllerState& state);

  void begin(uint32_t nowMs);
  void tick(uint32_t nowMs);
  bool requestStart(uint32_t nowMs);
  void requestStop(uint32_t nowMs);
  void manualOverride(uint32_t nowMs);
  void manualReleased(uint32_t nowMs);
  void tripFault(uint32_t nowMs);
  void resetFault(uint32_t nowMs);

 private:
  bool isBrewing() const;
  void enter(BrewState state, uint32_t nowMs);

  ControllerState& state_;
  BrewState current_ = BrewState::kBoot;
};

#endif  // COFFEE_CONTROLLER_ESPRESSO_STATE_MACHINE_H_
