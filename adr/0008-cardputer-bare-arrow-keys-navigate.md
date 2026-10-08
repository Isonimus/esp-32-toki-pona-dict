---
id: 0008
title: Cardputer bare arrow keys navigate (no fn)
status: accepted
date: 2026-10-08
supersedes: partially ADR-0005 (Cardputer keyboard -> InputEvent mapping)
---

# ADR-0008 — Cardputer bare arrow keys navigate (no fn)

ADR-0005 mapped the Cardputer's arrow layer behind `fn`: `fn+;` = Nav Up, `fn+.` = Nav Down,
with bare `;`/`.` typing their punctuation. On-device use of the detail-view language cycle
(ADR-0007) showed the two-key combo is awkward for what is the view's primary action, so this
amends that one row of the mapping.

## Context

Measured on the ADV during bring-up (ADR-0006): typing a query and then pressing `.` to move
down produced the query `to.` — the `.` was typed, not acted on — and the search *still*
returned the same results, because normalization folds `;` `.` (and other punctuation) out of
every query before it reaches the index. So:

- The punctuation those keys type is **never usable** in a search term; they are dead as
  character keys.
- Requiring `fn` for navigation makes the detail language cycle (ADR-0007) and result-list
  movement two-key gestures for no benefit.

## Decision

In the Cardputer keyboard driver, **the arrow-labelled keys navigate whether or not `fn` is
held**: bare `;` → Nav Up, bare `.` → Nav Down. `fn+;`/`fn+.` still navigate (redundant,
harmless). The consequence — `;` and `.` can no longer be typed — is acceptable because
normalization already discards them from queries.

This stays in the HAL driver, not the app: the `InputSource` emits abstract `InputEvent`s and
must not know which view is active (ADR-0001). So bare arrows navigate in **both** views — the
result list and the detail language cycle — consistently, from one key.

`fn` is still tracked as a modifier for `fn+esc`/`fn+delete` → Back; only the Up/Down rows move
out from behind it. The rest of ADR-0005's mapping (printable → Char, enter → Select,
backspace → Back) is unchanged.

## Consequences

- Single-key navigation everywhere; the detail language cycle is one keypress.
- `;` and `.` are not typeable — no loss for a dictionary whose queries fold them away.
- The change is one function (`mapPress`) in the device-only driver. Per ADR-0005 that driver is
  excluded from the host lane, so its acceptance is the on-device walk-through, not a host test
  (a follow-up to extract `mapPress` into a pure, host-testable unit is in the LEDGER).
