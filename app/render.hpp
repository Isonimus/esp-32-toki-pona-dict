#pragma once
#include "display_surface.hpp"
#include "ui_state.hpp"

// Draw the current UI state to a display (ADR-0004). This is the one place app state turns
// into pixels; it reads UiState and core tables and issues DisplaySurface calls only — no
// state mutation. Its correctness in pixels is a per-board bring-up proof (ADR-0001), not a
// host assertion, but it is pure of hardware and compiles/runs against any DisplaySurface.

namespace tp {

void render(const UiState& state, DisplaySurface& display);

}  // namespace tp
