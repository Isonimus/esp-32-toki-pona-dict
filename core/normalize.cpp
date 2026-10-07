#include "normalize.hpp"

#include <cstddef>
#include <cstdint>

namespace tp {
namespace {

// Append the folded form of one Unicode code point to `out`, dropping anything that
// is not part of the allowed key alphabet. See normalize.hpp for the contract.
void foldCodePoint(uint32_t cp, std::string& out) {
  if (cp < 0x80) {  // ASCII fast path
    char c = static_cast<char>(cp);
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == ' ' ||
        c == '\'' || c == '-') {
      out.push_back(c);
    }
    return;
  }

  // Multi-letter expansions (no single base letter).
  switch (cp) {
    case 0x00DF: case 0x1E9E: out += "ss"; return;  // ß ẞ
    case 0x00C6: case 0x00E6: out += "ae"; return;  // Æ æ
    case 0x0152: case 0x0153: out += "oe"; return;  // Œ œ
    default: break;
  }

  // Fold the Latin-1 Supplement uppercase letters onto their lowercase code points,
  // then map each lowercase accented letter to its base ASCII letter.
  uint32_t lc = (cp >= 0x00C0 && cp <= 0x00DE) ? cp + 0x20 : cp;
  char base = 0;
  switch (lc) {
    case 0x00E0: case 0x00E1: case 0x00E2: case 0x00E3: case 0x00E4: case 0x00E5:
      base = 'a'; break;
    case 0x00E7: base = 'c'; break;
    case 0x00E8: case 0x00E9: case 0x00EA: case 0x00EB: base = 'e'; break;
    case 0x00EC: case 0x00ED: case 0x00EE: case 0x00EF: base = 'i'; break;
    case 0x00F0: base = 'd'; break;  // ð
    case 0x00F1: base = 'n'; break;
    case 0x00F2: case 0x00F3: case 0x00F4: case 0x00F5: case 0x00F6: case 0x00F8:
      base = 'o'; break;             // includes ø
    case 0x00F9: case 0x00FA: case 0x00FB: case 0x00FC: base = 'u'; break;
    case 0x00FD: case 0x00FF: base = 'y'; break;
    default: base = 0;               // anything else is dropped
  }
  if (base) out.push_back(base);
}

}  // namespace

std::string normalize(const std::string& utf8) {
  // Decode UTF-8 to code points and fold each one.
  std::string folded;
  folded.reserve(utf8.size());
  for (size_t i = 0; i < utf8.size();) {
    uint8_t b = static_cast<uint8_t>(utf8[i]);
    uint32_t cp;
    int len;
    if (b < 0x80) { cp = b; len = 1; }
    else if ((b & 0xE0) == 0xC0) { cp = b & 0x1F; len = 2; }
    else if ((b & 0xF0) == 0xE0) { cp = b & 0x0F; len = 3; }
    else if ((b & 0xF8) == 0xF0) { cp = b & 0x07; len = 4; }
    else { ++i; continue; }  // invalid lead byte: skip it

    if (i + static_cast<size_t>(len) > utf8.size()) break;  // truncated tail
    bool ok = true;
    for (int k = 1; k < len; ++k) {
      uint8_t cont = static_cast<uint8_t>(utf8[i + k]);
      if ((cont & 0xC0) != 0x80) { ok = false; break; }
      cp = (cp << 6) | (cont & 0x3F);
    }
    if (!ok) { ++i; continue; }
    foldCodePoint(cp, folded);
    i += len;
  }

  // Collapse runs of spaces and trim the ends.
  std::string result;
  result.reserve(folded.size());
  bool prevSpace = true;  // start true so leading spaces are trimmed
  for (char c : folded) {
    if (c == ' ') {
      if (!prevSpace) { result.push_back(' '); prevSpace = true; }
    } else {
      result.push_back(c);
      prevSpace = false;
    }
  }
  if (!result.empty() && result.back() == ' ') result.pop_back();
  return result;
}

}  // namespace tp
