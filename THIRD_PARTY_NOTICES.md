# Third-party notices

This product includes third-party software. Their license notices are reproduced below and
**must travel with every distributed copy** of the firmware — source, binary, or flashed
device image. `tools/check-notices.mjs` (run by `npm run test:host`) fails the build if any
notice here drifts from its vendored source, so this obligation cannot rot silently.

---

## nasin nanpa (sitelen pona font)

- Upstream: https://github.com/etbcor/nasin-nanpa — release n4.0.2, UCSUR variant.
- Vendored at `third_party/nasin-nanpa/` (see its `PROVENANCE.md`).
- **Not** shipped or parsed on-device: the font is rasterized at build time (ADR-0003) into
  the fixed-size 1-bpp bitmaps in `core/sitelen_gen.cpp`. Those bitmaps are a derivative work
  of the font, so this notice accompanies the firmware that embeds them.

Verbatim license (`third_party/nasin-nanpa/LICENSE`):

```
MIT License

Copyright (c) 2022 jan Itan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
