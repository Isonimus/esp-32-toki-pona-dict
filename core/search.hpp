#pragma once
#include <cstdint>
#include "dictionary.hpp"

namespace tp {

// Prefix-search the index for `lang`. Writes up to `maxResults` matching entry ids
// into `outIds` (deduplicated, in index order) and returns how many were written.
// An empty or whitespace-only query matches nothing and returns 0. The query is
// normalized (core/normalize.hpp) before matching, so "agua" finds "água".
//
// No heap is used: results go into the caller's fixed buffer, and a result set larger
// than `maxResults` is truncated (the UI pages or narrows the query), which is a
// display limit, not an error.
int search(Lang lang, const char* query, uint16_t* outIds, int maxResults);

// Resolve an entry id from a search result. The id must come from search(); an
// out-of-range id is a programming error and aborts (fail loud) rather than reading
// past the table.
const Entry& entry(uint16_t id);

}  // namespace tp
