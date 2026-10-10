---
id: 0024
title: Release-driven CHANGELOG, written in the version-bump commit
status: accepted
date: 2026-10-10
---

# ADR-0024 — Release-driven CHANGELOG

With the first tagged release cut (v0.1.0, ADR-0022) and firmware heading to the M5Burner
catalog, the project gains a `CHANGELOG.md`: a curated, in-repo, offline "what changed per
version" surface. ADR-0022 enumerated the public-release infra it added and did not include a
changelog, so adding one is a new decision, recorded here.

## Context

Two audiences ask "what changed in this version": a catalog user who flashed a `.bin` and wants
to know what a later build adds, and a contributor reading the repo offline. The GitHub Release
page already carries auto-generated notes (`generate_release_notes: true` in `release.yml`), but
those are a per-PR/commit dump, only exist on GitHub, and are not curated. A hand-maintained
changelog answers both audiences in-repo — but a second, free-form history is exactly the kind of
artifact that drifts from git and from the releases it claims to describe. The same manual-sync
rot ADR-0023 refused for LEDGER↔Issues applies here.

## Decision

- **Keep a `CHANGELOG.md`** in [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) format,
  versioned by [SemVer](https://semver.org/), matching the firmware version in `package.json`.

- **Release-driven, not a running diary.** The changelog is not edited per PR. It carries one
  curated, human-written entry per released tag — highlights a user or contributor cares about,
  not a commit log. An `## [Unreleased]` section at the top accumulates nothing by default; notable
  changes may be parked there between releases, but the authoritative act is the per-release entry.

- **Written in the version-bump commit.** Cutting a release is already a ritual: bump
  `package.json`, commit `chore(release): X.Y.Z`, tag `vX.Y.Z`, push the tag (which fires
  `release.yml`). The changelog entry for `X.Y.Z` is written **in that same bump commit** — the
  `[Unreleased]` heading is renamed to `[X.Y.Z] - <date>` and a fresh empty `[Unreleased]` opened
  above it. Folding it into the one commit that already exists per release is what stops drift:
  there is no separate chore to forget.

- **Complements, does not replace, the GitHub auto-notes.** `release.yml` keeps
  `generate_release_notes: true`; the auto-notes remain the exhaustive per-PR record on the Release
  page, the `CHANGELOG.md` is the curated in-repo summary. Wiring the release body to read from the
  changelog instead is a possible later change, not part of this decision.

- **The v0.1.0 entry is retroactive.** v0.1.0 was tagged (commit `ecbed08`) before this ADR, so its
  entry cannot have ridden its bump commit. It is seeded here as the changelog's first entry, in the
  same commit as this ADR. The write-in-the-bump-commit rule binds from the next release onward.

## Consequences

- One more file to touch at release time, but inside the existing bump commit — no new standalone
  step, and the diff-review of that commit now also reviews the release's public summary.
- A reader has one offline, curated version history; a catalog user comparing two firmware builds
  has a human summary without opening GitHub.
- The release checklist (currently implicit) now has a changelog step. Changing the cadence —
  per-PR entries, or sourcing the release body from the changelog — would be a new ADR superseding
  this one.
