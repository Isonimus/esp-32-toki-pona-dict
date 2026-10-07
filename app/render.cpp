#include "render.hpp"

#include <cstdio>
#include <cstring>

#include "search.hpp"

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
constexpr int kLineHeight = 12;
constexpr int kHeaderScale = 2;
constexpr int kBodyScale = 1;

// Join a nullptr-terminated gloss list into `out` (NUL-terminated, at most cap-1 chars),
// senses separated by ", ". Truncates to fit — the detail line is display, not data.
void joinGlosses(const char* const* glosses, char* out, int cap) {
  int len = 0;
  for (int i = 0; glosses[i] != nullptr && len < cap - 1; ++i) {
    if (i > 0 && len < cap - 1) {
      const char* sep = ", ";
      for (int s = 0; sep[s] != '\0' && len < cap - 1; ++s) out[len++] = sep[s];
    }
    for (const char* p = glosses[i]; *p != '\0' && len < cap - 1; ++p) out[len++] = *p;
  }
  out[len] = '\0';
}

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

  // Headword, large. The sitelen glyph is blitted beside it once the rasterised glyph table
  // (core/sitelen_gen) lands — the seam and ADR-0003 reserve that; LEDGER tracks the slice.
  display.drawText(kMargin, kMargin, e.tp, kColorAccent, kHeaderScale);

  // One line per gloss language (Toki Pona is the headword itself, so it is skipped here).
  int y = kMargin + kLineHeight * kHeaderScale + kMargin;
  char line[96];
  for (int lang = 0; lang < LangCount; ++lang) {
    if (static_cast<Lang>(lang) == Lang::Tp) continue;
    char glosses[80];
    joinGlosses(e.glossesByLang[lang], glosses, sizeof(glosses));
    std::snprintf(line, sizeof(line), "%s: %s", LANG_LABEL[lang], glosses);
    display.drawText(kMargin, y, line, kColorText, kBodyScale);
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
