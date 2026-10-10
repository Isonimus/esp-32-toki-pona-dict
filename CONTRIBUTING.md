# Contributing

Thanks for your interest in the project. Contributions — bug reports, new board drivers, gloss
fixes, new languages — are welcome.

## Before you start

- For anything larger than a small fix, **open an issue first** so we can agree on the approach.
- By contributing you agree your code is licensed MIT and any dictionary-data change is licensed
  CC BY-SA 4.0 (see [Licensing](README.md#licensing)).

## Building and testing

```sh
npm run test:host               # host lane: must pass before any core change
pio run -e tp_cardputer         # device lane: firmware must compile for a board
```

The **host lane** regenerates the dictionary/glyph tables, compiles the core under
`-Wall -Wextra`, and runs the native tests (normalization contract, symmetric search, the UI state
machine, the third-party-notice obligation, generated-table freshness). It needs no hardware and
is the bar for any logic change. CI runs exactly this plus a firmware compile.

Rendering and physical input cannot be checked by a host test. If your change affects what a board
shows or how it responds, add or update a **bring-up proof** under
[`docs/bringup/`](docs/bringup/) — a screenshot set plus a short input walk-through. See an existing
board's `README.md` there for the format.

## Architecture

Four layers, dependencies inward only (see [ADR-0001](adr/)):

- `core/` — pure C++17: dictionary tables, `normalize`, `search`. No Arduino, host-testable.
- `hal/` — display and input interfaces plus per-board drivers.
- `app/` — the UI state machine; one UI serves keyboard and button boards.
- `main/` — per-board wiring only.

**Adding a board** is a HAL driver plus a `platformio.ini` env; the core and app do not change for
it. **Changing the dictionary** means editing `data/dictionary.json` (the single source of truth)
and regenerating — never hand-edit the generated `core/*_gen.*` files.

## Decisions and commits

- Significant decisions are recorded as **ADRs** in [`adr/`](adr/), and an ADR ships in the same
  commit as the code it governs. Read them for the "why" behind the design. The *tooling* the
  maintainer uses to manage ADRs is not part of this repo — **you do not need to run anything
  special**; just add a plain Markdown ADR following the existing numbering if your change makes a
  design decision.
- Commits are conventional and atomic (`feat:`, `fix:`, `docs:`, …), one logical change each, with
  messages that explain *why*.
- Keep the quality bar: fail loud, no magic values, names that describe intent, and a regression
  test that fails before your fix and passes after. Everything written — code, comments, docs — is
  in English.

## Reporting bugs and requesting features

Use the issue templates. For anything security-sensitive, see [SECURITY.md](SECURITY.md) instead of
a public issue.
