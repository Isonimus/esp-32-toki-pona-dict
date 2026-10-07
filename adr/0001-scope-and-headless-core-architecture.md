---
id: 0001
title: Project scope and a headless, pure-core architecture
status: accepted
date: 2026-10-07
---

# ADR-0001 — Project scope and a headless, pure-core architecture

On 2026-10-07 we chose to build the Toki Pona dictionary as a pure, host-testable core
with hardware behind a HAL, so one firmware runs on many ESP32 boards, because the
project's whole cost is rendering and input portability, not the dictionary data.

## Context

The device is an offline two-way dictionary between **Toki Pona** and five languages
(English, Spanish, French, German, Portuguese — the set is declared in
`data/languages.json` and is a data change, not a code change). It must flash onto several
ESP32-based boards with different displays and inputs, mirroring the HAL approach of
`../esp32-wifi-sapper`.

Two facts shaped the architecture, both established before any code:

- **The data is tiny.** ~137 headwords × 5 languages is well under 100 KB of text. It fits
  in flash as a constant; there is no database, no filesystem, no network.
- **The cost is the surfaces.** Boards differ in screen driver and in whether they have a
  keyboard. The language set was deliberately restricted to Latin scripts (ADR rationale
  below), so rendering is one Latin+diacritics font plus the sitelen pona glyphs
  (ADR-0003) — no CJK, no RTL, no shaping engine.

## Decision

Four layers, dependencies pointing inward only:

| Layer | Contents | Depends on |
|---|---|---|
| `core/` | dictionary tables (generated), normalization, search | nothing but libstdc++ |
| `hal/` | `DisplaySurface`, `InputSource` interfaces + per-board drivers | core types |
| `app/` | UI state machine: consumes abstract `InputEvent`, queries core, draws | core, hal |
| `main/` | per-board wiring | all |

- **`core/` is pure C++17**: no Arduino, no LovyanGFX, no `delay()`, no heap in the search
  path (results go into a caller buffer). It compiles and is unit-tested on the host with
  plain `g++` (`npm run test:host`), exactly as the sapper's capture core is.
- **Input is capability-based, not board-specific.** Drivers emit an abstract
  `InputEvent` (`Char`, `Nav(up/down)`, `Select`, `Back`). A keyboard board emits `Char`;
  a button/encoder board emits `Nav`/`Select`. The single search-and-browse UI consumes
  these events, so a keyboard board gets type-to-search and a button board gets
  scroll-to-browse **with no separate code path** — this is what "board-dependent input"
  means in practice.
- **Display is a seam.** `DisplaySurface` is a thin interface over LovyanGFX; the panel is
  selected per board exactly as the sapper does. The app renders views (search + result
  list, entry detail) against the seam and never calls the panel directly.

### Why Latin-only five, not the five most-spoken languages

The top five by speakers (English, Mandarin, Hindi, Spanish, Arabic) would force a CJK
glyph set (megabytes of flash), RTL layout, and Devanagari/Arabic complex shaping — a
large multiple of the engineering and flash budget, for a device whose dictionary payload
is otherwise trivial. A Latin-only set needs one font and no shaper. The set is config, so
a later complex-script language is an additive font+shaper module, not a rewrite.

## Consequences

- A new board is a new HAL driver + build profile; the core and app never change for it.
- The core's correctness is a host unit test that runs with no hardware and no device
  toolchain, so regressions are caught in CI (ADR-0002 wires the generation step).
- Rendering and on-device input cannot be asserted by a host unit test; they need a
  bring-up proof per board, tracked in `LEDGER.md`.
