---
id: 0009
title: Boot splash and an About colophon reached by Back from empty search
status: accepted
date: 2026-10-08
supersedes: partially ADR-0004 (the two-view set; empty-query Back is inert)
---

# ADR-0009 — Boot splash and an About colophon reached by Back from empty search

Two UX additions long parked in the LEDGER: a brief sitelen pona wordmark on boot, and an
on-device About/credits view for the nasin nanpa font (ADR-0003). This decides how both fit the
existing pure state machine (ADR-0004) and its input vocabulary, which has no "menu" verb.

## Context

- **The on-device notice is a courtesy, not a legal must.** The MIT obligation for the nasin
  nanpa font is already discharged by `THIRD_PARTY_NOTICES.md`, which ships with the firmware
  and is enforced against the vendored license by `tools/check-notices.mjs` in `npm run
  test:host` (ADR-0003). An on-device credit is a nicety that also closes the LEDGER item — it
  does not have to reproduce the verbatim legal text.
- **The input vocabulary has no menu verb.** ADR-0004 deliberately kept the abstract events to
  `Char / Nav / Select / Back` and rejected hidden modes. A new "open About" verb would be
  exactly the kind of discoverable-only mode ADR-0004 argued against, and the smallest boards
  barely have a keyboard.
- **The controller is the correctness oracle, and it is clockless.** ADR-0004 made the state
  machine pure and host-tested, with rendering a separate per-board bring-up proof. It has no
  concept of time; anything timed must come from `main/`, which owns `millis()`.
- ADR-0004 made `Back` on an empty query **inert** ("Search is the root; nothing to delete and
  no view to leave"). That is a currently-wasted event at exactly the place a colophon belongs.

## Decision

- **The view set grows to four: `Splash`, `Search`, `Detail`, `About`.** `Splash` is the initial
  view; `Search` remains the steady-state root.
- **The splash is dismissed by the first input of any kind, or by a timeout `main/` drives.**
  The controller handles the transition (`Splash` + any non-`None` event → `Search`, and a
  public `dismissSplash()` for the timed path); `main/` owns the clock and calls
  `dismissSplash()` once the hold elapses. The dismissing keypress is **swallowed**, not typed,
  so mashing a key during the splash cannot seed the query. `None` leaves the splash up, so a
  bare poll never skips it. This keeps the transition host-tested; only the duration is
  device-only (a bring-up concern, like all timing).
- **About is reached by `Back` from the *empty* search view.** This repurposes the event
  ADR-0004 left inert: "nothing behind the root → show the colophon." No new input verb, and no
  dictionary-data pollution (a colophon is not a headword, so it does not belong in
  `dictionary.json`). `Back` from About returns to `Search`; `Back` with a non-empty query is
  still backspace, unchanged. Everything else in About is inert — it is read-only.
- **About shows a condensed colophon, one screen, no scroll.** The "toki pona" sitelen wordmark,
  then the font credit (nasin nanpa, `(c) 2022 jan Itan`, MIT), then a pointer to
  `THIRD_PARTY_NOTICES.md`. The **verbatim** MIT permission text is deliberately *not* shown
  on-device: the bundled file already carries it and discharges the obligation (ADR-0003), so a
  ~20-line scroll engine would add state, clamping, and clipped rendering for no legal gain.
- **The splash shows the same wordmark** — the product's name in its own script — plus the nasin
  nanpa credit line, foreshadowing the colophon.

## Consequences

- Splash and About transitions are asserted in the host UI test like every other transition
  (ADR-0004 ethos); the only device-only, bring-up-proven piece is the splash *duration*.
- The LEDGER on-device-notice item is closed in its condensed form; the legally-operative
  verbatim text stays in the enforced, shipped file.
- The colophon credit strings are hand-kept in the renderer, duplicating the attribution in
  `THIRD_PARTY_NOTICES.md`. Acceptable for a short human-readable credit (building a generator
  for one line would be over-engineering); the drift-protected copy is the file, not the screen.
- A board that cannot type (future button/encoder board, ADR-0005/LEDGER) still reaches About
  with a single `Back` from the empty root — the gesture needs no keyboard.
- Needs a bring-up proof update: splash on boot, and `Back` from the empty search view opening
  About. Tracked in the LEDGER. Capturing the splash frame (ADR-0006) must happen within the
  hold window, since it auto-dismisses.
