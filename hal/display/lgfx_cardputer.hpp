#pragma once

// Explicit LovyanGFX device config for the Cardputer panel (ADR-0005). Device-only: pulls
// LovyanGFX and is never compiled on the host lane. Both Cardputer variants (original and ADV)
// share this exact panel, so one class serves both.
//
// The panel driver, pins, offsets, inversion and RGB order are the proven M5CardputerADV
// configuration (reused verbatim from ../esp32-wifi-sapper, where they were confirmed on the
// physical ADV panel). We drive the panel through LovyanGFX directly, not M5GFX (ADR-0005).
// Note: the backlight must be switched on with setBrightness() after init() — a panel that
// inits fine still shows black until then (handled by LgfxDisplaySurface::begin).

#ifndef UNIT_TEST  // device-only: LovyanGFX is not available on the host lane.

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

namespace tp {

class LGFX_Cardputer : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 panel_;
  lgfx::Bus_SPI bus_;
  lgfx::Light_PWM light_;

 public:
  LGFX_Cardputer() {
    {
      auto cfg = bus_.config();
      cfg.spi_host = SPI3_HOST;  // proven M5CardputerADV config
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read = 16000000;
      cfg.spi_3wire = true;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = 36;  // display SCLK
      cfg.pin_mosi = 35;  // display MOSI
      cfg.pin_miso = -1;
      cfg.pin_dc = 34;    // LCD_DC
      bus_.config(cfg);
      panel_.setBus(&bus_);
    }
    {
      auto cfg = panel_.config();
      cfg.pin_cs = 37;        // LCD_CS
      cfg.pin_rst = 33;       // LCD_RST
      cfg.pin_busy = -1;
      cfg.panel_width = 135;  // physical portrait; rotation 1 presents 240x135
      cfg.panel_height = 240;
      cfg.offset_x = 52;      // confirmed on hardware (sapper slice-0005)
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
      cfg.pin_bl = 38;  // LCD_BL
      cfg.invert = false;
      cfg.freq = 44100;
      cfg.pwm_channel = 7;
      light_.config(cfg);
    }
    panel_.setLight(&light_);
    setPanel(&panel_);
  }
};

}  // namespace tp

#endif  // UNIT_TEST
