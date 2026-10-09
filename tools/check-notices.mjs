// Enforce third-party attribution obligations for every work we redistribute (ADR-0003,
// ADR-0012, ADR-0016).
//
// Two kinds of obligation, two checks:
//   - Verbatim-license works (MIT, the Adobe/X11 font terms): the copyright + permission notice
//     must travel verbatim with every distributed copy. We assert the vendored LICENSE is
//     reproduced byte-for-byte in THIRD_PARTY_NOTICES.md (VENDORED_LICENSES below).
//   - Cite-by-reference works (CC BY-SA 4.0 for the lipu Linku gloss data): the license permits
//     citing itself by URI instead of reproducing its full text, but still requires the
//     attribution elements — source, license identifier + URI, and a statement of changes. We
//     assert those elements are present (REQUIRED_ATTRIBUTIONS below).
//
// Either way the legal obligation becomes a CI-enforced invariant, not a doc that rots. Add a
// work to the matching list when it is vendored or its data is incorporated.
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join, relative } from "node:path";

const ROOT = dirname(dirname(fileURLToPath(import.meta.url)));
const NOTICES_PATH = join(ROOT, "THIRD_PARTY_NOTICES.md");

// Each vendored work whose LICENSE must appear verbatim in THIRD_PARTY_NOTICES.md.
const VENDORED_LICENSES = [
  { name: "nasin nanpa", licensePath: join(ROOT, "third_party", "nasin-nanpa", "LICENSE") },
  { name: "Adobe Helvetica (u8g2 font)", licensePath: join(ROOT, "third_party", "u8g2-fonts", "LICENSE") },
  { name: "Noto Sans CJK SC (subset)", licensePath: join(ROOT, "third_party", "noto-sans-cjk-subset", "LICENSE") },
];

// Works whose license is cited by reference rather than reproduced in full (ADR-0016). For each,
// the notice must still carry the required attribution elements; we assert every substring below
// appears in THIRD_PARTY_NOTICES.md, so dropping the source, the license, its URI, or the
// changes statement fails the build.
const REQUIRED_ATTRIBUTIONS = [
  {
    name: "lipu Linku (dictionary glosses, CC BY-SA 4.0)",
    mustContain: [
      "lipu Linku",
      "github.com/lipu-linku/sona",
      "CC BY-SA 4.0",
      "https://creativecommons.org/licenses/by-sa/4.0/",
      "Changes made",
    ],
  },
];

// Normalize to compare license bodies regardless of surrounding Markdown, indentation, or
// trailing whitespace: strip trailing spaces per line and the leading/trailing blank lines.
function normalize(text) {
  return text
    .replace(/\r\n/g, "\n")
    .split("\n")
    .map((line) => line.replace(/\s+$/, ""))
    .join("\n")
    .trim();
}

const notices = normalize(readFileSync(NOTICES_PATH, "utf8"));

const problems = [];
for (const { name, licensePath } of VENDORED_LICENSES) {
  const license = normalize(readFileSync(licensePath, "utf8"));
  if (!notices.includes(license)) {
    problems.push(`${name}: verbatim license missing (${relative(ROOT, licensePath)})`);
  }
}

for (const { name, mustContain } of REQUIRED_ATTRIBUTIONS) {
  const missingParts = mustContain.filter((part) => !notices.includes(part));
  if (missingParts.length > 0) {
    problems.push(`${name}: missing attribution element(s) ${missingParts.map((p) => JSON.stringify(p)).join(", ")}`);
  }
}

if (problems.length > 0) {
  console.error(
    `FAIL: THIRD_PARTY_NOTICES.md does not discharge every attribution obligation:\n  - ${problems.join(
      "\n  - "
    )}\nThese notices must ship with the firmware; fix them in ${relative(ROOT, NOTICES_PATH)}.`
  );
  process.exit(1);
}

console.log(
  `PASS: third-party notices (${VENDORED_LICENSES.length} verbatim license(s), ${REQUIRED_ATTRIBUTIONS.length} cited attribution(s))`
);
