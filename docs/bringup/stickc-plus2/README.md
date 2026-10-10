# M5StickC Plus2 — bring-up proof

The ADR-0001/0005 acceptance bar for a board: a screenshot set plus a short input walk-through,
proving the HAL (display + input) and app render and respond on real hardware. CI only proves the
firmware links; this is the on-device evidence. This is the first keyboard-less board, so it also
proves the browse mode and the three-button scheme ADR-0020 added.

- **Board:** M5StickC Plus2 (ESP32-PICO-V3-02; ST7789 135×240 panel; three buttons, no keyboard).
- **Date:** 2026-10-10.
- **Firmware:** `tp_stickc_plus2` env (board `esp32-pico-devkitm-2`), run in **landscape**
  (rotation 1, 240×135) to reuse the Cardputer's already-proven layout — ADR-0007's one-sense-per-
  line is tuned for 240px width. Panel pins/offsets are M5GFX's `board_M5StickCPlus2` values; this
  proof is where they were verified on the panel. Screenshots captured over USB with the debug
  build (ADR-0006): `PLATFORMIO_BUILD_FLAGS=-DTP_DEBUG_SCREENSHOT pio run -e tp_stickc_plus2 -t
  upload`, then `python3 tools/fb_capture.py --out <file>` (the CH9102 bridge is a real UART, so
  this is the first capture that needed fb_capture's baud set — a USB-CDC board ignores it).

## Walk-through

The scheme is three-button cyclic (ADR-0005): BtnA (G37) = `Nav` Down, BtnB (G39) = `Select`,
PWR (G35) = `Back`. There is no `Char` — the board cannot type.

| Step | Input | Result | Frame |
|---|---|---|---|
| 1 | (boot) | Browse list: the whole corpus, headwords alphabetical, focused row yellow, no `>` query line | `01-browse.png` |
| 2 | BtnA ×N | the list scrolls as a ring, the window keeping the focus visible (here on `jan`) | `01-browse.png` |
| 3 | BtnB (`Select`) | Detail of the focused word (`jasima`): `< English  1/6 >`, glosses one per line, sitelen glyph top-right | `02-detail-en.png` |
| 4 | BtnA (`Nav`) | the detail language ring advances to `< Español  2/6 >` (`espejo` / `reflejar` / `opuesto` / `invertir`) | `03-detail-es.png` |
| 5 | PWR (`Back`) | returns to the browse list | (confirmed live) |

This exercises the board's whole input vocabulary — `Nav` (BtnA, in both the list and the detail
ring), `Select` (BtnB), `Back` (PWR) — and all three views a Buttons board reaches.

## What it confirms

- **The browse mode renders (ADR-0020).** With no query the empty root is the whole corpus, not
  the keyboard board's teaching prompt and not a blank screen — the first-class signal that a
  keyboard-less board can now reach a word. The focused row is accent-yellow and the window keeps
  it visible as BtnA scrolls; `01-browse.png` shows eight rows (the landscape panel's capacity),
  not all 137, so the windowing bounds the draw.
- **`Nav` is a ring driven by one button.** BtnA alone walks the browse list and, in Detail,
  cycles the language `English → Español` (`02` → `03`) — the wrapping Nav ADR-0020 made uniform,
  which a single-direction button needs. `Select` (BtnB) opens the focused entry; `Back` (PWR)
  returns to the list.
- **Landscape reuses the proven layout.** The detail view (`02`/`03`) is pixel-identical in
  structure to the Cardputer's: bold yellow headword, sitelen accent top-right, `< lang  n/6 >`
  label, one sense per line, no overflow at 240px. No per-board layout tuning was needed.
- **The M5GFX panel config is correct on hardware.** The panel lights, geometry is the right
  240×135, and colours are right (ST7789 `invert` true) — the pins, offsets and backlight sourced
  from `board_M5StickCPlus2` (SCLK 13 / MOSI 15 / DC 14 / CS 5 / RST 12, offset 52/40, BL 27) are
  verified, not just linked. The GPIO4 power-hold keeps the board alive off USB.
