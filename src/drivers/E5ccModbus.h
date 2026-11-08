#ifndef COFFEE_CONTROLLER_E5CC_MODBUS_H_
#define COFFEE_CONTROLLER_E5CC_MODBUS_H_

#include <Arduino.h>
#include <HardwareSerial.h>

#include "AppTypes.h"

class E5ccModbus {
 public:
  void begin();
  void configure(const E5ccConfig& config);
  bool configurationMatches(const E5ccConfig& config) const;
  bool readPv(float& celsius);
  bool readSp(float& celsius);
  bool writeSp(float celsius);
  uint32_t consecutiveFailures() const { return consecutiveFailures_; }
  void resetFailures() { consecutiveFailures_ = 0; }

 private:
  bool readRegister(uint16_t address, uint16_t& value);
  bool writeRegister(uint16_t address, uint16_t value);
  bool transact(const uint8_t* request, size_t requestLength, uint8_t* response,
                size_t responseLength);
  static uint16_t crc16(const uint8_t* data, size_t length);
  void setTransmit(bool transmit);

  E5ccConfig config_;
  uint32_t consecutiveFailures_ = 0;
};

#endif  // COFFEE_CONTROLLER_E5CC_MODBUS_H_
