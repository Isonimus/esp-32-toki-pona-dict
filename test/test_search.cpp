// Native host test for the pure dictionary core. Builds with plain g++ (no device
// toolchain) so the search contract is verified on every change:
//
//   npm run test:host
//
// Covers the two things a unit test CAN assert: the normalization contract (shared
// with the Node generator) and symmetric prefix lookup including the diacritic-folding
// "type it unaccented" behaviour. Rendering and on-device input are verified elsewhere.

#include <cstdio>
#include <cstring>
#include <string>

#include "dictionary.hpp"
#include "normalize.hpp"
#include "normalization_fixtures_gen.h"
#include "search.hpp"

static int g_failures = 0;

static void check(bool cond, const std::string& what) {
  if (!cond) {
    std::printf("  FAIL: %s\n", what.c_str());
    ++g_failures;
  }
}

// True if a search in `lang` for `query` returns the entry whose headword is `tpWord`.
static bool finds(tp::Lang lang, const char* query, const char* tpWord) {
  uint16_t ids[32];
  int n = tp::search(lang, query, ids, 32);
  for (int i = 0; i < n; ++i) {
    if (std::strcmp(tp::entry(ids[i]).tp, tpWord) == 0) return true;
  }
  return false;
}

int main() {
  // 1. Normalization contract — identical cases to the Node generator.
  for (int i = 0; i < NORM_CASE_COUNT; ++i) {
    std::string got = tp::normalize(NORM_CASES[i].in);
    check(got == NORM_CASES[i].out,
          std::string("normalize(\"") + NORM_CASES[i].in + "\") = \"" + got +
              "\", want \"" + NORM_CASES[i].out + "\"");
  }

  // 2. Toki Pona -> language: pick a headword, confirm it resolves and carries data.
  {
    uint16_t ids[32];
    int n = tp::search(tp::Lang::Tp, "suli", ids, 32);
    check(n == 1, "tp 'suli' resolves to exactly one entry");
    if (n == 1) {
      const tp::Entry& e = tp::entry(ids[0]);
      check(std::strcmp(e.tp, "suli") == 0, "headword is suli");
      check(std::strcmp(e.sitelen, "suli") == 0, "sitelen glyph id defaults to headword");
      check(std::strcmp(e.glossesByLang[(int)tp::Lang::En][0], "big") == 0,
            "suli's first English gloss is 'big'");
    }
  }

  // 3. Language -> Toki Pona, both directions symmetric across all five languages.
  check(finds(tp::Lang::En, "big", "suli"), "en 'big' -> suli");
  check(finds(tp::Lang::Es, "grande", "suli"), "es 'grande' -> suli");
  check(finds(tp::Lang::Fr, "parler", "toki"), "fr 'parler' -> toki");
  check(finds(tp::Lang::De, "wasser", "telo"), "de 'wasser' -> telo");
  check(finds(tp::Lang::Pt, "comer", "moku"), "pt 'comer' -> moku");

  // 4. The killer behaviour: typing a word WITHOUT its diacritics still matches.
  check(finds(tp::Lang::Es, "agua", "telo"), "es 'agua' (no accent) -> telo (água)");
  check(finds(tp::Lang::Pt, "agua", "telo"), "pt 'agua' (no accent) -> telo (água)");
  check(finds(tp::Lang::De, "mussen", "wile"), "de 'mussen' -> wile (müssen)");
  check(finds(tp::Lang::De, "gross", "suli"), "de 'gross' -> suli (groß)");

  // 5. Prefix search: a partial query matches.
  check(finds(tp::Lang::En, "spea", "toki"), "en prefix 'spea' -> toki (speak)");
  check(finds(tp::Lang::En, "wa", "telo"), "en prefix 'wa' -> telo (water)");

  // 6. A query that matches nothing returns nothing.
  {
    uint16_t ids[32];
    check(tp::search(tp::Lang::En, "zzzz", ids, 32) == 0, "no match returns 0");
    check(tp::search(tp::Lang::En, "   ", ids, 32) == 0, "whitespace query returns 0");
  }

  // 7. nimi ku suli corpus: the 17 words beyond nimi pu resolve two-way, keep diacritic
  //    folding, and each carries a sitelen glyph (its headword id). Fails on the nimi-pu-only
  //    corpus, so it guards the extension actually reaching the generated tables.
  {
    uint16_t ids[32];
    int n = tp::search(tp::Lang::Tp, "tonsi", ids, 32);
    check(n == 1, "tp 'tonsi' (nimi ku suli) resolves to exactly one entry");
    if (n == 1) {
      check(std::strcmp(tp::entry(ids[0]).sitelen, "tonsi") == 0,
            "tonsi carries a sitelen glyph id (defaults to headword)");
    }
  }
  check(finds(tp::Lang::En, "transgender", "tonsi"), "en 'transgender' -> tonsi");
  check(finds(tp::Lang::Es, "mapache", "kijetesantakalu"), "es 'mapache' -> kijetesantakalu");
  check(finds(tp::Lang::Es, "epico", "epiku"), "es 'epico' (no accent) -> epiku (épico)");

  // 8. Display-only language (ADR-0017): Chinese ships glosses for the detail view but is excluded
  //    from the search index — it cannot be typed on the keyboard. Its index is null and empty, a
  //    query in zh finds nothing, yet the glosses remain on the entry for display.
  {
    const int zh = static_cast<int>(tp::Lang::Zh);
    check(tp::INDEX_SIZE_BY_LANG[zh] == 0, "zh carries no search index (display-only)");
    check(tp::INDEX_BY_LANG[zh] == nullptr, "zh index pointer is null (display-only)");

    uint16_t ids[32];
    check(tp::search(tp::Lang::Zh, "沟通", ids, 32) == 0, "a zh query finds nothing (not searchable)");

    int n = tp::search(tp::Lang::Tp, "moku", ids, 32);
    check(n == 1, "tp 'moku' resolves");
    if (n == 1) {
      check(tp::entry(ids[0]).glossesByLang[zh][0] != nullptr, "moku carries a zh gloss for display");
    }
  }

  if (g_failures == 0) {
    std::printf("PASS: all checks\n");
    return 0;
  }
  std::printf("FAILED: %d check(s)\n", g_failures);
  return 1;
}
