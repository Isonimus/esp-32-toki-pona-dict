#pragma once
#include "input_event.hpp"

// The board-agnostic input seam (ADR-0001). A driver implements this once per input
// hardware; the app holds an InputSource& and never knows which board it is on.
//
// Pure interface: no Arduino here. The concrete drivers (keyboard, button/encoder) are
// device-only and live behind UNIT_TEST guards, exactly as the sapper's input HAL does.

namespace tp {

class InputSource {
 public:
  virtual ~InputSource() = default;

  // Bring the input hardware up. Returns false on a real init failure (a present but
  // unresponsive controller), which the caller logs loud (§fail-loud) — a dictionary with
  // no working input is dead, so this is not softened into a fallback.
  virtual bool begin() = 0;

  // Non-blocking: return the next pending event, or InputEvent::none() when nothing is
  // queued. The app's loop polls this every tick, so it must be cheap when idle (the
  // device drivers gate the costly read behind an interrupt flag, per the sapper pattern).
  // One event per call keeps the app's state machine driven by a single event at a time.
  virtual InputEvent poll() = 0;
};

}  // namespace tp
