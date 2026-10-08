// Host test for the app layer (ADR-0004): the UI state machine's behaviour and that the
// renderer drives a DisplaySurface without crashing on either view. Plain g++, no hardware.
#include <cstdio>
#include <cstring>

#include "display_surface.hpp"
#include "render.hpp"
#include "search.hpp"
#include "ui_controller.hpp"
#include "ui_state.hpp"

using namespace tp;

static int failures = 0;

#define CHECK(cond)                                               \
  do {                                                            \
    if (!(cond)) {                                                \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
      ++failures;                                                 \
    }                                                             \
  } while (0)

// Feed a whole string as a run of Char events.
static void type(UiController& ui, const char* text) {
  for (const char* p = text; *p; ++p) ui.handle(InputEvent::character(*p));
}

// Whether any result row is the entry whose headword is `tp`.
static bool resultsContainHeadword(const UiController& ui, const char* tp) {
  const UiState& s = ui.state();
  for (int i = 0; i < s.resultCount; ++i) {
    if (std::strcmp(entry(s.results[i]).tp, tp) == 0) return true;
  }
  return false;
}

// A display that only tallies calls, to prove the renderer's wiring on host.
class FakeDisplay final : public DisplaySurface {
 public:
  bool begin() override { return true; }
  uint16_t width() const override { return 240; }
  uint16_t height() const override { return 135; }
  void fillScreen(uint16_t) override { ++fills; }
  void drawText(int32_t, int32_t, const char*, uint16_t, uint8_t) override { ++texts; }
  void drawGlyph1bpp(int32_t, int32_t, uint16_t, uint16_t, const uint8_t*, uint16_t) override {
    ++glyphs;
  }
  void present() override { ++presents; }
  int fills = 0, texts = 0, presents = 0, glyphs = 0;
};

int main() {
  // Fresh state: Search view, empty query, no results.
  {
    UiController ui;
    CHECK(ui.state().view == View::Search);
    CHECK(ui.state().queryLen == 0);
    CHECK(ui.state().resultCount == 0);
    // None and Nav/Select with no results are all inert.
    ui.handle(InputEvent::none());
    ui.handle(InputEvent::navigate(NavDir::Down));
    ui.handle(InputEvent::select());
    CHECK(ui.state().view == View::Search);
    CHECK(ui.state().resultCount == 0);
  }

  // Typing a Toki Pona headword finds it; Select opens Detail; Back returns, query intact.
  {
    UiController ui;
    type(ui, "telo");
    CHECK(std::strcmp(ui.state().query, "telo") == 0);
    CHECK(ui.state().resultCount > 0);
    CHECK(resultsContainHeadword(ui, "telo"));
    CHECK(ui.state().selected == 0);

    ui.handle(InputEvent::select());
    CHECK(ui.state().view == View::Detail);
    CHECK(std::strcmp(entry(ui.state().detailEntry).tp, "telo") == 0);

    // Detail ignores typing; only Back leaves it.
    ui.handle(InputEvent::character('x'));
    CHECK(ui.state().view == View::Detail);
    ui.handle(InputEvent::back());
    CHECK(ui.state().view == View::Search);
    CHECK(std::strcmp(ui.state().query, "telo") == 0);
    CHECK(resultsContainHeadword(ui, "telo"));
  }

  // Detail shows one language at a time, opening on the first cycle language; Nav cycles it
  // (wrapping both ways), and each fresh open resets to the first language (ADR-0007).
  {
    UiController ui;
    type(ui, "telo");
    ui.handle(InputEvent::select());
    CHECK(ui.state().view == View::Detail);
    CHECK(ui.state().detailLang == static_cast<uint8_t>(Lang::En));

    ui.handle(InputEvent::navigate(NavDir::Down));
    CHECK(ui.state().detailLang == static_cast<uint8_t>(Lang::Es));
    ui.handle(InputEvent::navigate(NavDir::Down));
    ui.handle(InputEvent::navigate(NavDir::Down));
    ui.handle(InputEvent::navigate(NavDir::Down));
    CHECK(ui.state().detailLang == static_cast<uint8_t>(Lang::Pt));
    ui.handle(InputEvent::navigate(NavDir::Down));
    CHECK(ui.state().detailLang == static_cast<uint8_t>(Lang::En));  // wrapped past the end

    ui.handle(InputEvent::navigate(NavDir::Up));
    CHECK(ui.state().detailLang == static_cast<uint8_t>(Lang::Pt));  // wrapped past the start

    // A fresh open resets to the first language (not "remember last"); Back still leaves.
    ui.handle(InputEvent::back());
    CHECK(ui.state().view == View::Search);
    ui.handle(InputEvent::select());
    CHECK(ui.state().detailLang == static_cast<uint8_t>(Lang::En));
  }

  // Symmetric lookup: an English gloss finds the same Toki Pona entry (ADR-0004).
  {
    UiController ui;
    type(ui, "water");
    CHECK(resultsContainHeadword(ui, "telo"));
  }

  // Diacritic-insensitive: "agua" finds "água"→telo (folding, ADR-0002).
  {
    UiController ui;
    type(ui, "agua");
    CHECK(resultsContainHeadword(ui, "telo"));
  }

  // Merged results are deduplicated by entry id.
  {
    UiController ui;
    type(ui, "a");
    const UiState& s = ui.state();
    for (int i = 0; i < s.resultCount; ++i) {
      for (int j = i + 1; j < s.resultCount; ++j) CHECK(s.results[i] != s.results[j]);
    }
  }

  // Backspace edits the query and re-runs the search.
  {
    UiController ui;
    type(ui, "telo");
    ui.handle(InputEvent::back());
    CHECK(std::strcmp(ui.state().query, "tel") == 0);
    CHECK(ui.state().resultCount > 0);  // "tel" still prefixes "telo"
    // Back on an empty query is inert and never leaves the root view.
    UiController empty;
    empty.handle(InputEvent::back());
    CHECK(empty.state().view == View::Search);
    CHECK(empty.state().queryLen == 0);
  }

  // Nav clamps at both ends, never wrapping.
  {
    UiController ui;
    type(ui, "a");
    CHECK(ui.state().resultCount > 1);
    ui.handle(InputEvent::navigate(NavDir::Up));  // already at 0
    CHECK(ui.state().selected == 0);
    for (int i = 0; i < kMaxResults + 5; ++i) ui.handle(InputEvent::navigate(NavDir::Down));
    CHECK(ui.state().selected == ui.state().resultCount - 1);  // clamped at the last row
    ui.handle(InputEvent::navigate(NavDir::Down));
    CHECK(ui.state().selected == ui.state().resultCount - 1);
  }

  // Query length is bounded; typing past the cap is dropped, not an overflow.
  {
    UiController ui;
    for (int i = 0; i < kMaxQueryLen + 10; ++i) ui.handle(InputEvent::character('a'));
    CHECK(ui.state().queryLen == kMaxQueryLen);
  }

  // Renderer: both views issue draws against the seam without crashing.
  {
    UiController ui;
    type(ui, "telo");
    FakeDisplay display;
    render(ui.state(), display);
    CHECK(display.fills == 1 && display.presents == 1);
    CHECK(display.texts >= 2);  // query line + at least one result row
    CHECK(display.glyphs == 0);  // the search view draws no sitelen glyph

    ui.handle(InputEvent::select());
    FakeDisplay detail;
    render(ui.state(), detail);
    CHECK(detail.fills == 1 && detail.presents == 1);
    CHECK(detail.texts >= 2);   // headword + gloss lines
    CHECK(detail.glyphs == 1);  // telo's sitelen glyph is blitted into the detail accent (ADR-0003)
  }

  if (failures == 0) {
    std::printf("PASS: UI state machine\n");
    return 0;
  }
  std::printf("FAILED: %d check(s)\n", failures);
  return 1;
}
