#pragma once

// Minimal stand-in for ESPHome's generated esphome.h, just enough for
// keypad_battery.h to compile in a host-native unit test build.
// keypad_battery.h only touches esphome::optional<T> at file scope, as
// capacity_from_sensor()'s return type; that function is a template that's
// never instantiated by these tests, so nothing else needs stubbing here.

#include <optional>

namespace esphome {
template<typename T>
using optional = std::optional<T>;
}  // namespace esphome
