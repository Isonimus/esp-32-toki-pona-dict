# Cardputer ADV — bring-up proof

The ADR-0001/0005 acceptance bar for a board: a screenshot set plus a short input
walk-through, proving the HAL (display + input) and app render and respond on real hardware.
CI only proves the firmware links; this is the on-device evidence.

- **Board:** M5Stack Cardputer ADV (M5StampS3, ESP32-S3, no PSRAM; TCA8418 I²C keyboard).
- **Date:** 2026-10-08.
- **Firmware:** `tp_cardputer` env. Screenshots captured over USB with the debug build
  (ADR-0006): `PLATFORMIO_BUILD_FLAGS=-DTP_DEBUG_SCREENSHOT pio run -e tp_cardputer -t upload`,
  then `python3 tools/fb_capture.py --out <file>`.

## Walk-through

| Step | Input | Result | Frame |
|---|---|---|---|
| 1 | (boot) | Empty search view: `>` prompt | `01-empty.png` |
| 2 | type `to` | Result list: `toki` (focused, yellow) + `tomo tonsi tawa lon kama lawa` | `02-list.png` |
| 3 | Enter | Detail view, `< English 1/5 >`: `to talk / speak / language / hello` + sitelen glyph | `03-detail-en.png` |
| 4 | bare `.` | Detail cycles to `< Español 2/5 >`: `hablar / idioma / lenguaje / hola` | `04-detail-es.png` |

This exercises the whole `InputEvent` vocabulary and both views: `Char` (step 2), `Select`
(step 3), `Nav` (step 4, language cycle — ADR-0007), and the TCA8418 keyboard path including
bare-arrow navigation with no `fn` (ADR-0008). `Back` (Detail→Search) and list-row `Nav` were
also confirmed live but are not pictured.

> Frames `01`–`04` predate ADR-0012 and show the old ASCII-only font (note `Espanol` in `04`).

## Latin-diacritic text fonts (ADR-0012), 2026-10-09

The text renderer now uses two vendored Latin-Extended fonts — a compact `7x13` body and a bold
`helvB14` headword, both native size (no scaling). These frames prove the diacritic rendering
that the old built-in ASCII font could not; captured on `pilin`, cycling the detail language:

| Input | Result | Frame |
|---|---|---|
| `pilin` → Enter → `.` | `< Español 2/5 >`: `corazón` / `sentimiento` / `sentir` / `pensar` | `05-detail-es-accents.png` |
| `.` again | `< Français 3/5 >`: `cœur` / `sentiment` / `ressentir` / `penser` | `06-detail-fr-oe.png` |

What they confirm: accented letters (`ó`, `ñ`, `ç`) render as glyphs, not boxes; the `œ` ligature
(U+0153, French `cœur`) renders — covered by the `_te` (Latin-Extended) glyph set, which Latin-1
lacks; the native language labels are restored (`Español`, `Français`); the headword is crisp (a
real bold font, not the old blocky 2× scale); and there is a gap below the headword.

## Sitelen glyph enlarged to 44×44 (ADR-0013), 2026-10-09

The sitelen pona glyph was re-rasterized from 32×32 to **44×44** so it reads as a deliberate
script accent, not a small mark (operator feedback at the ADR-0012 bring-up). Re-rendered from
the vector font, so it is crisp at the larger size rather than an upscaled 32px bitmap.

| Input | Result | Frame |
|---|---|---|
| `to` → Enter | `< English 1/5 >`: `to talk / speak / language / hello`, enlarged glyph top-right | `07-detail-sitelen-44.png` |

Compare `07` against `03-detail-en.png` (same view at 32×32): the glyph is visibly larger and
sits crisply beside the ADR-0012 headword. The size change is global — the splash/About wordmark
and the empty-root prompt glyphs also enlarge.

## Centered empty-search root (ADR-0014), 2026-10-09

With the glyphs enlarged, the empty search root was reworked to center the type-to-search prompt
against the full panel and drop the lone edit caret. Captured on this build:

| View | Result | Frame |
|---|---|---|
| Empty search root | `lukin` icon + `o alasa e nimi` centered in the panel; no `>` caret while empty | `09-empty-prompt.png` |

What it confirms: the empty-root prompt is centered in the full panel and lifts off the bottom edge
(it was previously centered only within the result-area slice, which pushed it against the bottom);
and the bare `>` edit line is suppressed while the query is empty (ADR-0014), returning with the
result list on the first keystroke.
