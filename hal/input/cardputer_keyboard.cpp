// Cardputer keyboard InputSource (ADR-0005). Device-only.
#ifndef UNIT_TEST

#include "input/cardputer_keyboard.hpp"

#include <Arduino.h>
#include <Wire.h>

namespace tp {
namespace {

// --- Cardputer internal keyboard bus (espp m5stack-cardputer; matches the sapper) ----------
constexpr uint8_t kTca8418Address = 0x34;
constexpr uint8_t kSdaPin = 8;
constexpr uint8_t kSclPin = 9;
constexpr uint8_t kIntPin = 11;  // TCA8418 INT: open-drain, active-low; needs a pull-up.

// TCA8418 registers (datasheet; the subset the key-event path touches).
constexpr uint8_t kRegCfg = 0x01;        // KE_IEN lives here.
constexpr uint8_t kRegIntStat = 0x02;    // write-1-to-clear; K_INT is bit 0.
constexpr uint8_t kRegKeyEventA = 0x04;  // pop one FIFO event (0x00 when empty).
constexpr uint8_t kRegKpGpio1 = 0x1D;    // +1,+2: which GPIOs form the key-scan matrix.

constexpr uint8_t kCfgKeIen = 0x01;      // enable key-event processing + INT.
constexpr uint8_t kIntStatKeyEvent = 0x01;  // K_INT bit, acked after each drain.

// 7 rows x 8 columns = 56 keys (matrix(7,8)): rows on bank 1 (0x7F), columns on bank 2 (0xFF).
constexpr uint8_t kMatrixRowsMask = 0x7F;
constexpr uint8_t kMatrixColsMask = 0xFF;

// Bound every FIFO drain a little above the 10-deep FIFO so a wedged bus can never spin.
constexpr int kFifoDrainLimit = 16;

// --- the Cardputer key grid (ADR-0005) ------------------------------------------------------
// 4 rows x 14 columns, the silkscreen layout, identical on both variants (espp key_map_). We
// fold case in the query path (ADR-0002), so only the unshifted value matters; `special` names
// the key's navigation/edit meaning, so a plain key and its arrow meaning are one table entry.
// Up/Down act whether or not fn is held (ADR-0008); esc/delete need fn (see mapPress).
enum class Special : uint8_t { None, Esc, Delete, Up, Down, Other };

struct Key {
  char value;        // the character this key types (0 for a modifier); '\n' '\b' '\t' as-is.
  Special special;   // its fn-layer meaning (Other = an fn key we ignore: left/right/F-keys).
};

constexpr int kRows = 4;
constexpr int kCols = 14;

// fn modifier position; the only modifier we track (the Cardputer's arrow layer).
constexpr int kFnRow = 2;
constexpr int kFnCol = 0;

constexpr Key kKeymap[kRows][kCols] = {
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

bool writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(kTca8418Address);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

uint8_t readRegister(uint8_t reg) {
  Wire.beginTransmission(kTca8418Address);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(kTca8418Address, static_cast<uint8_t>(1));
  return Wire.available() ? static_cast<uint8_t>(Wire.read()) : 0;
}

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

bool CardputerKeyboard::begin() {
  Wire.begin(kSdaPin, kSclPin);

  // Probe the TCA8418 to tell the variants apart (ADR-0005): an ACK is the ADV, no ACK is the
  // original (which repurposes GPIO 8/9 as 74HC138 address lines).
  Wire.beginTransmission(kTca8418Address);
  if (Wire.endTransmission() == 0) {
    variant_ = Variant::Adv;
    if (!beginTca8418()) {
      Serial.println("[FATAL] Cardputer ADV keyboard (TCA8418) failed to initialise");
      return false;
    }
    pinMode(kIntPin, INPUT_PULLUP);
    ready_ = true;
    return true;
  }

  // Original Cardputer: the 74HC138 matrix scan is not implemented yet (ADR-0005, LEDGER). Fail
  // loud rather than silently providing no input — runtime Maintenance of the mission depends
  // on knowing input is dead, and a dictionary with no input is useless.
  variant_ = Variant::Original;
  Serial.println(
      "[FATAL] original Cardputer detected (no TCA8418): the 74HC138 matrix keyboard driver is "
      "not implemented yet — see ADR-0005 / LEDGER. Input is unavailable on this board.");
  return false;
}

bool CardputerKeyboard::beginTca8418() {
  // Claim the key-scan matrix, flush any power-on events, clear INT, then enable key events.
  // Every write's ACK is checked so a controller that is present but not accepting config fails
  // begin() loud rather than half-initialising (§3). `ok` accumulates — one bad write fails.
  bool ok = true;
  ok &= writeRegister(kRegKpGpio1 + 0, kMatrixRowsMask);
  ok &= writeRegister(kRegKpGpio1 + 1, kMatrixColsMask);
  ok &= writeRegister(kRegKpGpio1 + 2, 0x00);

  for (int i = 0; i < kFifoDrainLimit && readRegister(kRegKeyEventA) != 0; ++i) {
  }
  ok &= writeRegister(kRegIntStat, 0xFF);
  ok &= writeRegister(kRegCfg, kCfgKeIen);
  return ok;
}

void CardputerKeyboard::drainTca8418() {
  // Drain the whole FIFO this cycle so a burst cannot leave a later event stuck behind, update
  // fn state as we go, and queue the events that map to the app's vocabulary.
  bool any = false;
  for (int i = 0; i < kFifoDrainLimit; ++i) {
    const uint8_t rawEvent = readRegister(kRegKeyEventA);
    if (rawEvent == 0) break;  // FIFO empty.
    any = true;

    const bool pressed = (rawEvent & 0x80) != 0;
    const uint8_t keyNumber = rawEvent & 0x7f;  // 1-based; 0 is "no key".
    if (keyNumber == 0) continue;

    // The TCA8418's own 7x8 (Tr,Tc) matrix maps to the 4x14 silkscreen grid (espp, ADR-0005):
    //   col = Tr*2 + (Tc > 3 ? 1 : 0);  row = Tc % 4
    const uint8_t n = keyNumber - 1;
    const uint8_t tr = n / 10;
    const uint8_t tc = n % 10;
    const int col = tr * 2 + (tc > 3 ? 1 : 0);
    const int row = tc % 4;
    if (row < 0 || row >= kRows || col < 0 || col >= kCols) continue;  // out of grid: ignore.

    if (row == kFnRow && col == kFnCol) {
      fnHeld_ = pressed;  // track the modifier on both edges; it emits no event itself.
      continue;
    }
    if (!pressed) continue;  // releases only update modifier state; we act on presses.

    const InputEvent event = mapPress(kKeymap[row][col], fnHeld_);
    if (!event.isNone()) enqueue(event);
  }
  if (any) writeRegister(kRegIntStat, kIntStatKeyEvent);  // ack so INT de-asserts.
}

void CardputerKeyboard::enqueue(const InputEvent& event) {
  if (qCount_ >= kQueueCap) return;  // full: a burst past 16 app-events is not a real input.
  queue_[(qHead_ + qCount_) % kQueueCap] = event;
  ++qCount_;
}

InputEvent CardputerKeyboard::poll() {
  if (!ready_) return InputEvent::none();

  if (qCount_ == 0 && variant_ == Variant::Adv) {
    // Nothing queued: refill only when INT is asserted (active-low). This is a GPIO read, not
    // I2C, so an idle poll never touches the bus.
    if (digitalRead(kIntPin) == LOW) drainTca8418();
  }

  if (qCount_ == 0) return InputEvent::none();
  const InputEvent event = queue_[qHead_];
  qHead_ = (qHead_ + 1) % kQueueCap;
  --qCount_;
  return event;
}

}  // namespace tp

#endif  // UNIT_TEST
