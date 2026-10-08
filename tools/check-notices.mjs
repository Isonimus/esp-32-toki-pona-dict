// Enforce the MIT redistribution obligation for every vendored third-party work (ADR-0003).
//
// MIT requires each work's copyright + permission notice to travel with every distributed
// copy. We discharge that with a top-level THIRD_PARTY_NOTICES.md that accompanies every
// release artifact. This check fails loud if any vendored LICENSE is not reproduced verbatim
// in that file, so the notice can never drift from its source or be dropped unnoticed — the
// legal obligation becomes a CI-enforced invariant, not a doc that rots.
//
// Add a work here when it is vendored under third_party/.
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join, relative } from "node:path";

const ROOT = dirname(dirname(fileURLToPath(import.meta.url)));
const NOTICES_PATH = join(ROOT, "THIRD_PARTY_NOTICES.md");

// Each vendored work whose LICENSE must appear verbatim in THIRD_PARTY_NOTICES.md.
const VENDORED_LICENSES = [
  { name: "nasin nanpa", licensePath: join(ROOT, "third_party", "nasin-nanpa", "LICENSE") },
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

const missing = [];
for (const { name, licensePath } of VENDORED_LICENSES) {
  const license = normalize(readFileSync(licensePath, "utf8"));
  if (!notices.includes(license)) {
    missing.push(`${name} (${relative(ROOT, licensePath)})`);
  }
}

if (missing.length > 0) {
  console.error(
    `FAIL: THIRD_PARTY_NOTICES.md is missing the verbatim license for:\n  - ${missing.join(
      "\n  - "
    )}\nMIT requires these notices to ship with the firmware; add them to ${relative(
      ROOT,
      NOTICES_PATH
    )}.`
  );
  process.exit(1);
}

console.log(`PASS: third-party notices (${VENDORED_LICENSES.length} vendored license(s))`);
