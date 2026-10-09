// Enforce that every character we display can actually be drawn by the device text font (ADR-0012).
//
// The device renders dictionary glosses and language labels with a vendored Latin font whose
// glyph set is "_te" = Unicode 32-383 (ASCII + Latin-1 Supplement + Latin Extended-A). A gloss
// edited to contain anything outside that range (an em/en dash, a smart quote, a CJK character,
// an emoji) would render on-device as a placeholder box with no build-time warning — the exact
// failure this font change exists to end. This check turns the font's coverage into a CI
// invariant over the source data, so such a character is caught in `npm run test:host` instead of
// discovered by eye on the panel.
//
// Rendering itself is not host-assertable (ADR-0001), so this is a necessary, not sufficient,
// guard: the vendored font is the authority on which in-range glyphs truly exist, and the
// on-device bring-up proof (LEDGER) is the final check. What this catches is the common, cheap
// class of error — a codepoint no Latin font could ever have.
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join, relative } from "node:path";

const ROOT = dirname(dirname(fileURLToPath(import.meta.url)));

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
    `FAIL: display text contains ${offenders.length} character(s) the device font cannot render ` +
      `(outside ASCII + Latin-1 + Latin Extended-A, ADR-0012):\n${lines.join(
        "\n"
      )}\nUse an in-range equivalent (e.g. an ASCII hyphen, not an en dash) or widen the font.`
  );
  process.exit(1);
}

console.log(`PASS: font coverage (${DATA_FILES.length} data file(s) within the device font's glyph set)`);
