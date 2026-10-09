# Third-party notices

This product includes third-party software. Their license notices are reproduced below and
**must travel with every distributed copy** of the firmware — source, binary, or flashed
device image. `tools/check-notices.mjs` (run by `npm run test:host`) fails the build if any
notice here drifts from its vendored source, so this obligation cannot rot silently.

---

## nasin nanpa (sitelen pona font)

- Upstream: https://github.com/etbcor/nasin-nanpa — release n4.0.2, UCSUR variant.
- Vendored at `third_party/nasin-nanpa/` (see its `PROVENANCE.md`).
- **Not** shipped or parsed on-device: the font is rasterized at build time (ADR-0003) into
  the fixed-size 1-bpp bitmaps in `core/sitelen_gen.cpp`. Those bitmaps are a derivative work
  of the font, so this notice accompanies the firmware that embeds them.

Verbatim license (`third_party/nasin-nanpa/LICENSE`):

```
MIT License

Copyright (c) 2022 jan Itan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## Adobe Helvetica Bold (device headword font, via u8g2)

- Source: Adobe Helvetica Bold (X11 `adobe-100dpi` BDF family), converted to the u8g2 font format
  (`u8g2_font_helvB14_te`, from olikraus/u8g2).
- Vendored at `third_party/u8g2-fonts/` (see its `PROVENANCE.md`); the font array is compiled
  into the firmware at `hal/display/fonts/text_fonts.c`.
- Shipped **on-device**: unlike the sitelen pona font, this font's glyph data is embedded in the
  firmware and decoded at runtime (ADR-0012) to render the headword. The embedded array is a copy
  of the licensed font, so this notice accompanies the firmware.
- The device **body** font in the same file (`u8g2_font_7x13_te`, X11 Misc-Fixed) is public
  domain and carries no notice obligation; see `PROVENANCE.md`.

Verbatim license (`third_party/u8g2-fonts/LICENSE`):

```
Copyright 1984-1989, 1994 Adobe Systems Incorporated.
Copyright 1988, 1994 Digital Equipment Corporation.

Adobe is a trademark of Adobe Systems Incorporated which may be
registered in certain jurisdictions.
Permission to use these trademarks is hereby granted only in
association with the images described in this file.

Permission to use, copy, modify, distribute and sell this software
and its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notices appear in all
copies and that both those copyright notices and this permission
notice appear in supporting documentation, and that the names of
Adobe Systems and Digital Equipment Corporation not be used in
advertising or publicity pertaining to distribution of the software
without specific, written prior permission.  Adobe Systems and
Digital Equipment Corporation make no representations about the
suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.
```
