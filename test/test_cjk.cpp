// Host test for the CJK gloss glyph lookup (ADR-0018): cjkGlyph() resolves a covered codepoint to
// its cell and an uncovered one to nullptr, and the generated table is codepoint-sorted so the
// binary search is valid. Plain g++, no hardware. (Whether a cell is legible is a device bring-up
// question, not host-assertable — ADR-0001.)
#include <cstdio>

#include "cjk.hpp"

using namespace tp;

static int failures = 0;

#define CHECK(cond)                                               \
  do {                                                            \
    if (!(cond)) {                                                \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
      ++failures;                                                 \
    }                                                             \
  } while (0)

int main() {
  CHECK(CJK_GLYPH_COUNT > 0);

  // The table must be strictly ascending by codepoint — the binary search in cjkGlyph depends on it.
  for (uint16_t i = 1; i < CJK_GLYPH_COUNT; ++i) {
    CHECK(CJK_GLYPHS[i - 1].codepoint < CJK_GLYPHS[i].codepoint);
  }

  // Every table entry resolves to its own bitmap.
  for (uint16_t i = 0; i < CJK_GLYPH_COUNT; ++i) {
    CHECK(cjkGlyph(CJK_GLYPHS[i].codepoint) == CJK_GLYPHS[i].bitmap);
  }

  // Covered codepoints resolve: the zh native label 中文, and a space (present as an empty cell so
  // a run of text advances through it).
  CHECK(cjkGlyph(0x4E2D) != nullptr);  // 中
  CHECK(cjkGlyph(0x6587) != nullptr);  // 文
  CHECK(cjkGlyph(0x0020) != nullptr);  // space — covered, draws an empty cell

  // Codepoints outside the gloss set resolve to nullptr: an emoji never appears in a gloss, and a
  // Latin letter is drawn by the text font (ADR-0012), not the CJK cells.
  CHECK(cjkGlyph(0x1F600) == nullptr);  // emoji
  CHECK(cjkGlyph(0x0041) == nullptr);   // 'A'

  if (failures == 0) {
    std::printf("PASS: CJK glyph table (%u glyphs)\n", static_cast<unsigned>(CJK_GLYPH_COUNT));
    return 0;
  }
  std::printf("FAILED: %d check(s)\n", failures);
  return 1;
}
