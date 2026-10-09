---
id: 0020
title: Browse-the-whole-corpus mode and cyclic list navigation for keyboard-less boards
status: accepted
date: 2026-10-09
amends: ADR-0004 (adds the empty-query browse state it left as "no results", and changes the result-list Nav from clamped to wrapping); introduces the first board capability ADR-0005 deferred
---

# ADR-0020 — Browse mode and cyclic list Nav for keyboard-less boards

On 2026-10-09, bringing up the **M5StickC Plus2** (the keyboard-less 3-button board ADR-0005
deferred), we decided that a board which cannot type reaches a word by **browsing the whole
corpus** from the empty search root, and that the result/browse list is a **ring** (`Nav`
wraps) rather than clamped. Which of the two the empty root shows — the ADR-0010 teaching
prompt or the browse list — is chosen by a new, injected **board input-style capability**, the
first capability branch ADR-0005 said it would add "if and when the branching does."

## Context

ADR-0004 made "cleared query ⇒ no results" the rule and clamped the result list's `Nav`.
ADR-0005 then established that a keyboard board reaches every word against the unchanged app
(type to search), but a keyboard-less board cannot reach a *single* word: with no query there
are no results, so `Nav`/`Select` have nothing to act on. The StickC Plus2 is that board, and
its recorded control scheme (ADR-0005) is **three-button cyclic** — BtnA = `Nav` Down, BtnB =
`Select`, PWR = `Back`. Two facts about that scheme force this decision:

- **There is no `Nav` Up and no `Char`.** The only list motion is Down. With a clamped list a
  user who passes their target can never return to it — the list must **wrap** so Down alone
  traverses the whole ring. ADR-0004's clamp was justified as "predictable on a keyboard's
  arrows and an encoder's detents"; that holds for a two-direction control but strands a
  one-button board. The detail-language selector already wraps for every board (ADR-0007), so
  a wrapping list is the established idiom, not a new one.

- **The empty root must do different things on different boards.** On a keyboard board the
  ADR-0010 prompt teaches *type to search* — correct there, but actively wrong on a board with
  no keyboard, where the right empty-root behaviour is to browse. This is a genuine per-board
  difference in app behaviour, which is exactly the capability branch ADR-0005 anticipated and
  deferred. It is introduced here, minimally — one injected enum, not the sapper's capability
  struct, which stays speculative until a *second* capability (not just input style) needs it.

The corpus is 2272 rows across the merged indices but 137 headwords (measured, this build); the
137-entry browse list still cannot fit the 32-id result buffer (ADR-0004) and must not need a
second buffer of its own.

## Decision

- **Board input style is an injected capability.** `UiController` is constructed with an
  `InputStyle` — `Keyboard` (type-to-search) or `Buttons` (browse) — stamped into `UiState` so
  the pure renderer can read it. `main/` passes the board's style; the default is `Keyboard`,
  so every existing board and test is unchanged. This is one enum field, host-tested on both
  values; no capability *struct* (KISS, ADR-0005's deferral still holds for anything beyond
  input style).

- **An empty query on a `Buttons` board browses the whole corpus.** The Search view's list is
  **derived**, not a second buffer: with a typed query it is the merged `results` (ADR-0004);
  with an empty query on a `Buttons` board it is every entry in table order, where a row's index
  *is* its entry id; with an empty query on a `Keyboard` board it is empty and the ADR-0010/0014
  prompt is drawn instead. `listCount(state)` and `entryIdAt(state, row)` express this in one
  place, shared by the controller and the renderer. `Select` opens `entryIdAt(selected)`;
  `Back` on the empty root still opens About (ADR-0009), which on a `Buttons` board is Back's
  only target (there is nothing to delete) and returns to the browse root.

- **The list is a ring: `Nav` wraps, superseding ADR-0004's clamp.** `Nav` Down past the last
  row returns to the first and Up past the first goes to the last, over `listCount` rows, for
  **all** boards — mandatory for the one-button board, harmless for a two-arrow board, and
  uniform with the detail-language ring (ADR-0007). Wrapping no-ops on an empty list.

- **No `View` or `InputEvent` change.** Browse is still the `Search` view driven by the
  existing `Nav`/`Select`/`Back`; `Char` simply never arrives on a `Buttons` board. The state
  machine stays pure and host-tested.

## Consequences

- The renderer **windows** the list to keep the focused row visible: a browse list of 137 rows
  (and, as it happens, any result set taller than the panel) cannot draw top-to-bottom. The
  scroll policy — centre the focused row, clamped at the list ends — is a render/bring-up
  concern, eyeballed on the panel, not host-asserted; this also bounds the previously unbounded
  result-list draw, which silently drew rows past the panel bottom (boyscout).

- The Cardputer's result-list `Nav` now wraps where it clamped before. This is a behaviour
  change to a brought-up board, re-checked at the StickC bring-up on the shared renderer; it is
  minor and arguably better (the detail ring on that same board already wraps).

- Host tests assert the discriminating facts: a `Buttons` controller on the empty root lists
  every entry, Down from the last row wraps to the first and Up from the first wraps to the
  last, and `Select` opens the focused entry id; a `Keyboard` controller on the empty root lists
  nothing (the prompt path) and still finds and opens words by typing. Colour, scroll window and
  panel are the StickC bring-up proof (ADR-0005/0006), tracked in `LEDGER.md`.

- The StickC Plus2 HAL (ST7789 135×240 panel class, the 3-button `InputSource`, the GPIO4
  power-hold, and its `platformio.ini` env) is a pure HAL/`main` addition under this app change;
  its pins and offsets come from M5GFX's `board_M5StickCPlus2` and are verified at bring-up, the
  same contract ADR-0005 set for the Cardputer.

- A third board that browses (an encoder board) reuses `InputStyle::Buttons` unchanged. The
  capability stays a single enum until a capability *other than* input style forces the struct.
