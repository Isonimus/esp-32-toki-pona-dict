#include "ui_controller.hpp"

#include "search.hpp"

namespace tp {
namespace {

// Detail cycles the non-Toki-Pona languages (Tp is the headword title itself), the contiguous
// enum range [En, Count) in cycle order (ADR-0007). Opening Detail starts on the first of them.
constexpr int kFirstDetailLang = static_cast<int>(Lang::En);
constexpr int kDetailLangCount = LangCount - kFirstDetailLang;

}  // namespace

void UiController::runSearch() {
  state_.resultCount = 0;

  if (state_.queryLen > 0) {
    // Search each language and merge, skipping ids already collected (ADR-0004). The buffer
    // fills in cycle order; once full, later languages' extra hits are dropped (truncation
    // is a display limit, ADR-0002).
    uint16_t hits[kMaxResults];
    for (int lang = 0; lang < LangCount && state_.resultCount < kMaxResults; ++lang) {
      const int room = kMaxResults - state_.resultCount;
      const int found = search(static_cast<Lang>(lang), state_.query, hits, room);
      for (int i = 0; i < found; ++i) {
        bool alreadyPresent = false;
        for (int j = 0; j < state_.resultCount; ++j) {
          if (state_.results[j] == hits[i]) {
            alreadyPresent = true;
            break;
          }
        }
        if (!alreadyPresent) state_.results[state_.resultCount++] = hits[i];
      }
    }
  }

  // Keep the focus valid: on the first row of a fresh list, clamped to the last row if the
  // list shrank below the old selection.
  if (state_.selected >= state_.resultCount) {
    state_.selected = state_.resultCount > 0 ? state_.resultCount - 1 : 0;
  }
}

void UiController::dismissSplash() {
  // The splash has no state of its own; dismissing it is just the move to the search root
  // (ADR-0009). Guarded so the timeout path and a stray late call cannot clobber another view.
  if (state_.view == View::Splash) state_.view = View::Search;
}

void UiController::handle(const InputEvent& event) {
  if (state_.view == View::Splash) {
    // Any real input dismisses the boot splash and is swallowed — not typed — so mashing a key
    // during it cannot seed the query. None leaves it up, so a bare poll never skips it; the
    // timed dismissal is main/'s job (ADR-0009).
    if (!event.isNone()) dismissSplash();
    return;
  }

  if (state_.view == View::About) {
    // A read-only colophon (ADR-0009): Back returns to the search root, everything else inert.
    if (event.kind == InputKind::Back) state_.view = View::Search;
    return;
  }

  if (state_.view == View::Detail) {
    // Detail shows one language at a time; Nav cycles it (wrapping, since a language selector is
    // a ring), Back returns to the list, everything else is inert (ADR-0007).
    switch (event.kind) {
      case InputKind::Back:
        state_.view = View::Search;
        break;
      case InputKind::Nav: {
        // Walk the [En, Count) ring; Up is "-1" expressed as +(count-1) to avoid negative mod.
        const int offset = state_.detailLang - kFirstDetailLang;
        const int step = (event.nav == NavDir::Down) ? 1 : kDetailLangCount - 1;
        state_.detailLang =
            static_cast<uint8_t>(kFirstDetailLang + (offset + step) % kDetailLangCount);
        break;
      }
      default:
        break;
    }
    return;
  }

  // View::Search
  switch (event.kind) {
    case InputKind::Char:
      if (state_.queryLen < kMaxQueryLen) {
        state_.query[state_.queryLen++] = event.ch;
        state_.query[state_.queryLen] = '\0';
        state_.selected = 0;  // a changed query restarts the list at the top
        runSearch();
      }
      break;

    case InputKind::Back:
      // With a non-empty query, Back edits it: drop the last character. With an empty query
      // there is nothing to delete and Search is the root — ADR-0004 left this inert; ADR-0009
      // repurposes that wasted event to open the About colophon ("nothing behind the root").
      if (state_.queryLen > 0) {
        state_.query[--state_.queryLen] = '\0';
        state_.selected = 0;
        runSearch();
      } else {
        state_.view = View::About;
      }
      break;

    case InputKind::Nav:
      // Move the focus without wrapping — predictable on both a keyboard's arrows and an
      // encoder's detents. Clamped at both ends.
      if (state_.resultCount > 0) {
        if (event.nav == NavDir::Down && state_.selected < state_.resultCount - 1) {
          ++state_.selected;
        } else if (event.nav == NavDir::Up && state_.selected > 0) {
          --state_.selected;
        }
      }
      break;

    case InputKind::Select:
      // Open the focused result. No results means nothing to open.
      if (state_.resultCount > 0) {
        state_.detailEntry = state_.results[state_.selected];
        state_.detailLang = static_cast<uint8_t>(kFirstDetailLang);  // open on the first language (ADR-0007)
        state_.view = View::Detail;
      }
      break;

    case InputKind::None:
      break;
  }
}

}  // namespace tp
