#pragma once

#include "esphome.h"
#include <cstring>
#include <type_traits>

namespace fram_mb85rc256 {

static constexpr uint16_t kFramSizeBytes = 32768;

class FramMB85RC256 {
 public:
  static constexpr uint8_t kDefaultMarker = 0xA5;

  FramMB85RC256(esphome::i2c::I2CBus *bus, uint8_t address) : bus_(bus), address_(address) {}

  bool read_u8(uint16_t mem_addr, uint8_t &value) const { return this->read_bytes(mem_addr, &value, 1); }

  bool write_u8(uint16_t mem_addr, uint8_t value) const { return this->write_bytes(mem_addr, &value, 1); }

  bool read_u8_or(uint16_t mem_addr, uint8_t fallback, uint8_t &value) const {
    if (this->read_u8(mem_addr, value)) {
      return true;
    }
    value = fallback;
    return false;
  }

  bool read_bool(uint16_t mem_addr, bool &value) const {
    uint8_t raw = 0;
    if (!this->read_u8(mem_addr, raw)) {
      return false;
    }
    value = raw != 0;
    return true;
  }

  bool write_bool(uint16_t mem_addr, bool value) const {
    const uint8_t raw = value ? 1 : 0;
    return this->write_u8(mem_addr, raw);
  }

  bool check_marker(uint16_t marker_addr, uint8_t expected = kDefaultMarker) const {
    uint8_t marker = 0;
    return this->read_u8(marker_addr, marker) && marker == expected;
  }

  bool write_marker(uint16_t marker_addr, uint8_t marker = kDefaultMarker) const {
    return this->write_u8(marker_addr, marker);
  }

  bool read_bytes(uint16_t mem_addr, uint8_t *data, size_t len) const {
    if (len == 0 || data == nullptr || !this->range_valid_(mem_addr, len)) {
      return false;
    }

    uint8_t addr[2] = {
        static_cast<uint8_t>((mem_addr >> 8) & 0xFF),
        static_cast<uint8_t>(mem_addr & 0xFF),
    };

    auto err = this->bus_->write(this->address_, addr, 2, false);
    if (err != esphome::i2c::ERROR_OK) {
      return false;
    }

    err = this->bus_->read(this->address_, data, len);
    return err == esphome::i2c::ERROR_OK;
  }

  bool write_bytes(uint16_t mem_addr, const uint8_t *data, size_t len) const {
    if (len == 0 || data == nullptr || !this->range_valid_(mem_addr, len)) {
      return false;
    }

    // For MB85RC256, I2C payloads can be sent as address prefix + data bytes.
    uint8_t tx[66];  // 2-byte address + up to 64 bytes payload per call.
    size_t offset = 0;

    while (offset < len) {
      const size_t chunk = (len - offset > 64) ? 64 : (len - offset);
      const uint16_t addr = mem_addr + static_cast<uint16_t>(offset);

      tx[0] = static_cast<uint8_t>((addr >> 8) & 0xFF);
      tx[1] = static_cast<uint8_t>(addr & 0xFF);
      std::memcpy(&tx[2], &data[offset], chunk);

      auto err = this->bus_->write(this->address_, tx, chunk + 2);
      if (err != esphome::i2c::ERROR_OK) {
        return false;
      }

      offset += chunk;
    }

    return true;
  }

  template<typename T> bool read_value(uint16_t mem_addr, T &value) const {
    static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
    uint8_t raw[sizeof(T)] = {0};
    if (!this->read_bytes(mem_addr, raw, sizeof(T))) {
      return false;
    }
    std::memcpy(&value, raw, sizeof(T));
    return true;
  }

  template<typename T> bool write_value(uint16_t mem_addr, const T &value) const {
    static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
    uint8_t raw[sizeof(T)] = {0};
    std::memcpy(raw, &value, sizeof(T));
    return this->write_bytes(mem_addr, raw, sizeof(T));
  }

 private:
  bool range_valid_(uint16_t mem_addr, size_t len) const {
    return (static_cast<size_t>(mem_addr) + len) <= kFramSizeBytes;
  }

  esphome::i2c::I2CBus *bus_;
  uint8_t address_;
};

}  // namespace fram_mb85rc256
