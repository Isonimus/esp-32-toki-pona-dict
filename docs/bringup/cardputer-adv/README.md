# Cardputer ADV — bring-up proof

The ADR-0001/0005 acceptance bar for a board: a screenshot set plus a short input
walk-through, proving the HAL (display + input) and app render and respond on real hardware.
CI only proves the firmware links; this is the on-device evidence.

- **Board:** M5Stack Cardputer ADV (M5StampS3, ESP32-S3, no PSRAM; TCA8418 I²C keyboard).
- **Date:** 2026-10-08.
- **Firmware:** `tp_cardputer` env. Screenshots captured over USB with the debug build
  (ADR-0006): `PLATFORMIO_BUILD_FLAGS=-DTP_DEBUG_SCREENSHOT pio run -e tp_cardputer -t upload`,
  then `python3 tools/fb_capture.py --out <file>`.

## Walk-through

| Step | Input | Result | Frame |
|---|---|---|---|
| 1 | (boot) | Empty search view: `>` prompt | `01-empty.png` |
| 2 | type `to` | Result list: `toki` (focused, yellow) + `tomo tonsi tawa lon kama lawa` | `02-list.png` |
| 3 | Enter | Detail view, `< English 1/5 >`: `to talk / speak / language / hello` + sitelen glyph | `03-detail-en.png` |
| 4 | bare `.` | Detail cycles to `< Español 2/5 >`: `hablar / idioma / lenguaje / hola` | `04-detail-es.png` |

This exercises the whole `InputEvent` vocabulary and both views: `Char` (step 2), `Select`
(step 3), `Nav` (step 4, language cycle — ADR-0007), and the TCA8418 keyboard path including
bare-arrow navigation with no `fn` (ADR-0008). `Back` (Detail→Search) and list-row `Nav` were
also confirmed live but are not pictured.

> Frames `01`–`04` predate ADR-0012 and show the old ASCII-only font (note `Espanol` in `04`).

## Latin-diacritic text fonts (ADR-0012), 2026-10-09

The text renderer now uses two vendored Latin-Extended fonts — a compact `7x13` body and a bold
`helvB14` headword, both native size (no scaling). These frames prove the diacritic rendering
that the old built-in ASCII font could not; captured on `pilin`, cycling the detail language:

| Input | Result | Frame |
|---|---|---|
| `pilin` → Enter → `.` | `< Español 2/5 >`: `corazón` / `sentimiento` / `sentir` / `pensar` | `05-detail-es-accents.png` |
| `.` again | `< Français 3/5 >`: `cœur` / `sentiment` / `ressentir` / `penser` | `06-detail-fr-oe.png` |

What they confirm: accented letters (`ó`, `ñ`, `ç`) render as glyphs, not boxes; the `œ` ligature
(U+0153, French `cœur`) renders — covered by the `_te` (Latin-Extended) glyph set, which Latin-1
lacks; the native language labels are restored (`Español`, `Français`); the headword is crisp (a
real bold font, not the old blocky 2× scale); and there is a gap below the headword.

## Sitelen glyph enlarged to 44×44 (ADR-0013), 2026-10-09

The sitelen pona glyph was re-rasterized from 32×32 to **44×44** so it reads as a deliberate
script accent, not a small mark (operator feedback at the ADR-0012 bring-up). Re-rendered from
the vector font, so it is crisp at the larger size rather than an upscaled 32px bitmap.

| Input | Result | Frame |
|---|---|---|
| `to` → Enter | `< English 1/5 >`: `to talk / speak / language / hello`, enlarged glyph top-right | `07-detail-sitelen-44.png` |

Compare `07` against `03-detail-en.png` (same view at 32×32): the glyph is visibly larger and
sits crisply beside the ADR-0012 headword. The size change is global — the splash/About wordmark
and the empty-root prompt glyphs also enlarge.

## Centered empty-search root (ADR-0014), 2026-10-09

With the glyphs enlarged, the empty search root was reworked to center the type-to-search prompt
against the full panel and drop the lone edit caret. Captured on this build:

| View | Result | Frame |
|---|---|---|
| Empty search root | `lukin` icon + `o alasa e nimi` centered in the panel; no `>` caret while empty | `09-empty-prompt.png` |

What it confirms: the empty-root prompt is centered in the full panel and lifts off the bottom edge
(it was previously centered only within the result-area slice, which pushed it against the bottom);
and the bare `>` edit line is suppressed while the query is empty (ADR-0014), returning with the
result list on the first keystroke.

## Font-composed toki pona logo on the splash (ADR-0015), 2026-10-09

The splash now leads with the merged toki pona logo — `toki` with the `pona` smile nested inside its
ring — instead of the two separate wordmark glyphs, and the `nasin nanpa` font credit is dropped
from it (it remains in the About colophon and `THIRD_PARTY_NOTICES.md`). The logo is composed from
the vendored font via its sitelen pona scaling joiner and rasterized 1-bpp at 88×83 (ADR-0015).
Captured on this build:

| View | Result | Frame |
|---|---|---|
| Boot splash | The merged toki pona logo centered as a hero; no font credit | `08-splash.png` |

What it confirms: the logo renders as the nested mark (three strokes above a circle with the smile
inside), not two side-by-side glyphs; it is centered as a single hero; and no credit text appears.
The frame was captured with `kSplashHoldMs` temporarily raised so the splash did not auto-dismiss
mid-capture; the layout is identical at the shipped 1.5 s hold.

## Simplified Chinese detail view (ADR-0017/0018), 2026-10-09

Chinese is a **display-only** language (ADR-0017): findable from a Toki Pona word, rendered in the
detail cycle, but not typeable to search (the keyboard emits Latin only). Its glosses render through
a **subset-rasterized 14×14 1-bpp CJK cell font** (ADR-0018), not the Latin text font — this is the
first on-device proof that the real panel renders CJK legibly at the shipped 14px size.

| Input | Result | Frame |
|---|---|---|
| `to` → Enter → Nav ×5 | `< 中文 6/6 >`: `沟通、说、讲、想、思考` / `对话、故事` / `语言`, sitelen glyph top-right | `10-detail-zh.png` |

What it confirms: the label `中文` and all three gloss lines render as fixed CJK cells while the
headword (`toki`) stays in the Latin font and the sitelen glyph is unchanged — the content-routed
CJK path (ADR-0018) engages only on the label and glosses. The `；`-split senses land on three
separate lines with `、` kept within each sense (ADR-0017). **14px is legible on the real panel** —
the dense glyphs (`讲`, `思`, `考`) are tight but readable, so the recorded 16px fallback (ADR-0018)
is not needed. The cycle label reads `6/6` (Chinese is the sixth language), its ASCII (`<`, `6`, `/`,
`>`) rendered as CJK cells too, a touch wide as ADR-0018 predicted but clean. No real gloss overflows
the 240px width (the longest line is nine cells ≈ 126px), so the right-edge clip is never reached.

## Condensed About colophon (ADR-0016), 2026-10-09

The About colophon was condensed when the gloss provenance was recorded (ADR-0016): it now shows the
wordmark plus two courtesy-credit lines — one per attributed work — and a pointer to the notices file,
replacing the longer per-work colophon. The verbatim legal text stays in `THIRD_PARTY_NOTICES.md`.
This layout had not been shot since ADR-0016.

| Input | Result | Frame |
|---|---|---|
| `Back` → `Back` (empty-search → About) | wordmark; `sitelen pona: nasin nanpa (MIT)` / `glosses: lipu Linku (CC BY-SA)` / `see THIRD_PARTY_NOTICES.md` | `11-about.png` |

What it confirms: the two-glyph `toki pona` wordmark heads the view (the colophon kept it when the
splash diverged to the merged logo in ADR-0015); both courtesy credits render on one line each and
fit the 240px width; and the notices pointer renders in accent yellow below them. The colophon draws
left-aligned, which is fine for a dense credit block (per the LEDGER note). The condensed layout is
the shipped ADR-0016 form.

## No-results affordance (ADR-0019), 2026-10-09

A non-empty query that matches nothing used to leave the result area blank (ADR-0010 deferred the
case). ADR-0019 fills it: the grey `ala` ("none") glyph above the sitelen sentence `nimi li weka`
("the word is absent"), centred below the query line. This also introduces the palette's first grey
(`kColorMuted = 0x8410`), so the frame doubles as the proof that grey is legible on the panel.

| Input | Result | Frame |
|---|---|---|
| type `zzzz` (matches nothing) | `>zzzz` on top; grey `ala` X over grey `nimi li weka`, centred | `12-no-results.png` |

What it confirms: the no-match state is now clearly signalled rather than blank; the query line stays
white on top while the affordance is grey; **the grey (`0x8410`) reads legibly against black** —
distinctly dimmer than the white query line but not washed out — so no re-tune is needed. One thing
the panel revealed that the headword alone could not: `weka`'s glyph is itself X-like, so the hero
`ala` X and the sentence's `weka` X are two X-family shapes. They stay distinguishable (the hero is a
clean thin X, centred and alone; `weka` is a denser X inside the three-glyph sentence), and `weka`
remains the least-duplicative choice — the alternatives (`lon ala`, `lukin ala`) would stack a second
*identical* `ala` under the hero. Accepted as shipped (ADR-0019).
