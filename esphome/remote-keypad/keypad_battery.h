#pragma once

#include <cmath>

#include "esphome.h"

namespace keypad_battery {

// Returns battery percentage (0-100) from voltage, or NAN if voltage is NaN.
inline float voltage_to_percent(float voltage) {
  constexpr float kMaxVoltage = 4.2f;
  constexpr float kMinVoltage = 3.2f;

  if (std::isnan(voltage)) return NAN;
  if (voltage >= kMaxVoltage) return 100.0f;
  if (voltage <= kMinVoltage) return 0.0f;
  return ((voltage - kMinVoltage) / (kMaxVoltage - kMinVoltage)) * 100.0f;
}

// Reads voltage_sensor and converts it to a battery percentage, matching the
// "no state yet" -> no-publish behavior a template sensor lambda needs.
// Takes a pointer: see the note on keypad_display::refresh_if_ready.
template<typename SensorT>
inline esphome::optional<float> capacity_from_sensor(SensorT *voltage_sensor) {
  if (!voltage_sensor->has_state()) return {};
  return voltage_to_percent(voltage_sensor->state);
}

}  // namespace keypad_battery
