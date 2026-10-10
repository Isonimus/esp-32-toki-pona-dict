---
id: 0022
title: Public-release infrastructure and maintainer-only curation
status: accepted
date: 2026-10-10
---

# ADR-0022 — Public-release infrastructure and curation

On 2026-10-10, preparing the first push to a public remote
(`github.com/Isonimus/esp-32-toki-pona-dict`), we added the community/contributor infrastructure a
public repository needs and recorded what stays maintainer-only. This mirrors the sibling
project's decision (`../adversary` ADR-0012) but **diverges on two points its rationale does not
carry to a dictionary**: we do **not** squash history, and we add no web flasher yet.

## Context

The repo was developed privately and is now going public. Two sibling-project reflexes were
re-examined against this project's actual risk, not inherited wholesale:

- **History.** Adversary squashed 378 commits to one public root specifically to avoid a
  per-commit secret/PII audit of *offensive-security* history (real BSSIDs, GPS, captured
  credentials). This project is an offline dictionary: no secrets, no captured data, no PII ever
  enters a commit. Its 31-commit history is small, and the one defect that existed — commits made
  under the operator's work identity — was already removed by a full `git-filter-repo` rewrite to
  the opensource identity (ADR-0021 context) while nothing was pushed. The history is therefore a
  clean asset: each ADR shipped in the same commit as its code, which is exactly the provenance a
  reader wants. Squashing would discard it to solve a problem this project does not have.

- **Maintainer-only material ("Stele").** The operator's Claude Code operator framework — command
  definitions, hooks, and `*.mjs` machinery under `.claude/`, plus `.agent/` and
  `.stele-vendored.json` — is noise to an outside contributor and must never be committed. None of
  it is tracked here today, but a preventive `.gitignore` keeps it that way. The open-work
  `LEDGER.md` is likewise maintainer-only.

- **Public contact.** `SECURITY.md` and `CODE_OF_CONDUCT.md` need a reporting contact, and the
  operator's personal/work email must not appear on any public surface.

## Decision

- **No squash.** The existing history is pushed as-is (ADR-0021 already made it uniformly
  `Isonimus`, unpushed). The adversary squash was risk-driven; that risk is absent here.

- **Add the public infrastructure a contributor expects:**
  - `README.md` — the project's front door: badges, hardware/board-support table, build and flash
    instructions, feature list, and on-device screenshots from `docs/bringup/`. (The repo had
    none; this is the largest gap.)
  - `CONTRIBUTING.md` — how to build and run the host + device lanes, the commit/ADR conventions,
    and an explicit note that the ADR workflow's *tooling* (Stele) is maintainer-only: a
    contributor reads the `adr/` records as static history and need not run any of it.
  - `SECURITY.md`, `CODE_OF_CONDUCT.md` (Contributor Covenant) — contact is
    **GitHub [@Isonimus](https://github.com/Isonimus)** plus GitHub private vulnerability
    reporting; no personal email anywhere.
  - `.github/`: `PULL_REQUEST_TEMPLATE.md`, `ISSUE_TEMPLATE/` (bug, feature, config),
    `dependabot.yml` (the GitHub Actions, pip, and npm ecosystems), and `workflows/release.yml`.
  - `.editorconfig` — baseline whitespace/charset rules matching the existing code.

- **Add the CI stages the sibling project has and this repo lacks** (mirroring `../adversary`):
  - A **`cppcheck`** job in `ci.yml` — static analysis of the C/C++ sources we own, alongside the
    existing `-Wall -Wextra` host compile.
  - A **build-size** report in the device job, so a firmware-size regression is visible in the CI
    log rather than only on flashing.
  - A **CodeQL** workflow (`workflows/codeql.yml` + `codeql/codeql-config.yml`) — semantic security
    analysis of the firmware, surfaced as Code Scanning alerts. It runs on push/PR **and weekly on a
    schedule**, and (as in adversary) filters results under `.pio/`, `third_party/`, and `test/` —
    vendored code and host mocks we do not own — out of the SARIF before upload, since CodeQL
    analyzes everything compiled and `paths-ignore` is inert for compiled languages.

- **Releases are tag-driven.** `release.yml` fires only on a `v*` tag: it builds both board
  firmwares and attaches the `.bin` images to a GitHub Release. No release exists yet; the
  workflow simply makes cutting one a `git tag` away. `package.json` stays at `0.0.1` until the
  first tag.

- **Defer the web flasher.** A browser flash page (ESP Web Tools + GitHub Pages, as adversary
  has) is deferred until there is a tagged release with hosted binaries and the board line-up is
  proven on hardware (the original Cardputer variant is still a LEDGER item). Until then the
  README documents flashing via PlatformIO. Adding it later is a normal change, tracked in
  `LEDGER.md`.

- **Curate maintainer-only material out, keeping it on disk:**

  | Published (tracked) | Kept local (gitignored, not deleted) |
  | --- | --- |
  | `core/ hal/ app/ main/ tools/ test/`, `platformio.ini`, `package.json` | `.claude/` (Claude Code operator config, hooks, Stele machinery) |
  | `adr/` (decision records, as static artifacts) | `.agent/`, `.stele-vendored.json` |
  | `data/`, `third_party/`, `THIRD_PARTY_NOTICES.md` | `LEDGER.md` (open-work list) |
  | `README CONTRIBUTING SECURITY CODE_OF_CONDUCT LICENSE CLAUDE.md` | `.venv/`, `scratchpad/`, build output |
  | `.github/`, `.editorconfig`, `.gitignore` | |

  `CLAUDE.md` is **published**: it documents the project's conventions and helps a contributor,
  and it is project doc, not Stele kit. `LEDGER.md` is **untracked** (`git rm --cached`) and
  gitignored: it is a maintainer worklist, and a public roadmap, if wanted, belongs on GitHub
  Issues — a separate decision.

## Consequences

- The repo gains a front door and the standard contributor surfaces. `ci.yml` gains the `cppcheck`
  job and the build-size report; a separate `codeql.yml` adds scheduled security scanning. The
  existing host + device jobs remain the backstop.
- `LEDGER.md` leaves version control but stays on disk, so the maintainer keeps using it; its
  history up to this commit remains in past commits (harmless — it holds no secrets), since we did
  not squash. Opening a public roadmap later is a normal commit, not a second decision to undo
  this one.
- The `.gitignore` now blocks the Stele kit pre-emptively, so an accidental `git add .` cannot
  publish operator config.
- No personal email is on any public surface; the single contact is the GitHub handle.
- Publishing currently-excluded material later (the web flasher, a tracked roadmap) is an ordinary
  change. Nothing here is irreversible.
