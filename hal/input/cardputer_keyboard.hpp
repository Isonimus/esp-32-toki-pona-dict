#pragma once

// The Cardputer keyboard as an InputSource (ADR-0005). Device-only: touches Wire/GPIO, behind
// the UNIT_TEST guard, never on the host lane.
//
// One target serves both Cardputer variants: begin() probes the TCA8418 at I2C 0x34 on GPIO
// 8/9 — an ACK is the ADV (read the TCA8418 FIFO), no ACK is the original (74HC138 matrix).
// The ADV path is implemented and brought up first; the matrix path is a fail-loud stub until
// its own slice (ADR-0005), so begin() returns false on an original board rather than
// pretending to work.
//
// poll() returns one abstract InputEvent per call (ADR-0001). It reads the TCA8418 FIFO only
// when the INT line is asserted (a cheap GPIO read when idle), decodes each raw event, and
// queues the ones that map to an app event; subsequent polls drain the queue before touching
// the bus again.

#ifndef UNIT_TEST  // device-only: Wire/GPIO are not available on the host lane.

#include <cstdint>

#include "input_event.hpp"
#include "input_source.hpp"

namespace tp {

class CardputerKeyboard : public InputSource {
 public:
  bool begin() override;
  InputEvent poll() override;

 private:
  // Detected variant. Only the ADV is wired this slice; Original makes begin() fail loud.
  enum class Variant : uint8_t { Unknown, Adv, Original };

  bool beginTca8418();   // ADV bring-up: the espp/Adafruit register sequence (ADR-0005).
  void drainTca8418();   // read the FIFO, decode + enqueue, track fn, ack INT.

  // The pending app events decoded from one FIFO drain, returned one per poll(). Sized above
  // the 10-deep TCA8418 FIFO so a full burst always fits.
  static constexpr int kQueueCap = 16;
  InputEvent queue_[kQueueCap];
  int qHead_ = 0;
  int qCount_ = 0;
  void enqueue(const InputEvent& event);

  Variant variant_ = Variant::Unknown;
  bool ready_ = false;   // begin() succeeded; poll() is inert until then.
  bool fnHeld_ = false;  // the only modifier we track — the Cardputer's arrow layer.
};

}  // namespace tp

#endif  // UNIT_TEST
