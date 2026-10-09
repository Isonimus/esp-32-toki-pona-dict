---
id: 0013
title: Sitelen glyph size enlarged to 44x44
status: accepted
date: 2026-10-09
supersedes: 0003 (the "glyph bitmap size measured: 32x32" amendment only)
---

# ADR-0013 — Sitelen glyph size enlarged to 44×44

The sitelen pona glyph size moves from **32×32 to 44×44**, re-rasterized from the vendored
nasin nanpa font at the larger cell. This supersedes only the size amendment of ADR-0003
("glyph bitmap size measured: 32×32"); everything else that ADR decided — build-time
rasterization, the nasin nanpa font, the 1-bpp row-major format, and the antialias-and-
threshold pipeline — stands unchanged.

## Why the old size was reconsidered

ADR-0003's size probe optimized one axis: **legibility at the lightest flash**. It found 32px
makes every glyph distinct (even the densest in the whole script) and explicitly judged 40/48px
to add "sharpness this closed, geometric glyph set does not need." That reasoning was correct
*for legibility* and is not in dispute — 32px is perfectly legible.

What it did not weigh is **visual presence**. In the detail view the sitelen is the one piece
of the native script on screen, an accent beside the headword, and at 32px on the 240×135 panel
it reads as a small mark rather than a deliberate element. The operator called this out directly
at the ADR-0012 bring-up ("the sitelen can be a bit bigger"). Two premises behind the old fit
argument have also shifted since:

- ADR-0003 sized the glyph "beside the **scale-2** headword". ADR-0012 replaced that blocky 2×
  headword with a native `helvB14`, so the glyph now sits beside a crisper, more refined
  headword that a 32px mark visually under-balances.
- ADR-0003 worried a large glyph "would eat a third of the panel height". It does occupy ~1/3
  of the 135px height (44/135 ≈ 33%), but it is blitted **top-right** while the glosses are
  short, left-aligned text — so the space it uses is otherwise empty and there is no collision
  (confirmed on-panel, frame 07).

So this is a genuine mind-change on the size tradeoff — hence a new ADR, not an edit to 0003.

## Decision

- **44×44, re-rasterized** (not an upscale). `WIDTH`/`HEIGHT` in `tools/rasterize_sitelen.py`
  and `kSitelenWidth`/`kSitelenHeight` in `core/sitelen.hpp` go to 44; `core/sitelen_gen.cpp`
  is regenerated so each glyph is rendered from the nasin nanpa **vector** at the larger em and
  thresholded afresh. The result is crisp at 44px, not a pixel-doubled 32px bitmap.
- **Global, not detail-only.** The alternative — keep the 32×32 table and scale only the
  detail-view blit by ~1.4× — was rejected: pixel-doubling a thresholded 1-bpp bitmap is
  exactly the blocky artifact ADR-0012 removed from the headword. Re-rastering the vector is
  the root-cause fix. Because the layout constants key off `kSitelenWidth`/`kSitelenHeight`
  (`app/render.cpp`), every view that draws a glyph — the detail accent, the splash/About
  wordmark, and the empty-root prompt row — re-flows from the one size change. One size, every
  view, crisp everywhere (KISS).

### Why 44 specifically (measured)

Candidate sizes, with the full 137-glyph table cost (1-bpp, byte-aligned `stride = ceil(w/8)`):

| Size | stride | bytes/glyph | table | vs 32×32 |
|---|---|---|---|---|
| 32×32 (old) | 4 | 128 | 17.5 KB | — |
| 40×40 | 5 | 200 | 26.8 KB | +9.6 KB |
| **44×44** | **6** | **264** | **35.3 KB** | **+18.2 KB** |
| 48×48 | 7 | 336 | 45.0 KB | +28.5 KB |

44×44 is a clear, legible step up (+37.5% linear) that still leaves the layouts uncrowded — the
width-constrained view is the empty-root prompt's 4-glyph row at `4·44 + 3·8 = 200px`, inside
the 240px panel. 48px reaches 216px and spends 1.5× the extra flash for a size the panel does
not need; 40px is a smaller bump for little saving. Flash is cheap here (16% used of 3.3 MB), so
the choice is driven by on-panel legibility, not the budget.

### Keeping it from regressing

The size is guarded on three sides, all fail-loud:

- `core/sitelen_gen.cpp` carries a `static_assert(kSitelenWidth == 44 && kSitelenHeight == 44)`;
  changing the header without regenerating fails the build.
- `tools/rasterize_sitelen.py`'s `pack_glyph` raises if any glyph's ink exceeds the cell, so a
  regenerate *is* the clipping check — at 44×44 all 137 glyphs rasterize inside the cell.
- `test/test_sitelen.cpp` asserts the dimensions, stride, and byte count (`npm run test:host`).

On-device proof: `docs/bringup/cardputer-adv/07-detail-sitelen-44.png` shows the enlarged accent
in the detail view, rendered crisply from the just-flashed firmware.

## Consequences

- Firmware flash: **516,099 → 534,747 bytes (15.4% → 16.0%), +18,648 B (~18.2 KiB)**, matching
  the +18.2 KB table growth; RAM flat at 7.2%. Measured on the production (`tp_cardputer`) build.
- The detail-view glyph reads as an intentional script accent beside the native headword.
- The splash/About wordmark and the empty-root prompt glyphs also grow, re-flowing from the one
  size constant — a uniformly larger, more present sitelen across every view, which reads better,
  not just in the detail view. (Their own bring-up frames remain the LEDGER item that already
  tracks the splash/About/empty-prompt views.)
- No data, API, or seam change: `drawGlyph1bpp` already takes width/height arguments, and the
  `sitelen` field and lookup are untouched. A future size change is again just the two constants
  plus a regenerate.
