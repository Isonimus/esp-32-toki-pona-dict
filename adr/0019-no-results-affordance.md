---
id: 0019
title: No-results affordance on the search view
status: accepted
date: 2026-10-09
amends: ADR-0010 (adds the no-results render state that decision deferred; renderer-only, no state or event change)
---

# ADR-0019 — No-results affordance on the search view

On 2026-10-09 we decided that a non-empty query matching nothing draws a passive "nothing found"
affordance — the `ala` ("none / not") sitelen glyph above the sitelen sentence `nimi li weka`
("the word is absent"), both in a new muted grey — in the result area, instead of the bare empty
list ADR-0010 left there. This adds the palette's first grey, its first colour beyond the black
background, white body, and accent yellow.

## Context

ADR-0010 gave the *empty* search root a teaching prompt and explicitly left a non-empty query that
finds nothing showing "an empty list, not the prompt ... it is not a 'no results' message." That
left a dead state: type a word with no match and the result area goes blank, with no signal that
the search ran and returned nothing — indistinguishable at a glance from a render glitch. The
empty-root prompt (ADR-0010/0014) and this are two different states — empty query vs.
non-empty-but-no-match — and must read differently, so this is a new affordance, not a reuse of the
prompt.

sitelen pona glyphs rasterize on-device for free (ADR-0003), and the whole UCSUR block is in the
table (every headword, not just the dictionary's), so `ala`, `nimi`, `li`, and `weka` are already
present — the affordance needs no new asset. The only new primitive is a colour.

## Decision

- **A non-empty query with zero results draws the no-results affordance** in the result area: the
  `ala` glyph centred above the sitelen sentence `nimi li weka`, both muted grey. The `>` query
  line is unchanged and stays at the top, so the user still sees the text that matched nothing. An
  *empty* query still draws the ADR-0010 prompt; a query *with* results still draws the list. The
  branch is `resultCount == 0` taken after the query line — the one fact that separates the three
  cases.

- **Hero `ala`, sentence without a second `ala`.** The hero carries the negation symbol; the
  sentence states the subject's absence with `weka` (absent / gone), not `lon ala` (not present). A
  sentence ending in `ala` would stack a second identical X-glyph directly under the hero and read
  like a duplicated-render bug; `weka` keeps the hero the sole, unambiguous "nothing" mark while the
  sentence adds "the word is absent." This mirrors ADR-0010's icon-and-sentence split, where the
  `lukin` icon does not recur in `o alasa e nimi`.

- **New palette entry `kColorMuted = 0x8410`** (RGB565, ~50% grey), the first colour beyond the
  three fixed ones. Grey signals "de-emphasised / nothing here" without competing with the accent.
  The exact shade is a render-legibility call like the glyph size (ADR-0013) and the CJK cell size
  (ADR-0018): a one-constant value the pending bring-up proof is the final arbiter of.

- **Renderer-only, reusing `drawGlyphRowCentered`.** No `View`, `UiState`, or `InputEvent` change —
  the no-results case is derived from the existing `resultCount`, so the state machine is untouched.
  The hero and sentence draw through the same centring helper the prompt and wordmark use (the
  rule-of-three reuse from ADR-0010), with the colour argument it already takes.

## Consequences

- The host test asserts the discriminating fact: a non-empty query with no match now blits the four
  affordance glyphs (hero + three-word sentence) and the one query-line text, where before it blit
  only the query line; a query with results still blits zero glyphs, and the empty root still blits
  its five prompt glyphs. Colour and centring are not host-assertable and ride the bring-up proof.

- A bring-up proof (a no-match frame) is a LEDGER follow-up, where the grey shade and the 44px glyph
  density in the result area get the on-panel check; the grey constant is the only thing that
  changes if it reads too dark or too light.

- The palette is no longer three fixed colours. Any future "muted / secondary" element should reuse
  `kColorMuted` rather than inventing another grey.
