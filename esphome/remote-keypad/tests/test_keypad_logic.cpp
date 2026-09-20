// Host-native unit tests for the pure-logic keypad headers -- no ESPHome
// device or ESP32 toolchain needed, since keypad_state.h and
// keypad_tones.h have zero ESPHome dependency, and keypad_battery.h's
// voltage_to_percent() doesn't touch anything hardware-specific either
// (only capacity_from_sensor(), a template never instantiated here, does).
//
// Build & run (from the esphome/ repo root):
//   g++ -std=c++17 -Wall -Wextra -I remote-keypad/tests/stub_esphome -I remote-keypad remote-keypad/tests/test_keypad_logic.cpp -o /tmp/keypad_tests && /tmp/keypad_tests

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "keypad_battery.h"
#include "keypad_state.h"
#include "keypad_tones.h"

namespace {

int failures = 0;

void expect(bool condition, const char *description) {
  if (!condition) {
    std::fprintf(stderr, "FAIL: %s\n", description);
    failures++;
  }
}

void test_note_for_key() {
  // Every mapped key returns a non-empty RTTTL fragment, and each mapped
  // key's fragment is distinct -- that's what gives each key its own beep.
  const char *keys = "123A456B789C*0#D";
  std::string seen[16];
  int count = 0;
  for (const char *k = keys; *k; ++k) {
    const char *note = keypad_tones::note_for_key(*k);
    expect(note[0] != '\0', "mapped key returns a non-empty note");
    for (int i = 0; i < count; i++) {
      expect(seen[i] != note, "each mapped key has a distinct note");
    }
    seen[count++] = note;
  }
  expect(std::strcmp(keypad_tones::note_for_key('Z'), "") == 0,
         "unmapped key falls back to an empty note");
}

void test_reset_pin_entry() {
  std::string actual_pin = "1234";
  std::string display_pin = "****";
  int lockout_state = keypad_state::LOCKED;

  keypad_state::reset_pin_entry(actual_pin, display_pin, lockout_state);

  expect(actual_pin.empty(), "reset clears actual_pin");
  expect(display_pin.empty(), "reset clears display_pin");
  expect(lockout_state == keypad_state::ACTIVE, "reset returns to ACTIVE");
}

void test_handle_key_digit_accumulation() {
  std::string actual_pin, display_pin;
  int lockout_state = keypad_state::ACTIVE;

  bool refresh = keypad_state::handle_key('5', actual_pin, display_pin, lockout_state);

  expect(refresh, "digit press requests a display refresh");
  expect(actual_pin == "5", "digit is appended to actual_pin");
  expect(display_pin == "*", "digit is masked in display_pin");
  expect(lockout_state == keypad_state::ACTIVE, "digit press stays ACTIVE");
}

void test_handle_key_submit() {
  std::string actual_pin = "1234";
  std::string display_pin = "****";
  int lockout_state = keypad_state::ACTIVE;

  bool refresh = keypad_state::handle_key('#', actual_pin, display_pin, lockout_state);

  expect(refresh, "submit requests a display refresh");
  expect(lockout_state == keypad_state::PENDING, "'#' moves to PENDING");
  expect(actual_pin == "1234", "'#' does not touch the pin buffer");
}

void test_handle_key_caps_at_eight_digits() {
  std::string actual_pin = "12345678";  // already at the cap
  std::string display_pin = "********";
  int lockout_state = keypad_state::ACTIVE;

  bool refresh = keypad_state::handle_key('9', actual_pin, display_pin, lockout_state);

  expect(!refresh, "a 9th digit is dropped, no refresh requested");
  expect(actual_pin == "12345678", "actual_pin is unchanged past the 8-digit cap");
  expect(display_pin == "********", "display_pin is unchanged past the 8-digit cap");
}

void test_voltage_to_percent() {
  expect(keypad_battery::voltage_to_percent(4.2f) == 100.0f, "voltage at max clamps to 100%");
  expect(keypad_battery::voltage_to_percent(5.0f) == 100.0f, "voltage above max clamps to 100%");
  expect(keypad_battery::voltage_to_percent(3.2f) == 0.0f, "voltage at min clamps to 0%");
  expect(keypad_battery::voltage_to_percent(3.0f) == 0.0f, "voltage below min clamps to 0%");

  float mid = keypad_battery::voltage_to_percent(3.7f);  // exact midpoint of 3.2-4.2
  expect(mid > 49.0f && mid < 51.0f, "midpoint voltage maps to ~50%");

  expect(std::isnan(keypad_battery::voltage_to_percent(NAN)), "NaN voltage stays NaN");
}

}  // namespace

int main() {
  test_note_for_key();
  test_reset_pin_entry();
  test_handle_key_digit_accumulation();
  test_handle_key_submit();
  test_handle_key_caps_at_eight_digits();
  test_voltage_to_percent();

  if (failures == 0) {
    std::printf("All keypad logic tests passed.\n");
    return 0;
  }
  std::fprintf(stderr, "%d test(s) failed.\n", failures);
  return 1;
}
