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

## Known limitation visible here

`Español` renders as `Espanol` — the built-in LovyanGFX font is ASCII, so es/fr/de/pt
diacritics drop or box. Tracked in `LEDGER.md` (Latin-diacritic font); Toki Pona and English
are fully legible meanwhile.
