#pragma once

// Pure ADV FIFO-byte decode; the variant-independent key policy stays in applyKey (ADR-0025).
#include <cstdint>

namespace tp {

struct CardputerKeyCell {
  int row;
  int col;
  bool pressed;
};

// TCA8418 key numbers use a ten-column stride even though the ADV enables only eight
// columns (TI datasheet SCPS215G, section 8.3.1.1). Bit 7 is the press/release edge.
// Preserve the device decoder's arithmetic; applyKey rejects out-of-grid cells. Key number
// zero has no cell, including 0x80, and must not underflow the one-based key number.
inline constexpr CardputerKeyCell decodeTca8418Event(uint8_t rawEvent) {
  constexpr uint8_t kPressedMask = 0x80;
  constexpr uint8_t kKeyNumberMask = 0x7f;
  constexpr int kControllerRowStride = 10;
  constexpr int kSilkscreenRows = 4;
  constexpr int kSilkscreenColsPerControllerRow = 2;

  const bool pressed = (rawEvent & kPressedMask) != 0;
  const uint8_t keyNumber = rawEvent & kKeyNumberMask;
  if (keyNumber == 0) return {-1, -1, pressed};

  const int n = keyNumber - 1;
  const int controllerRow = n / kControllerRowStride;
  const int controllerCol = n % kControllerRowStride;
  return {controllerCol % kSilkscreenRows,
          controllerRow * kSilkscreenColsPerControllerRow +
              (controllerCol >= kSilkscreenRows ? 1 : 0),
          pressed};
}

}  // namespace tp
