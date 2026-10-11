// Host test for the pure Cardputer keymap (ADR-0011): the key grid, the fn-layer / bare-arrow
// policy (ADR-0005, ADR-0008), and fn-modifier tracking. Plain g++, no hardware — this logic
// used to be trapped behind the driver's UNIT_TEST guard, proven only on-device.
#include <cstdio>

#include "input/cardputer_keymap.hpp"
#include "input/cardputer_tca8418.hpp"

using namespace tp;

static int failures = 0;

#define CHECK(cond)                                               \
  do {                                                            \
    if (!(cond)) {                                                \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
      ++failures;                                                 \
    }                                                             \
  } while (0)

// Grid cells under test, named by their silkscreen key so the expectations read as the layout
// (ADR-0005), never as a bare (row, col). These coordinates ARE the keymap contract.
constexpr int kBacktickRow = 0, kBacktickCol = 0;    // ` / fn=esc
constexpr int kBackspaceRow = 0, kBackspaceCol = 13;  // backspace / fn=delete
constexpr int kQRow = 1, kQCol = 1;                   // q (a plain letter)
constexpr int kShiftRow = 2, kShiftCol = 1;           // shift (a modifier, value 0)
constexpr int kSemicolonRow = 2, kSemicolonCol = 11;  // ; / fn=up — bare-arrow nav (ADR-0008)
constexpr int kEnterRow = 2, kEnterCol = 13;          // enter
constexpr int kCommaRow = 3, kCommaCol = 10;          // , / fn=left (an ignored fn key)
constexpr int kDotRow = 3, kDotCol = 11;              // . / fn=down — bare-arrow nav (ADR-0008)
constexpr int kSpaceRow = 3, kSpaceCol = 13;          // space

constexpr bool kPress = true;
constexpr bool kRelease = false;
constexpr bool kFnHeld = true;
constexpr bool kNoFn = false;

// Whether an outcome's event is exactly this kind (and char, for Char/Nav).
static bool isChar(const KeyOutcome& o, char c) {
  return o.event.kind == InputKind::Char && o.event.ch == c;
}
static bool isNav(const KeyOutcome& o, NavDir dir) {
  return o.event.kind == InputKind::Nav && o.event.nav == dir;
}
static bool isKind(const KeyOutcome& o, InputKind kind) { return o.event.kind == kind; }

int main() {
  // Raw key numbers laid out by silkscreen cell, independent of the decoder's arithmetic.
  // The gaps between 8/11, 18/21, ... guard the controller's ten-column numbering stride.
  constexpr uint8_t kRawKeyNumbers[kCardputerKeyRows][kCardputerKeyCols] = {
      {1, 5, 11, 15, 21, 25, 31, 35, 41, 45, 51, 55, 61, 65},
      {2, 6, 12, 16, 22, 26, 32, 36, 42, 46, 52, 56, 62, 66},
      {3, 7, 13, 17, 23, 27, 33, 37, 43, 47, 53, 57, 63, 67},
      {4, 8, 14, 18, 24, 28, 34, 38, 44, 48, 54, 58, 64, 68},
  };
  for (int row = 0; row < kCardputerKeyRows; ++row) {
    for (int col = 0; col < kCardputerKeyCols; ++col) {
      const CardputerKeyCell released = decodeTca8418Event(kRawKeyNumbers[row][col]);
      const CardputerKeyCell pressed = decodeTca8418Event(kRawKeyNumbers[row][col] | 0x80);
      CHECK(released.row == row && released.col == col && !released.pressed);
      CHECK(pressed.row == row && pressed.col == col && pressed.pressed);
    }
  }
  const uint8_t absentEvents[] = {0x00, 0x80};
  for (uint8_t raw : absentEvents) {
    const CardputerKeyCell absent = decodeTca8418Event(raw);
    CHECK(absent.row == -1 && absent.col == -1);
    CHECK(applyKey(absent.row, absent.col, absent.pressed, kFnHeld).fnHeld == kFnHeld);
    CHECK(applyKey(absent.row, absent.col, absent.pressed, kFnHeld).event.isNone());
  }

  // Replay FIFO bytes through the same decode -> applyKey seam used by the driver: fn down,
  // ; down/up, fn up, then q. Neither releases nor an absent key may leave fn stuck.
  bool fnHeld = false;
  const uint8_t script[] = {0x83, 0xb9, 0x39, 0x03, 0x86};
  KeyOutcome outcomes[5];
  for (int i = 0; i < 5; ++i) {
    const CardputerKeyCell key = decodeTca8418Event(script[i]);
    outcomes[i] = applyKey(key.row, key.col, key.pressed, fnHeld);
    fnHeld = outcomes[i].fnHeld;
  }
  CHECK(outcomes[0].event.isNone() && outcomes[0].fnHeld);
  CHECK(isNav(outcomes[1], NavDir::Up) && outcomes[1].fnHeld);
  CHECK(outcomes[2].event.isNone() && outcomes[2].fnHeld);
  CHECK(outcomes[3].event.isNone() && !outcomes[3].fnHeld);
  CHECK(isChar(outcomes[4], 'q') && !outcomes[4].fnHeld);

  // Bare arrow keys navigate whether or not fn is held (ADR-0008 — the regression this test
  // exists to guard). ; is Up, . is Down; fn changes nothing for them.
  CHECK(isNav(applyKey(kSemicolonRow, kSemicolonCol, kPress, kNoFn), NavDir::Up));
  CHECK(isNav(applyKey(kSemicolonRow, kSemicolonCol, kPress, kFnHeld), NavDir::Up));
  CHECK(isNav(applyKey(kDotRow, kDotCol, kPress, kNoFn), NavDir::Down));
  CHECK(isNav(applyKey(kDotRow, kDotCol, kPress, kFnHeld), NavDir::Down));

  // The fn layer: fn+esc (`) and fn+delete (backspace) are Back; bare backspace is also Back
  // (edit the query), and bare ` types its character.
  CHECK(isKind(applyKey(kBacktickRow, kBacktickCol, kPress, kFnHeld), InputKind::Back));
  CHECK(isChar(applyKey(kBacktickRow, kBacktickCol, kPress, kNoFn), '`'));
  CHECK(isKind(applyKey(kBackspaceRow, kBackspaceCol, kPress, kFnHeld), InputKind::Back));
  CHECK(isKind(applyKey(kBackspaceRow, kBackspaceCol, kPress, kNoFn), InputKind::Back));

  // Enter commits (Select); a plain letter and space type their character.
  CHECK(isKind(applyKey(kEnterRow, kEnterCol, kPress, kNoFn), InputKind::Select));
  CHECK(isChar(applyKey(kQRow, kQCol, kPress, kNoFn), 'q'));
  CHECK(isChar(applyKey(kSpaceRow, kSpaceCol, kPress, kNoFn), ' '));

  // Keys with no place in the vocabulary yield None: a modifier (shift, value 0), fn + a letter
  // (the fn layer only maps esc/delete/arrows), and fn + an ignored fn key (fn+, = left).
  CHECK(isKind(applyKey(kShiftRow, kShiftCol, kPress, kNoFn), InputKind::None));
  CHECK(isKind(applyKey(kQRow, kQCol, kPress, kFnHeld), InputKind::None));
  CHECK(isKind(applyKey(kCommaRow, kCommaCol, kPress, kFnHeld), InputKind::None));
  // ...but bare , still types its punctuation (folded out of queries by normalization, ADR-0002).
  CHECK(isChar(applyKey(kCommaRow, kCommaCol, kPress, kNoFn), ','));

  // A key release emits nothing and leaves fn state untouched; only the press acts.
  {
    const KeyOutcome released = applyKey(kQRow, kQCol, kRelease, kNoFn);
    CHECK(isKind(released, InputKind::None));
    CHECK(released.fnHeld == kNoFn);
  }

  // The fn key itself emits nothing and tracks the modifier on both edges — the press turns fn on,
  // the release turns it off (ADR-0011: fn tracking is pure, carried in the outcome).
  {
    const KeyOutcome down = applyKey(kCardputerFnRow, kCardputerFnCol, kPress, kNoFn);
    CHECK(isKind(down, InputKind::None));
    CHECK(down.fnHeld == true);  // press -> fn held
    const KeyOutcome up = applyKey(kCardputerFnRow, kCardputerFnCol, kRelease, kFnHeld);
    CHECK(isKind(up, InputKind::None));
    CHECK(up.fnHeld == false);  // release -> fn released
  }

  // Out-of-grid coordinates (a bad hardware decode) are ignored: None, fn unchanged — applyKey is
  // total, so the driver need not pre-validate the cell.
  CHECK(isKind(applyKey(-1, 0, kPress, kNoFn), InputKind::None));
  CHECK(isKind(applyKey(0, kCardputerKeyCols, kPress, kNoFn), InputKind::None));
  CHECK(applyKey(kCardputerKeyRows, 0, kPress, kFnHeld).fnHeld == kFnHeld);

  if (failures == 0) {
    std::printf("PASS: Cardputer keymap\n");
    return 0;
  }
  std::printf("FAILED: %d check(s)\n", failures);
  return 1;
}
