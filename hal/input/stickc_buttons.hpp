#pragma once

// The M5StickC Plus2's three buttons as an InputSource (ADR-0005, ADR-0020). Device-only: touches
// GPIO, behind the UNIT_TEST guard, never on the host lane.
//
// The board is keyboard-less, so it browses the corpus (ADR-0020) with the three-button cyclic
// scheme ADR-0005 recorded: BtnA (G37) = Nav Down (the list wraps, so Down alone traverses it),
// BtnB (G39) = Select, PWR (G35) = Back. All three sit on input-only pins (ESP32 GPIO 34-39 have
// no internal pull resistors), so they are read as plain INPUT against the board's external
// pull-ups: idle HIGH, pressed LOW.
//
// poll() returns one abstract InputEvent per call (ADR-0001), firing on the press edge of a
// debounced button. It is a cheap GPIO read when idle — no interrupt needed for three buttons.

#ifndef UNIT_TEST  // device-only: Arduino GPIO is not available on the host lane.

#include <cstdint>

#include "input_event.hpp"
#include "input_source.hpp"

namespace tp {

class StickCButtons : public InputSource {
 public:
  bool begin() override;
  InputEvent poll() override;

 private:
  // BtnA, BtnB, PWR in that order — the scheme ADR-0005 recorded. Index into the parallel arrays
  // below; eventFor() maps each to its app event.
  static constexpr int kButtonCount = 3;
  static constexpr uint8_t kPins[kButtonCount] = {37, 39, 35};

  // A transition is accepted only once the pin has been stable for this long, swallowing contact
  // bounce (a few ms on these tactile buttons); 25 ms is comfortably above the bounce and far below
  // a human press interval.
  static constexpr uint32_t kDebounceMs = 25;

  InputEvent eventFor(int button) const;

  bool ready_ = false;             // begin() ran; poll() is inert until then.
  bool wasDown_[kButtonCount] = {};     // last accepted (debounced) state, pressed = true
  uint32_t lastChangeMs_[kButtonCount] = {};  // when that state was last accepted
};

}  // namespace tp

#endif  // UNIT_TEST
