---
id: 0006
title: Serial framebuffer capture for reproducible bring-up proofs
status: accepted
date: 2026-10-08
---

# ADR-0006 — Serial framebuffer capture for reproducible bring-up proofs

On 2026-10-08, with a Cardputer ADV in hand, we came to produce the bring-up proof ADR-0001
and ADR-0005 require ("a screenshot + a short input walk-through"). A phone photo of the 1.14"
panel satisfies the letter of that bar but not its spirit: it is not reproducible, it cannot
be diffed against a later build, and its fidelity depends on lighting and angle. This decision
adds a pixel-exact capture over the link we already have.

## Context

- The firmware already holds the exact frame the panel shows: `LgfxDisplaySurface` composites
  into an off-screen `LGFX_Sprite` canvas and blits it whole in `present()` (ADR-0005). The
  canvas *is* the ground truth; a camera only re-samples it worse.
- Every target so far is an ESP32-S3 whose native USB enumerates as a CDC serial port, already
  wired for logs (`ARDUINO_USB_CDC_ON_BOOT=1`). Keyboard input arrives on the TCA8418 I²C bus,
  not on serial, so the serial receive line carries no application traffic and is free for a
  debug command.
- The repository already depends on Pillow as a build-time Python dependency for the sitelen
  rasterizer (ADR-0003, `tools/requirements-sitelen.txt`), so decoding an image on the host
  adds no new dependency class.

## Decision

1. **The capture is a concrete-driver debug concern, never part of the seam.** The method
   lives on `LgfxDisplaySurface` only; the pure `DisplaySurface` interface (ADR-0001) gains
   nothing and stays host-compilable. The app never sees it.

2. **The frame is streamed as a binary PPM (P6).** The method walks the canvas one row at a
   time with `readRect` into a fixed RGB565 row buffer sized to the widest panel we support,
   expands each pixel to RGB888 with the standard 5/6/5-bit replication, and writes the bytes
   to a `Print&`. Row-at-a-time means no ~100 KB frame allocation on a PSRAM-less board; going
   through `readRect` + an explicit 565→888 expansion means the on-the-wire byte order is
   defined by this code, not by LovyanGFX's internal buffer layout. PPM because it is the
   simplest self-describing format Pillow reads directly.

3. **It is compiled only under `-DTP_DEBUG_SCREENSHOT`.** Production firmware does not carry
   it (it is debug scaffolding, not a feature). There is **no new PlatformIO env**: one env
   means one board (ADR-0001), and a debug env would break that. The proof build is a
   documented one-liner that layers the flag onto the real board build:

   ```
   PLATFORMIO_BUILD_FLAGS=-DTP_DEBUG_SCREENSHOT pio run -e tp_cardputer -t upload
   ```

4. **A command byte on serial triggers one frame.** Under the same guard, `loop()` dumps the
   canvas when it reads a single agreed byte from `Serial`. On demand (not a boot auto-dump)
   because the walk-through captures distinct states — empty view, a typed query, a selected
   entry — and each wants its own frame. A begin-marker line precedes the PPM so the host can
   synchronise past boot and `[FATAL]` log lines; the host then trusts the PPM header's
   dimensions and reads exactly `width*height*3` bytes. It never scans the binary for an
   end-marker, because pixel data can contain any byte sequence.

5. **The host decoder is `tools/fb_capture.py`.** Standard library + Pillow; it opens the CDC
   device file directly (after putting it in raw mode with `stty`), so it needs no `pyserial`.
   It sends the command byte, reads the framed PPM, and writes both a `.png` (for review and
   diffing) and the raw `.ppm`. The capture needs hardware, so CI never runs it; but its pure
   framing parser is covered by `--selftest`, which the host lane (`npm run test:host`) runs, so
   a regression in the marker sync or header parse fails without a board (mirroring the
   `rasterize_sitelen.py --check` idiom).

## Consequences

- The bring-up proof becomes a reproducible, pixel-exact PNG for any board built on
  `LgfxDisplaySurface`, usable for first bring-up and for later regression captures.
- Production builds are unchanged and stay lean; the debug path exists only in a build made
  with the flag.
- The dump is serial-bound and slow (~100 KB a frame), which is irrelevant to an interactive
  bring-up.
- A future board whose surface is not `LgfxDisplaySurface` would need its own capture (or the
  guarded `loop()` block adjusted); that cost is deferred to when such a board exists, not
  paid speculatively now.
