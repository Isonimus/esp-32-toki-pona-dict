---
id: 0026
title: Release ships a bootable factory image per board
status: accepted
date: 2026-10-12
amends: ADR-0022 (refines the tag-driven release's artifact set)
---

# ADR-0026 — Release ships a bootable factory image per board

ADR-0022 made releases tag-driven: a `v*` tag "builds both board firmwares and attaches the
`.bin` images to a GitHub Release." It did not say *which* image. The pipeline attached
PlatformIO's bare `firmware.bin` — an **application** image, meant for the app partition at
`0x10000`. v0.1.0 shipped exactly that for both boards.

## Context

The M5Burner catalog (and `esptool write-flash`, and ESP web flashers) writes an uploaded
firmware at flash offset **`0x0`**, where the second-stage bootloader must live. An app-only
image has no bootloader and no partition table, so flashing it at `0x0` leaves the chip nothing
to boot. M5Burner rejected the Cardputer (ADV) submission with "failed to boot the device";
the StickC submission carried the same defect.

This was proven on hardware, not inferred. `esptool image-info` on the published
`tp_cardputer-v0.1.0.bin` showed only application segments (DROM `0x3c04_0020`, IROM
`0x4200_0020`) — no bootloader. Wrapping those **exact app bytes** (SHA-256 identical to the
release asset) with the v0.1.0 bootloader + partition table + boot_app0 into a merged image and
flashing it at `0x0` booted a connected Cardputer (StampS3) cleanly to the dictionary. The
firmware was always correct; only the shipped artifact was incomplete.

The sibling project already solved this (`../adversary` slices 0014 and 0016). It ships two
images because there are two install paths, and it carries a hard-won bricking lesson. We adopt
its approach rather than re-derive it.

## Decision

- **Two images per board, plus checksums.** Each release carries, per board:
  - `<env>-<tag>-app.bin` — `firmware.bin` unmodified, the OTA application image an
    OTA installer writes to the app slot at `0x10000`.
  - `<env>-<tag>-factory.bin` — a merged, complete flash image (bootloader + partition table +
    boot_app0 + app) for a fresh install at `0x0`: **this is the one M5Burner, `esptool`, and a
    web flasher consume.**
  - `SHA256SUMS.txt` over all `.bin` assets, so a download can be verified.

- **`merge-bin --flash-mode dio`, never `qio`, with a publish-time guard.** The ESP32/-S3 ROM
  reads the second-stage bootloader using the flash mode in the bootloader header *before*
  quad mode is enabled, so a `qio` header (byte `0x00`) makes the ROM misread the bootloader and
  hard-brick into a `TG0WDT` reset loop — a failure `esptool image-info` does **not** catch, as
  it validates image structure, not ROM-bootability (adversary slice-0014's 2026-09-08
  amendment, a device it actually bricked). The images are *built* `dio`; `qio` is a runtime
  setting, never the header. The pipeline forces `--flash-mode dio` and then asserts the factory
  bootloader header byte is `0x02`, refusing to publish otherwise. The assert reads the byte at
  the bootloader's real offset in the merged file (`bootloader-offset + 2`).

- **Per-chip bootloader offset.** The S3 (Cardputer) bootloader sits at `0x0`; the classic
  ESP32 (StickC Plus2) bootloader sits at `0x1000`. Everything else is the shared
  `default` OTA layout: partition table `0x8000`, boot_app0/otadata `0xe000`, app0 `0x10000`.
  Both our boards use an OTA partition table, so both get an app image — unlike adversary, which
  excludes its `huge_app.csv` m5stick from the OTA artifact.

- **Measured, not guessed.** `boot_app0.bin` is located with `find` under the versioned
  framework package, not hardcoded. `esptool` is pinned (`==5.2.0`) so `merge-bin` and the
  `--flash-*` spelling are deterministic. Flash params (`dio`/`80m`/`8MB`) match both boards'
  built bootloaders.

- **Publish with `gh`, not a third-party action.** `gh release create --generate-notes` uses
  the preinstalled CLI and the built-in token, adding no action to the supply chain — consistent
  with ADR-0022's security posture, and it drops the `softprops/action-gh-release` dependency the
  previous pipeline carried.

## Consequences

- An operator downloads one unambiguous flashable file per board (`factory`) for M5Burner or a
  web flasher, and a separate `app` image for OTA installers; the `app`/`factory` suffix names
  the install path so the wrong one is not flashed.
- The M5Burner rejection is resolved at the source: every future tag produces bootable factory
  images, so the manual fix applied for v0.1.0 (merging by hand) is never needed again.
- v0.1.0's attached assets remain app-only and non-bootable-at-`0x0`; v0.1.1 supersedes them.
- Releases are cut `--prerelease` while the project is v0.x. **Caveat for the deferred web
  flasher (LEDGER):** GitHub's `releases/latest/download/` redirect resolves only the release
  marked "Latest" and only for a stably-named asset — a prerelease is not "Latest", so a flasher
  manifest pointing at `/latest/` would 404 (adversary slice-0016). Enabling the flasher means
  dropping `--prerelease` (or pinning the manifest to a fixed tag) and adding a stable-named
  factory copy then; out of scope here.
- Changing the artifact set again (adding a web-flasher copy, sourcing release notes from the
  CHANGELOG, a distinct ADV target) is a new ADR superseding this one.
