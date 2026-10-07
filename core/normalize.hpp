#pragma once
#include <string>

namespace tp {

// Fold a UTF-8 string to its search key: lowercase, strip Latin diacritics to the
// base letter (á->a, ñ->n, ü->u, ç->c), expand ß->ss / æ->ae / œ->oe, keep only
// [a-z0-9 space ' -], collapse runs of spaces, and trim the ends.
//
// This is the ONE normalization contract shared by the on-device query path and the
// build-time index generator (tools/generate.mjs). The two implementations must agree
// exactly, or a folded query cannot match a folded index entry; the canonical cases
// live in data/normalization-fixtures.json and are asserted on both sides (ADR-0002).
std::string normalize(const std::string& utf8);

}  // namespace tp
