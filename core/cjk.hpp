#pragma once
#include <cstdint>

// CJK gloss glyphs as fixed-size 1-bpp cells (ADR-0018). A display-only language (ADR-0017), such
// as Simplified Chinese, cannot be drawn by the Latin text font (ADR-0012); its detail label and
// glosses are rendered one cell per codepoint instead. Each cell is rasterized at build time from
// the vendored Noto Sans CJK SC subset (see third_party/) by tools/rasterize_sitelen.py, which
// emits the table in cjk_gen.cpp. This header is the stable interface; the lookup is in cjk.cpp.
// Pure data + search, no display here (ADR-0001): the HAL blits the bitmap, core only finds it.
//
// Bitmap format: identical to sitelen (the DisplaySurface::drawGlyph1bpp contract) but a different
// cell size — kCjkWidth x kCjkHeight, 1 bit/pixel, row-major top row first, each row kCjkStride
// bytes, MSB = leftmost pixel, a set bit is ink. CJK is square and monospaced, so a fixed cell is
// the natural layout and the renderer advances by exactly kCjkWidth per glyph.

namespace tp {

// 14x14 (measured legibility vs. flash, ADR-0018). The generated table carries a static_assert on
// these, so changing them here without re-running the rasterizer fails the build loudly.
constexpr int kCjkWidth = 14;
constexpr int kCjkHeight = 14;
constexpr int kCjkStride = (kCjkWidth + 7) / 8;      // bytes per row, byte-aligned
constexpr int kCjkBytes = kCjkStride * kCjkHeight;   // bytes per glyph

// One cell: the Unicode codepoint it renders and its packed bitmap. Codepoints are all in the BMP
// (the generator asserts it), so a uint16 key suffices. The table is sorted by codepoint for a
// binary search.
struct CjkGlyph {
  uint16_t codepoint;
  const uint8_t* bitmap;  // kCjkBytes bytes, format above
};

// Defined by the generated translation unit cjk_gen.cpp.
extern const CjkGlyph CJK_GLYPHS[];
extern const uint16_t CJK_GLYPH_COUNT;

// The bitmap for a codepoint, or nullptr if the CJK font does not cover it. Callers render the
// cell only when this is non-null (the build-time coverage guard keeps the data within the set).
const uint8_t* cjkGlyph(uint32_t codepoint);

}  // namespace tp
