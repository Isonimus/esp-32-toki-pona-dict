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

---

## lipu Linku (dictionary glosses)

- Source: lipu Linku's `sona` data — https://github.com/lipu-linku/sona (the
  `translations.*.definition` fields). lipu Linku is the Toki Pona community's multilingual
  dictionary.
- Used: the English, Spanish, French, German, Portuguese and Simplified Chinese glosses in
  `data/dictionary.json` derive from this data (Chinese as a display-only language, ADR-0017).
  They are embedded in the firmware through the generated `core/dictionary_gen.cpp` (ADR-0002), so
  this notice accompanies the firmware.
- License: **Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0)** —
  https://creativecommons.org/licenses/by-sa/4.0/. The dictionary data is redistributed under the
  same license (ADR-0016). CC BY-SA 4.0 is satisfied by citing the license by URI; its full text
  is not reproduced here, unlike the MIT/Adobe font notices above which require reproduction.
- Changes made (per the CC BY "indicate if changes were made" term): each single `definition`
  string was restructured into our per-language sense arrays (for Chinese, split on the fullwidth
  semicolon into senses, ADR-0017); a diacritic-folded search index was built from the searchable
  glosses (ADR-0002); and the data was subset to the 137 headwords of the UCSUR sitelen pona block
  (ADR-0003).

---

## Noto Sans CJK SC (dictionary CJK gloss font)

- Upstream: Noto Sans CJK SC, Version 2.004 (© 2014-2021 Adobe; packaged by Google as the Noto
  fonts). A codepoint **subset** is vendored at `third_party/noto-sans-cjk-subset/` (see its
  `PROVENANCE.md`); the full font is not.
- **Not** shipped or parsed on-device: like nasin nanpa (ADR-0003) the font is rasterized at build
  time (ADR-0018) into the 1-bpp cells in `core/cjk_gen.cpp`, which render the Simplified Chinese
  glosses. Those cells are a document created with the font — outside the OFL's copyleft on the
  Font Software — but the vendored subset is itself a Modified Version distributed under the OFL, so
  this notice accompanies it. Noto Sans CJK declares no Reserved Font Name.

Verbatim license (`third_party/noto-sans-cjk-subset/LICENSE`):

```
Copyright © 2014-2021 Adobe (http://www.adobe.com/).

-----------------------------------------------------------
SIL OPEN FONT LICENSE Version 1.1 - 26 February 2007
-----------------------------------------------------------

PREAMBLE

The goals of the Open Font License (OFL) are to stimulate worldwide
development of collaborative font projects, to support the font
creation efforts of academic and linguistic communities, and to provide
a free and open framework in which fonts may be shared and improved in
partnership with others.

The OFL allows the licensed fonts to be used, studied, modified and
redistributed freely as long as they are not sold by themselves. The
fonts, including any derivative works, can be bundled, embedded,
redistributed and/or sold with any software provided that any reserved
names are not used by derivative works. The fonts and derivatives,
however, cannot be released under any other type of license. The
requirement for fonts to remain under this license does not apply to
any document created using the fonts or their derivatives.

DEFINITIONS

"Font Software" refers to the set of files released by the Copyright
Holder(s) under this license and clearly marked as such. This may
include source files, build scripts and documentation.

"Reserved Font Name" refers to any names specified as such after the
copyright statement(s).

"Original Version" refers to the collection of Font Software components
as distributed by the Copyright Holder(s).

"Modified Version" refers to any derivative made by adding to,
deleting, or substituting -- in part or in whole -- any of the
components of the Original Version, by changing formats or by porting
the Font Software to a new environment.

"Author" refers to any designer, engineer, programmer, technical writer
or other person who contributed to the Font Software.

PERMISSION & CONDITIONS

Permission is hereby granted, free of charge, to any person obtaining a
copy of the Font Software, to use, study, copy, merge, embed, modify,
redistribute, and sell modified and unmodified copies of the Font
Software, subject to the following conditions:

1) Neither the Font Software nor any of its individual components, in
Original or Modified Versions, may be sold by itself.

2) Original or Modified Versions of the Font Software may be bundled,
redistributed and/or sold with any software, provided that each copy
contains the above copyright notice and this license. These can be
included either as stand-alone text files, human-readable headers or
in the appropriate machine-readable metadata fields within text or
binary files as long as those fields can be easily viewed by the user.

3) No Modified Version of the Font Software may use the Reserved Font
Name(s) unless explicit written permission is granted by the
corresponding Copyright Holder. This restriction only applies to the
primary font name as presented to the users.

4) The name(s) of the Copyright Holder(s) or the Author(s) of the Font
Software shall not be used to promote, endorse or advertise any
Modified Version, except to acknowledge the contribution(s) of the
Copyright Holder(s) and the Author(s) or with their explicit written
permission.

5) The Font Software, modified or unmodified, in part or in whole, must
be distributed entirely under this license, and must not be distributed
under any other license. The requirement for fonts to remain under this
license does not apply to any document created using the Font Software.

TERMINATION

This license becomes null and void if any of the above conditions are
not met.

DISCLAIMER

THE FONT SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
OF COPYRIGHT, PATENT, TRADEMARK, OR OTHER RIGHT. IN NO EVENT SHALL THE
COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
INCLUDING ANY GENERAL, SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL
DAMAGES, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF THE USE OR INABILITY TO USE THE FONT SOFTWARE OR FROM
OTHER DEALINGS IN THE FONT SOFTWARE.
```
