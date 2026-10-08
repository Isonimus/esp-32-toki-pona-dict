# LEDGER

Open work: deferrals, defects, follow-ups. One line each; delete the line when closed.
Facts about *decisions* go in `adr/`, not here.

## Sitelen pona (ADR-0003)
- [ ] When an on-device About/credits view exists (needs the app/HAL work below), display the
      nasin nanpa MIT notice on-device too. Until then `THIRD_PARTY_NOTICES.md` discharges the
      MIT obligation for the distributed release, enforced by `tools/check-notices.mjs`
      (run in `npm run test:host`); the firmware release must bundle that file.

## HAL / app (ADR-0001, ADR-0005)
- [ ] Original Cardputer: implement the 74HC138 matrix scan in `CardputerKeyboard` (today a
      fail-loud stub; its decode is specified in ADR-0005) and add its bring-up proof. No new
      env or app change — one image already serves both variants by runtime detection. (The ADV
      bring-up proof is done: `docs/bringup/cardputer-adv/`.)
- [ ] Extract the Cardputer key→`InputEvent` mapping (`mapPress` + keymap) into a pure,
      host-testable unit so the keymap — incl. bare-arrow nav (ADR-0008) — gets a host regression
      test. Today the driver is device-only (ADR-0005), so the mapping is proven only on-device.
- [ ] Latin-diacritic text font for `LgfxDisplaySurface::drawText`: the built-in LovyanGFX font
      is ASCII, so accented es/fr/de/pt glosses render as placeholder boxes until a font with
      Latin-1/extended coverage is wired (ADR-0005). TP + EN are fully legible meanwhile.
- [ ] M5StickC Plus2 slice: its ST7789 135x240 panel class + a 3-button `InputSource`
      (three-button cyclic, ADR-0005), **and** a new ADR amending ADR-0004 for a
      browse-the-whole-corpus mode + cyclic (wrapping) Nav, so a keyboard-less board can reach a
      word (an empty query yields nothing today).

## Project
- [ ] Confirm the five-language set with the operator (currently en/es/fr/de/pt in
      `data/languages.json`); swapping one is a data change.
