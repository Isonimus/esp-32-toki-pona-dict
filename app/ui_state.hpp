#pragma once
#include <cstdint>
#include "dictionary.hpp"

// The UI's observable state (ADR-0004). Pure data, held by value — no heap, no hardware —
// so the controller that mutates it is host-tested, and the renderer that draws it is a
// separate step against the DisplaySurface seam.

namespace tp {

// Max characters the user can type before the query must be narrowed, and the max merged
// results kept. Both bound fixed buffers (no heap, ADR-0001). A result set larger than the
// cap is truncated — a display limit, not an error (ADR-0002/0004).
constexpr int kMaxQueryLen = 32;
constexpr int kMaxResults = 32;

enum class View : uint8_t {
  Splash,  // transient boot wordmark (ADR-0009); dismissed by first input or a timeout
  Search,  // typing a query, browsing the merged result list
  Detail,  // one entry: its glosses in one language at a time (cycled) + its sitelen glyph
  About,   // read-only colophon: the sitelen wordmark + third-party font credit (ADR-0009)
};

// The board's input style — the first board capability (ADR-0020), injected by main/ and stamped
// into the state so the pure renderer can branch on it. It decides only what the *empty* search
// root does: a Keyboard board teaches type-to-search (ADR-0010); a Buttons board, which cannot
// type, browses the whole corpus instead. Everything else is identical across boards (ADR-0001).
enum class InputStyle : uint8_t { Keyboard, Buttons };

struct UiState {
  View view = View::Splash;  // boot shows the splash (ADR-0009), dismissed into Search

  // Board capability, set once by the UiController ctor; defaults to Keyboard so every existing
  // board and test is unchanged (ADR-0020).
  InputStyle inputStyle = InputStyle::Keyboard;

  char query[kMaxQueryLen + 1] = {};  // NUL-terminated typed query
  int queryLen = 0;

  uint16_t results[kMaxResults] = {};  // merged, deduped entry ids (ADR-0004)
  int resultCount = 0;
  int selected = 0;  // focused row, always in [0, listCount()) when the list is non-empty

  uint16_t detailEntry = 0;  // entry shown in Detail view; meaningful only when view==Detail
  // Language shown in Detail, one at a time, cycled with Nav (ADR-0007). A Lang index in
  // [En, Pt]; set when Detail opens, meaningful only when view==Detail.
  uint8_t detailLang = static_cast<uint8_t>(Lang::En);
};

// The browsable list the Search view shows, derived (no second buffer, ADR-0020): a typed query
// lists the merged `results`; an empty query on a Buttons board lists the whole corpus, where a
// row's index IS its entry id; an empty query on a Keyboard board lists nothing (its empty root
// draws the ADR-0010 prompt instead). Shared by the controller's Nav/Select and the renderer.
inline int listCount(const UiState& s) {
  if (s.queryLen > 0) return s.resultCount;
  return s.inputStyle == InputStyle::Buttons ? static_cast<int>(ENTRY_COUNT) : 0;
}

inline uint16_t entryIdAt(const UiState& s, int row) {
  return s.queryLen > 0 ? s.results[row] : static_cast<uint16_t>(row);
}

}  // namespace tp
