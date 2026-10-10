---
id: 0021
title: Firmware code license (MIT), kept distinct from the CC BY-SA 4.0 data
status: accepted
date: 2026-10-10
---

# ADR-0021 — Firmware code license (MIT)

On 2026-10-10 we licensed the **firmware code** of this project under the **MIT License** and
added a top-level `LICENSE`, completing the data/code split ADR-0016 opened: the dictionary
*data* is CC BY-SA 4.0, the *code* is MIT, and the two never merge.

## Context

ADR-0016 declared the dictionary **data** CC BY-SA 4.0 and already recorded that "the firmware
code is separately licensed" — but never named the code's license. The repo therefore shipped
code (`core/` logic, `hal/`, `app/`, `main/`, `tools/`, `test/`) with **no declared license at
all**, which under default copyright means "all rights reserved": nobody — including the stated
goal of one firmware reused across many boards — may legally reuse it. This ADR closes that gap.

The choice is genuinely free, because the data's ShareAlike does not reach the code. CC BY-SA
4.0's copyleft binds **Adapted Material** — derivatives of the licensed glosses. Our engine is
not an adaptation of Linku's glosses; it is a separate work of authorship merely *combined* with
the data when the generated tables are compiled in. So the code may carry its own license, and
keeping that license distinct is exactly the separation ADR-0016 asked for.

Every component the firmware combines is permissive-compatible, so nothing is forced on the
code (verified against `THIRD_PARTY_NOTICES.md` and the vendored/linked sources):

| Component | License | Reaches firmware by |
|---|---|---|
| Dictionary data, `core/dictionary_gen.*` | CC BY-SA 4.0 | generated from `data/` (ADR-0002, ADR-0016) |
| nasin nanpa (sitelen pona) | MIT | rasterized → `core/sitelen_gen.cpp` (ADR-0003) |
| Noto Sans CJK subset | OFL 1.1 | rasterized → `core/cjk_gen.cpp` (ADR-0018) |
| u8g2 Adobe Helvetica | Adobe permissive | embedded (ADR-0012) |
| LovyanGFX | FreeBSD/BSD-2 | linked library |

MIT was chosen over the alternatives weighed: **Apache-2.0** (adds a patent grant and NOTICE
mechanism we have no present need for — speculative generality), **GPLv3** (copyleft runs
against the reusable one-firmware-many-boards ethos, imposing friction on anyone building on the
HAL), and **BSD-2-Clause** (MIT-equivalent but less conventional here). MIT is the simplest
permissive license, is compatible with every dependency above, lets the HAL and engine be
reused freely, and matches the sibling project this repo follows, `../esp32-wifi-sapper` (also
MIT) — one less licensing model for a reader who knows that project.

## Decision

- **The firmware code is licensed MIT.** A top-level `LICENSE` carries the MIT text, copyright
  "2026 Isonimus".

- **The `LICENSE` states its scope**, so the carve-out is visible to anyone who opens the file,
  not only to a reader of this ADR. MIT covers the code we authored: `core/` (except the
  generated files), `hal/`, `app/`, `main/`, `tools/`, `test/`. It explicitly does **not** cover,
  each keeping its own terms: the dictionary data and `core/dictionary_gen.*` (CC BY-SA 4.0); the
  build-time-rasterized asset files `core/sitelen_gen.cpp`, `core/cjk_gen.cpp`, `core/logo_gen.cpp`
  and the embedded fonts (their upstream MIT/OFL/Adobe licenses); and linked libraries.

- **`package.json` gains `"license": "MIT"`**, the SPDX identifier for the code and build tooling
  the npm manifest describes — the machine-readable half of the same declaration.

- **The data's self-declaration is left authoritative and made to point here.** The `_license`
  field in `data/dictionary.json` already says the data is CC BY-SA 4.0 and the code is separate;
  its pointer is updated from "see adr/0016" to name this ADR, so the data file and the `LICENSE`
  agree on where the code's terms are written.

## Consequences

- The repo is now fully licensed: no file is left under implicit "all rights reserved". The reuse
  ADR-0001 designed for — a new board is just a HAL driver + a platformio env, built on the shared
  core and app — is now legally permitted, not merely technically possible.

- The data/code boundary is stated in three places that must agree: this ADR, the `LICENSE`
  scope header, and `data/dictionary.json`'s `_license` field. They are cross-referenced so a
  future reader finds the same split from any entry point; `THIRD_PARTY_NOTICES.md` remains the
  discharge for the third-party obligations and is unchanged by this decision.

- No source-file license headers were added. For a single-author repo with one top-level
  `LICENSE` and a scope note, per-file SPDX tags would be noise; if the project later takes
  outside contributions or splits a reusable library out, adding `SPDX-License-Identifier` headers
  becomes worthwhile and gets its own ADR then — not pre-emptively now.
