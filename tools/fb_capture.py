#!/usr/bin/env python3
"""Capture the device framebuffer as a PNG over USB serial (ADR-0006), for bring-up proofs.

    python3 tools/fb_capture.py --out build/bringup/empty.png
    python3 tools/fb_capture.py --port /dev/ttyACM0 --out build/bringup/query.png
    python3 tools/fb_capture.py --selftest        # host-only parser test, no hardware (CI)

Requires a firmware built with -DTP_DEBUG_SCREENSHOT (see ADR-0006):

    PLATFORMIO_BUILD_FLAGS=-DTP_DEBUG_SCREENSHOT pio run -e tp_cardputer -t upload

It sends the one-byte screenshot command, syncs on the begin-marker the firmware prints (so
boot and [FATAL] log lines are skipped), trusts the PPM header's dimensions, and reads exactly
width*height*3 bytes — never scanning the binary raster for an end-marker, since pixel data can
contain any byte. Writes both a .png (for review and diffing) and the raw .ppm beside it.

Hardware-only, so CI never runs the capture; it needs no pyserial (it reads the CDC device file
directly after putting it in raw mode with stty) and only Pillow beyond the standard library.
The pure framing parser is covered by --selftest, which the host lane runs.
"""

import argparse
import os
import re
import select
import subprocess
import sys
import time

# Must match main/main.cpp kScreenshotCommand (ADR-0006).
SCREENSHOT_COMMAND = b"\x10"
BEGIN_MARKER = b"--TP-PPM-BEGIN--"
# A full frame is serial-bound (~100 KB). On a real UART bridge at 115200 baud that is ~8.5 s to
# transfer (a native-USB CDC board does it in well under a second), so the window must clear that
# with margin while still failing rather than hanging if the firmware lacks the debug flag.
TIMEOUT_SECONDS = 15.0
# "P6", width, height, maxval, then exactly one whitespace before the binary raster (PPM spec).
PPM_HEADER = re.compile(rb"^\s*P6\s+(\d+)\s+(\d+)\s+(\d+)\s")


class FrameReader:
    """Buffered byte reader with the framing the device emits (ADR-0006). A subclass supplies
    bytes through _fill; the parsing here is shared and host-testable (see selftest)."""

    def __init__(self):
        self._buf = bytearray()

    def _fill(self, deadline):
        raise NotImplementedError

    def read_past_marker(self, marker, deadline):
        while marker not in self._buf:
            self._fill(deadline)
        del self._buf[: self._buf.index(marker) + len(marker)]

    def read_ppm_header(self, deadline):
        while True:
            match = PPM_HEADER.match(self._buf)
            if match:
                # Read the groups BEFORE mutating the buffer: a re.Match over a bytearray keeps
                # a reference to it and re-slices group() lazily, so deleting the prefix first
                # would make group() return the shifted (wrong) bytes — here, the pixel data.
                width, height = int(match.group(1)), int(match.group(2))
                del self._buf[: match.end()]
                return width, height
            self._fill(deadline)

    def read_exact(self, count, deadline):
        while len(self._buf) < count:
            self._fill(deadline)
        data = bytes(self._buf[:count])
        del self._buf[:count]
        return data


class SerialReader(FrameReader):
    """Reads a raw tty device file directly (no pyserial)."""

    def __init__(self, path, baud):
        super().__init__()
        # Set the line baud explicitly: a real UART bridge (e.g. the StickC Plus2's CH9102) runs at
        # this rate, so a wrong baud yields garbage and the marker never syncs. A native-USB CDC
        # board (the Cardputer) ignores baud, which is why early captures worked without setting it.
        # Must match the firmware's Serial.begin / platformio monitor_speed.
        subprocess.run(["stty", "-F", path, str(baud), "raw", "-echo"], check=True)
        self._fd = os.open(path, os.O_RDWR | os.O_NOCTTY)

    def close(self):
        os.close(self._fd)

    def request_frame(self):
        """Discard anything queued (so a capture never returns a stale frame), then ask for one."""
        while select.select([self._fd], [], [], 0)[0]:
            if not os.read(self._fd, 65536):
                break
        self._buf.clear()
        os.write(self._fd, SCREENSHOT_COMMAND)

    def _fill(self, deadline):
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise TimeoutError("no framebuffer data (is the firmware built with -DTP_DEBUG_SCREENSHOT?)")
        if select.select([self._fd], [], [], remaining)[0]:
            self._buf += os.read(self._fd, 65536)


def decode_frame(reader, deadline):
    reader.read_past_marker(BEGIN_MARKER, deadline)
    width, height = reader.read_ppm_header(deadline)
    raster = reader.read_exact(width * height * 3, deadline)
    return width, height, raster


def capture(port_path, out_path, baud):
    from PIL import Image  # deferred so --selftest needs only the standard library

    reader = SerialReader(port_path, baud)
    try:
        reader.request_frame()
        width, height, raster = decode_frame(reader, time.monotonic() + TIMEOUT_SECONDS)
    finally:
        reader.close()

    os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
    ppm_path = os.path.splitext(out_path)[0] + ".ppm"
    with open(ppm_path, "wb") as ppm:
        ppm.write(f"P6\n{width} {height}\n255\n".encode("ascii"))
        ppm.write(raster)
    Image.frombytes("RGB", (width, height), raster).save(out_path)
    print(f"captured {width}x{height} -> {out_path} (+ {ppm_path})", file=sys.stderr)


class _ChunkReader(FrameReader):
    """Feeds a fixed byte string a few bytes at a time, exercising the streaming parser (partial
    header, mutate-after-match) without hardware. selftest only."""

    def __init__(self, data, chunk=7):
        super().__init__()
        self._data = data
        self._pos = 0
        self._chunk = chunk

    def _fill(self, deadline):
        if self._pos >= len(self._data):
            raise TimeoutError("synthetic stream exhausted")
        end = self._pos + self._chunk
        self._buf += self._data[self._pos:end]
        self._pos = end


def selftest():
    width, height = 3, 2
    # The first pixel is all-zero on purpose: it is the byte that immediately follows the header,
    # so the mutate-before-read bug this guards against decodes b'\x00\x00\x00' as a dimension.
    pixels = bytes([0, 0, 0, 10, 20, 30, 40, 50, 60, 255, 0, 0, 0, 255, 0, 0, 0, 255])
    stream = (
        b"boot\n[FATAL] noise\n\n--TP-PPM-BEGIN--\n"
        + f"P6\n{width} {height}\n255\n".encode("ascii")
        + pixels
        + b"\n--TP-PPM-END--\n"
    )
    w, h, raster = decode_frame(_ChunkReader(stream), time.monotonic() + 5)
    assert (w, h) == (width, height), f"dimensions parsed as {w}x{h}"
    assert raster == pixels, "raster mismatch"
    print("fb_capture selftest OK", file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(description="Capture the device framebuffer as a PNG (ADR-0006).")
    parser.add_argument("--port", default="/dev/ttyACM0", help="serial device (default: /dev/ttyACM0)")
    parser.add_argument("--baud", type=int, default=115200,
                        help="line baud (default: 115200; match the firmware's Serial.begin / "
                             "monitor_speed — required for UART-bridge boards, ignored by USB CDC)")
    parser.add_argument("--out", help="output PNG path (a .ppm is written beside it)")
    parser.add_argument("--selftest", action="store_true", help="run the host-only parser test and exit")
    args = parser.parse_args()

    if args.selftest:
        selftest()
        return
    if not args.out:
        parser.error("--out is required unless --selftest")
    capture(args.port, args.out, args.baud)


if __name__ == "__main__":
    main()
