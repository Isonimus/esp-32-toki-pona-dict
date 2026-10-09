// Host test for the sitelen pona glyph table (ADR-0003): the generated bitmaps and the
// name lookup over them. Plain g++, no hardware, no font — it asserts the committed
// core/sitelen_gen.cpp, which is what ships. Rendering real pixels is a device concern.
#include <cstdio>
#include <cstring>

#include "dictionary.hpp"
#include "sitelen.hpp"

using namespace tp;

static int failures = 0;

#define CHECK(cond)                                               \
  do {                                                            \
    if (!(cond)) {                                                \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
      ++failures;                                                 \
    }                                                             \
  } while (0)

// A glyph has ink iff at least one bit is set across its packed bytes.
static bool hasInk(const uint8_t* bitmap) {
  for (int i = 0; i < kSitelenBytes; ++i) {
    if (bitmap[i] != 0) return true;
  }
  return false;
}

int main() {
  // The closed UCSUR sitelen pona block is 137 glyphs (ADR-0003); the generator emits them all.
  CHECK(SITELEN_GLYPH_COUNT == 137);
  CHECK(kSitelenWidth == 44 && kSitelenHeight == 44);  // ADR-0013 enlarged from 32x32
  CHECK(kSitelenStride == 6 && kSitelenBytes == 264);  // ceil(44/8)=6 bytes/row * 44 rows

  // Table invariants the binary search depends on: sorted by name in byte order, every row
  // named and non-empty. A duplicate or out-of-order name would silently break lookups.
  for (int i = 0; i < SITELEN_GLYPH_COUNT; ++i) {
    CHECK(SITELEN_GLYPHS[i].name != nullptr && SITELEN_GLYPHS[i].name[0] != '\0');
    CHECK(SITELEN_GLYPHS[i].bitmap != nullptr);
    CHECK(hasInk(SITELEN_GLYPHS[i].bitmap));  // no glyph rasterized to blank
    if (i > 0) CHECK(std::strcmp(SITELEN_GLYPHS[i - 1].name, SITELEN_GLYPHS[i].name) < 0);
  }

  // Lookup resolves a known headword to the exact table entry (binary search agrees with the
  // table), and the result carries ink.
  const uint8_t* telo = sitelenGlyph("telo");
  CHECK(telo != nullptr);
  CHECK(hasInk(telo));
  bool teloMatchesTable = false;
  for (int i = 0; i < SITELEN_GLYPH_COUNT; ++i) {
    if (std::strcmp(SITELEN_GLYPHS[i].name, "telo") == 0) {
      CHECK(SITELEN_GLYPHS[i].bitmap == telo);
      teloMatchesTable = true;
    }
  }
  CHECK(teloMatchesTable);

  // First and last names in the sorted table resolve too (binary-search boundary cases).
  CHECK(sitelenGlyph(SITELEN_GLYPHS[0].name) == SITELEN_GLYPHS[0].bitmap);
  CHECK(sitelenGlyph(SITELEN_GLYPHS[SITELEN_GLYPH_COUNT - 1].name) ==
        SITELEN_GLYPHS[SITELEN_GLYPH_COUNT - 1].bitmap);

  // Negative paths: an unknown word and a null name both return nullptr (render then skips
  // the glyph), never a crash or a stray pointer.
  CHECK(sitelenGlyph("notarealword") == nullptr);
  CHECK(sitelenGlyph("") == nullptr);
  CHECK(sitelenGlyph(nullptr) == nullptr);

  // The font covers our whole corpus: every dictionary entry's sitelen id resolves to a
  // glyph. If a future headword has none (e.g. a word outside UCSUR), this fails loud so the
  // gap is a conscious decision, not a silently glyph-less entry.
  for (uint16_t id = 0; id < ENTRY_COUNT; ++id) {
    const char* name = ENTRIES[id].sitelen;
    if (sitelenGlyph(name) == nullptr) {
      std::printf("FAIL entry %u (%s): no sitelen glyph for '%s'\n", id, ENTRIES[id].tp, name);
      ++failures;
    }
  }

  if (failures == 0) {
    std::printf("PASS: sitelen glyph table (%u glyphs)\n", SITELEN_GLYPH_COUNT);
    return 0;
  }
  std::printf("FAILED: %d check(s)\n", failures);
  return 1;
}
