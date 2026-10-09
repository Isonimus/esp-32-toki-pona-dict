# LEDGER

Open work: deferrals, defects, follow-ups. One line each; delete the line when closed.
Facts about *decisions* go in `adr/`, not here.

## Sitelen pona (ADR-0003)
- The on-device credit now exists: the About colophon (ADR-0009) shows the nasin nanpa font
      credit on-device. The **verbatim** MIT text stays in `THIRD_PARTY_NOTICES.md`, which ships
      with the firmware and discharges the obligation (enforced by `tools/check-notices.mjs` in
      `npm run test:host`); the on-device view is a condensed courtesy credit, not the legal text.

## Sitelen pona rendering (ADR-0003)
- [ ] Enlarge the detail-view sitelen glyph (operator feedback at ADR-0012 bring-up: "a bit
      bigger"). Glyphs are the fixed **32x32** build-time raster (ADR-0003, a *measured* size), so
      a crisp enlargement means re-rasterizing larger (e.g. 44-48px) — an ADR-0003 amendment — which
      also enlarges the splash/About wordmark and the empty-prompt glyphs and needs those layouts
      retuned. Decide the size and whether to scale globally or only the detail accent with the
      operator; its own commit, not bundled with the text font.

## HAL / app (ADR-0001, ADR-0005)
- [ ] Bring-up proof for the splash, About, and empty-root prompt views (ADR-0009, ADR-0010):
      extend `docs/bringup/cardputer-adv/` with the boot splash frame, the empty search root
      showing the `lukin` + "o alasa e nimi" prompt, and the `Back`-from-empty-search → About
      colophon frame. Rendering is not host-assertable (ADR-0001); capture the splash within its
      hold window (`kSplashHoldMs`), since it auto-dismisses.
- [ ] Original Cardputer: implement the 74HC138 matrix scan in `CardputerKeyboard` (today a
      fail-loud stub; its decode is specified in ADR-0005) and add its bring-up proof. No new
      env or app change — one image already serves both variants by runtime detection. (The ADV
      bring-up proof is done: `docs/bringup/cardputer-adv/`.)
- [ ] Host-test the TCA8418 raw-byte→`(row, col)` decode (the 7x8→4x14 mapping in
      `CardputerKeyboard::drainTca8418`). ADR-0011 left it in the device shell as ADV-I/O-specific,
      so this arithmetic is still proven only on-device; extract it alongside the 74HC138 scan when
      that variant lands, so both decodes feed the host-tested `applyKey` through a tested boundary.
- [ ] Bring-up proof for the Latin-diacritic text fonts (ADR-0012): the fonts are wired
      (`hal/display/fonts/text_fonts` — 7x13 body + helvB14 header; host lane green), but rendering
      is not host-assertable. Capture an es detail (accented, e.g. `pilin` -> `corazón`) and an fr
      detail proving the `œ` ligature (`pilin` -> `cœur`) into `docs/bringup/cardputer-adv/`,
      confirming glyphs not boxes and a crisp (non-blocky) headword; update that proof's README.
- [ ] M5StickC Plus2 slice: its ST7789 135x240 panel class + a 3-button `InputSource`
      (three-button cyclic, ADR-0005), **and** a new ADR amending ADR-0004 for a
      browse-the-whole-corpus mode + cyclic (wrapping) Nav, so a keyboard-less board can reach a
      word (an empty query yields nothing today).

## Project
- [ ] Confirm the five-language set with the operator (currently en/es/fr/de/pt in
      `data/languages.json`); swapping one is a data change.
