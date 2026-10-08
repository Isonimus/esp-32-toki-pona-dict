# nasin nanpa — vendored sitelen pona font

Build-time input for the sitelen pona glyph rasterizer (ADR-0003). Rasterized to 1-bpp
bitmaps at build; the font itself is **not** shipped to the device or parsed on-device.

## Source

- Font: **nasin nanpa**, a monospaced sitelen pona typeface by jan Itan.
- Upstream: https://github.com/etbcor/nasin-nanpa
- Release: **n4.0.2** (`nasin-nanpa-4.0.2-UCSUR.otf`), the UCSUR-mapped variant.
- Fetched: 2026-10-07.

The UCSUR variant is the one we vendor on purpose: it maps glyphs onto the Unicode
Private-Use-Area sitelen pona block, so the rasterizer enumerates glyphs by codepoint
derived from each headword — no parallel name→glyph table (keeps ADR-0002's single source
of truth intact).

## License

**MIT**, Copyright (c) 2022 jan Itan — see `LICENSE` in this directory.

MIT permits redistribution (including rasterized derivatives in firmware) and carries one
obligation: the copyright and permission notice must ship with the product. Track that in
`LEDGER.md` as a firmware-packaging item; it is lighter than OFL (no reserved-name rule).

## UCSUR coverage we depend on (verified with `fc-query`)

- `U+F1900–U+F1988` — the main sitelen pona block, **137 glyphs**, the closed set that maps
  1:1 onto headwords. This is the range the rasterizer iterates.
- `U+F1990–U+F19A6`, `U+F19AE`, `U+F19C0` — cartouche/joiner and punctuation extensions,
  not needed for one-glyph-per-headword rendering.

## Reproducing

    curl -L -o nasin-nanpa-UCSUR.otf \
      https://github.com/etbcor/nasin-nanpa/releases/download/n4.0.2/nasin-nanpa-4.0.2-UCSUR.otf
    curl -L -o LICENSE \
      https://raw.githubusercontent.com/etbcor/nasin-nanpa/main/LICENSE
