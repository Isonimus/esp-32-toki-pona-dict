---
id: 0003
title: Sitelen pona as build-time rasterized bitmaps
status: accepted
date: 2026-10-07
---

# ADR-0003 — Sitelen pona as build-time rasterized bitmaps

On 2026-10-07 we chose to render each sitelen pona glyph as a monochrome bitmap
rasterized from a sitelen pona font at build time and stored in flash, rather than
shipping a font + text-shaping engine to the device, because Toki Pona's glyph set is
small and closed, which makes the simplest approach also the cheapest.

## Context

Sitelen pona is the logographic script for Toki Pona: one glyph per word. Unlike CJK it is
a **closed set** of ~137 glyphs that maps 1:1 onto our headwords. Fonts exist (UCSUR /
Private-Use-Area mapped, e.g. linja pona). Rendering a PUA font on-device would require a
font engine aware of those codepoints on every board's display stack.

## Decision

- Each `Entry` carries a `sitelen` glyph id (defaulting to the headword).
- At build time, a rasterizer (`tools/`, to be written) renders each distinct glyph from a
  sitelen pona font to a fixed-size 1-bpp bitmap (target 48×48; the exact size is a probe,
  recorded here when measured) and emits a `core/sitelen_gen.*` table keyed by glyph id.
  At 48×48 1-bpp, ~137 glyphs ≈ 40 KB of flash.
- The display HAL blits the bitmap next to the Latin text in the entry-detail view. No PUA
  handling, no font engine, and no text shaping runs on the device.

## Consequences

- The glyph set is fixed at build time, which is correct: sitelen pona is a closed script,
  so there is nothing to shape or compose at runtime.
- The build gains a dependency on a sitelen pona font file with a known license. Sourcing
  that font and writing the rasterizer is a separable slice, tracked in `LEDGER.md`; the
  data model already reserves the `sitelen` field so no data migration is needed when it
  lands.
- The glyph bitmap size is a measured decision (legibility vs. flash), not an estimate;
  the probe result will be appended here as an amendment.

## Amendment 2026-10-07 — font selected: nasin nanpa (MIT)

Sourcing (the slice this ADR deferred to `LEDGER.md`) resolved to **nasin nanpa** by jan
Itan, vendored at `third_party/nasin-nanpa/` (release n4.0.2, UCSUR variant). Provenance and
reproduction steps live beside the file.

Alternatives weighed: linja pona (OFL, geometric/monospace, the canonical face) and Fairfax
HD (OFL, native bitmap — no FreeType needed). nasin nanpa won on breadth of coverage (nimi
ku suli, not just nimi pu) and the lightest license. The operator considered shipping two
fonts; rejected for now because both render the *same closed glyph set* (the difference is
purely typeface), and an on-device switch would need an input verb the mode-free UI
(ADR-0004) deliberately lacks. The rasterizer is written font-parameterized so adding or
swapping a font is a build-input change, not a code change — a second set, if ever wanted,
is its own decision.

License: **MIT**, Copyright (c) 2022 jan Itan (verified from the repository's LICENSE blob).
Note a correction to pre-selection research, which had reported nasin nanpa as CC0: it is
not. MIT still permits rasterized derivatives in firmware and is lighter than OFL (no
reserved-name rule); its one obligation — shipping the copyright + permission notice with the
product — is tracked in `LEDGER.md`.

Enumeration: the rasterizer iterates the UCSUR sitelen pona block **U+F1900–U+F1988 (137
glyphs)**, confirmed present via `fc-query`, keyed by the codepoint derived from each
headword. No parallel name→glyph table, preserving ADR-0002's single source of truth.

The glyph **bitmap size** is still an open probe (legibility vs. flash); its result will be
appended as a further amendment when the rasterizer measures it.

## Amendment 2026-10-07 — glyph bitmap size measured: 32×32 (17.1 KB)

The size probe (deferred above) is resolved to **32×32 1-bpp**, superseding the 48×48/40 KB
*estimate* in the Decision. Measured, not guessed: the vendored nasin nanpa glyphs were
rasterized at 24/32/40/48 px and inspected at each size against flash cost.

- Flash for the full 137-glyph set: 24px ≈ 9.6 KB, **32px ≈ 17.1 KB**, 40px ≈ 26.8 KB,
  48px ≈ 38.5 KB (1 bpp, row-major, rows byte-aligned: `stride = ceil(w/8)`).
- Legibility: at 32px every glyph is distinct, including the densest in the *whole* script
  (`kijetesantakalu`, `kokosila`, `misikeke` — the stress cases, beyond our current nimi pu
  corpus). At 24px `kijetesantakalu`'s legs/tail blur together. 40/48px add sharpness this
  closed, geometric glyph set does not need, at 1.6×–2.3× the flash.
- Fit: on the ~240×135 reference panel a 32px glyph is a detail-view accent beside the
  scale-2 headword, leaving room for the gloss lines; a 48px glyph would eat a third of the
  panel height.

A **rasterization-quality** finding also informs the pipeline: a hard 1-bpp rasterizer
(FreeType/Pillow `mode='1'`) shreds the thin curved strokes into scanline artifacts at these
sizes. The rasterizer therefore renders **antialiased grayscale and thresholds at 50%**,
which keeps curves intact — recorded here because it is a correctness constraint on the
tool, not a free choice.

Pipeline note: the rasterizer derives each glyph's codepoint from the font's own cmap
(glyph name `<headword>Tok` → PUA codepoint), so the headword→glyph mapping lives in the
font, not in a hand-maintained table (ADR-0002 single-source upheld). Implemented in Python
+ Pillow (FreeType-backed), not Node: binary font rasterization is Pillow's native job and
it ships FreeType, whereas the Node toolchain has no first-party rasterizer. The two
generators stay decoupled — `generate.mjs` still emits the `sitelen` *string* per entry;
`rasterize_sitelen.py` emits the string→bitmap table — so neither needs to know the other's
internals.
