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
