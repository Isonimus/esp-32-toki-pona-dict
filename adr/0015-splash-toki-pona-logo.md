---
id: 0015
title: Font-composed toki pona logo on the splash; font credit dropped from it
status: accepted
date: 2026-10-09
supersedes: ADR-0009 (splash presentation — the two-glyph wordmark + font credit)
extends: ADR-0003 (adds one more build-time-rasterized asset from the same font)
---

# ADR-0015 — Font-composed toki pona logo on the splash; font credit dropped from it

The boot splash (ADR-0009) drew the name "toki pona" as its two sitelen glyphs side by side, with
the `nasin nanpa` font credit beneath. The operator raised two problems:

1. The official toki pona logo is not two separate glyphs — it is the single merged mark: `toki`
   (a ring with three strokes above it) with the `pona` smile **nested inside** the ring. Two
   glyphs side by side is the word written out, not the logo.
2. Writing the font's name (`nasin nanpa`) on the opening screen makes the font too prominent for
   what is primarily a *Toki Pona* dictionary — the splash should lead with the product's own mark.

## Decision

- **The splash draws the merged logo as one centered hero**, replacing the two-glyph wordmark.
- **The `nasin nanpa` credit is removed from the splash.** This supersedes ADR-0009's splash
  presentation. The credit is not lost: the About colophon still shows `sitelen pona: nasin nanpa`
  (ADR-0009), and the legally-operative verbatim MIT notice still ships in `THIRD_PARTY_NOTICES.md`
  (enforced by `tools/check-notices.mjs`). The splash credit was always a courtesy, not the legal
  discharge, so dropping it is not a licensing regression — confirmed against the LEDGER note on
  where the obligation is actually met.

### The logo is composed from the vendored font, not an embedded bitmap

The merged logo could have been shipped as the official logo image. It is **not**, for two reasons:

- **The official logo artwork is its own IP**, separate from the MIT-licensed font. The font is
  already vendored and license-cleared (ADR-0003); reusing it avoids introducing a second, more
  encumbered asset for one screen.
- **nasin nanpa composes the exact logo itself.** sitelen pona has a *scaling* joiner (U+F1996)
  whose GSUB shaping nests the following glyph inside the preceding one. The three-codepoint
  sequence `toki` + joiner + `pona` (`U+F196C U+F1996 U+F1954`) shapes to precisely the logo: the
  ring with three strokes, the smile nested inside. So the mark is derived from the same font as
  every other glyph — one more rasterized output under ADR-0003's existing font-dependency
  analysis, not a new dependency.

On a 1-bpp single-color display every glyph is a monochrome silhouette anyway (`drawGlyph1bpp`
takes one foreground color), so an embedded two-tone logo would flatten to the same silhouette the
font composition already produces — the bitmap would buy nothing and cost the IP question.

### How it is built and stored

- `tools/rasterize_sitelen.py` gains a logo step: it shapes the three-codepoint sequence through
  the font with Pillow's **raqm** layout engine (required — the default layout applies no GSUB and
  would lay the three glyphs out side by side), then packs the antialiased mask to 1-bpp at a 50%
  threshold, the same pipeline as the sitelen glyphs. The shared `pack_mask` primitive packs both.
- The logo is **not** a `SITELEN_GLYPHS` entry: that table is keyed by headword and fixed at the
  44×44 cell (ADR-0013). The logo is a standalone asset at its own **tight bounding box**, emitted
  to `core/logo_gen.cpp` and exposed as a single `const Logo TP_LOGO { width, height, bitmap }`
  through the hand-written `core/logo.hpp`. Carrying its own dimensions means there is no fixed
  size constant to desync, and `drawGlyph1bpp` already derives stride from the passed width, so the
  device driver blits a non-44-wide bitmap with no change.
- **Size, measured not guessed:** rendered at em 88 the logo's ink bounding box is 88×83 px — a
  hero exactly 2× the 44px sitelen cell height, centered on the 240×135 panel with ~26px of margin
  above and below. Candidate sizes (ink bbox): em 72 → 72×68, em 80 → 80×76, em 88 → 88×83,
  em 96 → 96×91. 88 reads as a deliberate brand hero without crowding the panel. Cost: 913 bytes of
  flash (stride 11 × 83 rows), negligible beside the 35.3 KB glyph table.

### Keeping it from regressing

Layout is not host-assertable (ADR-0001), but the draw-call shape is: the splash now issues exactly
**one** `drawGlyph1bpp` (the logo) and **zero** `drawText` (`test/test_ui.cpp` asserts
`glyphs == 1 && texts == 0`, where it was `glyphs == 2 && texts >= 1` for the old wordmark +
credit). That check fails on the pre-change renderer and passes after. The generated `logo_gen.cpp`
is covered by the existing `--check` drift guard (ADR-0003), extended to both generated files.
On-device proof: `docs/bringup/cardputer-adv/08-splash.png`.

## Consequences

- One new build-time asset and one small generated translation unit; no new runtime seam, view, or
  state. The `textWidth` seam method that an earlier draft of the splash rework added (to center a
  text credit) is **not** introduced — the logo centers on its own width/height and the splash
  draws no text, so the measurement primitive had no remaining caller (KISS).
- The About colophon keeps its two-glyph wordmark (ADR-0009): it is a credit screen, not a brand
  moment, and was out of scope. The splash and About therefore differ deliberately — the splash
  leads with the mark, About spells the name above its credits.
- The logo composition depends on the font's scaling joiner and GSUB tables; if a future font
  revision changes them, a regenerate would change the bitmap (and the `--check` guard would catch
  an un-regenerated drift), the same contract as every sitelen glyph.
