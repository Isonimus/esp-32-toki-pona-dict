# LEDGER

Open work: deferrals, defects, follow-ups. One line each; delete the line when closed.
Facts about *decisions* go in `adr/`, not here.

## Core / data
- [ ] Optional: extend the corpus beyond nimi pu (120) with common *nimi ku suli* words
      (e.g. kijetesantakalu, tonsi, kipisi, monsuta) if the operator wants them.
- [ ] Wire a CI lane that runs `npm run test:host` and fails if `node tools/generate.mjs`
      produces a diff against the committed `core/*_gen.*` (ADR-0002).

## Sitelen pona (ADR-0003)
- [ ] Source a sitelen pona font with a known, compatible license.
- [ ] Probe the glyph bitmap size (legibility vs. flash) and amend ADR-0003 with the number.
- [ ] Write the rasterizer (`tools/`) that emits `core/sitelen_gen.*`.

## HAL / app (ADR-0001)
- [ ] Keyboard input driver (first reference board) and a button/encoder driver.
- [ ] LovyanGFX display driver behind `DisplaySurface`, panel auto-detect per board.
- [ ] Wire the sitelen glyph blit into the detail view's reserved slot once
      `core/sitelen_gen` exists (`app/render.cpp` renderDetail; depends on ADR-0003 slice).
- [ ] `platformio.ini`: real build profiles per board; first reference board bring-up proof.

## Project
- [ ] Confirm the five-language set with the operator (currently en/es/fr/de/pt in
      `data/languages.json`); swapping one is a data change.
