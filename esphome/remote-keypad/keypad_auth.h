#pragma once

#include <string>

#include "esphome.h"
#include "keypad_display.h"
#include "keypad_fram_store.h"
#include "keypad_state.h"
#include "keypad_tones.h"

// Orchestrates the lower-level keypad_state/keypad_fram_store/keypad_display
// primitives for each YAML call site, so the YAML lambdas stay one-liners
// instead of each re-composing "update state, persist to FRAM, refresh
// display" by hand.
namespace keypad_auth {

// Boot-time restore of failed_attempts + derived lockout_state from FRAM.
inline void restore_on_boot(esphome::i2c::I2CBus *bus, int &failed_attempts, int &lockout_state) {
  failed_attempts = keypad_fram::restore_failed_attempts(bus);
  lockout_state = (failed_attempts >= 3) ? keypad_state::LOCKED : keypad_state::ACTIVE;
}

// Plays the per-key tone, logs on submit, and applies the keypress to the
// pin-entry state machine, refreshing the display if it changed.
//
// rtttl_player/display are pointers (not references): `id(x)` used as a
// plain function argument (no immediately-following `.`) expands to the raw
// component pointer, not a dereferenced object -- ESPHome only dereferences
// id() when it's written as `id(x).method()` directly in the lambda text.
template<typename RtttlT, typename DisplayT>
inline void handle_key_press(char key, RtttlT *rtttl_player, DisplayT *display, int failed_attempts,
                              std::string &actual_pin, std::string &display_pin, int &lockout_state) {
  rtttl_player->play(std::string("beep:d=16,o=6,b=500:") + keypad_tones::note_for_key(key));
  if (key == '#') {
    ESP_LOGI("keypad", "failed_attempts before submit: %d", failed_attempts);
  }
  if (keypad_state::handle_key(key, actual_pin, display_pin, lockout_state)) {
    keypad_display::refresh_if_ready(display);
  }
}

// Syncs failed_attempts from the Home Assistant input_number entity.
template<typename DisplayT>
inline void sync_from_ha(float ha_value, esphome::i2c::I2CBus *bus, int &failed_attempts, int &lockout_state,
                          DisplayT *display) {
  int from_ha = static_cast<int>(ha_value + 0.5f);
  if (from_ha < 0) from_ha = 0;
  if (failed_attempts == from_ha) return;

  failed_attempts = from_ha;
  ESP_LOGI("keypad", "Synced failed_attempts from HA: %d", failed_attempts);
  keypad_fram::persist_failed_attempts(bus, static_cast<uint8_t>(failed_attempts), "ha_sync");
  if (failed_attempts >= 3) {
    lockout_state = keypad_state::LOCKED;
  }
  keypad_display::refresh_if_ready(display);
}

template<typename DisplayT>
inline void handle_auth_success(esphome::i2c::I2CBus *bus, int &failed_attempts, int &lockout_state,
                                 DisplayT *display) {
  failed_attempts = 0;
  ESP_LOGI("keypad", "Set failed_attempts after auth_success: %d", failed_attempts);
  keypad_fram::persist_failed_attempts(bus, 0, "auth_success");
  lockout_state = keypad_state::UNLOCKED;
  keypad_display::refresh_if_ready(display);
}

template<typename DisplayT>
inline void handle_auth_fail(esphome::i2c::I2CBus *bus, int &failed_attempts, int &lockout_state, DisplayT *display) {
  lockout_state = keypad_state::LOCKED;
  failed_attempts += 1;
  ESP_LOGI("keypad", "Set failed_attempts after auth_fail: %d", failed_attempts);
  keypad_fram::persist_failed_attempts(bus, static_cast<uint8_t>(failed_attempts), "auth_fail");
  keypad_display::refresh_if_ready(display);
}

// Auto-retry after a fail with attempts remaining: clear the pin buffers
// and refresh, without touching failed_attempts.
template<typename DisplayT>
inline void reset_after_retry(std::string &actual_pin, std::string &display_pin, int &lockout_state,
                               DisplayT *display) {
  keypad_state::reset_pin_entry(actual_pin, display_pin, lockout_state);
  keypad_display::refresh_if_ready(display);
}

// Home Assistant force-locking the keypad via the lockout switch.
template<typename DisplayT>
inline void lock_from_ha(int &failed_attempts, int &lockout_state, DisplayT *display) {
  failed_attempts = 3;
  lockout_state = keypad_state::LOCKED;
  keypad_display::refresh_if_ready(display);
}

// Home Assistant clearing the lockout via the lockout switch.
template<typename DisplayT>
inline void unlock_from_ha(std::string &actual_pin, std::string &display_pin, int &failed_attempts,
                            int &lockout_state, DisplayT *display) {
  failed_attempts = 0;
  keypad_state::reset_pin_entry(actual_pin, display_pin, lockout_state);
  keypad_display::refresh_if_ready(display);
}

}  // namespace keypad_auth
