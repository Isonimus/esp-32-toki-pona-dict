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
constexpr int kWordmarkGap = 8;  // pixels between the two wordmark glyphs

// On-device colophon credit (ADR-0009). The legally-operative verbatim notice lives in
// THIRD_PARTY_NOTICES.md (shipped with the firmware, enforced by tools/check-notices.mjs);
// these lines are a human-readable courtesy credit, hand-kept in sync with that file's
// attribution. "(c)" is ASCII on purpose — the built-in font has no "©" glyph.
constexpr const char* kSplashCredit = "nasin nanpa";
constexpr const char* kAboutFontCredit = "sitelen pona: nasin nanpa";
constexpr const char* kAboutCopyright = "(c) 2022 jan Itan - MIT";
constexpr const char* kAboutNoticeLead = "Full notices:";
constexpr const char* kAboutNoticeFile = "THIRD_PARTY_NOTICES.md";

// Empty-search-root prompt (ADR-0010): a "look/search" icon above a one-line sitelen sentence,
// teaching type-to-search. Both reuse glyphs already in the bundled font (ADR-0003), so the
// prompt needs no new asset. `alasa` ("hunt/seek/search") names the action, complementing the
// `lukin` ("see/look") icon rather than restating it.
constexpr const char* kPromptIcon[] = {"lukin"};
constexpr const char* kPromptSentence[] = {"o", "alasa", "e", "nimi"};
constexpr int kPromptIconGlyphs = 1;
constexpr int kPromptSentenceGlyphs = 4;

// Draw a row of sitelen glyphs centred horizontally across the display at `top`, each
// kSitelenWidth wide, separated by kWordmarkGap. A glyph the font lacks is skipped but still
// reserves its slot (the ADR-0003 degradation contract), so the row stays aligned rather than
// crashing. Shared by the wordmark (ADR-0009) and the empty-root prompt (ADR-0010).
void drawGlyphRowCentered(DisplaySurface& display, int top, const char* const* names, int count,
                          uint16_t color) {
  const int rowWidth = count * kSitelenWidth + (count - 1) * kWordmarkGap;
  int x = (static_cast<int>(display.width()) - rowWidth) / 2;
  for (int i = 0; i < count; ++i) {
    if (const uint8_t* glyph = sitelenGlyph(names[i])) {
      display.drawGlyph1bpp(x, top, kSitelenWidth, kSitelenHeight, glyph, color);
    }
    x += kSitelenWidth + kWordmarkGap;
  }
}

// The "toki pona" wordmark: the product's name in its own script, two sitelen glyphs centred at
// `top`. Shared by the splash and the About colophon (ADR-0009). Returns the y just below the
// wordmark for the caller's text.
int drawWordmark(DisplaySurface& display, int top) {
  static const char* const kWordmark[] = {"toki", "pona"};
  drawGlyphRowCentered(display, top, kWordmark, 2, kColorAccent);
  return top + kSitelenHeight + kMargin;
}

void renderSplash(DisplaySurface& display) {
  // Wordmark roughly centred vertically, with the font credit beneath (ADR-0009). Text is
  // left-aligned at the margin like every other view: the seam exposes no text-width metric to
  // centre against, and the layout constants are retuned per board at bring-up anyway.
  const int top = (static_cast<int>(display.height()) - kSitelenHeight) / 2 - kLineHeight;
  const int creditY = drawWordmark(display, top);
  display.drawText(kMargin, creditY, kSplashCredit, kColorText, kBodyScale);
}

void renderAbout(DisplaySurface& display) {
  // Condensed colophon (ADR-0009): the wordmark, then the third-party font credit. The verbatim
  // MIT text is intentionally absent — it ships in THIRD_PARTY_NOTICES.md (ADR-0003); this view
  // is a human-readable courtesy credit, not the legal notice.
  int y = drawWordmark(display, kMargin);
  display.drawText(kMargin, y, kAboutFontCredit, kColorText, kBodyScale);
  y += kLineHeight;
  display.drawText(kMargin, y, kAboutCopyright, kColorText, kBodyScale);
  y += kLineHeight + kMargin;
  display.drawText(kMargin, y, kAboutNoticeLead, kColorText, kBodyScale);
  y += kLineHeight;
  display.drawText(kMargin + kGlossIndent, y, kAboutNoticeFile, kColorAccent, kBodyScale);
}

// The passive prompt shown on the empty search root (ADR-0010): the `lukin` search icon above
// the "o alasa e nimi" sentence, centred in the result area. Drawn only while the query is
// empty; the first keystroke replaces it with the result list.
void renderEmptyPrompt(DisplaySurface& display) {
  const int areaTop = kMargin + kLineHeight + kMargin;  // where result rows would begin
  const int blockHeight = kSitelenHeight * 2 + kMargin;  // icon row + sentence row
  const int iconTop = areaTop + (static_cast<int>(display.height()) - areaTop - blockHeight) / 2;
  drawGlyphRowCentered(display, iconTop, kPromptIcon, kPromptIconGlyphs, kColorAccent);
  drawGlyphRowCentered(display, iconTop + kSitelenHeight + kMargin, kPromptSentence,
                       kPromptSentenceGlyphs, kColorText);
}

void renderSearch(const UiState& state, DisplaySurface& display) {
  // Query line at the top; a leading '>' marks it as the edit field.
  char prompt[kMaxQueryLen + 2];
  prompt[0] = '>';
  std::strcpy(prompt + 1, state.query);
  display.drawText(kMargin, kMargin, prompt, kColorText, kBodyScale);

  // The empty root teaches type-to-search with a sitelen prompt instead of a bare screen
  // (ADR-0010). A non-empty query that finds nothing falls through to an empty list — the
  // prompt is for the root, not a "no results" message.
  if (state.queryLen == 0) {
    renderEmptyPrompt(display);
    return;
  }

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
  switch (state.view) {
    case View::Splash:
      renderSplash(display);
      break;
    case View::Detail:
      renderDetail(state, display);
      break;
    case View::About:
      renderAbout(display);
      break;
    case View::Search:
      renderSearch(state, display);
      break;
  }
  display.present();
}

}  // namespace tp
