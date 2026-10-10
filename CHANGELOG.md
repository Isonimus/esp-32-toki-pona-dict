# Changelog

All notable changes to this project are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to
[Semantic Versioning](https://semver.org/). Entries are curated per release, not per commit; the
GitHub Release page carries the exhaustive auto-generated notes (ADR-0024).

## [Unreleased]

## [0.1.0] - 2026-10-10

First public, pre-release build. Offline Toki Pona dictionary firmware for ESP32 boards.

### Added

- Offline Toki Pona ⇄ multi-language dictionary: the full *nimi pu* corpus (120 words) glossed in
  English, Spanish, French, German, and Portuguese, plus Chinese as a display-only language.
- **sitelen pona** glyphs for every headword, rasterized at build time from the font.
- Mode-free symmetric search: one query box matches Toki Pona or any gloss language, with
  diacritic-folding so an unaccented query finds accented glosses and vice versa.
- Two-view UI — results list and a one-language-at-a-time detail view — serving both keyboard and
  keyboardless boards from a single app layer.
- Board support: **M5Cardputer** (type-to-search) and **M5StickC Plus2** (button/encoder browse),
  from one firmware behind the HAL (ADR-0001).
- Boot splash with the Toki Pona logo and an about/colophon screen.
- Tagged-release pipeline: a `v*` tag builds every board image and publishes a GitHub Release with
  one `.bin` per board, for flashing via the M5Burner catalog.

[Unreleased]: https://github.com/Isonimus/esp-32-toki-pona-dict/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/Isonimus/esp-32-toki-pona-dict/releases/tag/v0.1.0
