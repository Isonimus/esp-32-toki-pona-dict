---
id: 0011
title: Host-testable Cardputer keymap split out of the device driver
status: accepted
date: 2026-10-09
amends: ADR-0001 (hal/ is not uniformly device-only — its pure parts join the host lane)
---

# ADR-0011 — Host-testable Cardputer keymap split out of the device driver

The Cardputer key→`InputEvent` mapping (`mapPress`) and the keymap table it reads — including
the bare-arrow navigation of ADR-0008 — lived inside `hal/input/cardputer_keyboard.cpp`, a
translation unit wrapped **entirely** in `#ifndef UNIT_TEST`. That is pure logic (a lookup
table and a branch on `Special`/fn state — no Arduino, no Wire, no GPIO) trapped behind an I/O
guard, so it was proven only on-device. ADR-0008 changed the arrow policy with no host
regression test; the only guard against a silkscreen-position or fn-layer regression was
eyeballing it on hardware, which `CLAUDE.md` rejects as the bar.

## Context

ADR-0001 put device drivers in `hal/` behind `UNIT_TEST` because they touch hardware the host
lane cannot link. But a driver is rarely all hardware: this one is an I2C/GPIO shell wrapped
around a pure decode. Guarding the whole file out of the host build discards the testable part
with the untestable part.

Two seams could carry the split:

- **At the raw TCA8418 event byte** — one function `(rawByte, fnHeld) → outcome`. Maximises
  what the test drives, but couples the pure unit to the TCA8418's event encoding and its 7×8
  key-number layout, which are specific to the **ADV** variant. The original Cardputer scans a
  74HC138 matrix (a fail-loud stub today, ADR-0005/LEDGER) and produces no such byte.
- **At grid coordinates** — `(row, col, pressed, fnHeld) → outcome`. The keymap, the fn layer,
  and the ADR-0008 arrow policy all live here; the hardware-specific "which byte is which cell"
  stays in the driver. This boundary is **variant-agnostic**: both the TCA8418 FIFO decode and
  the future 74HC138 scan resolve a press to a `(row, col)`, so both feed the same pure unit.

## Decision

- **Extract the pure keymap into `hal/input/cardputer_keymap.{hpp,cpp}`, _not_ behind
  `UNIT_TEST`,** so the host lane compiles and tests it. It owns the grid table, the grid
  dimensions and fn-key position (named constants), grid-bounds rejection, fn-modifier tracking,
  and the `mapPress` policy — exposed as one function
  `applyKey(row, col, pressed, fnHeld) → KeyOutcome { InputEvent event; bool fnHeld; }`.
  `KeyOutcome` returns the next fn state alongside the event so fn tracking is pure too (the fn
  key press/release updates the modifier and emits nothing; every other release emits nothing).
- **The device driver keeps only I/O.** `cardputer_keyboard.cpp` stays behind `UNIT_TEST` and
  does the Wire/GPIO, the FIFO drain, and the TCA8418 raw-byte→`(row, col)` decode; per cell it
  calls `applyKey` and acts on the result. The bounds check and fn-cell branch move out of the
  driver into `applyKey`, so the shell no longer knows the grid shape.
- **The seam is grid coordinates, not the raw byte** (the second option above), so the original
  Cardputer's 74HC138 scan will reuse `applyKey` unchanged when it lands.

This clarifies ADR-0001: `hal/` is **not uniformly device-only**. A driver's pure logic is
extracted into an unguarded sibling translation unit and joins the host lane; the `UNIT_TEST`
guard is reserved for the hardware shell that actually links Arduino/Wire/GPIO.

## Consequences

- A new host test (`test/test_cardputer_keymap.cpp`, in `npm run test:host`) asserts the keymap
  and policy that were previously host-invisible: bare `;`/`.` navigate **with and without fn**
  (the ADR-0008 regression guard), fn+esc and fn+delete are Back, Enter is Select, backspace is
  Back, a printable key is Char, and modifiers / fn-layer misses / key releases / out-of-grid
  coordinates are all None — plus the fn key press/release toggling the tracked state.
- The TCA8418 `keyNumber → (row, col)` arithmetic stays in the driver and remains
  host-unverified: it is ADV-I/O-format-specific, not keymap, and small. Kept out of this slice
  deliberately; a LEDGER follow-up records the remaining gap so it is not silently dropped.
- One behaviour moved rather than changed, so the extraction is behaviour-preserving for the
  device: the driver's per-event work is now `applyKey` plus enqueue. The ADV bring-up proof
  (ADR-0006, `docs/bringup/cardputer-adv/`) still covers the device path end to end.
