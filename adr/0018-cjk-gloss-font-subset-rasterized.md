---
id: 0018
title: Subset-rasterized CJK gloss font (14px, 1-bpp)
status: accepted
date: 2026-10-09
---

# ADR-0018 — Subset-rasterized CJK gloss font (14px, 1-bpp)

On 2026-10-09 we chose to render Chinese glosses (ADR-0017) with a build-time
**subset-rasterized** bitmap font: the exact 865 codepoints our `zh` glosses use, rasterized
from Noto Sans CJK SC at **14×14 1-bpp** into a codepoint-keyed table of **~31 KB**, extending
the sitelen pipeline (ADR-0003) rather than shipping a whole CJK font — because our gloss text
is a tiny, closed subset and a full font would cost 8×–500× the flash for glyphs we never use.

## Context

The device text path (ADR-0012) covers only U+0020–U+017F, and its coverage guard rejects CJK
outright. Chinese needs CJK glyphs. A whole CJK face is 250 KB (a GB2312 u8g2 bitmap font) to
16 MB (Noto Sans CJK OTF). But our `zh` glosses use only **865 unique codepoints** — Simplified
Hanzi, a handful of CJK punctuation marks, and the ASCII that appears in glosses and in the
detail-cycle label. CJK is square and monospaced, so a fixed cell is the natural, correct layout, unlike
proportional Latin — which makes this exactly the sitelen pattern (ADR-0003): build-time
rasterization to fixed-size 1-bpp bitmaps, keyed for lookup, blitted with `drawGlyph1bpp`.

## Decision

- **Extend `tools/rasterize_sitelen.py`** to emit a second, codepoint-keyed glyph set
  (`core/cjk_gen.cpp`, interface `core/cjk.hpp`): every codepoint appearing in the `zh` glosses,
  plus the detail-label charset (`<`, `>`, `/`, space, `0`–`9`), rasterized at **14×14 px,
  1-bpp, threshold 128** — the same format and threshold as sitelen. Keyed by a sorted,
  binary-searchable codepoint array; all codepoints are in the BMP.

- **Font source: Noto Sans CJK SC (OFL).** To keep the repo lean and the build reproducible, a
  **157 KB subset TTF** carrying only our codepoints (GSUB/GPOS/hinting dropped) is vendored at
  `third_party/noto-sans-cjk-subset/`; the multi-MB full font is *not* vendored. The subset is
  a build input only — like nasin nanpa (ADR-0003) it is never shipped to or parsed on the
  device. The rasterized bitmaps are a derivative work, so `THIRD_PARTY_NOTICES.md` carries the
  verbatim OFL notice and `PROVENANCE.md` records how to regenerate the subset.

- **Flash cost (measured, not estimated):** 865 glyphs × 28 B = 23.7 KB of bitmaps + a ~7 KB
  codepoint→cell lookup table (`CjkGlyph[]`, the same struct-with-pointer shape as `SitelenGlyph`)
  = **~31 KB**. For comparison, a GB2312 u8g2 font is ~250–400 KB and the full OTF ~16 MB. The
  subset rasterized with **0 glyphs missing** from the SC face.

- **Cell size: 14px (measured legibility vs flash).** The 865 glyphs were rasterized at 12/14/16
  px and inspected at the device's 1-bpp threshold. At 12px the dense glyphs (激, 墨, 感) muddy as
  strokes merge; at 14px they are legible though tight and match the body-text scale; 16px is
  clearest at +~4 KB. 14px is the balance. As with ADR-0003's size probe, **on-device bring-up is
  the final arbiter**: if the real panel proves 14px too dense, 16px is the recorded fallback and
  only the rasterizer's size constant changes.

- **Rendering a display-only language.** `app/render.cpp` draws a display-only language's detail
  label *and* gloss lines entirely through fixed 14px cells via `drawGlyph1bpp`: a small UTF-8
  decoder walks each string and blits one cell per codepoint. This keeps every Chinese character
  in one glyph set — no inline Latin/CJK font mixing, and no text-width measurement (which
  ADR-0014 deliberately removed) — and stays host-assertable by glyph count. The ASCII that
  appears in `zh` content is rasterized into the same cell set, so mixed lines render uniformly
  (a space is an empty cell). Searchable languages are unchanged: they still render via
  `drawText` and the Latin font.

- **Coverage guard updated.** `tools/check-font-coverage.mjs` allows CJK codepoints in a
  non-searchable language's glosses, checked against the *rasterized CJK set* (the real
  coverage), so a gloss edit adding a Hanzi we did not rasterize still fails `test:host` loud
  rather than rendering a blank cell on-device.

## Consequences

- **~31 KB** of flash for the CJK glyph set; **+157 KB** vendored build input that never enters
  the firmware.

- The rasterizer now emits three generated artifacts — `sitelen_gen`, `logo_gen`, `cjk_gen`; the
  `check:generated` drift guard covers all three, and `package.json` compiles `cjk_gen.cpp` into
  the host render tests.

- Fixed-cell layout makes ASCII inside Chinese lines (digits, parens, the cycle label) render a
  touch wide; acceptable on a dictionary panel, and bring-up will confirm.

- If the `zh` gloss set ever changes, the vendored subset must be regenerated from the full Noto
  font (steps in `PROVENANCE.md`). The gloss set is the closed pu/ku corpus, so this is rare.

- A device bring-up proof (a Chinese detail frame) is pending, tracked in `LEDGER.md`; this ADR
  will take an amendment if 14px is revised at bring-up.
