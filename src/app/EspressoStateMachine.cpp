#include "EspressoStateMachine.h"

#include "Config.h"

EspressoStateMachine::EspressoStateMachine(ControllerState& state)
    : state_(state) {}

void EspressoStateMachine::begin(uint32_t nowMs) {
  enter(BrewState::kIdle, nowMs);
}

bool EspressoStateMachine::isBrewing() const {
  return current_ == BrewState::kPreInfusion ||
         current_ == BrewState::kPressureRamp ||
         current_ == BrewState::kExtraction || current_ == BrewState::kStop;
}

void EspressoStateMachine::enter(BrewState state, uint32_t nowMs) {
  if (current_ == state) {
    return;
  }
  current_ = state;
  state_.enterBrewState(state, nowMs);
}

bool EspressoStateMachine::requestStart(uint32_t nowMs) {
  const ControllerSnapshot snapshot = state_.snapshot();
  if (snapshot.faultMask != kFaultNone ||
      !snapshot.boilerAtSetpoint || !snapshot.modbusOnline ||
      !snapshot.pressureSensorValid || snapshot.knobActive ||
      (current_ != BrewState::kReady && current_ != BrewState::kIdle)) {
    return false;
  }
  enter(BrewState::kPreInfusion, nowMs);
  return true;
}

void EspressoStateMachine::requestStop(uint32_t nowMs) {
  if (current_ == BrewState::kFault) {
    return;
  }
  if (isBrewing()) {
    enter(BrewState::kStop, nowMs);
  }
}

void EspressoStateMachine::manualOverride(uint32_t nowMs) {
  if (current_ != BrewState::kFault) {
    enter(BrewState::kManual, nowMs);
  }
}

void EspressoStateMachine::manualReleased(uint32_t nowMs) {
  if (current_ == BrewState::kManual) {
    const ControllerSnapshot snapshot = state_.snapshot();
    enter(snapshot.boilerAtSetpoint ? BrewState::kReady : BrewState::kIdle,
          nowMs);
  }
}

void EspressoStateMachine::tripFault(uint32_t nowMs) {
  enter(BrewState::kFault, nowMs);
}

void EspressoStateMachine::resetFault(uint32_t nowMs) {
  if (current_ == BrewState::kFault) {
    enter(BrewState::kIdle, nowMs);
  }
}

void EspressoStateMachine::tick(uint32_t nowMs) {
  const ControllerSnapshot snapshot = state_.snapshot();
  if (snapshot.faultMask != kFaultNone) {
    tripFault(nowMs);
    return;
  }

  const BrewProfile profile = state_.settings().settings.profile;
  const uint32_t stageElapsed = snapshot.stageElapsedMs;

  switch (current_) {
    case BrewState::kBoot:
    case BrewState::kIdle:
      if (snapshot.knobActive) {
        enter(BrewState::kManual, nowMs);
      } else if (snapshot.boilerAtSetpoint &&
                 snapshot.modbusOnline &&
                 snapshot.pressureSensorValid) {
        enter(BrewState::kReady, nowMs);
      }
      break;

    case BrewState::kReady:
      if (snapshot.knobActive) {
        enter(BrewState::kManual, nowMs);
      } else if (!snapshot.boilerAtSetpoint) {
        enter(BrewState::kIdle, nowMs);
      }
      break;

    case BrewState::kPreInfusion:
      if (snapshot.knobActive) {
        enter(BrewState::kManual, nowMs);
      } else if (stageElapsed >= profile.preinfusionTimeMs) {
        enter(BrewState::kPressureRamp, nowMs);
      }
      break;

    case BrewState::kPressureRamp:
      if (snapshot.knobActive) {
        enter(BrewState::kManual, nowMs);
      } else if (snapshot.pressureSensorValid &&
                 snapshot.pressureBar >=
                     profile.extractionPressureBar -
                         profile.pressureToleranceBar) {
        enter(BrewState::kExtraction, nowMs);
      } else if (stageElapsed >= profile.rampTimeoutMs) {
        enter(BrewState::kExtraction, nowMs);
      }
      break;

    case BrewState::kExtraction:
      if (snapshot.knobActive) {
        enter(BrewState::kManual, nowMs);
      } else if (stageElapsed >= profile.extractionTimeMs) {
        enter(BrewState::kStop, nowMs);
      }
      break;

    case BrewState::kStop:
      if (snapshot.knobActive) {
        enter(BrewState::kManual, nowMs);
      } else if (stageElapsed >= config::STOP_SETTLE_MS) {
        enter(snapshot.boilerAtSetpoint ? BrewState::kReady
                                        : BrewState::kIdle,
              nowMs);
      }
      break;

    case BrewState::kManual:
      if (!snapshot.knobActive) {
        enter(snapshot.boilerAtSetpoint ? BrewState::kReady
                                        : BrewState::kIdle,
              nowMs);
      }
      break;

    case BrewState::kFault:
      break;
  }
}
