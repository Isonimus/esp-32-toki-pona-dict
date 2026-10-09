#pragma once
#include <cstdint>

// Pure data model for the dictionary. No Arduino, no display, no allocation here.
// The tables themselves are defined in the GENERATED translation unit
// dictionary_gen.cpp (built from data/*.json by tools/generate.mjs).
//
// On the ESP32 these `const` tables live in memory-mapped flash (.rodata) and are
// read through ordinary pointers, so they cost no RAM and need no PROGMEM shim.

namespace tp {

// Query/display languages, in the order the UI cycles them. Keep in sync with
// data/languages.json "query_order"; the generated tables initialize fixed-size arrays of
// LangCount, so a mismatch fails the C++ build. Zh (Chinese) is display-only (ADR-0017): it has
// glosses and a label but a null, zero-length search index (it cannot be typed on the keyboard).
enum class Lang : uint8_t { Tp = 0, En, Es, Fr, De, Pt, Zh, Count };
constexpr int LangCount = static_cast<int>(Lang::Count);

// One headword and its glosses. The glosses for a language are a nullptr-terminated
// array of C strings in DISPLAY spelling (diacritics intact). glossesByLang[Tp] is a
// one-element list holding the headword itself, so Toki Pona is uniform with the rest.
struct Entry {
  const char* tp;                             // headword, e.g. "suli"
  const char* sitelen;                        // sitelen pona glyph id (defaults to tp)
  const char* const* glossesByLang[LangCount];
};

// A row of a per-language search index: a diacritic-folded, lowercased term (see
// core/normalize.hpp) and the entry it points at. Rows are sorted by `norm` in byte
// order, so a lookup is a binary search followed by a prefix scan.
struct IndexRow {
  const char* norm;
  uint16_t entryId;
};

// Defined by the generated translation unit.
extern const Entry ENTRIES[];
extern const uint16_t ENTRY_COUNT;
extern const IndexRow* const INDEX_BY_LANG[LangCount];
extern const uint16_t INDEX_SIZE_BY_LANG[LangCount];

// Display label per language, in Lang order (e.g. "English"). Sourced from
// data/languages.json; used by the app's detail view to head each gloss line.
extern const char* const LANG_LABEL[LangCount];

}  // namespace tp
