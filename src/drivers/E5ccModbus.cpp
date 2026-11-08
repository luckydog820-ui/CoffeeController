#include "E5ccModbus.h"

#include <cmath>
#include <cstring>

#include "Config.h"

namespace {

HardwareSerial modbusSerial(1);
constexpr uint32_t kResponseTimeoutMs = 180;

}  // namespace

void E5ccModbus::begin() {
  if (user_config::PIN_RS485_DE >= 0) {
    pinMode(user_config::PIN_RS485_DE, OUTPUT);
    setTransmit(false);
  }
  modbusSerial.begin(user_config::E5CC_BAUD, SERIAL_8E1,
                     user_config::PIN_RS485_RX,
                     user_config::PIN_RS485_TX);
  modbusSerial.setTimeout(30);
  while (modbusSerial.available()) {
    modbusSerial.read();
  }
}

void E5ccModbus::setTransmit(bool transmit) {
  if (user_config::PIN_RS485_DE < 0) {
    return;
  }
  digitalWrite(user_config::PIN_RS485_DE,
               transmit ? HIGH : LOW);
}

uint16_t E5ccModbus::crc16(const uint8_t* data, size_t length) {
  uint16_t crc = 0xFFFF;
  for (size_t index = 0; index < length; ++index) {
    crc ^= data[index];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x0001) ? (crc >> 1) ^ 0xA001 : crc >> 1;
    }
  }
  return crc;
}

bool E5ccModbus::transact(const uint8_t* request, size_t requestLength,
                          uint8_t* response, size_t responseLength) {
  while (modbusSerial.available()) {
    modbusSerial.read();
  }

  setTransmit(true);
  delayMicroseconds(100);
  modbusSerial.write(request, requestLength);
  modbusSerial.flush();
  delayMicroseconds(120);
  setTransmit(false);

  const uint32_t started = millis();
  size_t received = 0;
  while (received < responseLength &&
         millis() - started < kResponseTimeoutMs) {
    if (!modbusSerial.available()) {
      delay(1);
      continue;
    }
    response[received++] = static_cast<uint8_t>(modbusSerial.read());
  }
  while (modbusSerial.available()) {
    modbusSerial.read();
  }

  if (received != responseLength || response[0] != request[0]) {
    return false;
  }
  const uint16_t expected =
      static_cast<uint16_t>(response[responseLength - 2]) |
      (static_cast<uint16_t>(response[responseLength - 1]) << 8);
  return crc16(response, responseLength - 2) == expected;
}

bool E5ccModbus::readRegister(uint16_t address, uint16_t& value) {
  uint8_t request[8] = {
      user_config::E5CC_SLAVE_ID,
      0x03,
      static_cast<uint8_t>(address >> 8),
      static_cast<uint8_t>(address & 0xFF),
      0x00,
      0x01,
      0x00,
      0x00,
  };
  const uint16_t crc = crc16(request, 6);
  request[6] = static_cast<uint8_t>(crc & 0xFF);
  request[7] = static_cast<uint8_t>(crc >> 8);

  uint8_t response[7] = {};
  if (!transact(request, sizeof(request), response, sizeof(response)) ||
      response[1] != 0x03 || response[2] != 0x02) {
    ++consecutiveFailures_;
    return false;
  }
  value = static_cast<uint16_t>(response[3] << 8) | response[4];
  consecutiveFailures_ = 0;
  return true;
}

bool E5ccModbus::writeRegister(uint16_t address, uint16_t value) {
  uint8_t request[8] = {
      user_config::E5CC_SLAVE_ID,
      0x06,
      static_cast<uint8_t>(address >> 8),
      static_cast<uint8_t>(address & 0xFF),
      static_cast<uint8_t>(value >> 8),
      static_cast<uint8_t>(value & 0xFF),
      0x00,
      0x00,
  };
  const uint16_t crc = crc16(request, 6);
  request[6] = static_cast<uint8_t>(crc & 0xFF);
  request[7] = static_cast<uint8_t>(crc >> 8);

  uint8_t response[8] = {};
  const bool ok =
      transact(request, sizeof(request), response, sizeof(response)) &&
      memcmp(request, response, 6) == 0;
  if (ok) {
    consecutiveFailures_ = 0;
  } else {
    ++consecutiveFailures_;
  }
  return ok;
}

bool E5ccModbus::readPv(float& celsius) {
  uint16_t raw = 0;
  if (!readRegister(user_config::E5CC_PV_REGISTER, raw)) {
    return false;
  }
  celsius =
      static_cast<int16_t>(raw) * user_config::E5CC_TEMPERATURE_SCALE;
  return true;
}

bool E5ccModbus::readSp(float& celsius) {
  uint16_t raw = 0;
  if (!readRegister(user_config::E5CC_SP_REGISTER, raw)) {
    return false;
  }
  celsius =
      static_cast<int16_t>(raw) * user_config::E5CC_TEMPERATURE_SCALE;
  return true;
}

bool E5ccModbus::writeSp(float celsius) {
  const int16_t scaled = static_cast<int16_t>(
      lroundf(celsius / user_config::E5CC_TEMPERATURE_SCALE));
  return writeRegister(user_config::E5CC_SP_REGISTER,
                       static_cast<uint16_t>(scaled));
}
