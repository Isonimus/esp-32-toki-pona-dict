---
id: 0014
title: Centered empty-search root, with the edit caret suppressed while empty
status: accepted
date: 2026-10-09
amends: ADR-0010 (empty-root presentation; the '>' edit line is now suppressed while empty)
---

# ADR-0014 — Centered empty-search root, with the edit caret suppressed while empty

Enlarging the sitelen glyphs to 44×44 (ADR-0013) made the empty search root (ADR-0010) read as
unbalanced, which the operator flagged. Two things were wrong:

- The `lukin` + `o alasa e nimi` prompt block was centered within the *result-area slice* (the
  region below where result rows begin), not the whole panel, which pushed the block hard against
  the bottom edge and left a large gap above it.
- A bare `>` edit line floated alone in the top-left corner, disconnected from the centered prompt.
  At 44px glyphs the centered prompt already *is* the empty state; the lone caret read as an
  unfinished screen beside it.

## Decision

- **The prompt block is centered against the full panel height.** The `lukin` icon row and the
  `o alasa e nimi` sentence row are treated as one block whose height is measured, then centered in
  `height()` — so it lifts off the bottom edge and the top gap closes. This needs only the panel
  size the seam already exposes and `drawGlyphRowCentered`, which already centers a glyph row in
  `width()`; no new seam primitive.
- **The bare `>` edit line is suppressed while the query is empty.** This supersedes ADR-0010's
  "the `>` edit line is unchanged". No affordance is lost: the prompt itself teaches type-to-search
  (ADR-0010), and the `>` reappears with the result list on the first keystroke — a cleaner
  empty→active transition than a caret that was always on screen.
- **The splash and About are out of scope here.** The splash is reworked separately into the merged
  logo (ADR-0015); the About colophon is a dense, left-aligned credit list, not a centered brand
  moment, and the operator flagged neither as part of this change.

### Why not a boot menu / locale selector

The operator weighed a boot menu that sets a UI language and offers "search", keeping the choice as
the detail default. Rejected for the keyboard board, for two reasons grounded in the code:

- **Two-way search already exists and is already mode-free.** `UiController` searches *all six*
  language indexes per keystroke and merges the hits (`app/ui_controller.cpp`), so either side of
  the pair is a valid query today (ADR-0004). A menu would not enable two-way search; requiring a
  language to be chosen first would *reintroduce* the mode ADR-0004 deliberately removed.
- **There is almost no chrome to localize, and what exists is deliberately Toki Pona** — the prompt
  and wordmark are sitelen, the detail labels are each language's own native name. A localization
  system would be machinery for a designed-away problem.

A menu does have a real home — input-poor boards (the M5StickC Plus2's 3 buttons) cannot reach a
word from an empty query — but that is a **per-board** browse affordance behind the HAL, already
tracked in `LEDGER.md`, not a boot screen forced on the keyboard board. A "detail opens on my
preferred language" feature is also worth having, but as a sticky, NVS-persisted default learned
from use — no menu, no mode — and is left as a separate future decision.

### Keeping it from regressing

Layout is not host-assertable (ADR-0001), but the one behavioral change is: the empty root now
draws **zero** text (`test/test_ui.cpp` asserts `texts == 0`, where it was `1` for the old `>`
line) while still blitting the five prompt glyphs. That check fails on the pre-change renderer and
passes after. On-device proof: `docs/bringup/cardputer-adv/09-empty-prompt.png` (centered block, no
corner caret).

## Consequences

- No new `View`, `UiState`, `InputEvent`, or seam method — the mode-free spine (ADR-0004) and the
  `DisplaySurface` interface are untouched; this is a renderer-only change. No flash or RAM cost.
- The empty root's first keystroke now also *adds* the `>` line (it was always drawn before), a
  cleaner empty→active transition.
- Centering against the full panel is the same pattern the splash logo uses (ADR-0015), so the two
  text-light views now share one centering approach.
