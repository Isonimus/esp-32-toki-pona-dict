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
// A keyboard board: the empty root teaches type-to-search (ADR-0010/0020).
constexpr tp::InputStyle kInputStyle = tp::InputStyle::Keyboard;
}  // namespace

#elif defined(TP_BOARD_STICKC_PLUS2)
#include "display/lgfx_stickc_plus2.hpp"
#include "display_lgfx.hpp"
#include "input/stickc_buttons.hpp"

namespace {
// The StickC Plus2's 135x240 panel driven as 240x135 landscape (LovyanGFX rotation 1), reusing the
// Cardputer's already-proven layout exactly — ADR-0007's one-sense-per-line is tuned for 240px.
constexpr uint8_t kStickCRotation = 1;
tp::LGFX_StickCPlus2 gPanel;
tp::LgfxDisplaySurface gDisplay(gPanel, kStickCRotation);
tp::StickCButtons gInput;
// A keyboard-less board: the empty root browses the whole corpus (ADR-0020).
constexpr tp::InputStyle kInputStyle = tp::InputStyle::Buttons;
// GPIO4 (HOLD) must be driven HIGH early in setup() or the Stick powers itself off when running on
// battery (M5StickC Plus2 power latch). Harmless over USB, essential off USB.
constexpr int kPowerHoldPin = 4;
}  // namespace

#else
#error "No TP_BOARD_* selected — define one in platformio.ini build_flags (ADR-0005)."
#endif

namespace {
tp::UiController gUi(kInputStyle);

// How long the boot splash (ADR-0009) stays up before auto-dismissing to the search view. The
// controller is clockless (ADR-0004), so main/ owns this timeout; a keypress dismisses sooner
// via handle(). Tuned at bring-up; long enough to read the wordmark, short enough not to annoy.
constexpr uint32_t kSplashHoldMs = 1500;
uint32_t gSplashShownAtMs = 0;

#ifdef TP_DEBUG_SCREENSHOT
// The host (tools/fb-capture.py) sends this one byte to request a framebuffer dump (ADR-0006).
// DLE: not something a serial monitor emits by accident, and serial RX is otherwise unused
// here (keys arrive on the TCA8418 I2C bus), so a stray match would at worst emit a frame.
constexpr uint8_t kScreenshotCommand = 0x10;
#endif
}  // namespace

void setup() {
#ifdef TP_BOARD_STICKC_PLUS2
  // Latch power on before anything else (ADR-0020): on battery the Stick cuts power unless HOLD is
  // held HIGH through startup.
  pinMode(kPowerHoldPin, OUTPUT);
  digitalWrite(kPowerHoldPin, HIGH);
#endif

  Serial.begin(115200);

  // Fail loud on a dead surface (§3): a dictionary is its screen and its input, so a failure is
  // announced on the serial console rather than swallowed. begin() logs the specific cause.
  if (!gDisplay.begin()) Serial.println("[FATAL] display init failed");
  if (!gInput.begin()) Serial.println("[FATAL] input init failed");

  gSplashShownAtMs = millis();
  tp::render(gUi.state(), gDisplay);  // draw the boot splash once (ADR-0009); loop() dismisses it.
}

void loop() {
#ifdef TP_DEBUG_SCREENSHOT
  // Service a screenshot request before input so a capture reflects the current frame (ADR-0006).
  if (Serial.available() && Serial.read() == kScreenshotCommand) gDisplay.writePpm(Serial);
#endif

  // Auto-dismiss the boot splash once its hold elapses (ADR-0009): the controller owns the
  // transition, main/ owns the clock. Checking the view (not the time alone) means a keypress
  // that dismissed the splash early below is not re-triggered here. Unsigned subtraction is
  // millis()-rollover safe.
  if (gUi.state().view == tp::View::Splash && millis() - gSplashShownAtMs >= kSplashHoldMs) {
    gUi.dismissSplash();
    tp::render(gUi.state(), gDisplay);
  }

  // Redraw only when an event actually changed the state — no busy repaint, no tearing.
  const tp::InputEvent event = gInput.poll();
  if (!event.isNone()) {
    gUi.handle(event);
    tp::render(gUi.state(), gDisplay);
  }
}
