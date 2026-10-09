#!/usr/bin/env python3
"""Rasterize the sitelen pona font to fixed-size 1-bpp bitmaps (ADR-0003, ADR-0015).

    python3 tools/rasterize_sitelen.py            # regenerate core/sitelen_gen.cpp + core/logo_gen.cpp
    python3 tools/rasterize_sitelen.py --check    # verify the committed files are up to date,
                                                  # exit 1 on drift, write nothing (CI guard)

Reads the vendored fonts (third_party/) and emits three generated files:

  - core/sitelen_gen.cpp: for every codepoint in the UCSUR sitelen pona block, the glyph
    rendered to a WIDTH x HEIGHT 1-bpp bitmap (consumed through core/sitelen.hpp).
  - core/logo_gen.cpp: the toki pona logo — `toki` with the `pona` smile nested inside its
    ring — shaped from three codepoints through the font and rasterized to its own tight
    bounding box (consumed through core/logo.hpp, ADR-0015).
  - core/cjk_gen.cpp: the CJK gloss font — each codepoint a display-only language (ADR-0017)
    needs, rendered to a CJK_WIDTH x CJK_HEIGHT 1-bpp cell from the vendored Noto Sans CJK SC
    subset, keyed by codepoint (consumed through core/cjk.hpp, ADR-0018).

Neither font is shipped to the device and nothing parses them on-device; these bitmaps are the
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

# --- the CJK gloss font (ADR-0018): fixed-cell 1-bpp glyphs for display-only languages --------
# Simplified Chinese (and any future display-only language, ADR-0017) renders its detail label and
# glosses as fixed-size cells blitted one per codepoint (app/render.cpp), because CJK is square and
# monospaced — the same 1-bpp pattern as sitelen, keyed by codepoint instead of by headword. Only
# the codepoints the data actually uses are rasterized, from the vendored Noto Sans CJK SC subset;
# a whole CJK font would cost 10x-600x the flash (ADR-0018).
CJK_WIDTH = 14
CJK_HEIGHT = 14
CJK_STRIDE = (CJK_WIDTH + 7) // 8               # bytes per row, byte-aligned
CJK_GLYPH_BYTES = CJK_STRIDE * CJK_HEIGHT
CJK_EM_PX = CJK_HEIGHT - 1  # render 1px under the cell so the tallest ink (ASCII parens) fits
# The detail-cycle label "< %s  %d/%d >" is drawn through the CJK cells for a display-only language
# (app/render.cpp), so its ASCII furniture must be rasterized into the same set.
CJK_LABEL_CHARS = "<>/ 0123456789"
CJK_FONT_PATH = ROOT / "third_party" / "noto-sans-cjk-subset" / "NotoSansCJKsc-Regular-subset.ttf"
CJK_OUT_PATH = ROOT / "core" / "cjk_gen.cpp"
LANG_PATH = ROOT / "data" / "languages.json"


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


def cjk_codepoints():
    """The exact codepoint set the CJK cell font must cover (ADR-0018): for every display-only
    (non-searchable) language, its native label and all its glosses, plus the detail-cycle label
    charset. Computed from the data so it stays a data-driven set, never a hand-kept list, and
    sorted so the emitted table is binary-searchable by codepoint."""
    import json

    languages = json.loads(LANG_PATH.read_text())["languages"]
    entries = json.loads(DICT_PATH.read_text())["entries"]
    display_only = [l for l, m in languages.items() if m.get("searchable", True) is False]
    if not display_only:
        raise ValueError("no display-only language found; the CJK font exists to serve one (ADR-0017)")
    cps = {ord(c) for c in CJK_LABEL_CHARS}
    for lang in display_only:
        cps.update(ord(c) for c in languages[lang]["native"])
        for e in entries:
            for sense in e.get(lang, []):
                cps.update(ord(c) for c in sense)
    for cp in cps:
        if cp > 0xFFFF:
            raise ValueError(f"U+{cp:X} is outside the BMP; core/cjk.hpp keys codepoints as uint16")
    return sorted(cps)


def build_cjk():
    """Rasterize every CJK-font codepoint to a packed CJK_WIDTH x CJK_HEIGHT 1-bpp cell from the
    vendored Noto subset (ADR-0018). Returns [(codepoint, packed_bytes)], codepoint-sorted. Fails
    loud if the subset lacks a glyph for a needed codepoint — a signal to re-subset (PROVENANCE).
    A codepoint present but with no ink (e.g. space) packs to an empty cell, which only advances."""
    tt = TTFont(str(CJK_FONT_PATH))
    cmap = tt.getBestCmap()
    font = ImageFont.truetype(str(CJK_FONT_PATH), CJK_EM_PX)
    out = []
    for cp in cjk_codepoints():
        if cp not in cmap:
            raise ValueError(
                f"U+{cp:04X} ({chr(cp)!r}) is missing from the vendored Noto subset; re-subset the "
                f"font per third_party/noto-sans-cjk-subset/PROVENANCE.md"
            )
        mask = font.getmask(chr(cp), mode="L")
        if mask.size == (0, 0):
            packed = bytes(CJK_GLYPH_BYTES)  # whitespace: an empty cell
        else:
            try:
                packed, _ink = pack_mask(mask, CJK_WIDTH, CJK_HEIGHT)
            except ValueError as exc:
                raise ValueError(f"U+{cp:04X}: {exc} at CJK_EM_PX={CJK_EM_PX}") from exc
        out.append((cp, packed))
    return out


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
    cjk_glyphs = build_cjk()
    cjk_text = render_cjk(cjk_glyphs)
    report_dictionary_coverage({h for h, _ in glyphs})

    wrote_sitelen = emit(OUT_PATH, sitelen_text, check_only)
    wrote_logo = emit(LOGO_OUT_PATH, logo_text, check_only)
    wrote_cjk = emit(CJK_OUT_PATH, cjk_text, check_only)
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
    if wrote_cjk:
        total = len(cjk_glyphs) * CJK_GLYPH_BYTES
        print(
            f"generated {len(cjk_glyphs)} CJK glyphs at {CJK_WIDTH}x{CJK_HEIGHT} "
            f"({total} bytes = {total / 1024:.1f} KB) -> {CJK_OUT_PATH.relative_to(ROOT)}"
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


def render_cjk(glyphs):
    """Build the full core/cjk_gen.cpp text in memory (ADR-0018). Pure, like render(). Per-glyph
    arrays are named cjk_<hex> and the table rows are `{ 0x<hex>, cjk_<hex> }`, so the covered
    codepoint set is greppable — tools/check-font-coverage.mjs reads it straight from this file."""
    lines = [
        "// GENERATED by tools/rasterize_sitelen.py from the vendored Noto Sans CJK SC subset — DO NOT EDIT.",
        '#include "cjk.hpp"',
        "",
        "namespace tp {",
        "",
        "// Guards the cell dimensions against cjk.hpp: a header change without a regenerate fails",
        "// the build rather than desyncing the bitmap stride.",
        f"static_assert(kCjkWidth == {CJK_WIDTH} && kCjkHeight == {CJK_HEIGHT},",
        '              "CJK cell size changed; re-run tools/rasterize_sitelen.py");',
        "",
    ]
    for cp, packed in glyphs:
        body = ", ".join(f"0x{b:02x}" for b in packed)
        lines.append(f"static const uint8_t cjk_{cp:04x}[kCjkBytes] = {{ {body} }};")
    lines.append("")
    lines.append("const CjkGlyph CJK_GLYPHS[] = {")
    for cp, _ in glyphs:
        lines.append(f"  {{ 0x{cp:04x}, cjk_{cp:04x} }},")
    lines.append("};")
    lines.append(f"const uint16_t CJK_GLYPH_COUNT = {len(glyphs)};")
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
