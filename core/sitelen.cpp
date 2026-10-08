#include "sitelen.hpp"

#include <cstring>

namespace tp {

const uint8_t* sitelenGlyph(const char* name) {
  if (name == nullptr) return nullptr;

  // SITELEN_GLYPHS is sorted by `name` in byte order (the generator guarantees it), so a
  // binary search finds the glyph in O(log n) over the closed ~137-glyph set.
  int lo = 0, hi = static_cast<int>(SITELEN_GLYPH_COUNT);
  while (lo < hi) {
    const int mid = lo + (hi - lo) / 2;
    const int cmp = std::strcmp(SITELEN_GLYPHS[mid].name, name);
    if (cmp < 0) lo = mid + 1;
    else if (cmp > 0) hi = mid;
    else return SITELEN_GLYPHS[mid].bitmap;
  }
  return nullptr;
}

}  // namespace tp
