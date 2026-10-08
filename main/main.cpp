// Per-board wiring (ADR-0001 main/ layer): instantiate the board's concrete HAL drivers, bind
// them to the one app, and run the poll -> handle -> render loop. Device-only; the board is
// chosen by a single -DTP_BOARD_* flag, and a build with none is a hard error (ADR-0005).

#include <Arduino.h>

#include "render.hpp"
#include "ui_controller.hpp"

#if defined(TP_BOARD_CARDPUTER)
#include "display/lgfx_cardputer.hpp"
#include "display_lgfx.hpp"
#include "input/cardputer_keyboard.hpp"

namespace {
// The Cardputer's 240x135 landscape is LovyanGFX rotation 1.
constexpr uint8_t kCardputerRotation = 1;
tp::LGFX_Cardputer gPanel;
tp::LgfxDisplaySurface gDisplay(gPanel, kCardputerRotation);
tp::CardputerKeyboard gInput;
}  // namespace

#else
#error "No TP_BOARD_* selected — define one in platformio.ini build_flags (ADR-0005)."
#endif

namespace {
tp::UiController gUi;

#ifdef TP_DEBUG_SCREENSHOT
// The host (tools/fb-capture.py) sends this one byte to request a framebuffer dump (ADR-0006).
// DLE: not something a serial monitor emits by accident, and serial RX is otherwise unused
// here (keys arrive on the TCA8418 I2C bus), so a stray match would at worst emit a frame.
constexpr uint8_t kScreenshotCommand = 0x10;
#endif
}  // namespace

void setup() {
  Serial.begin(115200);

  // Fail loud on a dead surface (§3): a dictionary is its screen and its input, so a failure is
  // announced on the serial console rather than swallowed. begin() logs the specific cause.
  if (!gDisplay.begin()) Serial.println("[FATAL] display init failed");
  if (!gInput.begin()) Serial.println("[FATAL] input init failed");

  tp::render(gUi.state(), gDisplay);  // draw the empty search view once at boot.
}

void loop() {
#ifdef TP_DEBUG_SCREENSHOT
  // Service a screenshot request before input so a capture reflects the current frame (ADR-0006).
  if (Serial.available() && Serial.read() == kScreenshotCommand) gDisplay.writePpm(Serial);
#endif

  // Redraw only when an event actually changed the state — no busy repaint, no tearing.
  const tp::InputEvent event = gInput.poll();
  if (!event.isNone()) {
    gUi.handle(event);
    tp::render(gUi.state(), gDisplay);
  }
}
