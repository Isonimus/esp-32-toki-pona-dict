#pragma once
#include <cstdint>

// Sitelen pona glyphs as fixed-size 1-bpp bitmaps (ADR-0003). Each glyph is rasterized at
// build time from the vendored nasin nanpa font (see third_party/) by
// tools/rasterize_sitelen.py, which emits the data table in sitelen_gen.cpp. This header is
// the stable interface; the lookup is in sitelen.cpp. Pure data + search, no display here
// (ADR-0001): the HAL blits the bitmap, core only stores and finds it.
//
// Bitmap format (the contract a DisplaySurface::drawGlyph1bpp driver must honour):
//   - kSitelenWidth x kSitelenHeight pixels, measured in ADR-0003 (32x32).
//   - 1 bit per pixel, row-major, top row first. Each row is byte-aligned: a row occupies
//     kSitelenStride bytes and a glyph occupies kSitelenBytes bytes.
//   - Within a byte the most significant bit is the leftmost pixel. A set bit is a
//     foreground (ink) pixel; a clear bit is background.

namespace tp {

// Measured in ADR-0003 (2026-10-07): 32x32 is legible for the densest glyph in the whole
// script at the lightest flash. The generated table carries a static_assert on these, so
// changing them here without re-running the rasterizer fails the build loudly.
constexpr int kSitelenWidth = 32;
constexpr int kSitelenHeight = 32;
constexpr int kSitelenStride = (kSitelenWidth + 7) / 8;   // bytes per row, byte-aligned
constexpr int kSitelenBytes = kSitelenStride * kSitelenHeight;  // bytes per glyph

// One glyph: the headword it renders (e.g. "telo") and its packed bitmap. `name` matches
// the Entry::sitelen string. The table is sorted by `name` in byte order for binary search.
struct SitelenGlyph {
  const char* name;
  const uint8_t* bitmap;  // kSitelenBytes bytes, format above
};

// Defined by the generated translation unit sitelen_gen.cpp.
extern const SitelenGlyph SITELEN_GLYPHS[];
extern const uint16_t SITELEN_GLYPH_COUNT;

// The bitmap for a glyph name, or nullptr if the font has no glyph for it (e.g. a headword
// outside the UCSUR sitelen pona block). Callers render the glyph only when this is non-null.
const uint8_t* sitelenGlyph(const char* name);

}  // namespace tp
