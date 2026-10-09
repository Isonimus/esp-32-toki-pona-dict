// Enforce that every character we display can actually be drawn by a device font (ADR-0012, ADR-0018).
//
// The device renders most glosses and labels with a vendored Latin font whose glyph set is "_te" =
// Unicode 32-383 (ASCII + Latin-1 Supplement + Latin Extended-A), and a display-only language's
// glosses (ADR-0017) with the subset-rasterized CJK font (ADR-0018). A gloss edited to contain
// anything outside BOTH sets (an em/en dash, a smart quote, an emoji, or a Hanzi not in the CJK
// subset) would render on-device as a placeholder box or a blank cell with no build-time warning —
// the exact failure this check exists to end. It turns both fonts' coverage into a CI invariant
// over the source data, so such a character is caught in `npm run test:host` instead of by eye.
//
// Rendering itself is not host-assertable (ADR-0001), so this is a necessary, not sufficient,
// guard: the vendored font is the authority on which in-range glyphs truly exist, and the
// on-device bring-up proof (LEDGER) is the final check. What this catches is the common, cheap
// class of error — a codepoint no Latin font could ever have.
import { readFileSync, existsSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join, relative } from "node:path";

const ROOT = dirname(dirname(fileURLToPath(import.meta.url)));

// The CJK gloss font (ADR-0018) covers exactly the code points tools/rasterize_sitelen.py put in
// core/cjk_gen.cpp — the glyphs a display-only language (ADR-0017) uses. Read that set so a CJK
// character counts as renderable iff it was actually rasterized: a gloss edited to add a Hanzi the
// font does not cover then fails here (and in check:generated) instead of drawing a blank cell.
// Parse only the CJK_GLYPHS table rows (`{ 0x4e2d, cjk_4e2d }`), not the bitmap byte arrays.
function readCjkCoverage() {
  const path = join(ROOT, "core", "cjk_gen.cpp");
  if (!existsSync(path)) return new Set();
  const set = new Set();
  for (const m of readFileSync(path, "utf8").matchAll(/\{\s*0x([0-9a-fA-F]+),\s*cjk_/g)) {
    set.add(parseInt(m[1], 16));
  }
  return set;
}
const cjkCoverage = readCjkCoverage();

// Source files whose string values become on-device display text (ADR-0002 single source of
// truth): dictionary glosses + headwords, and the native language labels shown in the detail view.
const DATA_FILES = [
  join(ROOT, "data", "dictionary.json"),
  join(ROOT, "data", "languages.json"),
];

// The vendored font's glyph coverage (u8g2 "_te"): ASCII printable, Latin-1 letters/punctuation,
// and Latin Extended-A. Tab/newline are allowed as harmless structural whitespace. C1 controls
// (0x7F-0x9F) are deliberately excluded — they are not glyphs.
function isRenderable(cp) {
  if (cp === 0x09 || cp === 0x0a) return true;
  if (cp >= 0x20 && cp <= 0x7e) return true; // ASCII printable
  if (cp >= 0xa0 && cp <= 0x17f) return true; // Latin-1 Supplement + Latin Extended-A
  if (cjkCoverage.has(cp)) return true; // in the rasterized CJK gloss font (ADR-0018)
  return false;
}

// Walk a parsed JSON value, yielding every string it contains (keys are structural, not displayed).
function* strings(value) {
  if (typeof value === "string") {
    yield value;
  } else if (Array.isArray(value)) {
    for (const item of value) yield* strings(item);
  } else if (value && typeof value === "object") {
    for (const item of Object.values(value)) yield* strings(item);
  }
}

const offenders = [];
for (const file of DATA_FILES) {
  const data = JSON.parse(readFileSync(file, "utf8"));
  for (const text of strings(data)) {
    for (const ch of text) {
      const cp = ch.codePointAt(0);
      if (!isRenderable(cp)) {
        offenders.push({ file: relative(ROOT, file), cp, ch, text });
      }
    }
  }
}

if (offenders.length > 0) {
  const lines = offenders.map(
    ({ file, cp, ch, text }) =>
      `  ${file}: U+${cp.toString(16).toUpperCase().padStart(4, "0")} ${JSON.stringify(
        ch
      )} in ${JSON.stringify(text)}`
  );
  console.error(
    `FAIL: display text contains ${offenders.length} character(s) no device font can render ` +
      `(outside ASCII + Latin-1 + Latin Extended-A (ADR-0012) and not in the rasterized CJK set ` +
      `(ADR-0018)):\n${lines.join("\n")}\nUse an in-range equivalent (e.g. an ASCII hyphen, not an ` +
      `en dash); for a new CJK character, re-subset the Noto font and run \`npm run generate:sitelen\`.`
  );
  process.exit(1);
}

console.log(`PASS: font coverage (${DATA_FILES.length} data file(s) within the device font's glyph set)`);
