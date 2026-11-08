#include "ExtractionValve.h"

#include <Arduino.h>

#include "Config.h"

namespace {

void writeValve(bool open) {
  if (user_config::PIN_EXTRACTION_VALVE < 0) {
    return;
  }
  const bool level =
      user_config::EXTRACTION_VALVE_ACTIVE_HIGH ? open : !open;
  digitalWrite(user_config::PIN_EXTRACTION_VALVE, level ? HIGH : LOW);
}

}  // namespace

void ExtractionValve::begin() {
  open_ = false;
  if (user_config::PIN_EXTRACTION_VALVE < 0) {
    return;
  }
  // 先把锁存输出置为关闭，再切换到输出模式，减少上电误开阀。
  writeValve(false);
  pinMode(user_config::PIN_EXTRACTION_VALVE, OUTPUT);
  writeValve(false);
}

void ExtractionValve::setOpen(bool open) {
  open_ = open;
  writeValve(open);
}

void ExtractionValve::emergencyClose() {
  open_ = false;
  writeValve(false);
}
