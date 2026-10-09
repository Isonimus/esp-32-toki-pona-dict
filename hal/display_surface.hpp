#pragma once
#include <cstdint>

// The board-agnostic display seam (ADR-0001), a thin interface over LovyanGFX. The app
// renders its views — search + result list, entry detail — against this and never touches
// the panel directly; the panel is selected per board in the concrete driver (LEDGER).
//
// RGB565 throughout, matching LovyanGFX's native canvas format (mirrors the sapper). Pure
// interface: no LovyanGFX include here, so it is host-compilable.
//
// Kept deliberately small: it carries only the primitives the two planned views need, and
// grows only when a view needs more (KISS), the same discipline as the sapper's IDisplay.
// A dictionary has no "screenless board" case — the screen is the product — so unlike the
// sapper there is no Null implementation; a board without a panel is simply not a target.

namespace tp {

// Which text tier a string belongs to. The app expresses intent (a headword vs a gloss/list/query
// line); the board maps each role to a concrete font (ADR-0012). This replaced an integer "size"
// multiplier, which assumed one font scaled up — the headword then looked blocky, and two distinct
// native fonts (a bold header, a compact body) read far better on the small panel.
enum class TextRole : uint8_t { Body, Header };

class DisplaySurface {
 public:
  virtual ~DisplaySurface() = default;

  // Initialise the panel and off-screen canvas. Returns false on a real init failure,
  // logged loud by the caller (§fail-loud).
  virtual bool begin() = 0;

  virtual uint16_t width() const = 0;
  virtual uint16_t height() const = 0;

  // Fill the whole canvas with one colour (clears a frame before redraw).
  virtual void fillScreen(uint16_t color565) = 0;

  // Draw NUL-terminated UTF-8 `text` with its top-left at (x, y), in `color565`, in the font the
  // board assigns to `role` (ADR-0012). UTF-8 because glosses keep their diacritics for display
  // (ADR-0002); the fonts cover Latin + diacritics (ADR-0012).
  virtual void drawText(int32_t x, int32_t y, const char* text, uint16_t color565,
                        TextRole role) = 0;

  // Blit a sitelen pona glyph: a 1-bpp bitmap, `w`x`h`, MSB-first, row-major, top-left at
  // (x, y). Set bits draw in `fgColor565`; clear bits are left untouched (the glyph is
  // composited onto whatever is already there). This is the seam ADR-0003 reserves for the
  // build-time-rasterised glyph tables (core/sitelen_gen, not yet generated).
  virtual void drawGlyph1bpp(int32_t x, int32_t y, uint16_t w, uint16_t h, const uint8_t* bits,
                             uint16_t fgColor565) = 0;

  // Push the off-screen canvas to the physical panel.
  virtual void present() = 0;
};

}  // namespace tp
