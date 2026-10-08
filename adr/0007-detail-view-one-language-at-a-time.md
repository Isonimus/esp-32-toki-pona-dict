---
id: 0007
title: Detail view shows one language at a time, cycled with Nav
status: accepted
date: 2026-10-08
supersedes: partially ADR-0004 (detail view layout)
---

# ADR-0007 — Detail view: one language at a time, cycled with Nav

During the Cardputer ADV bring-up (ADR-0005/0006), the detail view — which ADR-0004 specified
as *all five languages' glosses at once* — proved illegible on the real 240×135 panel. This
decision replaces that part of ADR-0004.

## Context

ADR-0004 fixed the detail view as the TP headword, its sitelen glyph, and one line per
language (`English: to talk, speak, language, hello`). The first on-device capture
(`build/bringup/`, ADR-0006) showed why that does not survive contact with the hardware:

- A joined gloss line runs to ~40 characters. At the built-in 6px font that exceeds the 240px
  width, and LovyanGFX's `print()` wraps at the right edge **at the font height**, not at the
  layout's line height. The overflow of one language lands on top of the next language's line.
- Raising `kLineHeight` cannot fix it: the wrap inserts its own sub-line regardless. The only
  ways to make five languages fit are to truncate each line (which drops glosses — unacceptable
  for a dictionary, its glosses *are* the product) or to scroll (machinery, and still one
  cramped line at a time while scrolling).

The measured conclusion: five multi-sense gloss lines cannot be shown legibly on this panel.

## Decision

1. **The detail view shows exactly one language at a time**, given the full panel width. Layout:
   the TP headword (large) and sitelen glyph as before; a label line `< English  1/5 >` that
   both names the language and signals it is selectable with a position in the cycle; then the
   glosses **one sense per line**, indented. One sense per line is short enough never to
   overflow the width, which is the whole defect being fixed — no truncation, no wrapping.

2. **`Nav` (Up/Down) cycles the language; it wraps.** The detail view already received `Nav` and
   discarded it (it was a leaf that honoured only `Back`), so this adds no new `InputEvent` kind
   and no new key — it gives an already-delivered input a meaning. Wrapping (unlike the result
   list's clamped `Nav`, ADR-0004) because a language selector is a ring with no first or last.
   Only the non-Toki-Pona languages cycle — Tp is the headword shown as the title — i.e. the
   contiguous enum range `[En, Count)` in the cycle order ADR-0002 already defines.

3. **Opening an entry always starts on the first cycle language** (English today). Predictable
   and plumbing-free. "Open on the language that matched the query" was considered and rejected
   for now: a result can match several languages, and a TP-headword match has no non-TP match to
   land on, so it needs matched-language tracking and a fallback rule — deferred to the LEDGER,
   not built speculatively.

`Back` still returns to the result list; `Char`/`Select` remain inert in detail.

## Consequences

- Every language is fully legible; nothing is truncated or overlaps. Seeing all languages at a
  glance is traded away, which is the right trade on a screen that cannot render them legibly
  anyway.
- `handle()`'s detail branch and `renderDetail()` change; `ui_state` gains one `detailLang`
  field. All host-tested (`test/test_ui.cpp`: opens on En, cycles and wraps both ways, resets on
  re-open). The per-board render itself remains a bring-up proof (ADR-0005/0006).
- ADR-0004 stands except for the detail view's "all languages at once" layout, which this
  supersedes. The mode-free symmetric search and the two-view (list + detail) structure are
  unchanged.
