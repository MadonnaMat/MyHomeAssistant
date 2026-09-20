#pragma once

#include "esphome.h"
#include "fram_mb85rc256.h"

namespace keypad_fram {

constexpr uint8_t kFramI2CAddress = 0x50;
constexpr uint16_t kAddrMarker = 0x0000;
constexpr uint16_t kAddrFailedAttempts = 0x0001;

// Fail-open boot restore: returns the cached failed_attempts count, or 0
// (re-initializing the FRAM marker) if the cache is missing/unreadable.
inline uint8_t restore_failed_attempts(esphome::i2c::I2CBus *bus) {
  fram_mb85rc256::FramMB85RC256 fram(bus, kFramI2CAddress);
  uint8_t cached_attempts = 0;

  if (fram.check_marker(kAddrMarker) && fram.read_u8(kAddrFailedAttempts, cached_attempts)) {
    ESP_LOGI("keypad", "Restored failed_attempts from FRAM: %d", cached_attempts);
    return cached_attempts;
  }

  // Fail-open policy: allow typing immediately even if FRAM read fails.
  fram.write_marker(kAddrMarker);
  fram.write_u8(kAddrFailedAttempts, 0);
  ESP_LOGW("keypad", "FRAM cache unavailable, defaulting failed_attempts to 0");
  return 0;
}

// Writes failed_attempts to FRAM. `context` labels the warning log on failure.
inline bool persist_failed_attempts(esphome::i2c::I2CBus *bus, uint8_t count, const char *context) {
  fram_mb85rc256::FramMB85RC256 fram(bus, kFramI2CAddress);
  if (!fram.write_u8(kAddrFailedAttempts, count)) {
    ESP_LOGW("keypad", "Failed to write failed_attempts to FRAM (%s)", context);
    return false;
  }
  return true;
}

}  // namespace keypad_fram
