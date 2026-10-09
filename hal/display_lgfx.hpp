#pragma once

// LovyanGFX implementation of the DisplaySurface seam (ADR-0005). Device-only: it is the one
// place that knows LovyanGFX, included solely by main/ for the board it wires, so it is
// header-only (no separate translation unit) and lives behind the UNIT_TEST guard — the host
// lane renders against the pure DisplaySurface seam, never this.
//
// It draws to an off-screen LGFX_Sprite canvas and pushes it whole in present(), so a frame
// never tears and a redraw is one blit. The panel class (pins, driver, offsets) is injected,
// so this file is board-agnostic: the Cardputer passes LGFX_Cardputer, a later board its own.

#ifndef UNIT_TEST  // device-only: LovyanGFX is not available on the host lane.

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include "display_surface.hpp"
#include "display/fonts/text_fonts.h"  // vendored Latin-Extended body + header fonts (ADR-0012)
#include "sitelen.hpp"  // the 1-bpp glyph packing contract drawGlyph1bpp must honour

namespace tp {

class LgfxDisplaySurface : public DisplaySurface {
 public:
  // `panel` is the board's configured LovyanGFX device; `rotation` is its LovyanGFX rotation
  // index (the Cardputer's 240x135 landscape is 1). Both are board facts owned by main/.
  LgfxDisplaySurface(lgfx::LGFX_Device& panel, uint8_t rotation)
      : panel_(panel), canvas_(&panel), rotation_(rotation) {}

  bool begin() override {
    if (!panel_.init()) return false;  // real init failure — the caller logs it loud (§3).
    panel_.setRotation(rotation_);
    panel_.setBrightness(255);  // backlight on: the panel shows black until this (ADR-0005).
    canvas_.setColorDepth(16);  // RGB565, matching the seam and the panel (ADR-0001).
    if (canvas_.createSprite(panel_.width(), panel_.height()) == nullptr) {
      return false;  // sprite allocation failed (out of SRAM) — a real failure, not softened.
    }
    return true;
  }

  uint16_t width() const override { return canvas_.width(); }
  uint16_t height() const override { return canvas_.height(); }

  void fillScreen(uint16_t color565) override { canvas_.fillScreen(color565); }

  void drawText(int32_t x, int32_t y, const char* text, uint16_t color565,
                TextRole role) override {
    // A real font per role at native size (ADR-0012) — no setTextSize scaling, which is what made
    // the old 2x headword blocky. UTF-8 is decoded by the U8g2 bridge, so diacritics render.
    canvas_.setFont(role == TextRole::Header ? &headerFont_ : &bodyFont_);
    canvas_.setTextColor(color565);
    canvas_.setCursor(x, y);
    canvas_.print(text);
  }

  // Composite a 1-bpp glyph (core/sitelen.hpp packing: MSB-first, row-major, byte-aligned
  // rows). Only set bits are drawn, in fgColor565; clear bits are left untouched, so the glyph
  // overlays whatever is already on the canvas.
  void drawGlyph1bpp(int32_t x, int32_t y, uint16_t w, uint16_t h, const uint8_t* bits,
                     uint16_t fgColor565) override {
    const int stride = (w + 7) / 8;  // bytes per row, byte-aligned (matches kSitelenStride)
    for (uint16_t row = 0; row < h; ++row) {
      const uint8_t* rowBits = bits + row * stride;
      for (uint16_t col = 0; col < w; ++col) {
        if (rowBits[col >> 3] & (0x80 >> (col & 7))) {
          canvas_.drawPixel(x + col, y + row, fgColor565);
        }
      }
    }
  }

  void present() override { canvas_.pushSprite(0, 0); }

#ifdef TP_DEBUG_SCREENSHOT
  // Stream the off-screen canvas as a binary PPM (P6) to `out` for a reproducible bring-up
  // screenshot (ADR-0006). Debug-only: compiled out of production firmware. We read the canvas
  // one row at a time (no ~100 KB frame buffer on a PSRAM-less board) into rgb888_t, letting
  // readRect normalise each pixel — so the capture reflects the logical colour regardless of the
  // sprite's internal byte order (a raw uint16_t read returns it byte-swapped). The begin-marker
  // lets the host sync past boot/log lines before the P6 header.
  void writePpm(Print& out) {
    const int w = canvas_.width();
    const int h = canvas_.height();
    if (w > kMaxRowPixels) {  // fail loud: a wider panel needs a bigger row buffer (§3).
      out.printf("\n--TP-PPM-ERROR-- width %d exceeds kMaxRowPixels %d\n", w, kMaxRowPixels);
      return;
    }
    lgfx::rgb888_t row[kMaxRowPixels];
    out.printf("\n--TP-PPM-BEGIN--\nP6\n%d %d\n255\n", w, h);
    for (int y = 0; y < h; ++y) {
      canvas_.readRect(0, y, w, 1, row);
      for (int x = 0; x < w; ++x) {
        const uint8_t rgb[3] = {row[x].r, row[x].g, row[x].b};  // PPM wants R,G,B in that order
        out.write(rgb, sizeof(rgb));
      }
    }
    out.print("\n--TP-PPM-END--\n");
  }
#endif  // TP_DEBUG_SCREENSHOT

 private:
#ifdef TP_DEBUG_SCREENSHOT
  // One scanline of RGB565, sized to the widest panel we target (the Cardputer and StickC are
  // 240 wide); writePpm fails loud above this rather than overflowing (ADR-0006).
  static constexpr int kMaxRowPixels = 320;
#endif

  lgfx::LGFX_Device& panel_;
  lgfx::LGFX_Sprite canvas_;
  // The two vendored text fonts wrapped for LovyanGFX (ADR-0012), one per TextRole. 7x13 is a
  // compact 13px body font; helvB14 is the ~18px bold headword font. Both drawn at native size.
  lgfx::U8g2font bodyFont_{u8g2_font_7x13_te};
  lgfx::U8g2font headerFont_{u8g2_font_helvB14_te};
  const uint8_t rotation_;
};

}  // namespace tp

#endif  // UNIT_TEST
