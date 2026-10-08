#pragma once
#include "input_event.hpp"
#include "ui_state.hpp"

// The one UI state machine (ADR-0004). It consumes the abstract InputEvent vocabulary and
// queries core::search, so a keyboard board (emitting Char) and a button/encoder board
// (emitting Nav/Select) drive the same code with no separate path (ADR-0001). Pure: depends
// on core only, no DisplaySurface, no Arduino — fully host-tested.

namespace tp {

class UiController {
 public:
  const UiState& state() const { return state_; }

  // Advance the state machine by one event. None is ignored, so the app can poll the input
  // source and forward whatever it gets unconditionally.
  void handle(const InputEvent& event);

  // Dismiss the boot splash into the search root (ADR-0009). The controller is clockless, so
  // main/ calls this once the splash hold elapses; the first input dismisses it sooner via
  // handle(). Idempotent: a no-op once the splash is already gone.
  void dismissSplash();

 private:
  // Rebuild `results` from the current query by searching every language and merging the
  // hits (deduped, cycle order), then clamp `selected` into range. Mode-free symmetric
  // search (ADR-0004). A cleared query yields no results.
  void runSearch();

  UiState state_;
};

}  // namespace tp
