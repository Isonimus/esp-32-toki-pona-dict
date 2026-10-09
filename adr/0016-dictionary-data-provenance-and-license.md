---
id: 0016
title: Dictionary data provenance and license (lipu Linku, CC BY-SA 4.0)
status: accepted
date: 2026-10-09
---

# ADR-0016 — Dictionary data provenance and license (lipu Linku, CC BY-SA 4.0)

On 2026-10-09 we recorded that every gloss in the dictionary derives from **lipu Linku**'s
`sona` data, which is licensed **CC BY-SA 4.0**, and we declared the dictionary *data* —
`data/*.json` and the generated `core/dictionary_gen.*` that mechanically transforms it —
licensed under CC BY-SA 4.0 with attribution, because ShareAlike requires it and the
obligation had gone unrecorded since the first glosses were added.

## Context

`data/dictionary.json` is the single source of truth (ADR-0002). Its `_comment` documented
only the **word list** — "nimi pu (120) + nimi ku suli (17)" — never where the *glosses*
(en/es/fr/de/pt) came from or under what terms. There is no `LICENSE` file in the repo at
all, and `THIRD_PARTY_NOTICES.md` credited only the two fonts (ADR-0003, ADR-0012), not the
gloss data.

Adding Chinese glosses from the same source (ADR-0017) forced the question, but it was never
specific to Chinese: the operator confirmed the existing five languages' glosses were **also
taken from lipu Linku**. So the whole dictionary is, and always has been, an adaptation of
Linku's multilingual data — the attribution and ShareAlike obligations already applied and
were simply never written down. This ADR corrects that omission and puts the data's licensing
on a correct, declared footing before the dictionary grows further.

The upstream license was verified from the horse's mouth — `lipu-linku/sona`'s `LICENSE`
blob: **Creative Commons Attribution-ShareAlike 4.0 International**. We take it at face value
and comply in full rather than argue our use under a copyright threshold (CC BY-SA 4.0
explicitly reaches sui-generis database rights, so a "definitions are too short to protect"
argument would be exactly the corner-cut this project forbids).

## Decision

- **Provenance, recorded.** All non-Toki-Pona glosses in `data/dictionary.json` derive from
  lipu Linku's `sona` data (`https://github.com/lipu-linku/sona`, `translations.*.definition`).
  The Toki Pona headwords themselves are the language, not Linku's authorship.

- **License of the data, declared.** The dictionary **data** is licensed **CC BY-SA 4.0**.
  That covers `data/dictionary.json`, `data/languages.json`, and the mechanically-generated
  `core/dictionary_gen.cpp` and `core/normalization_fixtures_gen.h` that are pure transforms
  of it. `data/dictionary.json` carries machine-readable `_source` and `_license` fields next
  to its `_comment`, so the provenance travels with the single source of truth and cannot
  drift from a notice kept elsewhere.

- **Attribution (the BY obligation).** `THIRD_PARTY_NOTICES.md` gains a lipu Linku section —
  source, URL, the CC BY-SA 4.0 identifier and link, and an explicit statement of the changes
  we make (below). This file ships with the firmware and is the legal discharge (enforced by
  `tools/check-notices.mjs`, as for the fonts). The About colophon (ADR-0009) gains a
  condensed courtesy credit for the gloss data, on the same principle ADR-0003 set for the
  font: the on-device view is a human-readable courtesy credit, not the verbatim legal text.

- **Changes we make (stated, per BY "indicate if changes were made").** We restructure
  Linku's single `definition` string into our per-language sense arrays; we build a
  diacritic-folded search index from the glosses (ADR-0002); we subset the data to the 137
  headwords of our glyph block (ADR-0003); and for Chinese we split the definition on the
  fullwidth semicolon into senses (ADR-0017).

- **Scope: data only.** The firmware **code** (`core/` logic, `hal/`, `app/`, `main/`) is not
  a derivative of the gloss data — CC BY-SA's ShareAlike reaches the adapted *data*, not a
  program that merely reads it (BY-SA is not a copyleft software license like the GPL). The
  code's own licensing remains undeclared and is explicitly **out of scope** here, tracked as
  a separate follow-up in `LEDGER.md`.

## Consequences

- The repository is now explicitly **mixed-license**: CC BY-SA 4.0 data plus separately
  (not-yet-)licensed code. That split is common and legitimate, but only if declared — which
  this ADR and the `_license` field now do. Nothing about the code changes.

- Distributing the firmware binary embeds the generated data, so the binary distribution
  carries the BY obligation; `THIRD_PARTY_NOTICES.md` ships with the product and the public
  repository provides the BY-SA source data, together satisfying BY (attribution travels with
  the binary) and SA (the adapted data is available under the same license).

- `tools/check-notices.mjs` is extended to assert the lipu Linku notice is present, so a
  future edit that drops the attribution fails `npm run test:host` rather than shipping a
  silent license violation — the same fail-loud guard the fonts already have.

- This is a no-behavior-change commit: notices, a data-license declaration, and one condensed
  About line. It stands on its own and would be correct even if Chinese were never added; it
  is sequenced first for exactly that reason.

- We are not lawyers; this records a good-faith, face-value compliance posture with a
  permissive-to-us copyleft-data license, not legal advice. If a rights question is ever
  raised, the posture to defend is "we attributed, we shared the adapted data alike, and we
  said what we changed" — all of which this decision puts in place.
