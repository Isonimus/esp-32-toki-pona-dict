---
id: 0017
title: Chinese as a display-only language (no search index)
status: accepted
date: 2026-10-09
---

# ADR-0017 — Chinese as a display-only language (no search index)

On 2026-10-09 we added Simplified Chinese (`zh`) as the sixth gloss language, as a
**display-only** language: it renders in the detail view but contributes no rows to the
search index. The Cardputer keyboard emits only Latin keystrokes and has no IME, so Chinese
cannot be typed to search; but the primary use of the dictionary — Toki Pona → the reader's
language — is the *display* direction, which works fully, so the one-way limitation is an
accepted trade, not a defect.

## Context

**Reach.** The firmware's main distribution channel is M5Burner, whose user base skews
Chinese-reading, and none of the existing five languages (en/es/fr/de/pt) serve them.
lipu Linku covers Simplified Chinese for all 137 of our headwords (verified), so the data
exists and is already our source (ADR-0016).

**The keyboard constraint is architectural, not cosmetic.** Search is symmetric today
(ADR-0004) only because every language romanizes into the ASCII the keyboard produces, and
ADR-0002's normalization folds diacritics onto that same ASCII. There is nothing to fold
Hanzi onto, and no IME to type it. So Chinese is reachable one way only: `telo` → 水, never
水 → `telo`.

**Why that is acceptable.** The primary use of a pocket Toki Pona dictionary is "I am
reading/learning Toki Pona — what does this word mean in my language?", i.e. TP → reader's
language, which is exactly the display direction. A Chinese reader also keeps two typeable
entry points — the Toki Pona headword itself and English — so they are never locked out of
search; they simply cannot search *in* Chinese. The asymmetry degrades a secondary feature
(gloss-language → TP composition lookup), not the primary one.

## Decision

- **Add `zh` to the language set** in `data/languages.json`: native label `中文`, placed
  after `pt` in the query/display order, so the detail cycle (ADR-0007) runs `1/6`…`6/6`
  over en…zh.

- **Introduce a display-only language attribute.** A language carries `"searchable": false`
  in `data/languages.json` to mark that it renders but is excluded from the generated search
  index. `zh` is the first and, for now, only instance. This amends ADR-0002 (the index now
  covers only searchable languages) and ADR-0004 (mode-free symmetric lookup holds for
  searchable languages; a display-only language is find-forward-only).

- **The generator honours it.** `tools/generate.mjs` emits no index rows for a non-searchable
  language; its glosses still ship in the per-entry tables for display. The folding contract
  (ADR-0002) is untouched — it simply never sees the Chinese strings.

- **Sense-split rule for zh.** Linku stores one `definition` string per word, with `；`
  (U+FF1B, fullwidth semicolon) between major senses and `、` (U+3001, fullwidth enumeration
  comma) between near-synonyms *within* a sense. We split on `；` only and keep `、` intact,
  stripping surrounding whitespace, so `moku`'s `吃、喝、进食、咽下、摄入；食物、可食用物品`
  becomes `["吃、喝、进食、咽下、摄入", "食物、可食用物品"]`. Splitting on `、` would wrongly
  shred a single "liquid, e.g. water/petrol/…" sense into fake senses.

- **Accepted limitation, recorded.** `zh` is find-forward-only and never searchable. This is
  permanent on keyboard boards without an IME. A future keyboard-less *browse* mode (tracked
  in `LEDGER.md`) would let a display-only language be reached by browsing rather than typing;
  until then, the limitation stands and is not a bug to be "fixed".

## Consequences

- The search index gains **zero** rows from `zh` (the five searchable languages' row count is
  unchanged); the per-entry tables gain the Chinese glosses for display only.

- The detail view now cycles six languages; the label reads `< 中文  6/6 >`. Rendering Chinese
  needs CJK glyphs, which the Latin text path (ADR-0012) cannot draw — the font and the
  detail-view rendering of a display-only language are **ADR-0018**. This ADR is only about the
  language-class semantics.

- The data stays CC BY-SA 4.0 (ADR-0016); the `zh` glosses come from the same lipu Linku
  source, so `THIRD_PARTY_NOTICES.md` and the `_license` "changes" note are extended to name
  Chinese and the `；`-split.

- A host test asserts the display-only contract directly: a language marked non-searchable
  yields no search hit for its gloss text, while its glosses remain present on the entry for
  display.
