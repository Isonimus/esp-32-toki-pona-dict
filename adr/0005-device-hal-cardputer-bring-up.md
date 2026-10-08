---
id: 0005
title: Device HAL — LovyanGFX display, Cardputer keyboard, and per-board bring-up
status: accepted
date: 2026-10-08
---

# ADR-0005 — Device HAL: LovyanGFX display, Cardputer keyboard, per-board bring-up

On 2026-10-08 we began filling the `hal/` and `main/` layers ADR-0001 reserved, bringing up
the **M5Stack Cardputer** first. The pure core and the host-tested app already exist; this
decision is only about the device surfaces — how a panel and a keyboard become the
`DisplaySurface` and `InputSource` the app already consumes — and how a board is selected and
built. It follows `../esp32-wifi-sapper`, whose LovyanGFX-only display and capability-based
input this project mirrors, and whose proven Cardputer panel config and TCA8418 register
sequence we reuse directly.

## Context

The targets the operator named are the **Cardputer** (both the original and the ADV) and the
**M5StickC Plus2**. Investigating them against the existing code (the espp
`m5stack-cardputer` component and M5's own libraries are the measured basis) established three
facts that shape this decision:

- **The two Cardputers are the same board but for keyboard read-out.** Both are an M5StampS3
  (ESP32-S3, no PSRAM, 8 MB flash), the same 1.14" ST7789 240×135 panel on the same pins. The
  *original* scans a 74HC138 GPIO key matrix; the *ADV* reads a TCA8418 I²C keypad controller
  on the internal bus (SDA 8 / SCL 9 / INT 11) — the same two pins the original repurposes as
  74HC138 address lines. espp tells them apart at runtime by probing I²C `0x34`.
- **A keyboard board needs no app change; a keyboard-less board does.** The app runs a query
  only when one has been typed (`ui_controller.cpp`: empty query ⇒ zero results), so a
  keyboard board reaches every word against the *unchanged* app, exactly as ADR-0001 promised
  ("a new board is a HAL driver + a build profile; the core and app never change"). The
  StickC Plus2 has no keyboard, so it cannot reach a single word today — ADR-0001's
  "scroll-to-browse" was never built, and ADR-0004 made "cleared query ⇒ no results" the rule.
- **The Cardputer keyboard layout is build-independent data.** The 4×14 key grid, its
  characters, and the fn-layer arrows are the silkscreen, identical on both variants; only the
  (row,col) read-out differs. So one keymap serves both read-out paths.

## Decision

1. **Board selection is a compile-time `-DTP_BOARD_*` flag with a hard-error default** —
   `main/` resolves the flag to its wiring, and a build with no flag is a compile error, never
   a silent default (mirrors the sapper's `active_board.h`). We **deliberately do not** build
   the sapper's `BoardProfile` capability struct yet: each driver owns its own pins, `main`
   wires one board's concrete drivers directly, and nothing branches on a *capability* at
   runtime. A capability table is speculative generality until a second distinct board forces
   capability branching (KISS, rule-of-three). This is a considered divergence from the
   sapper, recorded here rather than taken silently; the table arrives, with its own note, if
   and when the branching does.

2. **The Cardputer is one target (`TP_BOARD_CARDPUTER`) that serves both variants by runtime
   detection.** `InputSource::begin()` probes the TCA8418 at I²C `0x34` on GPIO 8/9; an ACK is
   the ADV (drive the TCA8418), no ACK is the original (drive the 74HC138 matrix). This is the
   direct expression of ADR-0001's "one firmware runs on many boards": one image, one flash,
   either Cardputer. The register sequence and the event decode are the sapper's / espp's
   proven values, cited in the driver.

3. **The display is a LovyanGFX-backed `DisplaySurface` over an off-screen sprite canvas**,
   parameterised by a per-board panel class and rotation. The Cardputer panel class is the
   sapper's `LGFX_Cardputer` (ST7789, SPI3, offsets confirmed on real ADV hardware). We stay
   on LovyanGFX directly — **not** M5Unified / M5GFX — for consistency with the sapper and
   because M5GFX would collide with this stack; a hand-rolled matrix scanner and TCA8418
   driver are a few dozen lines each and keep the dependency surface to LovyanGFX alone.

4. **Device code is quarantined from the host lane.** Every translation unit that touches
   Arduino / LovyanGFX / `Wire` lives behind `#ifndef UNIT_TEST` and is compiled only by the
   PlatformIO device build, never listed in `npm run test:host`. The host lane stays the
   correctness oracle for everything pure (core + app + the event mapping's pure parts); the
   device build's job is that it *compiles and links*, and CI gains that leg with this first
   profile (it was dead config before — LEDGER).

5. **Rendering and on-device input are proven per board, not asserted in CI.** A host test
   cannot see a panel or a keypress (ADR-0001), so each board ships a bring-up proof —
   screenshot + a short input walk-through — tracked in `LEDGER.md`. The Cardputer ADV path is
   brought up and proven first (its panel and TCA8418 are the sapper's already-proven
   hardware); the original Cardputer's 74HC138 matrix path is a **fail-loud stub** (its
   `begin()` returns false with a clear log) until its own bring-up slice writes and proves the
   scan, rather than shipping a second never-run driver beside a first that is about to be
   hardware-validated.

### Keyboard → InputEvent mapping

The app's vocabulary is `Char / Nav / Select / Back` (ADR-0001). Case folds away in the query
path (ADR-0002), so the driver emits the **unshifted** character and tracks only the fn
modifier (the Cardputer's only arrow source):

| Key | Event |
|---|---|
| a printable key (letter, digit, space, `'`, `-`, …) | `Char(value)` |
| enter | `Select` |
| backspace, or fn+backspace (delete), or fn+` (esc) | `Back` |
| fn+`;` (up), fn+`.` (down) | `Nav(Up/Down)` |
| modifiers, fn+left/right, fn+F-keys | ignored |

### M5StickC Plus2 (deferred to its own slice + ADR)

The StickC Plus2 is a different panel (ST7789 135×240) and a keyboard-less, 3-button board, so
it needs more than a HAL driver: a **browse-the-whole-corpus** UI mode (so Nav/Select have
something to act on with no typed query) and **cyclic** navigation (the list wraps; the UI
clamps today). That amends ADR-0004's "cleared query ⇒ no results" and changes the app, so it
is its own decision, not folded in here. The operator's chosen control scheme is recorded for
that slice: three-button cyclic — BtnA (G37) = Nav Down (wraps), BtnB (G39) = Select, PWR
(G35) = Back.

## Consequences

- Adding the original Cardputer is finishing one function (the 74HC138 scan + the shared
  keymap are already specified here) and a bring-up proof — no new env, no app change, no ADR.
- One firmware image flashes onto either Cardputer; the operator never picks a variant.
- The device build is the only place the pin/panel facts live outside this ADR; a wrong pin
  fails the bring-up proof, not a host test — the same contract ADR-0001 set.
- The StickC Plus2 carries a real app change behind it, so its slice is larger than a Cardputer
  variant; sequencing it last is deliberate.
- Text with diacritics is not yet legible: LovyanGFX's built-in font is ASCII, so Toki Pona
  and English render in full while accented es/fr/de/pt glosses show placeholder boxes until a
  Latin-diacritic font lands (LEDGER). The glosses keep their diacritics in the data (ADR-0002)
  regardless; this is a render-font gap, not a data one.
