// Cardputer keyboard InputSource (ADR-0005). Device-only.
#ifndef UNIT_TEST

#include "input/cardputer_keyboard.hpp"

#include <Arduino.h>
#include <Wire.h>

#include "input/cardputer_keymap.hpp"  // pure keymap + key policy (ADR-0011)
#include "input/cardputer_tca8418.hpp"  // pure ADV FIFO decode (ADR-0025)

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

// The key grid and the key→InputEvent policy (ADR-0008) now live in the pure cardputer_keymap
// unit (ADR-0011), host-tested; the ADV raw-byte→cell decode is also pure (ADR-0025).
// This file keeps the TCA8418 I/O and event queue.

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

    const CardputerKeyCell key = decodeTca8418Event(rawEvent);
    const KeyOutcome outcome = applyKey(key.row, key.col, key.pressed, fnHeld_);
    fnHeld_ = outcome.fnHeld;  // applyKey tracks the fn modifier (both edges) for us.
    if (!outcome.event.isNone()) enqueue(outcome.event);
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
