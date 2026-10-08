# LEDGER

Open work: deferrals, defects, follow-ups. One line each; delete the line when closed.
Facts about *decisions* go in `adr/`, not here.

## Sitelen pona (ADR-0003)
- [ ] When an on-device About/credits view exists (needs the app/HAL work below), display the
      nasin nanpa MIT notice on-device too. Until then `THIRD_PARTY_NOTICES.md` discharges the
      MIT obligation for the distributed release, enforced by `tools/check-notices.mjs`
      (run in `npm run test:host`); the firmware release must bundle that file.

## HAL / app (ADR-0001)
- [ ] Keyboard input driver (first reference board) and a button/encoder driver.
- [ ] LovyanGFX display driver behind `DisplaySurface`, panel auto-detect per board. Must
      implement `drawGlyph1bpp` to the packing in `core/sitelen.hpp` (MSB-first, row-major,
      byte-aligned rows); `app/render.cpp` already issues the blit, host-tested in `test_ui`.
- [ ] `platformio.ini`: real build profiles per board; first reference board bring-up proof.
      With the first profile, add a device-build leg to `.github/workflows/ci.yml` (the host
      lane already runs `check:generated` + `test:host`); until a profile exists a board matrix
      would be dead config.

## Project
- [ ] Confirm the five-language set with the operator (currently en/es/fr/de/pt in
      `data/languages.json`); swapping one is a data change.
