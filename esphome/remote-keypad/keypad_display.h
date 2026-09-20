#pragma once

#include <cmath>
#include <string>

#include "esphome.h"
#include "keypad_state.h"

namespace keypad_display {

// Renders the battery icon plus the 4-state PIN/lockout UI onto the
// keypad's OLED. `battery_percent` should be NAN when unknown (skips the
// fill). `ha_connected` drives a small top-right status dot: filled once the
// API client (Home Assistant) is connected, hollow while it isn't -- boot
// takes a few seconds to associate, so the dot starts hollow and fills in
// once that completes. Templated on DisplayT to avoid pinning this header to
// ESPHome's exact display base-class name/hierarchy across versions.
template<typename DisplayT>
void render(DisplayT &it, int lockout_state, const std::string &display_pin, int failed_attempts,
            float battery_percent, esphome::font::Font *font_large, esphome::font::Font *font_small,
            bool ha_connected) {
  // --- 1. Draw Battery Icon (Top Left, ~30px high) ---
  it.rectangle(0, 2, 14, 28);        // Main body
  it.filled_rectangle(4, 0, 6, 2);   // Top nub

  if (!std::isnan(battery_percent)) {
    float pct = battery_percent / 100.0f;
    int h = static_cast<int>(24 * pct);  // Max fill height is 24px
    if (h > 0) {
      // Fill from bottom up
      it.filled_rectangle(2, 28 - h + 1, 10, h);
    }
  }

  // --- 2. Draw HA Connection Indicator (Top Right, small dot) ---
  if (ha_connected) {
    it.filled_circle(121, 6, 4);
  } else {
    it.circle(121, 6, 4);
  }

  // --- 3. Draw State Machine (Input vs Padlocks vs Loading) ---
  if (lockout_state == keypad_state::ACTIVE) {
    // Show PIN entry asterisks
    it.printf(64, 30, font_large, esphome::display::TextAlign::TOP_CENTER, "%s", display_pin.c_str());
  } else if (lockout_state == keypad_state::PENDING) {
    // Loading State: Draw a primitive hourglass centered on the screen
    it.line(52, 20, 76, 20);
    it.line(52, 44, 76, 44);
    it.line(54, 20, 74, 44);
    it.line(74, 20, 54, 44);
    it.filled_circle(64, 38, 2);
  } else if (lockout_state == keypad_state::LOCKED) {
    // Locked Padlock
    it.filled_rectangle(54, 34, 20, 16);
    it.line(58, 34, 58, 26);
    it.line(70, 34, 70, 26);
    it.line(58, 26, 70, 26);

    // Calculate and display attempts remaining
    int attempts_left = 3 - failed_attempts;
    if (attempts_left < 0) attempts_left = 0;  // Prevent negative numbers

    // Draw black text inside the filled white padlock
    it.printf(64, 42, font_small, esphome::Color::BLACK, esphome::display::TextAlign::CENTER, "%d", attempts_left);
  } else if (lockout_state == keypad_state::UNLOCKED) {
    // Unlocked Padlock
    it.filled_rectangle(54, 34, 20, 16);
    it.line(58, 34, 58, 20);
    it.line(58, 20, 70, 20);
  }
}

// Refreshes the display if its controller is ready. Consolidates the
// `if (id(my_display).is_ready()) { id(my_display).update(); }` guard that
// otherwise gets repeated after every state change.
//
// Takes a pointer (not a reference): `id(my_display)` used as a plain
// function argument (no immediately-following `.`) expands to the raw
// component pointer, not a dereferenced object -- ESPHome only dereferences
// id() when it's written as `id(x).method()` directly in the lambda text.
template<typename DisplayT>
inline void refresh_if_ready(DisplayT *display) {
  if (display->is_ready()) {
    display->update();
  }
}

template<typename DisplayT>
inline void power_on(DisplayT *display) {
  if (display->is_ready()) {
    display->turn_on();
  }
}

template<typename DisplayT>
inline void power_off(DisplayT *display) {
  if (display->is_ready()) {
    display->turn_off();
  }
}

// Blanks the screen and pushes the blank frame, without powering off.
template<typename DisplayT>
inline void clear(DisplayT *display) {
  if (display->is_ready()) {
    display->fill(esphome::Color::BLACK);
    display->update();
  }
}

// Blanks the screen and powers the controller off in one step.
template<typename DisplayT>
inline void clear_and_power_off(DisplayT *display) {
  if (display->is_ready()) {
    display->fill(esphome::Color::BLACK);
    display->update();
    display->turn_off();
  }
}

}  // namespace keypad_display
