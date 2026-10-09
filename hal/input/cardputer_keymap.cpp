// The pure Cardputer keymap and key→InputEvent policy (ADR-0011). Host-compilable; no hardware.
#include "input/cardputer_keymap.hpp"

namespace tp {
namespace {

// --- the Cardputer key grid (ADR-0005) ------------------------------------------------------
// The silkscreen layout, identical on both variants (espp key_map_). We fold case in the query
// path (ADR-0002), so only the unshifted value matters; `special` names the key's
// navigation/edit meaning, so a plain key and its arrow meaning are one table entry. Up/Down act
// whether or not fn is held (ADR-0008); esc/delete need fn (see mapPress).
enum class Special : uint8_t { None, Esc, Delete, Up, Down, Other };

struct Key {
  char value;       // the character this key types (0 for a modifier); '\n' '\b' '\t' as-is.
  Special special;  // its fn-layer meaning (Other = an fn key we ignore: left/right/F-keys).
};

constexpr Key kKeymap[kCardputerKeyRows][kCardputerKeyCols] = {
    // row 0: ` 1 2 3 4 5 6 7 8 9 0 - = backspace  (fn layer: esc, F1..F12, delete)
    {{'`', Special::Esc}, {'1', Special::Other}, {'2', Special::Other}, {'3', Special::Other},
     {'4', Special::Other}, {'5', Special::Other}, {'6', Special::Other}, {'7', Special::Other},
     {'8', Special::Other}, {'9', Special::Other}, {'0', Special::Other}, {'-', Special::Other},
     {'=', Special::Other}, {'\b', Special::Delete}},
    // row 1: tab q w e r t y u i o p [ ] backslash
    {{'\t', Special::None}, {'q', Special::None}, {'w', Special::None}, {'e', Special::None},
     {'r', Special::None}, {'t', Special::None}, {'y', Special::None}, {'u', Special::None},
     {'i', Special::None}, {'o', Special::None}, {'p', Special::None}, {'[', Special::None},
     {']', Special::None}, {'\\', Special::None}},
    // row 2: fn shift a s d f g h j k l ;(fn=up) ' enter
    {{0, Special::None}, {0, Special::None}, {'a', Special::None}, {'s', Special::None},
     {'d', Special::None}, {'f', Special::None}, {'g', Special::None}, {'h', Special::None},
     {'j', Special::None}, {'k', Special::None}, {'l', Special::None}, {';', Special::Up},
     {'\'', Special::None}, {'\n', Special::None}},
    // row 3: ctrl opt alt z x c v b n m ,(fn=left) .(fn=down) /(fn=right) space
    {{0, Special::None}, {0, Special::None}, {0, Special::None}, {'z', Special::None},
     {'x', Special::None}, {'c', Special::None}, {'v', Special::None}, {'b', Special::None},
     {'n', Special::None}, {'m', Special::None}, {',', Special::Other}, {'.', Special::Down},
     {'/', Special::Other}, {' ', Special::None}},
};

// Resolve a pressed key to the app event it means, or None (modifiers, ignored fn keys).
InputEvent mapPress(const Key& key, bool fnHeld) {
  // Arrow-labelled keys navigate whether or not fn is held: their punctuation (; .) is folded
  // out of every query by normalization, so it is never typed, and single-key nav is friendlier
  // in both the result list and the detail language cycle (ADR-0008).
  if (key.special == Special::Up) return InputEvent::navigate(NavDir::Up);
  if (key.special == Special::Down) return InputEvent::navigate(NavDir::Down);

  if (fnHeld) {
    switch (key.special) {
      case Special::Esc:
      case Special::Delete: return InputEvent::back();
      default: return InputEvent::none();  // fn+(left/right/F-key) is not in our vocabulary.
    }
  }
  if (key.value == '\n') return InputEvent::select();
  if (key.value == '\b') return InputEvent::back();
  // Any other printable key types a character; the query path folds case + diacritics anyway.
  if (key.value >= 0x20 && key.value < 0x7f) return InputEvent::character(key.value);
  return InputEvent::none();  // a modifier (value 0) or a control key we don't map.
}

}  // namespace

KeyOutcome applyKey(int row, int col, bool pressed, bool fnHeld) {
  // Out of grid: the hardware decode can land outside the 4x14 layout; ignore it (fail-soft on a
  // bad cell, but see below — the driver still fails loud on a dead bus). fn state is unchanged.
  if (row < 0 || row >= kCardputerKeyRows || col < 0 || col >= kCardputerKeyCols) {
    return {InputEvent::none(), fnHeld};
  }
  // The fn modifier tracks on both edges and emits no event itself.
  if (row == kCardputerFnRow && col == kCardputerFnCol) {
    return {InputEvent::none(), pressed};
  }
  // Every other key acts on its press; a release only ever mattered through fn state (above).
  if (!pressed) {
    return {InputEvent::none(), fnHeld};
  }
  return {mapPress(kKeymap[row][col], fnHeld), fnHeld};
}

}  // namespace tp
