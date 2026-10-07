---
id: 0002
title: Dictionary data format, build-time generation, and the normalization contract
status: accepted
date: 2026-10-07
---

# ADR-0002 — Dictionary data format, build-time generation, and the normalization contract

On 2026-10-07 we chose a single JSON source of truth compiled to sorted C++ index tables
at build time, with one shared diacritic-folding contract, because it makes lookup a
zero-heap binary search on-device while keeping the linguistic data reviewable, and it
removes the only subtle correctness risk (query and index folding disagreeing).

## Context

Lookup must be symmetric: Toki Pona → each language, and each language → Toki Pona. Users
type on a small keyboard and will omit accents ("agua" for "água"), so matching must be
diacritic-insensitive while display stays correctly spelled.

## Decision

- **Source of truth:** `data/dictionary.json`, one record per Toki Pona headword, with a
  gloss array per language in **display spelling** (diacritics intact). `sitelen` is the
  glyph id and defaults to the headword. `data/languages.json` declares the language set
  and the UI's cycle order.
- **Build-time generation:** `tools/generate.mjs` emits `core/dictionary_gen.cpp` — the
  `Entry` table plus one `IndexRow[]` per language, each **sorted by the folded key** in
  byte order. On-device lookup is therefore `lower_bound` + a prefix scan: O(log n), no
  heap, instant startup. Nothing is parsed on the device.
- **Generated files are committed but never hand-edited.** They carry a `DO NOT EDIT`
  banner. CI regenerates and fails on any diff, so the device build needs no Node (the
  diff check is a `LEDGER` item until the CI lane exists).
- **The normalization contract (the one shared invariant):** fold to lowercase, strip
  Latin diacritics to the base letter, expand `ß`→`ss` / `æ`→`ae` / `œ`→`oe`, keep only
  `[a-z0-9 space ' -]`, collapse spaces, trim. It is implemented **twice** — in
  `core/normalize.cpp` (device query path) and in `tools/generate.mjs` (index build) —
  because one runs in C++ on-device and the other in Node at build time. The two MUST
  agree: a query folded one way cannot match an index folded the other. The canonical
  cases live in `data/normalization-fixtures.json`; the generator asserts them before
  emitting, and the host test asserts the same generated fixtures against the C++ side.
  Divergence fails the build.

## Consequences

- Adding or editing a word is a one-file JSON edit plus a regenerate; glosses stay
  human-readable and diffable in review.
- The folding rule is duplicated by necessity (two languages, two runtimes). The
  duplication is made safe by a shared fixture asserted on both sides — the dangerous
  version (an untested second implementation) is explicitly avoided.
- `uint16` entry ids cap the corpus at 65535 entries; the generator fails loud above it.
  Far beyond Toki Pona's ~137 words.
