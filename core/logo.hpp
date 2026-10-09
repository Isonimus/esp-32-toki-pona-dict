#pragma once
#include <cstdint>

// The toki pona logo as a single 1-bpp bitmap (ADR-0015). Unlike the per-headword sitelen
// glyphs (core/sitelen.hpp), this is one standalone brand asset: `toki` (a ring with three
// strokes) with the `pona` smile nested inside the ring — the official logo. It is composed at
// build time by tools/rasterize_sitelen.py, which shapes three codepoints through the vendored
// nasin nanpa font (the scaling joiner nests `pona` in `toki`) and emits the data in
// logo_gen.cpp. The font is not shipped; only this bitmap is.
//
// Bitmap format is the same as a sitelen glyph (the drawGlyph1bpp contract): `width` x `height`
// pixels, 1 bit per pixel, row-major with byte-aligned rows (stride = (width + 7) / 8), MSB =
// leftmost pixel, a set bit is ink. The logo carries its own dimensions because it is sized to
// its own ink bounding box, not the fixed sitelen cell — so no separate size constant to desync.

namespace tp {

struct Logo {
  uint16_t width;
  uint16_t height;
  const uint8_t* bitmap;  // (width+7)/8 * height bytes, format above
};

// Defined by the generated translation unit logo_gen.cpp.
extern const Logo TP_LOGO;

}  // namespace tp
