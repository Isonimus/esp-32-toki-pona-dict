# u8g2 Latin text fonts — vendored device fonts

The device text renderer (`LgfxDisplaySurface::drawText`, ADR-0012) draws two text roles with two
fonts. Unlike the sitelen pona font (rasterized at build), these **are** shipped to the device:
their glyph arrays are compiled into the firmware and decoded on-device by the LovyanGFX U8g2 font
bridge (`lgfx::U8g2font`).

Both are the **`_te`** ("text, extended") glyph set = Unicode 32–383 (ASCII + Latin-1 Supplement +
Latin Extended-A) — the smallest stock u8g2 set covering every non-ASCII codepoint in our dictionary
(measured in ADR-0012), including `œ` (U+0153), which Latin-1 (`_tf`) lacks. Converted to the u8g2
font format by the **u8g2** project (olikraus/u8g2, `csrc/u8g2_fonts.c`). Fetched: 2026-10-09.

Vendored at `hal/display/fonts/text_fonts.{c,h}` (they must live under a compiled source tree;
`third_party/` is not in the firmware `build_src_filter`). The only edit to each upstream array is
removing the `U8G2_FONT_SECTION(...)` wrapper, a u8g2.h-internal macro the LovyanGFX bridge does not
need — the bridge consumes the raw `const uint8_t[]`.

## Body font — `u8g2_font_7x13_te`

- **X11 "Misc-Fixed"** terminal bitmap font, 13px (BDF `-Misc-Fixed-Medium-R-Normal--13-120-75-75-C-70-ISO10646-1`).
- Used for gloss, result-list, and query text — crisp and compact on the 240×135 panel.
- **License: public domain** ("Public domain font. Share and enjoy." — the font's own BDF
  copyright). No notice obligation; recorded here for provenance.

## Headword font — `u8g2_font_helvB14_te`

- **Adobe Helvetica Bold**, 14pt / 100dpi (BDF `-Adobe-Helvetica-Bold-R-Normal--20-140-100-100-P-105-ISO10646-1`).
- Used for the large headword, at native size (not a 2× scale — ADR-0012).
- **License: Adobe/DEC X11 font license** (permissive, BSD-like) — see `LICENSE` in this directory.
  It permits redistribution, including the embedded array in firmware, provided the copyright and
  permission notice travel with every copy. That notice is reproduced verbatim in the top-level
  `THIRD_PARTY_NOTICES.md` and enforced by `tools/check-notices.mjs`.

## Reproducing

    # The whole u8g2 font collection is one ~40 MB C file; we vendor two arrays from it.
    curl -L -o u8g2_fonts.c \
      https://raw.githubusercontent.com/olikraus/u8g2/master/csrc/u8g2_fonts.c
    # Extract the u8g2_font_7x13_te and u8g2_font_helvB14_te comment + declaration blocks, stripping
    # the U8G2_FONT_SECTION wrapper. (The arrays are ASCII with octal escapes and no raw high bytes,
    # so they are portable C source.)

    # License (Adobe Helvetica is the X11 adobe-100dpi family):
    curl -L -o LICENSE \
      https://gitlab.freedesktop.org/xorg/font/adobe-100dpi/-/raw/master/COPYING
