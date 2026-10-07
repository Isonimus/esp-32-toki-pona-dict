#include "search.hpp"

#include <cassert>
#include <cstring>
#include <string>

#include "normalize.hpp"

namespace tp {
namespace {

// Does `row` begin with the already-normalized prefix `q`?
bool startsWith(const char* row, const std::string& q) {
  return std::strncmp(row, q.c_str(), q.size()) == 0;
}

}  // namespace

int search(Lang lang, const char* query, uint16_t* outIds, int maxResults) {
  assert(outIds != nullptr && maxResults > 0);
  const std::string q = normalize(query ? query : "");
  if (q.empty()) return 0;

  const int li = static_cast<int>(lang);
  const IndexRow* idx = INDEX_BY_LANG[li];
  const int n = INDEX_SIZE_BY_LANG[li];

  // Binary search for the first row whose term is >= the query prefix.
  int lo = 0, hi = n;
  while (lo < hi) {
    int mid = lo + (hi - lo) / 2;
    if (std::strcmp(idx[mid].norm, q.c_str()) < 0) lo = mid + 1;
    else hi = mid;
  }

  // Scan forward over every row that has the query as a prefix, collecting distinct
  // entry ids in order.
  int count = 0;
  for (int i = lo; i < n && startsWith(idx[i].norm, q); ++i) {
    const uint16_t id = idx[i].entryId;
    bool seen = false;
    for (int k = 0; k < count; ++k) {
      if (outIds[k] == id) { seen = true; break; }
    }
    if (seen) continue;
    outIds[count++] = id;
    if (count >= maxResults) break;
  }
  return count;
}

const Entry& entry(uint16_t id) {
  assert(id < ENTRY_COUNT);
  return ENTRIES[id];
}

}  // namespace tp
