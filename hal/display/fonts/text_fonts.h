#pragma once

// Declarations for the vendored device text fonts (ADR-0012); definitions in text_fonts.c. Global
// `const` arrays share the same (unmangled) symbol in C and C++, so no extern "C" is strictly
// needed, but it is declared for clarity and to match the font's C translation unit. The LovyanGFX
// U8g2font bridge wraps each raw array.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const uint8_t u8g2_font_7x13_te[];    // body text (X11 Misc-Fixed, public domain)
extern const uint8_t u8g2_font_helvB14_te[];  // headword (Adobe Helvetica Bold)

#ifdef __cplusplus
}
#endif
