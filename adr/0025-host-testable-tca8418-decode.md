---
id: 0025
title: Host-testable ADV FIFO decode alongside the shared keymap
status: proposed
date: 2026-10-11
amends: ADR-0011 (the ADV-specific raw-byte decode also joins the host lane)
---

# ADR-0025 — Host-testable ADV FIFO decode alongside the shared keymap

ADR-0011 deliberately left the TCA8418 byte-to-cell arithmetic inside the device driver.
Issue #4 identifies the remaining test gap: that arithmetic is pure even though its encoding
is specific to the ADV keyboard controller.

## Decision

Extract `decodeTca8418Event(rawEvent)` to a small, header-only pure HAL sibling. It returns
the silkscreen row, column and edge, which the device shell passes to the existing `applyKey`.
The variant-independent keymap boundary stays unchanged: a future original-Cardputer scanner
can still feed grid cells without depending on a TCA8418 byte format.

Keep the existing arithmetic, including the controller's ten-column numbering stride (TI
TCA8418 datasheet SCPS215G, sections 8.3.1.1 and 8.3.1.3). Key number zero returns an invalid
cell; `applyKey` emits None and preserves fn state, matching the driver's previous skip.
No new filtering of other raw values, I2C operations, FIFO handling or key policy is introduced.

## Consequences

The existing keymap host test now covers all 56 enabled positions on both edges using an
explicit raw-number grid, plus zero-key handling and a raw fn/navigation/letter sequence.
Changing the stride from ten to eight must fail the position checks. The new boundary is
host-visible without Arduino, Wire, or a new build dependency.

This is an extraction of device behavior, so the existing ADV bring-up evidence still
describes the input path. A host run and firmware link verify the extraction; they do not
claim a new physical-device test.
