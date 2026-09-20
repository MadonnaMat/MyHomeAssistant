#pragma once

#include <string>

namespace keypad_state {

enum LockoutState : int {
  ACTIVE = 0,
  PENDING = 1,
  LOCKED = 2,
  UNLOCKED = 3,
};

// Clears the PIN buffers and returns to ACTIVE.
inline void reset_pin_entry(std::string &actual_pin, std::string &display_pin, int &lockout_state) {
  actual_pin = "";
  display_pin = "";
  lockout_state = ACTIVE;
}

// Applies one keypress while ACTIVE: '#' moves to PENDING (submit), any
// other key (up to an 8-digit cap) is appended to both buffers. Returns
// true if the caller should refresh the display.
inline bool handle_key(char key, std::string &actual_pin, std::string &display_pin, int &lockout_state) {
  if (key == '#') {
    lockout_state = PENDING;
    return true;
  }
  if (actual_pin.length() < 8) {
    actual_pin += key;
    display_pin += "*";
    return true;
  }
  return false;
}

}  // namespace keypad_state
