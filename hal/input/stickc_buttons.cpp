// M5StickC Plus2 three-button InputSource (ADR-0005, ADR-0020). Device-only.

#ifndef UNIT_TEST  // device-only: Arduino GPIO is not available on the host lane.

#include "stickc_buttons.hpp"

#include <Arduino.h>

namespace tp {

constexpr uint8_t StickCButtons::kPins[];

bool StickCButtons::begin() {
  // Input-only pins (G34-39) with the board's external pull-ups: plain INPUT, idle HIGH. Seed the
  // debounced state from the current read so a button held down at boot does not fire a phantom
  // press on the first poll. There is no controller to fail here, so begin() always succeeds.
  const uint32_t now = millis();
  for (int i = 0; i < kButtonCount; ++i) {
    pinMode(kPins[i], INPUT);
    wasDown_[i] = (digitalRead(kPins[i]) == LOW);
    lastChangeMs_[i] = now;
  }
  ready_ = true;
  return true;
}

InputEvent StickCButtons::poll() {
  if (!ready_) return InputEvent::none();

  const uint32_t now = millis();
  for (int i = 0; i < kButtonCount; ++i) {
    const bool down = (digitalRead(kPins[i]) == LOW);  // active-low against the external pull-up
    if (down == wasDown_[i]) continue;                 // no transition on this pin
    if (now - lastChangeMs_[i] < kDebounceMs) continue;  // still bouncing — ignore until stable
    lastChangeMs_[i] = now;
    wasDown_[i] = down;
    if (down) return eventFor(i);  // one event per call, on the press edge only (release is silent)
  }
  return InputEvent::none();
}

InputEvent StickCButtons::eventFor(int button) const {
  // The scheme ADR-0005 recorded for this board.
  switch (button) {
    case 0: return InputEvent::navigate(NavDir::Down);  // BtnA (G37): the list is a ring, Down wraps
    case 1: return InputEvent::select();                 // BtnB (G39)
    case 2: return InputEvent::back();                   // PWR (G35)
    default: return InputEvent::none();
  }
}

}  // namespace tp

#endif  // UNIT_TEST
