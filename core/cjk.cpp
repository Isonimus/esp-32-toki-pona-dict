#include "cjk.hpp"

namespace tp {

const uint8_t* cjkGlyph(uint32_t codepoint) {
  // CJK_GLYPHS is sorted by codepoint (the generator guarantees it), so a binary search finds the
  // cell in O(log n) over the closed gloss-codepoint set.
  int lo = 0, hi = static_cast<int>(CJK_GLYPH_COUNT);
  while (lo < hi) {
    const int mid = lo + (hi - lo) / 2;
    const uint32_t c = CJK_GLYPHS[mid].codepoint;
    if (c < codepoint) lo = mid + 1;
    else if (c > codepoint) hi = mid;
    else return CJK_GLYPHS[mid].bitmap;
  }
  return nullptr;
}

}  // namespace tp
