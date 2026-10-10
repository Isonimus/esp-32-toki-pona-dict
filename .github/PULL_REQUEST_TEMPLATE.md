<!-- Thanks for contributing! Keep PRs atomic — one logical change. -->

## What and why

<!-- What does this change, and why? Link any issue (e.g. "Closes #12"). -->

## How it was verified

- [ ] `npm run test:host` passes (host lane: normalization, search, UI, notices, generated-table freshness)
- [ ] Firmware compiles for an affected board (`pio run -e tp_cardputer` / `tp_stickc_plus2`)
- [ ] If rendering or input changed: a bring-up proof under `docs/bringup/` was added or updated
- [ ] If the dictionary changed: `data/dictionary.json` was edited (not the generated files) and tables regenerated

## Notes

<!-- Anything a reviewer should know: a new ADR, a tradeoff, a follow-up left for the LEDGER. -->
