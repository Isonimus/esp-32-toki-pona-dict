// Host test for the pure HAL types (ADR-0001): the InputEvent vocabulary and that the
// DisplaySurface / InputSource seams are actually implementable. No hardware; plain g++.
#include <cstdio>

#include "display_surface.hpp"
#include "input_event.hpp"
#include "input_source.hpp"

using namespace tp;

static int failures = 0;

#define CHECK(cond)                                                        \
  do {                                                                     \
    if (!(cond)) {                                                         \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);          \
      ++failures;                                                          \
    }                                                                      \
  } while (0)

// A fake input source replaying a fixed script, proving InputSource is implementable and
// that poll() drains to none() when exhausted — the contract the app loop relies on.
class ScriptedInput final : public InputSource {
 public:
  ScriptedInput(const InputEvent* script, int count) : script_(script), count_(count) {}
  bool begin() override { return true; }
  InputEvent poll() override { return pos_ < count_ ? script_[pos_++] : InputEvent::none(); }

 private:
  const InputEvent* script_;
  int count_;
  int pos_ = 0;
};

// A fake display tallying calls, proving DisplaySurface is implementable with these exact
// signatures (a mismatch would fail to compile here, not later on the device).
class CountingDisplay final : public DisplaySurface {
 public:
  bool begin() override { return true; }
  uint16_t width() const override { return 240; }
  uint16_t height() const override { return 135; }
  void fillScreen(uint16_t) override { ++fills; }
  void drawText(int32_t, int32_t, const char*, uint16_t, TextRole) override { ++texts; }
  void drawGlyph1bpp(int32_t, int32_t, uint16_t, uint16_t, const uint8_t*, uint16_t) override {
    ++glyphs;
  }
  void present() override { ++presents; }
  int fills = 0, texts = 0, glyphs = 0, presents = 0;
};

int main() {
  // Factories set exactly the right kind and payload, and leave unused fields inert.
  CHECK(InputEvent::none().kind == InputKind::None);
  CHECK(InputEvent::none().isNone());

  const InputEvent typed = InputEvent::character('a');
  CHECK(typed.kind == InputKind::Char);
  CHECK(typed.ch == 'a');
  CHECK(!typed.isNone());

  const InputEvent up = InputEvent::navigate(NavDir::Up);
  CHECK(up.kind == InputKind::Nav);
  CHECK(up.nav == NavDir::Up);

  const InputEvent down = InputEvent::navigate(NavDir::Down);
  CHECK(down.kind == InputKind::Nav);
  CHECK(down.nav == NavDir::Down);

  CHECK(InputEvent::select().kind == InputKind::Select);
  CHECK(InputEvent::back().kind == InputKind::Back);

  // InputSource: the scripted events come back in order, then none() forever.
  const InputEvent script[] = {InputEvent::character('t'), InputEvent::navigate(NavDir::Down),
                               InputEvent::select()};
  ScriptedInput input(script, 3);
  CHECK(input.begin());
  CHECK(input.poll().ch == 't');
  CHECK(input.poll().nav == NavDir::Down);
  CHECK(input.poll().kind == InputKind::Select);
  CHECK(input.poll().isNone());
  CHECK(input.poll().isNone());

  // DisplaySurface: the seam is implementable and each primitive is reachable.
  CountingDisplay display;
  CHECK(display.begin());
  CHECK(display.width() == 240);
  display.fillScreen(0x0000);
  display.drawText(0, 0, "toki", 0xFFFF, TextRole::Body);
  const uint8_t glyphBits[] = {0xFF};
  display.drawGlyph1bpp(0, 0, 1, 1, glyphBits, 0xFFFF);
  display.present();
  CHECK(display.fills == 1 && display.texts == 1 && display.glyphs == 1 && display.presents == 1);

  if (failures == 0) {
    std::printf("PASS: HAL types\n");
    return 0;
  }
  std::printf("FAILED: %d check(s)\n", failures);
  return 1;
}
