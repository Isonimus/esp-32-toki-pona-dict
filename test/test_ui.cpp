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

// A controller advanced past the boot splash to the search root (ADR-0009) — the steady state
// the pre-splash behaviour tests are written against.
static UiController atSearch() {
  UiController ui;
  ui.dismissSplash();
  return ui;
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
  void drawText(int32_t, int32_t, const char*, uint16_t, TextRole) override { ++texts; }
  void drawGlyph1bpp(int32_t, int32_t, uint16_t, uint16_t, const uint8_t*, uint16_t) override {
    ++glyphs;
  }
  void present() override { ++presents; }
  int fills = 0, texts = 0, presents = 0, glyphs = 0;
};

int main() {
  // Boot shows the splash; the first real input dismisses it to Search and is swallowed (not
  // typed); None leaves it up; dismissSplash() is the timeout path main/ drives (ADR-0009).
  {
    UiController ui;
    CHECK(ui.state().view == View::Splash);
    ui.handle(InputEvent::none());
    CHECK(ui.state().view == View::Splash);  // a bare poll must not skip the splash
    ui.handle(InputEvent::character('x'));
    CHECK(ui.state().view == View::Search);
    CHECK(ui.state().queryLen == 0);  // the dismissing key was swallowed, not typed into the query

    UiController timed;
    timed.dismissSplash();
    CHECK(timed.state().view == View::Search);  // the timed path, with no input at all
    timed.dismissSplash();                      // idempotent once past the splash
    CHECK(timed.state().view == View::Search);
  }

  // Fresh search root: empty query, no results; None and Nav/Select with no results are inert.
  {
    UiController ui = atSearch();
    CHECK(ui.state().view == View::Search);
    CHECK(ui.state().queryLen == 0);
    CHECK(ui.state().resultCount == 0);
    ui.handle(InputEvent::none());
    ui.handle(InputEvent::navigate(NavDir::Down));
    ui.handle(InputEvent::select());
    CHECK(ui.state().view == View::Search);
    CHECK(ui.state().resultCount == 0);
  }

  // About colophon (ADR-0009): Back from the empty search root opens it (ADR-0004 left that
  // inert); typing and Nav are inert while it is up; Back returns to Search. A non-empty query
  // keeps Back as backspace, never opening About.
  {
    UiController ui = atSearch();
    ui.handle(InputEvent::back());
    CHECK(ui.state().view == View::About);
    ui.handle(InputEvent::character('a'));
    ui.handle(InputEvent::navigate(NavDir::Down));
    CHECK(ui.state().view == View::About);
    CHECK(ui.state().queryLen == 0);  // About swallows typing — the query stays empty
    ui.handle(InputEvent::back());
    CHECK(ui.state().view == View::Search);

    UiController typed = atSearch();
    typed.handle(InputEvent::character('t'));
    typed.handle(InputEvent::back());  // backspace, not About
    CHECK(typed.state().view == View::Search);
    CHECK(typed.state().queryLen == 0);
  }

  // Typing a Toki Pona headword finds it; Select opens Detail; Back returns, query intact.
  {
    UiController ui = atSearch();
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
    UiController ui = atSearch();
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
    CHECK(ui.state().detailLang == static_cast<uint8_t>(Lang::Zh));  // display-only still cycles (ADR-0017)
    ui.handle(InputEvent::navigate(NavDir::Down));
    CHECK(ui.state().detailLang == static_cast<uint8_t>(Lang::En));  // wrapped past the end

    ui.handle(InputEvent::navigate(NavDir::Up));
    CHECK(ui.state().detailLang == static_cast<uint8_t>(Lang::Zh));  // wrapped past the start

    // A fresh open resets to the first language (not "remember last"); Back still leaves.
    ui.handle(InputEvent::back());
    CHECK(ui.state().view == View::Search);
    ui.handle(InputEvent::select());
    CHECK(ui.state().detailLang == static_cast<uint8_t>(Lang::En));
  }

  // Symmetric lookup: an English gloss finds the same Toki Pona entry (ADR-0004).
  {
    UiController ui = atSearch();
    type(ui, "water");
    CHECK(resultsContainHeadword(ui, "telo"));
  }

  // Diacritic-insensitive: "agua" finds "água"→telo (folding, ADR-0002).
  {
    UiController ui = atSearch();
    type(ui, "agua");
    CHECK(resultsContainHeadword(ui, "telo"));
  }

  // Merged results are deduplicated by entry id.
  {
    UiController ui = atSearch();
    type(ui, "a");
    const UiState& s = ui.state();
    for (int i = 0; i < s.resultCount; ++i) {
      for (int j = i + 1; j < s.resultCount; ++j) CHECK(s.results[i] != s.results[j]);
    }
  }

  // Backspace edits the query and re-runs the search. (Back on an *empty* query opens About,
  // ADR-0009 — covered in the About block above, not repeated here.)
  {
    UiController ui = atSearch();
    type(ui, "telo");
    ui.handle(InputEvent::back());
    CHECK(std::strcmp(ui.state().query, "tel") == 0);
    CHECK(ui.state().resultCount > 0);  // "tel" still prefixes "telo"
  }

  // The result list is a ring: Nav wraps at both ends (ADR-0020, superseding ADR-0004's clamp),
  // so a one-Nav-button board can traverse it with Down alone.
  {
    UiController ui = atSearch();
    type(ui, "a");
    const int count = ui.state().resultCount;
    CHECK(count > 1);
    CHECK(ui.state().selected == 0);
    ui.handle(InputEvent::navigate(NavDir::Up));  // from the first row, wrap to the last
    CHECK(ui.state().selected == count - 1);
    ui.handle(InputEvent::navigate(NavDir::Down));  // from the last row, wrap to the first
    CHECK(ui.state().selected == 0);
    for (int i = 0; i < count; ++i) ui.handle(InputEvent::navigate(NavDir::Down));
    CHECK(ui.state().selected == 0);  // a full lap of Downs returns to the start
  }

  // A Buttons board (StickC Plus2, ADR-0020) browses the whole corpus from the empty root: the
  // list is every entry, Nav wraps over it with Down alone, and Select opens the focused entry.
  // A Keyboard board's empty root browses nothing (it shows the ADR-0010 prompt instead).
  {
    UiController keyboard = atSearch();  // default style
    CHECK(keyboard.state().queryLen == 0);
    CHECK(listCount(keyboard.state()) == 0);  // no browse without a query on a keyboard board

    UiController buttons(InputStyle::Buttons);
    buttons.dismissSplash();
    CHECK(buttons.state().queryLen == 0);
    CHECK(listCount(buttons.state()) == static_cast<int>(ENTRY_COUNT));  // the whole corpus
    CHECK(buttons.state().selected == 0);

    // A browse row's index is its entry id, so Select on row 0 opens entry 0.
    buttons.handle(InputEvent::navigate(NavDir::Up));  // Down-only board still reaches the end: wrap
    CHECK(buttons.state().selected == static_cast<int>(ENTRY_COUNT) - 1);
    buttons.handle(InputEvent::navigate(NavDir::Down));  // and wraps back to the first
    CHECK(buttons.state().selected == 0);
    buttons.handle(InputEvent::navigate(NavDir::Down));
    CHECK(buttons.state().selected == 1);
    buttons.handle(InputEvent::select());
    CHECK(buttons.state().view == View::Detail);
    CHECK(buttons.state().detailEntry == entryIdAt(buttons.state(), 1));
    CHECK(buttons.state().detailEntry == 1);
    buttons.handle(InputEvent::back());  // Back returns to the browse list, not About
    CHECK(buttons.state().view == View::Search);
  }

  // Renderer: a Buttons board's empty root draws the windowed browse list — headword rows, no
  // query '>' line and no prompt glyphs (ADR-0020) — filling the panel height. The window is
  // capped at the rows that fit, so the 137-entry corpus draws a bounded handful, not all of them.
  {
    UiController buttons(InputStyle::Buttons);
    buttons.dismissSplash();
    FakeDisplay browse;
    render(buttons.state(), browse);
    CHECK(browse.fills == 1 && browse.presents == 1);
    CHECK(browse.glyphs == 0);  // browse lists headword text, no sitelen prompt
    const int capacity = (135 - 4) / 16;  // panel height / line height from listTop = kMargin
    CHECK(browse.texts == capacity);  // exactly the window's worth of rows, not ENTRY_COUNT
    CHECK(static_cast<int>(ENTRY_COUNT) > capacity);  // the cap actually bit
  }

  // Query length is bounded; typing past the cap is dropped, not an overflow.
  {
    UiController ui = atSearch();
    for (int i = 0; i < kMaxQueryLen + 10; ++i) ui.handle(InputEvent::character('a'));
    CHECK(ui.state().queryLen == kMaxQueryLen);
  }

  // Renderer: every view issues draws against the seam without crashing.
  {
    UiController ui = atSearch();
    type(ui, "telo");
    FakeDisplay display;
    render(ui.state(), display);
    CHECK(display.fills == 1 && display.presents == 1);
    CHECK(display.texts >= 2);  // query line + at least one result row
    CHECK(display.glyphs == 0);  // a non-empty search list draws no sitelen glyph (ADR-0010)

    ui.handle(InputEvent::select());
    FakeDisplay detail;
    render(ui.state(), detail);
    CHECK(detail.fills == 1 && detail.presents == 1);
    CHECK(detail.texts >= 2);   // headword + gloss lines
    CHECK(detail.glyphs == 1);  // telo's sitelen glyph is blitted into the detail accent (ADR-0003)
  }

  // Renderer: a non-empty query that matches nothing draws the no-results affordance (ADR-0019) —
  // the grey `ala` icon plus the `nimi li weka` sentence, four glyphs — in place of the result
  // list, with the '>' query line still the only text. Before ADR-0019 the area fell blank (zero
  // glyphs); the empty-root prompt and a query with results are asserted separately below/above.
  {
    UiController ui = atSearch();
    type(ui, "zzzz");  // normalizes intact, prefixes/matches no gloss in any language
    CHECK(ui.state().queryLen > 0);
    CHECK(ui.state().resultCount == 0);
    FakeDisplay noResults;
    render(ui.state(), noResults);
    CHECK(noResults.fills == 1 && noResults.presents == 1);
    CHECK(noResults.texts == 1);   // only the '>' query line
    CHECK(noResults.glyphs == 4);  // 1 ala icon + 3 sentence glyphs (nimi li weka)
  }

  // Renderer: a display-only language's detail (ADR-0017/0018) draws its label and glosses as CJK
  // cells, not Latin text — only the Toki Pona headword stays a drawText. Cycling the detail to
  // Chinese moves the label + glosses from text to glyphs, the observable mark of the CJK path.
  {
    UiController ui = atSearch();
    type(ui, "telo");
    ui.handle(InputEvent::select());
    for (int i = 0; i < 5; ++i) ui.handle(InputEvent::navigate(NavDir::Down));  // En -> ... -> Zh
    CHECK(ui.state().detailLang == static_cast<uint8_t>(Lang::Zh));
    FakeDisplay zh;
    render(ui.state(), zh);
    CHECK(zh.fills == 1 && zh.presents == 1);
    CHECK(zh.texts == 1);    // only the Toki Pona headword draws as Latin text
    CHECK(zh.glyphs >= 3);   // sitelen accent + CJK cells for the label and the gloss
  }

  // Renderer: the empty search root draws the type-to-search prompt — the lukin icon plus the
  // "o alasa e nimi" sentence, five glyphs in all (ADR-0010). It replaces the bare screen only
  // while the query is empty; a non-empty list draws no glyph (asserted above). The '>' edit line
  // is suppressed on the empty root (ADR-0014), so the prompt draws no text at all.
  {
    UiController ui = atSearch();
    CHECK(ui.state().queryLen == 0);
    FakeDisplay display;
    render(ui.state(), display);
    CHECK(display.fills == 1 && display.presents == 1);
    CHECK(display.glyphs == 5);  // 1 lukin icon + 4 sentence glyphs
    CHECK(display.texts == 0);   // no '>' line and no result rows while the query is empty
  }

  // Renderer: the splash draws the single merged toki pona logo and no text (ADR-0015) — the
  // font credit moved off the opening screen to the About colophon. About draws the two-glyph
  // wordmark plus its condensed courtesy-credit lines (ADR-0009, ADR-0016).
  {
    UiController splashUi;  // fresh = boot splash
    CHECK(splashUi.state().view == View::Splash);
    FakeDisplay splash;
    render(splashUi.state(), splash);
    CHECK(splash.fills == 1 && splash.presents == 1);
    CHECK(splash.glyphs == 1);  // the merged logo is one 1-bpp blit, not two wordmark glyphs
    CHECK(splash.texts == 0);   // no font credit on the splash anymore (it lives in About)

    UiController aboutUi = atSearch();
    aboutUi.handle(InputEvent::back());  // empty query -> About
    CHECK(aboutUi.state().view == View::About);
    FakeDisplay about;
    render(aboutUi.state(), about);
    CHECK(about.fills == 1 && about.presents == 1);
    CHECK(about.glyphs == 2);  // same wordmark
    CHECK(about.texts >= 3);   // sitelen credit + gloss (lipu Linku) credit + notices pointer
  }

  if (failures == 0) {
    std::printf("PASS: UI state machine\n");
    return 0;
  }
  std::printf("FAILED: %d check(s)\n", failures);
  return 1;
}
