# LEDGER

Open work: deferrals, defects, follow-ups. One line each; delete the line when closed.
Facts about *decisions* go in `adr/`, not here.

## Sitelen pona (ADR-0003)
- The on-device credit now exists: the About colophon (ADR-0009) shows the nasin nanpa font
      credit on-device. The **verbatim** MIT text stays in `THIRD_PARTY_NOTICES.md`, which ships
      with the firmware and discharges the obligation (enforced by `tools/check-notices.mjs` in
      `npm run test:host`); the on-device view is a condensed courtesy credit, not the legal text.

## HAL / app (ADR-0001, ADR-0005)
- [ ] Bring-up proof for the splash + About views (ADR-0009): extend `docs/bringup/cardputer-adv/`
      with the boot splash frame and the `Back`-from-empty-search → About colophon frame.
      Rendering is not host-assertable (ADR-0001); capture the splash within its hold window
      (`kSplashHoldMs`), since it auto-dismisses.
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

## UX / polish (ADR-0004)
- [ ] Boot splash: a brief sitelen pona wordmark/title on startup before the search view.
      Design it together with the on-device About/credits view (Sitelen pona item above) — the
      splash is the natural home for the nasin nanpa MIT notice, so they are one screen's worth
      of work, not two. Needs an ADR (amending ADR-0004's view set) when built.
- [ ] Empty-search-view affordance: a short sitelen pona prompt sentence + a "look/search"
      icon beside today's bare `>`, to signal type-to-search and the two-way lookup. The sitelen
      sentence is free (glyphs already render, ADR-0003); the icon is the open question — either
      a small bitmap asset or reuse the sitelen glyph `lukin` ("see/look") to avoid a new asset.
      Cosmetic, no Latin-diacritic font dependency. ADR when built.

## Project
- [ ] Confirm the five-language set with the operator (currently en/es/fr/de/pt in
      `data/languages.json`); swapping one is a data change.
