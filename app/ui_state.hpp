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

struct UiState {
  View view = View::Splash;  // boot shows the splash (ADR-0009), dismissed into Search

  char query[kMaxQueryLen + 1] = {};  // NUL-terminated typed query
  int queryLen = 0;

  uint16_t results[kMaxResults] = {};  // merged, deduped entry ids (ADR-0004)
  int resultCount = 0;
  int selected = 0;  // focused row, always in [0, resultCount) when resultCount > 0

  uint16_t detailEntry = 0;  // entry shown in Detail view; meaningful only when view==Detail
  // Language shown in Detail, one at a time, cycled with Nav (ADR-0007). A Lang index in
  // [En, Pt]; set when Detail opens, meaningful only when view==Detail.
  uint8_t detailLang = static_cast<uint8_t>(Lang::En);
};

}  // namespace tp
