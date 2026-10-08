#include "render.hpp"

#include <cstdio>
#include <cstring>

#include "search.hpp"
#include "sitelen.hpp"

namespace tp {
namespace {

// RGB565 palette. Named so the board bring-up can retune them in one place, never as bare
// hex scattered through the layout.
constexpr uint16_t kColorBackground = 0x0000;  // black
constexpr uint16_t kColorText = 0xFFFF;         // white
constexpr uint16_t kColorAccent = 0xFFE0;       // yellow — focused row, headword

// Layout metrics in pixels at text size 1. Tuned per board at bring-up; the values here are
// a sane default for a ~240x135 panel and do not affect host tests.
constexpr int kMargin = 4;
constexpr int kLineHeight = 16;
constexpr int kHeaderScale = 2;
constexpr int kBodyScale = 1;
constexpr int kGlossIndent = 8;  // senses are indented under their language label

void renderSearch(const UiState& state, DisplaySurface& display) {
  // Query line at the top; a leading '>' marks it as the edit field.
  char prompt[kMaxQueryLen + 2];
  prompt[0] = '>';
  std::strcpy(prompt + 1, state.query);
  display.drawText(kMargin, kMargin, prompt, kColorText, kBodyScale);

  // Result rows below, each the Toki Pona headword (the pivot, ADR-0004). The focused row
  // is drawn in the accent colour — no fill primitive needed on the seam.
  int y = kMargin + kLineHeight + kMargin;
  for (int i = 0; i < state.resultCount; ++i) {
    const uint16_t color = (i == state.selected) ? kColorAccent : kColorText;
    display.drawText(kMargin, y, entry(state.results[i]).tp, color, kBodyScale);
    y += kLineHeight;
  }
}

void renderDetail(const UiState& state, DisplaySurface& display) {
  const Entry& e = entry(state.detailEntry);

  // Headword, large.
  display.drawText(kMargin, kMargin, e.tp, kColorAccent, kHeaderScale);

  // The sitelen pona glyph (ADR-0003), blitted top-right as a detail accent. Absent for any
  // headword the font does not cover (sitelenGlyph -> nullptr), in which case we simply draw
  // no glyph — the text entry stands on its own.
  if (const uint8_t* glyph = sitelenGlyph(e.sitelen)) {
    const int32_t glyphX = display.width() - kSitelenWidth - kMargin;
    display.drawGlyph1bpp(glyphX, kMargin, kSitelenWidth, kSitelenHeight, glyph, kColorAccent);
  }

  // One language at a time (ADR-0007): the all-languages lines overflowed the panel width and
  // wrapped illegibly. The label doubles as a cycling affordance — `< English  1/5 >` — and
  // the glosses sit one sense per line, each short enough never to overflow the width.
  const int lang = state.detailLang;
  int y = kMargin + kLineHeight * kHeaderScale + kMargin;

  char label[32];
  const int langPosition = lang - static_cast<int>(Lang::Tp);  // En -> 1 ... Pt -> LangCount-1
  std::snprintf(label, sizeof(label), "< %s  %d/%d >", LANG_LABEL[lang], langPosition,
                LangCount - 1);
  display.drawText(kMargin, y, label, kColorAccent, kBodyScale);
  y += kLineHeight;

  for (const char* const* gloss = e.glossesByLang[lang]; *gloss != nullptr; ++gloss) {
    display.drawText(kMargin + kGlossIndent, y, *gloss, kColorText, kBodyScale);
    y += kLineHeight;
  }
}

}  // namespace

void render(const UiState& state, DisplaySurface& display) {
  display.fillScreen(kColorBackground);
  if (state.view == View::Detail) {
    renderDetail(state, display);
  } else {
    renderSearch(state, display);
  }
  display.present();
}

}  // namespace tp
