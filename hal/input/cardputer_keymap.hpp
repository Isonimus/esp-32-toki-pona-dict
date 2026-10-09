#pragma once

// The pure part of the Cardputer keyboard (ADR-0011), split out of the device driver so the
// keymap and its fn-layer / bare-arrow policy (ADR-0005, ADR-0008) get a host regression test.
//
// This translation unit is pure: no Arduino, no Wire, no GPIO — it compiles on the host lane and
// is NOT behind the UNIT_TEST guard. The device shell (cardputer_keyboard.cpp) owns the I2C/GPIO
// and the hardware-specific decode of a raw event to a grid cell; it feeds that cell in here.
//
// The seam is grid coordinates, not a raw hardware byte, because that is the variant-agnostic
// boundary (ADR-0011): both the ADV's TCA8418 FIFO decode and the original Cardputer's future
// 74HC138 matrix scan resolve a press to the same (row, col), so both reuse applyKey unchanged.

#include "input_event.hpp"

namespace tp {

// The Cardputer's silkscreen key grid: 4 rows x 14 columns, identical on both variants
// (ADR-0005). Public so the driver and the host test refer to the shape by name, never as a
// bare 4/14.
inline constexpr int kCardputerKeyRows = 4;
inline constexpr int kCardputerKeyCols = 14;

// The fn modifier's position in that grid (the Cardputer's arrow layer, the only modifier we
// track). Public so the test can press it by name rather than by a magic cell.
inline constexpr int kCardputerFnRow = 2;
inline constexpr int kCardputerFnCol = 0;

// The result of applying one key event: the app event it produces (None for a modifier, a
// release, an unmapped key, or out-of-grid coordinates) and the fn-modifier state afterwards.
struct KeyOutcome {
  InputEvent event;
  bool fnHeld;
};

// Resolve one key event at grid cell (row, col) — `pressed` true on the key-down edge, false on
// key-up — against the current fn state, to the app event it means and the fn state after it.
// Pure and total: out-of-grid coordinates yield None and leave fn unchanged (the driver need not
// pre-validate). The fn key itself updates the modifier on both edges and emits nothing; every
// other release emits nothing; a press maps through the keymap (ADR-0008 arrow policy included).
KeyOutcome applyKey(int row, int col, bool pressed, bool fnHeld);

}  // namespace tp
