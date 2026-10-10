---
id: 0023
title: Promote select LEDGER items to GitHub Issues (promote, don't mirror)
status: accepted
date: 2026-10-10
---

# ADR-0023 — Promote select LEDGER items to GitHub Issues

On 2026-10-10, with the first public push (ADR-0022) to
`github.com/Isonimus/esp-32-toki-pona-dict`, we open a public contributor surface by promoting
a curated subset of `LEDGER.md` to GitHub Issues. This is the "separate decision" ADR-0022
foresaw when it kept `LEDGER.md` maintainer-only ("a public roadmap, if wanted, belongs on
GitHub Issues — a separate decision"). It adopts the sibling project's promote-don't-mirror rule
(`../adversary` ADR-0022) and records one deliberate divergence from it.

## Context

`LEDGER.md` is the single internal worklist and is now untracked/gitignored (ADR-0022), so a
contributor cannot see it. A public repo needs a visible, legible surface for the work that
genuinely wants an outside contributor. Copying the ledger wholesale into Issues would create
two trackers for one item and the manual-sync rot that follows. The resolution, from adversary
ADR-0022, is to change which surface *owns* an item — never to keep it alive in both.

## Decision

- **Adopt promote-don't-mirror.** Issues are a public, external-contributor surface, not a
  mirror of the ledger. `LEDGER.md` stays the default home; most items never become issues. When
  an item is promoted it **moves**: its ledger line is removed as the issue is filed, so each
  item lives in exactly one tracker. Because `LEDGER.md` is untracked, "removed in the same
  change" means the on-disk line is deleted when the issue is created — there is no ledger commit
  to pair it with.

- **Promotion criteria (all three must hold):** legible to an outsider without deep project
  context; self-contained and bounded; genuinely wants an outside contributor. Label
  `good first issue` only when it is additionally small and teaches the codebase.

- **Promote two items now:**

  | LEDGER item | Label | Why |
  | --- | --- | --- |
  | Host-test the TCA8418 7×8→4×14 decode (`drainTca8418`) | `good first issue` | Host-only, no hardware; self-contained — extract the pure mapping to a host-tested boundary and add a fail-before/pass-after regression test feeding `applyKey`; teaches the core/hal/test split. The ledger's "do it alongside the 74HC138 scan" is an efficiency grouping, not a dependency, so it stands alone. |
  | Original Cardputer 74HC138 matrix scan + bring-up proof | `help wanted` + `hardware` | A bounded, well-specified implementation task (decode specified in ADR-0005), but it needs the physical original-variant board. Not a first issue. |

- **Diverge from adversary on the hardware-gated item.** Adversary ADR-0022 keeps hardware-gated
  on-device items internal, on the ground that as public issues they "rot or mislead a newcomer."
  We promote the 74HC138 item anyway, because its gating hardware — an *original* M5Cardputer — is
  exactly what an outside owner is likely to have and the maintainer may not: this item's best
  contributor is precisely someone the private ledger cannot reach. We mitigate the "mislead a
  newcomer" risk the adversary rule guards against by labelling it `help wanted` + `hardware`
  (never `good first issue`) and stating the board requirement in the issue body. The
  justification is recorded here rather than taken as a silent exception.

- **Do not promote:**
  - The web flasher — blocked on a tagged release with hosted binaries and a hardware-proven
    board line-up (ADR-0022); not yet actionable, so it stays in the ledger until unblocked.
  - Any future on-device verification, design deferral, or deep-context item — these stay in the
    ledger per the adopted criteria.

- **One-way at promotion.** A closed or declined issue does not return to the ledger; its outcome
  is the PR / git log. An internal follow-up discovered while working an issue is a new ledger
  line, not a copy-back of the issue.

## Consequences

- There are now two open-work surfaces — `LEDGER.md` (private) and Issues (public) — but never the
  same item in both, so single-ownership-per-item holds.
- The ledger's "open a public roadmap/known-issues surface" item is satisfied by this ADR and its
  curated issues; that line is removed.
- Issues are filed only after the first push (the remote must exist); sequencing is
  push → file the two issues (removing each promoted ledger line as it is filed) → the issues
  carry the work.
- Promoting more items later is ordinary grooming under these criteria. Reintroducing same-item
  mirroring across surfaces would require a new ADR superseding this one.
