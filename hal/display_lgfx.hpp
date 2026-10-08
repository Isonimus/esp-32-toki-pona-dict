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

  void drawText(int32_t x, int32_t y, const char* text, uint16_t color565, uint8_t size) override {
    canvas_.setTextColor(color565);
    canvas_.setTextSize(size);
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

 private:
  lgfx::LGFX_Device& panel_;
  lgfx::LGFX_Sprite canvas_;
  const uint8_t rotation_;
};

}  // namespace tp

#endif  // UNIT_TEST
