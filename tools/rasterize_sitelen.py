#!/usr/bin/env python3
"""Rasterize the sitelen pona font to fixed-size 1-bpp bitmaps (ADR-0003, ADR-0015).

    python3 tools/rasterize_sitelen.py            # regenerate core/sitelen_gen.cpp + core/logo_gen.cpp
    python3 tools/rasterize_sitelen.py --check    # verify the committed files are up to date,
                                                  # exit 1 on drift, write nothing (CI guard)

Reads the vendored nasin nanpa font (third_party/) and emits two generated files:

  - core/sitelen_gen.cpp: for every codepoint in the UCSUR sitelen pona block, the glyph
    rendered to a WIDTH x HEIGHT 1-bpp bitmap (consumed through core/sitelen.hpp).
  - core/logo_gen.cpp: the toki pona logo — `toki` with the `pona` smile nested inside its
    ring — shaped from three codepoints through the font and rasterized to its own tight
    bounding box (consumed through core/logo.hpp, ADR-0015).

The font is NOT shipped to the device and nothing parses it on-device; these bitmaps are the
whole of the font dependency.

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
# pack_mask fails loud if any glyph's ink exceeds its cell, so a regenerate is itself the
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
LOGO_OUT_PATH = ROOT / "core" / "logo_gen.cpp"
DICT_PATH = ROOT / "data" / "dictionary.json"

# --- the toki pona logo (ADR-0015): one shaped composition, not a per-headword glyph --------
# The official logo is `toki` (a ring with three strokes above it) with the `pona` smile nested
# inside the ring. nasin nanpa builds exactly that from three codepoints — toki, the sitelen pona
# *scaling* joiner, pona — shaped through the font's GSUB table, which requires the raqm layout
# engine. It is not a headword glyph, so it is emitted as one standalone asset at its own tight
# bounding box, separate from the WIDTH x HEIGHT SITELEN_GLYPHS cell.
LOGO_CODEPOINTS = "\U000F196C\U000F1996\U000F1954"  # toki + joinScale (U+F1996) + pona
LOGO_EM_PX = 88   # renders to an ~88x83 hero — 2x the sitelen cell height (measured, ADR-0015)


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


def pack_mask(mask, cell_w, cell_h):
    """Pack a grayscale ink mask into a `cell_w` x `cell_h` 1-bpp bitmap: thresholded at 50%,
    centered in the cell, MSB-first and row-major (the core/sitelen.hpp contract). Returns
    (packed_bytes, ink_pixel_count). Fails loud if the ink is larger than the cell, so nothing
    is ever silently cropped — the single packing primitive for both the glyph cell and the logo
    (which passes a cell equal to its own bounding box)."""
    gw, gh = mask.size
    if gw > cell_w or gh > cell_h:
        raise ValueError(f"ink {gw}x{gh} exceeds the {cell_w}x{cell_h} cell")
    px = Image.frombytes("L", mask.size, bytes(mask)).load()
    stride = (cell_w + 7) // 8
    ox = (cell_w - gw) // 2          # center horizontally
    oy = (cell_h - gh) // 2          # center vertically

    out = bytearray(stride * cell_h)
    ink = 0
    for y in range(gh):
        for x in range(gw):
            if px[x, y] >= THRESHOLD:
                cx, cy = ox + x, oy + y
                out[cy * stride + (cx >> 3)] |= 0x80 >> (cx & 7)
                ink += 1
    return bytes(out), ink


def pack_glyph(font, codepoint):
    """Render one UCSUR codepoint to a packed WIDTH x HEIGHT 1-bpp glyph (see pack_mask)."""
    mask = font.getmask(chr(codepoint), mode="L")  # ink bbox, antialiased (raw ImagingCore)
    try:
        return pack_mask(mask, WIDTH, HEIGHT)
    except ValueError as exc:
        raise ValueError(f"U+{codepoint:04X}: {exc} at EM_PX={EM_PX}") from exc


def build_logo():
    """Render the shaped logo sequence to a tight 1-bpp bitmap (ADR-0015). Returns
    (width, height, packed_bytes). The raqm layout engine is required: it applies the font's
    GSUB shaping so the scaling joiner nests `pona` inside `toki`, which the default layout
    (no shaping) would not do — it would lay the three glyphs out side by side."""
    font = ImageFont.truetype(str(FONT_PATH), LOGO_EM_PX, layout_engine=ImageFont.Layout.RAQM)
    mask = font.getmask(LOGO_CODEPOINTS, mode="L")
    gw, gh = mask.size
    packed, ink = pack_mask(mask, gw, gh)  # tight bbox: the cell is the ink's own bounding box
    if ink == 0:
        raise ValueError("the logo rasterized to an empty bitmap")
    return gw, gh, packed


def c_identifier(headword):
    """A valid C identifier for a per-glyph array name. Headwords are lowercase ASCII, but
    sanitize defensively; uniqueness is asserted by the caller so a collision fails loud."""
    return "g_" + "".join(c if c.isalnum() else "_" for c in headword)


def cstr(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def emit(path, text, check_only):
    """Write `text` to `path`, or (check_only) compare to disk and fail loud on drift — the CI
    drift guard (ADR-0003), mirroring `tools/generate.mjs --check`. Shared by both generated
    files so each is checked identically. Returns True on write, False on a clean --check."""
    if check_only:
        on_disk = path.read_text() if path.exists() else None
        if on_disk != text:
            print(
                f"FAIL: {path.relative_to(ROOT)} is out of date — run "
                f"`npm run generate:sitelen` and commit the result.",
                file=sys.stderr,
            )
            sys.exit(1)
        print(f"PASS: {path.relative_to(ROOT)} up to date")
        return False
    path.write_text(text)
    return True


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

    sitelen_text = render(glyphs, idents)
    logo_w, logo_h, logo_bytes = build_logo()
    logo_text = render_logo(logo_w, logo_h, logo_bytes)
    report_dictionary_coverage({h for h, _ in glyphs})

    wrote_sitelen = emit(OUT_PATH, sitelen_text, check_only)
    wrote_logo = emit(LOGO_OUT_PATH, logo_text, check_only)
    if check_only:
        return

    if wrote_sitelen:
        print(
            f"generated {len(glyphs)} sitelen glyphs at {WIDTH}x{HEIGHT} "
            f"({len(glyphs) * GLYPH_BYTES} bytes = {len(glyphs) * GLYPH_BYTES / 1024:.1f} KB) "
            f"-> {OUT_PATH.relative_to(ROOT)}"
        )
    if wrote_logo:
        print(
            f"generated the toki pona logo at {logo_w}x{logo_h} "
            f"({len(logo_bytes)} bytes) -> {LOGO_OUT_PATH.relative_to(ROOT)}"
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


def render_logo(width, height, packed):
    """Build the full core/logo_gen.cpp text in memory (ADR-0015). Pure, like render()."""
    stride = (width + 7) // 8
    body = ", ".join(f"0x{b:02x}" for b in packed)
    lines = [
        "// GENERATED by tools/rasterize_sitelen.py from the vendored nasin nanpa font — DO NOT EDIT.",
        '#include "logo.hpp"',
        "",
        "namespace tp {",
        "",
        "// The toki pona logo (ADR-0015): `toki` with the `pona` smile nested in its ring, shaped",
        "// from three codepoints through the font and packed 1-bpp (core/sitelen.hpp bitmap format).",
        f"static const uint8_t kLogoBitmap[{stride * height}] = {{ {body} }};",
        "",
        f"const Logo TP_LOGO = {{ {width}, {height}, kLogoBitmap }};",
        "",
        "}  // namespace tp",
        "",
    ]
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
