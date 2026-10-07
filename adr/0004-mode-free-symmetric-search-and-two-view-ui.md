---
id: 0004
title: Mode-free symmetric search and a two-view UI state machine
status: accepted
date: 2026-10-07
---

# ADR-0004 — Mode-free symmetric search and a two-view UI state machine

On 2026-10-07 we chose that the app searches **every** language index on each query and
merges the hits, rather than making the user pick a query language, because the product is
a *fully symmetric* dictionary (either side as the query) and the input vocabulary
(ADR-0001) has no "switch language" verb — so a mode would need a verb we deliberately
don't have, while merging needs none.

## Context

The core exposes `search(Lang, query, …)` over one language's sorted index (ADR-0002). The
user chose fully symmetric lookup: typing `telo` must find the entry, and so must typing
`water`, `agua`, `wasser`, … The abstract `InputEvent` set is `Char / Nav / Select / Back`
(ADR-0001) — there is no key for cycling a "current language". On the smallest reference
boards there is barely a keyboard, so adding a hidden mode the user must discover and
toggle is the worst fit for the hardware.

## Decision

- **Search is mode-free and symmetric.** On each query change the app calls `core::search`
  once per language in `languages.json` cycle order and merges the results into one fixed
  buffer, skipping entry ids already present (dedup by id). There is no "current query
  language"; the user types in whatever language they know.
- **Result rows are keyed on the Toki Pona headword.** A merged result is an entry, and the
  list shows its `tp` headword — the canonical pivot of the dictionary — so a row means the
  same thing no matter which language matched it. No per-row "display language" mode.
- **Two views, one state machine (`app/`).** `Search` holds the typed query, the merged
  result ids, and the selected row; `Detail` shows one entry's glosses in all five
  languages plus its sitelen glyph. `Char` edits the query, `Nav` moves the selection,
  `Select` opens the focused row into `Detail`, `Back` deletes a character (or, with an
  empty query, is inert) in `Search` and returns to `Search` from `Detail`. One UI serves
  keyboard and button/encoder boards because it consumes only the abstract events.
- **The state machine is pure and host-tested.** It depends on `core` only, holds its
  buffers by value (no heap), and is driven event-by-event, so navigation, search
  triggering, and view transitions are asserted natively. Turning that state into pixels is
  a separate `render(state, DisplaySurface&)` step whose correctness is a per-board
  bring-up proof (ADR-0001), not a host assertion.

## Consequences

- The user never manages a mode; the cost is N binary searches per keystroke instead of
  one. N is the language count (6 including Toki Pona) over ~2018 total index rows, so a
  query change is a few thousand comparisons — trivial, and off the render path.
- Merged results past the fixed buffer are truncated, same display-limit contract as
  `core::search` — the user narrows the query. The buffer fills in cycle order, so an early
  language can crowd out a later one on a very broad prefix; acceptable because a one- or
  two-letter prefix is not a lookup, and it tightens immediately as the user types.
- A future complex-script language (ADR-0001 leaves the set open) joins the merge for free:
  it is another index to search, no UI change.
