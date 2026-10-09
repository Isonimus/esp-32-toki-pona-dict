---
id: 0012
title: Latin-diacritic text font for the device text renderer
status: accepted
date: 2026-10-09
---

# ADR-0012 — Latin-diacritic text font for the device text renderer

`LgfxDisplaySurface::drawText` drew text with LovyanGFX's built-in font, which covers only
ASCII (0x20–0x7E). Our dictionary ships glosses in display spelling with diacritics intact
(ADR-0002), so every accented es/fr/de/pt word — and the French `œ` ligature — rendered
on-device as placeholder boxes. The native language labels in the detail view had been
hand-folded to ASCII (`Espanol`, `Francais`, `Portugues`) as a workaround for the same gap.
Toki Pona and English were fully legible; the other four languages, a stated product goal, were
not.

## What the data actually needs (measured)

Scanning `data/dictionary.json` for non-ASCII codepoints yields **22 distinct characters**:

- **20 are in Latin-1** (U+00A0–U+00FF): the es/fr/de/pt accented letters and `ß`.
- **2 are beyond Latin-1**: `œ` (U+0153, Latin Extended-A, ×3 — `œil`, `cœur`) and an en dash
  `–` (U+2013, ×1).

The en dash was a typo for a hyphenated compound (`non–Toki Pona` → `non-Toki Pona`): punctuation,
not a diacritic, so it is corrected in the data (a hyphen is also the correct orthography). `œ`,
by contrast, is correct French spelling; folding it to `oe` would violate ADR-0002's
display-spelling rule. So the font must cover **ASCII + Latin-1 + at least U+0153** — i.e. Latin
Extended-A, not merely Latin-1.

## Options considered

- **Built-in GFX FreeFonts** (`fonts::FreeSans*`, bundled with LovyanGFX). Rejected: inspection of
  the installed library shows they are defined only for glyphs 0x20–0x7E. ASCII, same gap.
- **Bundled efont** (`lgfx_efont_*`). Rejected: the shipped subsets are full CJK (cn/ja/kr/tw),
  megabytes of flash, to obtain Latin glyphs that come along for the ride. Absurd cost.
- **A filesystem/VLW font loaded at runtime.** Rejected: needs a SPIFFS/LittleFS partition and
  file packaging; a compiled-in array is simpler and keeps the firmware self-contained (KISS).
- **The LovyanGFX U8g2 font bridge (`lgfx::U8g2font`) with one vendored compact Latin font.**
  Chosen. It is the library's supported path for Unicode glyphs, needs only a raw `const uint8_t[]`
  compiled in (no filesystem, no u8g2 dependency), and u8g2 offers a stock Latin-Extended glyph
  set (`_te`, Unicode 32–383) that covers every codepoint above exactly.

## Decision

Draw device text through the LovyanGFX U8g2 bridge with **two** vendored `_te` (Latin-Extended)
fonts, one per text role, each at its **native** size:

- **Body** (gloss / result list / query line) → **`u8g2_font_7x13_te`**, the X11 "Misc-Fixed"
  terminal font, 13px.
- **Header** (the headword) → **`u8g2_font_helvB14_te`**, Adobe Helvetica Bold, 18px.

- The arrays live at `hal/display/fonts/text_fonts.c` — it must sit under a compiled source tree
  (`third_party/` is not in the firmware `build_src_filter`). They are the upstream u8g2
  definitions verbatim, with only the `U8G2_FONT_SECTION(...)` macro (a u8g2.h internal the bridge
  does not use) removed.
- **Why two fonts, not one scaled.** The seam previously took an integer `size`, and the header was
  the body font at `size 2` — a 2× bitmap scale that looked blocky. The seam now takes a semantic
  `TextRole { Body, Header }` (`hal/display_surface.hpp`); the board maps each role to a real font
  drawn at native size, so the headword is crisp. Layout line pitch follows the measured metrics:
  `kLineHeight = 16` for the 13px body, `kHeaderHeight = 20` for the 18px header (decoded from the
  u8g2 font headers: 7x13 → 13px box; helvB14 → 14px ascent + 4px descent).
- **Why these two fonts specifically.** Chosen by comparing candidates *on the real panel* — a
  throwaway `TP_FONT_GALLERY` debug build rendered several `_te` fonts (helvR10/12, t0_13, 6x13,
  7x13) and header candidates (t0_17, helvB14) side by side in one flash, confirming legibility and
  `œ`/accent coverage for each. The initial single proportional `helvR12` read as blocky (scaled
  header) and cramped; 7x13 is crisper and more compact for prose glosses, and a bold native header
  stands apart from the body. The gallery scaffold was deleted once the choice was made.
- The native language labels are restored to correct spelling (`Español`, `Français`,
  `Português`) in `data/languages.json`, since the ASCII fold was only a workaround for this gap.

### Keeping it from regressing

Rendering is not host-assertable (ADR-0001), so the font swap itself is proven only by the
on-device bring-up proof. But the *failure mode* — a display string containing a character the
font cannot draw — is checkable on the source data. `tools/check-font-coverage.mjs` (in
`npm run test:host`, alongside `check-notices.mjs`) fails if any string in `dictionary.json` or
`languages.json` falls outside the fonts' glyph range. It fails on the pre-fix en dash and passes
after, and from now on guards every gloss edit against a stray em dash, smart quote, or non-Latin
character. This is a necessary, not sufficient, check: the vendored fonts are the authority on which
in-range glyphs truly exist, and the bring-up proof is the final word.

### Licensing

Both fonts are converted to the u8g2 format by olikraus/u8g2 and **shipped on-device** (unlike the
sitelen pona font, which is rasterized at build), so their notices must travel with the firmware.
`7x13` is X11 Misc-Fixed, **public domain** — no notice obligation, documented for provenance only.
`helvB14` is Adobe Helvetica Bold, under the permissive **Adobe/DEC** license: vendored verbatim at
`third_party/u8g2-fonts/LICENSE`, reproduced in `THIRD_PARTY_NOTICES.md`, enforced by
`tools/check-notices.mjs`. Provenance for both is in `third_party/u8g2-fonts/PROVENANCE.md`.

## Consequences

- Flash grows by the two font arrays (7x13 8564 B + helvB14 9868 B) plus the bridge decoder.
  Measured: **497,908 → 516,099 bytes (14.9% → 15.4%), +18,191 B (~17.8 KiB)**; RAM flat at 7.2%.
- All four accented languages, plus `œ` and the native labels, render as real glyphs; the headword
  is crisp rather than a blocky 2× scale.
- Two vendored fonts now ship on-device; the notices check covers the one with an obligation.
- The seam now carries text *intent* (`TextRole`), not a pixel scale — a cleaner contract that any
  future board maps to its own fonts; the app and every view are unchanged across boards.
- The coverage guard only knows the fonts' *range*, not their exact glyph tables, so a future gloss
  using an in-range glyph a font happens to lack would pass the host check and surface only
  on-device. Acceptable for our es/fr/de/pt set (all Latin-1 but `œ`); revisit if a language with
  sparse Latin-Extended needs is added.
