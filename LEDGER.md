# LEDGER

Open work: deferrals, defects, follow-ups. One line each; delete the line when closed.
Facts about *decisions* go in `adr/`, not here.

## Sitelen pona (ADR-0003)
- The on-device credit now exists: the About colophon (ADR-0009) shows the nasin nanpa font
      credit on-device. The **verbatim** MIT text stays in `THIRD_PARTY_NOTICES.md`, which ships
      with the firmware and discharges the obligation (enforced by `tools/check-notices.mjs` in
      `npm run test:host`); the on-device view is a condensed courtesy credit, not the legal text.

## HAL / app (ADR-0001, ADR-0005)
- [ ] Original Cardputer: implement the 74HC138 matrix scan in `CardputerKeyboard` (today a
      fail-loud stub; its decode is specified in ADR-0005) and add its bring-up proof. No new
      env or app change — one image already serves both variants by runtime detection. (The ADV
      bring-up proof is done: `docs/bringup/cardputer-adv/`.)
- [ ] Host-test the TCA8418 raw-byte→`(row, col)` decode (the 7x8→4x14 mapping in
      `CardputerKeyboard::drainTca8418`). ADR-0011 left it in the device shell as ADV-I/O-specific,
      so this arithmetic is still proven only on-device; extract it alongside the 74HC138 scan when
      that variant lands, so both decodes feed the host-tested `applyKey` through a tested boundary.
- [ ] M5StickC Plus2 slice: its ST7789 135x240 panel class + a 3-button `InputSource`
      (three-button cyclic, ADR-0005), **and** a new ADR amending ADR-0004 for a
      browse-the-whole-corpus mode + cyclic (wrapping) Nav, so a keyboard-less board can reach a
      word (an empty query yields nothing today).
- [ ] Bring-up proof for the no-results affordance (ADR-0019): capture a no-match frame (type a
      word with no result, e.g. `zzzz`) into `docs/bringup/cardputer-adv/`, confirming the grey
      `ala` icon + `nimi li weka` sentence read as "nothing found" on the real panel and the grey
      (`kColorMuted = 0x8410`) is legible against black. The grey constant is the only thing that
      changes if the shade reads too dark or too light; the 44px glyphs are inherited from ADR-0013.

## Project
- [ ] Declare the **firmware code** license. ADR-0016 licenses the dictionary *data* CC BY-SA 4.0,
      but the code (`core/` logic, `hal/`, `app/`, `main/`) has no `LICENSE` file and no declared
      license yet. Pick one and add a top-level `LICENSE`, keeping the data's BY-SA distinct.
