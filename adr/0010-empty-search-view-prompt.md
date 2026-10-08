---
id: 0010
title: Sitelen type-to-search prompt on the empty search view
status: accepted
date: 2026-10-08
amends: ADR-0004 (the empty Search view's presentation; no state or event change)
---

# ADR-0010 — Sitelen type-to-search prompt on the empty search view

On boot (after the splash, ADR-0009) the app lands on the search root with an empty query.
Until now that view showed a bare `>` on an otherwise black screen — nothing tells a new user
that this is a dictionary they type into, nor that either side of the language pair is a valid
query (the symmetric lookup of ADR-0004). This adds a small on-screen prompt so the empty view
teaches its own use.

## Context

The empty search root is the first thing a user sees once the splash clears. The input
vocabulary has no "help" or "menu" verb (ADR-0001/0004), and we are not adding one: the prompt
must be passive on-screen copy, not a reachable view. sitelen pona glyphs already rasterize
on-device for free (ADR-0003), so a sitelen sentence costs no new asset and no Latin-diacritic
font (which the gloss text still lacks — a separate LEDGER item). The only real question was the
"look/search" icon: a new bitmap asset, or reuse of a glyph already in the font.

## Decision

- **The empty search root (query length zero) draws a passive prompt** in the result area: a
  "look/search" icon above a one-line sitelen sentence. The `>` edit line is unchanged. The
  prompt is shown **only while the query is empty**; the first keystroke replaces it with the
  result list, and a non-empty query that happens to find nothing shows an empty list, not the
  prompt (the prompt teaches the *empty root*, it is not a "no results" message).
- **The icon reuses the bundled `lukin` ("see/look") glyph** — no new bitmap asset and no
  `generate.mjs` change. A dedicated search icon would buy nothing a learner reads better, at the
  cost of a hand-maintained asset outside the font pipeline (ADR-0003).
- **The sentence is `o alasa e nimi`** ("search for a word"): `alasa` = hunt/seek/search names
  the action, so it complements the `lukin` icon rather than restating it. All five glyphs
  (`lukin o alasa e nimi`) are in the bundled font.
- **This is a renderer-only change.** No `View`, `UiState`, or `InputEvent` changes; the state
  machine is untouched, so nothing new is host-asserted beyond the renderer driving the surface.
  The prompt copy and glyph list are named constants in `render.cpp`, beside the ADR-0009
  colophon strings.

## Consequences

- A glyph the font lacks is skipped but still reserves its slot (the ADR-0003 degradation
  contract), so the row stays aligned rather than crashing; on the bundled font all five render.
- The host test asserts the discriminating fact it can: the empty root now blits the prompt
  glyphs (icon + sentence) while a non-empty search still blits none. Visual layout (centering,
  spacing) is not host-assertable and rides the ADR-0001 bring-up proof.
- The centering math for a row of glyphs now serves three callers (the ADR-0009 wordmark, the
  prompt icon, the prompt sentence), so it is extracted into one `drawGlyphRowCentered` helper
  — the rule-of-three reuse, not speculative generality.
- Bring-up proof for this frame is a LEDGER follow-up: the empty-root prompt is a render concern
  and cannot be eyeballed by the host lane.
