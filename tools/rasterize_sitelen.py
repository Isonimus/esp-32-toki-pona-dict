#!/usr/bin/env python3
"""Rasterize the sitelen pona font to fixed-size 1-bpp bitmaps (ADR-0003).

    python3 tools/rasterize_sitelen.py            # regenerate core/sitelen_gen.cpp
    python3 tools/rasterize_sitelen.py --check    # verify the committed file is up to date,
                                                  # exit 1 on drift, write nothing (CI guard)

Reads the vendored nasin nanpa font (third_party/) and, for every codepoint in the UCSUR
sitelen pona block, renders the glyph to a WIDTH x HEIGHT 1-bpp bitmap and emits the C++
data table core/sitelen_gen.cpp (consumed through core/sitelen.hpp). The font is NOT shipped
to the device and nothing parses it on-device; this is the whole of the font dependency.

Why the headword->codepoint mapping needs no hand-maintained table (ADR-0002 single-source):
the font names each glyph `<headword>Tok` and its cmap maps that glyph to a PUA codepoint, so
the headword alone resolves to a glyph through the font's own tables.

Why grayscale+threshold and not a 1-bit rasterizer: a hard monochrome rasterizer shreds the
thin curved strokes into scanline artifacts at these sizes (measured, ADR-0003). Rendering
antialiased grayscale and thresholding at 50% keeps the curves intact.
"""

import sys
from pathlib import Path

from fontTools.ttLib import TTFont
from PIL import Image, ImageFont

# --- the glyph contract (must match core/sitelen.hpp; the emitted static_assert guards it) -
WIDTH = 44
HEIGHT = 44
STRIDE = (WIDTH + 7) // 8          # bytes per row, byte-aligned
GLYPH_BYTES = STRIDE * HEIGHT

# Render at a slightly smaller em than the cell so the ink sits inside it with ~1px margin.
# pack_glyph fails loud if any glyph's ink exceeds the cell, so a regenerate is itself the
# clipping check — no glyph is silently cropped (re-verified at 44x44 for ADR-0013).
EM_PX = HEIGHT - 2
THRESHOLD = 128                    # grayscale value (0..255) at/above which a pixel is ink

# The UCSUR sitelen pona main block: the closed glyph set, one glyph per headword (ADR-0003).
UCSUR_FIRST = 0xF1900
UCSUR_LAST = 0xF1988               # inclusive -> 137 codepoints
GLYPH_NAME_SUFFIX = "Tok"          # nasin nanpa names glyphs e.g. "teloTok" for headword "telo"

ROOT = Path(__file__).resolve().parent.parent
FONT_PATH = ROOT / "third_party" / "nasin-nanpa" / "nasin-nanpa-UCSUR.otf"
OUT_PATH = ROOT / "core" / "sitelen_gen.cpp"
DICT_PATH = ROOT / "data" / "dictionary.json"


def headword_for(codepoint, glyph_name):
    """The headword a UCSUR glyph renders, from its font glyph name. Fails loud on any name
    that does not follow the `<headword>Tok` convention — a silent guess would mis-key glyphs."""
    if not glyph_name.endswith(GLYPH_NAME_SUFFIX):
        raise ValueError(
            f"U+{codepoint:04X}: glyph name {glyph_name!r} does not end with "
            f"{GLYPH_NAME_SUFFIX!r}; the font's naming convention changed — update this tool."
        )
    headword = glyph_name[: -len(GLYPH_NAME_SUFFIX)]
    if not headword:
        raise ValueError(f"U+{codepoint:04X}: glyph name {glyph_name!r} has an empty headword")
    return headword


def pack_glyph(font, codepoint):
    """Render one codepoint to a packed 1-bpp bitmap: grayscale (antialiased), centered in the
    WIDTH x HEIGHT cell, thresholded at 50%, MSB-first and row-major (see core/sitelen.hpp)."""
    mask = font.getmask(chr(codepoint), mode="L")  # ink bbox, antialiased (raw ImagingCore)
    gw, gh = mask.size
    if gw > WIDTH or gh > HEIGHT:
        raise ValueError(
            f"U+{codepoint:04X}: ink {gw}x{gh} exceeds the {WIDTH}x{HEIGHT} cell at EM_PX={EM_PX}"
        )
    px = Image.frombytes("L", mask.size, bytes(mask)).load()
    ox = (WIDTH - gw) // 2          # center horizontally
    oy = (HEIGHT - gh) // 2         # center vertically

    out = bytearray(GLYPH_BYTES)
    ink = 0
    for y in range(gh):
        for x in range(gw):
            if px[x, y] >= THRESHOLD:
                cx, cy = ox + x, oy + y
                out[cy * STRIDE + (cx >> 3)] |= 0x80 >> (cx & 7)
                ink += 1
    return bytes(out), ink


def c_identifier(headword):
    """A valid C identifier for a per-glyph array name. Headwords are lowercase ASCII, but
    sanitize defensively; uniqueness is asserted by the caller so a collision fails loud."""
    return "g_" + "".join(c if c.isalnum() else "_" for c in headword)


def cstr(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def main():
    check_only = "--check" in sys.argv[1:]

    tt = TTFont(str(FONT_PATH))
    cmap = tt.getBestCmap()

    glyphs = []  # (headword, packed_bytes)
    for cp in range(UCSUR_FIRST, UCSUR_LAST + 1):
        if cp not in cmap:
            raise ValueError(
                f"U+{cp:04X} is missing from the font cmap, but the UCSUR block "
                f"U+{UCSUR_FIRST:04X}..U+{UCSUR_LAST:04X} is declared contiguous — "
                f"the vendored font changed; re-check ADR-0003."
            )
        headword = headword_for(cp, cmap[cp])
        packed, ink = pack_glyph(ImageFont.truetype(str(FONT_PATH), EM_PX), cp)
        if ink == 0:
            raise ValueError(f"U+{cp:04X} ({headword}) rasterized to an empty bitmap")
        glyphs.append((headword, packed))

    if not glyphs:
        raise ValueError("no glyphs produced — refusing to emit an empty table")

    glyphs.sort(key=lambda g: g[0].encode("utf-8"))  # byte order, to match std::strcmp
    idents = [c_identifier(h) for h, _ in glyphs]
    if len(set(idents)) != len(idents):
        raise ValueError("two headwords map to the same C identifier; add disambiguation")

    text = render(glyphs, idents)
    report_dictionary_coverage({h for h, _ in glyphs})

    # --check compares the fresh render to the committed file without writing it — the CI
    # drift guard (ADR-0003), mirroring `tools/generate.mjs --check`. Fails loud on drift.
    if check_only:
        on_disk = OUT_PATH.read_text() if OUT_PATH.exists() else None
        if on_disk != text:
            print(
                f"FAIL: {OUT_PATH.relative_to(ROOT)} is out of date — run "
                f"`npm run generate:sitelen` and commit the result (ADR-0003).",
                file=sys.stderr,
            )
            sys.exit(1)
        print(f"PASS: {OUT_PATH.relative_to(ROOT)} up to date ({len(glyphs)} glyphs)")
        return

    OUT_PATH.write_text(text)
    print(
        f"generated {len(glyphs)} sitelen glyphs at {WIDTH}x{HEIGHT} "
        f"({len(glyphs) * GLYPH_BYTES} bytes = {len(glyphs) * GLYPH_BYTES / 1024:.1f} KB) "
        f"-> {OUT_PATH.relative_to(ROOT)}"
    )


def render(glyphs, idents):
    """Build the full core/sitelen_gen.cpp text in memory. Pure: returns the string and
    touches no filesystem, so the same output serves both write and --check."""
    lines = [
        "// GENERATED by tools/rasterize_sitelen.py from the vendored nasin nanpa font — DO NOT EDIT.",
        '#include "sitelen.hpp"',
        "",
        "namespace tp {",
        "",
        '// Guards the glyph dimensions against sitelen.hpp: if the header changes without a',
        "// regenerate, this fails the build rather than desyncing the bitmap stride.",
        f"static_assert(kSitelenWidth == {WIDTH} && kSitelenHeight == {HEIGHT},",
        '              "sitelen glyph size changed; re-run tools/rasterize_sitelen.py");',
        "",
    ]
    for (headword, packed), ident in zip(glyphs, idents):
        body = ", ".join(f"0x{b:02x}" for b in packed)
        lines.append(f"// {headword}")
        lines.append(f"static const uint8_t {ident}[kSitelenBytes] = {{ {body} }};")
    lines.append("")
    lines.append("const SitelenGlyph SITELEN_GLYPHS[] = {")
    for (headword, _), ident in zip(glyphs, idents):
        lines.append(f"  {{ {cstr(headword)}, {ident} }},")
    lines.append("};")
    lines.append(f"const uint16_t SITELEN_GLYPH_COUNT = {len(glyphs)};")
    lines.append("")
    lines.append("}  // namespace tp")
    lines.append("")
    return "\n".join(lines)


def report_dictionary_coverage(glyph_headwords):
    """Informational: which dictionary headwords have no glyph. Not fatal — extending the
    corpus to nimi ku suli beyond the 137-glyph block is a planned, legitimate state
    (LEDGER); render just omits the glyph for those. Printed so the gap is never silent."""
    import json

    entries = json.loads(DICT_PATH.read_text())["entries"]
    missing = [e["tp"] for e in entries if e.get("sitelen", e["tp"]) not in glyph_headwords]
    if missing:
        print(f"note: {len(missing)} dictionary headword(s) have no sitelen glyph: "
              f"{', '.join(missing)}")
    else:
        print(f"all {len(entries)} dictionary headwords have a sitelen glyph")


if __name__ == "__main__":
    main()
