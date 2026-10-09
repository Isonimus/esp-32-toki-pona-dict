# Noto Sans CJK SC — subset (provenance)

A build-time **subset** of Noto Sans CJK SC, carrying only the codepoints our Simplified
Chinese glosses use. It is the font input the CJK gloss rasterizer reads (ADR-0018); like the
nasin nanpa font (ADR-0003) it is **never shipped to or parsed on the device** — only the 1-bpp
bitmaps it rasterizes into `core/cjk_gen.cpp` ship.

## Source

- Upstream: Noto Sans CJK SC, **Version 2.004** (© 2014-2021 Adobe; packaged by Google as the
  Noto fonts), the `Noto Sans CJK SC` face of `NotoSansCJK-Regular.ttc`.
- License: **SIL Open Font License, Version 1.1** (`http://scripts.sil.org/OFL`). Noto Sans CJK
  declares **no Reserved Font Name**, so this Modified Version is unrestricted on naming. The
  verbatim OFL is in `LICENSE` here and reproduced in the repo-root `THIRD_PARTY_NOTICES.md`.
- Only `LICENSE` and this file plus `NotoSansCJKsc-Regular-subset.ttf` are vendored; the full
  multi-MB font is not, to keep the repo lean.

## How the subset was produced

Subset with `fontTools` (`pyftsubset`) to exactly the codepoints that appear in the `zh` glosses
of `data/dictionary.json` plus the detail-cycle label charset (`<`, `>`, `/`, space, `0`–`9`),
dropping layout tables and hinting:

```
pyftsubset /usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc \
  --font-number=2 \
  --unicodes=<the 866 codepoints in the zh glosses + label charset> \
  --output-file=NotoSansCJKsc-Regular-subset.ttf \
  --no-hinting --desubroutinize --drop-tables+=GSUB,GPOS,GDEF
```

Font number 2 is the `Noto Sans CJK SC` face in the collection (Simplified forms — our glosses
are Simplified). Result: 865 glyphs, ~157 KB. The rendered 14×14 1-bpp bitmaps are a document
created with the font, outside the OFL's copyleft on the Font Software itself.

## Regeneration

The subset covers the current `zh` gloss set. **If the Chinese glosses change** (a new word, a
reworded definition introducing a new Hanzi), re-run the subset above with the updated codepoint
set, then `npm run generate:sitelen` to re-rasterize `core/cjk_gen.cpp`. The `zh` corpus is the
closed pu/ku word list, so this is rare. `npm run check:generated` fails loud if `cjk_gen.cpp`
drifts from the data, and `tools/check-font-coverage.mjs` fails loud if a gloss uses a Hanzi the
subset does not cover.
