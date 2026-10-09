#include "render.hpp"

#include <cstdio>
#include <cstring>

#include "cjk.hpp"
#include "logo.hpp"
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
constexpr int kLineHeight = 16;      // body text line pitch (7x13 font is ~13px tall, ADR-0012)
constexpr int kHeaderHeight = 26;    // headword line pitch: helvB14 (~18px) + a gap below the title
constexpr int kGlossIndent = 8;  // senses are indented under their language label
constexpr int kWordmarkGap = 8;  // pixels between the two wordmark glyphs

// On-device colophon credit (ADR-0009, ADR-0016). The legally-operative notices live in
// THIRD_PARTY_NOTICES.md (shipped with the firmware, enforced by tools/check-notices.mjs); these
// lines are a human-readable courtesy credit, one per attributed work, hand-kept in sync with that
// file. The device view is condensed to fit two attributions on the panel — the sitelen pona font
// (nasin nanpa, MIT; ADR-0003) and the gloss data (lipu Linku, CC BY-SA; ADR-0016) — with the full
// copyright, authorship and license text left to the notices file. The splash no longer credits
// the font (ADR-0015); this courtesy credit now lives only here.
constexpr const char* kAboutSitelenCredit = "sitelen pona: nasin nanpa (MIT)";
constexpr const char* kAboutGlossCredit = "glosses: lipu Linku (CC BY-SA)";
constexpr const char* kAboutNoticePointer = "see THIRD_PARTY_NOTICES.md";

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
  // Brand moment (ADR-0015): the toki pona logo — `toki` with the `pona` smile nested in its
  // ring — as one centred hero. This supersedes ADR-0009's two separate wordmark glyphs plus a
  // font credit here: the merged logo is the product's actual mark, and crediting the font on the
  // opening screen made nasin nanpa too prominent for what is primarily a Toki Pona dictionary.
  // The courtesy credit stays in the About colophon, the legal notice in THIRD_PARTY_NOTICES.md.
  const int x = (static_cast<int>(display.width()) - TP_LOGO.width) / 2;
  const int y = (static_cast<int>(display.height()) - TP_LOGO.height) / 2;
  display.drawGlyph1bpp(x, y, TP_LOGO.width, TP_LOGO.height, TP_LOGO.bitmap, kColorAccent);
}

void renderAbout(DisplaySurface& display) {
  // Condensed colophon (ADR-0009, ADR-0016): the wordmark, then one courtesy-credit line per
  // attributed work — the sitelen pona font and the gloss data — and a pointer to the notices
  // file. The verbatim legal text (full copyright, authorship, license bodies) is intentionally
  // absent; it ships in THIRD_PARTY_NOTICES.md (ADR-0003/0016). This view is a courtesy credit.
  int y = drawWordmark(display, kMargin);
  display.drawText(kMargin, y, kAboutSitelenCredit, kColorText, TextRole::Body);
  y += kLineHeight;
  display.drawText(kMargin, y, kAboutGlossCredit, kColorText, TextRole::Body);
  y += kLineHeight + kMargin;
  display.drawText(kMargin, y, kAboutNoticePointer, kColorAccent, TextRole::Body);
}

// The passive prompt shown on the empty search root (ADR-0010): the `lukin` search icon above
// the "o alasa e nimi" sentence, centred in the result area. Drawn only while the query is
// empty; the first keystroke replaces it with the result list.
void renderEmptyPrompt(DisplaySurface& display) {
  // The lukin icon above the "o alasa e nimi" sentence, the pair centred in the whole panel
  // (ADR-0014). The bare '>' edit line is suppressed on the empty root (see renderSearch), so this
  // prompt owns the screen and centres against the full height — not the old result-area slice,
  // which pushed the block low against the bottom edge.
  const int blockHeight = kSitelenHeight * 2 + kMargin;  // icon row + gap + sentence row
  const int iconTop = (static_cast<int>(display.height()) - blockHeight) / 2;
  drawGlyphRowCentered(display, iconTop, kPromptIcon, kPromptIconGlyphs, kColorAccent);
  drawGlyphRowCentered(display, iconTop + kSitelenHeight + kMargin, kPromptSentence,
                       kPromptSentenceGlyphs, kColorText);
}

void renderSearch(const UiState& state, DisplaySurface& display) {
  // The empty root teaches type-to-search with a centred sitelen prompt instead of a bare screen
  // (ADR-0010). The '>' edit line is suppressed while the query is empty (ADR-0014), so no lonely
  // caret floats in the corner beside the centred prompt; the first keystroke brings it back
  // below. A non-empty query that finds nothing falls through to an empty list — the prompt is for
  // the root, not a "no results" message.
  if (state.queryLen == 0) {
    renderEmptyPrompt(display);
    return;
  }

  // Query line at the top; a leading '>' marks it as the edit field.
  char prompt[kMaxQueryLen + 2];
  prompt[0] = '>';
  std::strcpy(prompt + 1, state.query);
  display.drawText(kMargin, kMargin, prompt, kColorText, TextRole::Body);

  // Result rows below, each the Toki Pona headword (the pivot, ADR-0004). The focused row
  // is drawn in the accent colour — no fill primitive needed on the seam.
  int y = kMargin + kLineHeight + kMargin;
  for (int i = 0; i < state.resultCount; ++i) {
    const uint16_t color = (i == state.selected) ? kColorAccent : kColorText;
    display.drawText(kMargin, y, entry(state.results[i]).tp, color, TextRole::Body);
    y += kLineHeight;
  }
}

// Decode one UTF-8 code point at `s` and advance `s` past it. The displayed strings are
// build-time-validated UTF-8 (tools/check-font-coverage.mjs), so this is a decoder, not a
// validator; it still advances at least one byte on a malformed lead so a caller loop cannot hang.
uint32_t nextCodepoint(const char*& s) {
  const unsigned char c0 = static_cast<unsigned char>(*s++);
  if (c0 < 0x80) return c0;
  int extra;
  uint32_t cp;
  if ((c0 & 0xE0) == 0xC0) { extra = 1; cp = c0 & 0x1F; }
  else if ((c0 & 0xF0) == 0xE0) { extra = 2; cp = c0 & 0x0F; }
  else if ((c0 & 0xF8) == 0xF0) { extra = 3; cp = c0 & 0x07; }
  else return 0xFFFD;
  for (int i = 0; i < extra; ++i) {
    const unsigned char cc = static_cast<unsigned char>(*s);
    if ((cc & 0xC0) != 0x80) return 0xFFFD;  // truncated; leave s at the offending byte
    cp = (cp << 6) | (cc & 0x3F);
    ++s;
  }
  return cp;
}

// Any CJK code point marks a line the Latin text font cannot draw. 0x2E80 is the first CJK block
// (Radicals Supplement); everything Latin-1/Extended sits below it, so this never misfires on the
// accented European glosses.
bool containsCjk(const char* text) {
  for (const char* p = text; *p;) {
    if (nextCodepoint(p) >= 0x2E80) return true;
  }
  return false;
}

// Draw a string as a row of fixed CJK cells (ADR-0018), one per code point, left to right. CJK is
// monospaced, so the advance is a constant kCjkWidth; a code point the font lacks reserves its slot
// but draws nothing (the ADR-0003 degradation contract). Stops at the panel's right edge rather
// than drawing off-screen — a long gloss is clipped, never wrapped (a bring-up concern, ADR-0018).
void drawCjkCells(DisplaySurface& display, int x, int y, const char* text, uint16_t color) {
  const int maxX = static_cast<int>(display.width()) - kCjkWidth;
  for (const char* p = text; *p && x <= maxX;) {
    const uint32_t cp = nextCodepoint(p);
    if (const uint8_t* cell = cjkGlyph(cp)) {
      display.drawGlyph1bpp(x, y, kCjkWidth, kCjkHeight, cell, color);
    }
    x += kCjkWidth;
  }
}

// One detail-view line (label or gloss). A display-only language (ADR-0017) renders through the CJK
// cell font; every other language uses the Latin text font. Routed by content — any CJK code point
// in the line — so the renderer needs no language table and a mixed ASCII/CJK line (the cycle
// label, parenthetical tags) draws uniformly as cells.
void drawDetailLine(DisplaySurface& display, int x, int y, const char* text, uint16_t color) {
  if (containsCjk(text)) {
    drawCjkCells(display, x, y, text, color);
  } else {
    display.drawText(x, y, text, color, TextRole::Body);
  }
}

void renderDetail(const UiState& state, DisplaySurface& display) {
  const Entry& e = entry(state.detailEntry);

  // Headword, large.
  display.drawText(kMargin, kMargin, e.tp, kColorAccent, TextRole::Header);

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
  int y = kMargin + kHeaderHeight + kMargin;

  char label[32];
  const int langPosition = lang - static_cast<int>(Lang::Tp);  // En -> 1 ... Pt -> LangCount-1
  std::snprintf(label, sizeof(label), "< %s  %d/%d >", LANG_LABEL[lang], langPosition,
                LangCount - 1);
  drawDetailLine(display, kMargin, y, label, kColorAccent);
  y += kLineHeight;

  for (const char* const* gloss = e.glossesByLang[lang]; *gloss != nullptr; ++gloss) {
    drawDetailLine(display, kMargin + kGlossIndent, y, *gloss, kColorText);
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
