#pragma once

// Explicit LovyanGFX device config for the M5StickC Plus2 panel (ADR-0005, ADR-0020). Device-only:
// pulls LovyanGFX and is never compiled on the host lane. The board is the keyboard-less 3-button
// browse target (ADR-0020); this file is only its display surface.
//
// The sapper (../esp32-wifi-sapper), whose Cardputer config this project reused verbatim, has no
// StickC Plus2 config, so the pins, offsets, inversion and backlight here are M5GFX's own
// board_M5StickCPlus2 values (m5stack/M5GFX, src/M5GFX.cpp) — the authoritative M5 source — and are
// verified at the bring-up proof, the same contract ADR-0005 set for the Cardputer: a wrong pin
// fails the on-panel proof, not a host test. As on the Cardputer, the backlight only lights after
// setBrightness() post-init (handled by LgfxDisplaySurface::begin).

#ifndef UNIT_TEST  // device-only: LovyanGFX is not available on the host lane.

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

namespace tp {

class LGFX_StickCPlus2 : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 panel_;
  lgfx::Bus_SPI bus_;
  lgfx::Light_PWM light_;

 public:
  LGFX_StickCPlus2() {
    {
      auto cfg = bus_.config();
      cfg.spi_host = SPI2_HOST;  // M5GFX board_M5StickCPlus2 (the Cardputer uses SPI3)
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read = 15000000;
      cfg.spi_3wire = true;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = 13;  // display SCLK
      cfg.pin_mosi = 15;  // display MOSI
      cfg.pin_miso = -1;  // no MISO (panel is write-only here)
      cfg.pin_dc = 14;    // LCD_DC
      bus_.config(cfg);
      panel_.setBus(&bus_);
    }
    {
      auto cfg = panel_.config();
      cfg.pin_cs = 5;         // LCD_CS
      cfg.pin_rst = 12;       // LCD_RST (M5GFX overrides the generic Plus default of 18)
      cfg.pin_busy = -1;
      cfg.panel_width = 135;  // physical portrait; rotation 1 presents 240x135 landscape (main/)
      cfg.panel_height = 240;
      cfg.offset_x = 52;      // M5GFX board_M5StickCPlus2
      cfg.offset_y = 40;
      cfg.offset_rotation = 0;
      cfg.readable = false;
      cfg.invert = true;      // ST7789 on this panel is inverted
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;
      panel_.config(cfg);
    }
    {
      auto cfg = light_.config();
      cfg.pin_bl = 27;  // LCD_BL
      cfg.invert = false;
      cfg.freq = 256;      // M5GFX board_M5StickCPlus2 backlight PWM
      cfg.pwm_channel = 7;
      light_.config(cfg);
    }
    panel_.setLight(&light_);
    setPanel(&panel_);
  }
};

}  // namespace tp

#endif  // UNIT_TEST
