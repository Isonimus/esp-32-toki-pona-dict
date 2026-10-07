# esp-32-toki-pona — Project Conventions

An offline **Toki Pona ⇄ five-language** dictionary (English, Spanish, French, German,
Portuguese) for ESP32-based devices, with **sitelen pona** glyphs. Two-way lookup, keyboard
type-to-search or button/encoder browse depending on the board. Hardware sits behind a HAL
so one firmware flashes onto many boards, following `../esp32-wifi-sapper`.

General working practices — quality bar, testing standard, commit hygiene, delegation,
correction, language — live in `~/.claude/CLAUDE.md` and apply here without being restated.
This file carries only what is specific to **this** repo.

## Architecture (ADR-0001)

Four layers, dependencies inward only:

- `core/` — **pure C++17**: dictionary tables, `normalize`, `search`. No Arduino, no
  display, no heap in the search path. Host-testable with plain `g++`.
- `hal/` — `DisplaySurface` (over LovyanGFX) and `InputSource` (emits abstract
  `InputEvent`) interfaces + per-board drivers.
- `app/` — UI state machine: consumes `InputEvent`, queries `core`, draws to
  `DisplaySurface`. One UI serves keyboard and button boards.
- `main/` — per-board wiring only.

A new board is a HAL driver + a `platformio.ini` env. The core and app never change for it.

## Data and the generated core (ADR-0002)

- `data/dictionary.json` is the **single source of truth** — one record per headword,
  glosses in display spelling (diacritics intact). `data/languages.json` sets the language
  set and cycle order.
- `tools/generate.mjs` emits `core/dictionary_gen.cpp` and `core/normalization_fixtures_gen.h`.
  **These are generated — never hand-edit them.** They carry a `DO NOT EDIT` banner and are
  committed; CI must regenerate and fail on any diff (pending, see LEDGER).
- **Normalization is one contract in two implementations** — `core/normalize.cpp` (device)
  and `tools/generate.mjs` (build). They must agree; `data/normalization-fixtures.json` is
  the canonical set, asserted on both sides. Editing the folding rule means editing both and
  the fixtures together, in the same change.

## Verify before done

`npm run test:host` regenerates the tables, compiles the core with `-Wall -Wextra`, and runs
the native test (`test/test_search.cpp`): the normalization contract and symmetric
diacritic-folding lookup. It needs no hardware and must pass before any core change is
"done".

Rendering and on-device input cannot be asserted by a host test. Each board ships a
bring-up proof (screenshot + a short input walk-through), tracked in `LEDGER.md` — eyeballing
it once by hand is not the bar.

## Decisions are immutable (ADR-NNNN)

Changing our minds means a **new** ADR that supersedes the old one and says why the old
reasoning was wrong — never an in-place edit of a decision's body. Deferrals and follow-ups
go in `LEDGER.md`, not scattered in ADRs or in assistant memory.
