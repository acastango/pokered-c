#!/usr/bin/env python3
"""
extract_title_screen.py -- Convert pokered title assets to GB 2bpp C arrays.

Generates:
  src/data/title_screen_data.h
  src/data/title_screen_data.c

Sources (pokered-master/gfx/title):
  - pokemon_logo.png  (128x56, 2bpp, 16x7 tiles = 112)
  - red_version.png   (80x8,   1bpp, 10x1 tiles = 10)
  - player.png        (40x56,  2bpp, 5x7 tiles = 35)
  - splash/copyright.png      (152x8, 2bpp, use first 5 tiles)
  - title/gamefreak_inc.png   (72x8,  2bpp, 9 tiles)
"""

import struct
import zlib
from pathlib import Path

POKERED = Path(r"C:\Users\Anthony\pokered\pokered-master")
OUT = Path(r"C:\Users\Anthony\pokered\src\data")
TITLE = POKERED / "gfx" / "title"
SPLASH = POKERED / "gfx" / "splash"

POKEMON_LOGO_PNG = TITLE / "pokemon_logo.png"
RED_VERSION_PNG = TITLE / "red_version.png"
PLAYER_PNG = TITLE / "player.png"
COPYRIGHT_PNG = SPLASH / "copyright.png"
GAMEFREAK_INC_PNG = TITLE / "gamefreak_inc.png"


def decode_png_grayscale(path):
    """Return (width, height, pixels, bit_depth) for grayscale PNG (1bpp or 2bpp)."""
    data = path.read_bytes()
    assert data[:8] == b"\x89PNG\r\n\x1a\n", f"Not a PNG: {path}"

    idat_chunks = []
    i = 8
    width = height = bit_depth = color_type = 0
    while i < len(data):
        length = struct.unpack(">I", data[i : i + 4])[0]
        chunk_type = data[i + 4 : i + 8]
        chunk_data = data[i + 8 : i + 8 + length]

        if chunk_type == b"IHDR":
            width, height = struct.unpack(">II", chunk_data[:8])
            bit_depth = chunk_data[8]
            color_type = chunk_data[9]
            assert color_type == 0, f"Expected grayscale, got color_type={color_type}"
            assert bit_depth in (1, 2), f"Expected 1bpp/2bpp grayscale, got bit_depth={bit_depth}"
        elif chunk_type == b"IDAT":
            idat_chunks.append(chunk_data)

        i += 12 + length

    raw = zlib.decompress(b"".join(idat_chunks))
    stride = (width * bit_depth + 7) // 8

    pixels = []
    prev_row = bytes(stride)
    pos = 0

    for _ in range(height):
        filt = raw[pos]
        pos += 1
        row = bytearray(raw[pos : pos + stride])
        pos += stride

        if filt == 0:
            pass
        elif filt == 1:
            for j in range(1, stride):
                row[j] = (row[j] + row[j - 1]) & 0xFF
        elif filt == 2:
            for j in range(stride):
                row[j] = (row[j] + prev_row[j]) & 0xFF
        elif filt == 3:
            for j in range(stride):
                a = row[j - 1] if j > 0 else 0
                b = prev_row[j]
                row[j] = (row[j] + (a + b) // 2) & 0xFF
        elif filt == 4:
            for j in range(stride):
                a = row[j - 1] if j > 0 else 0
                b = prev_row[j]
                c = prev_row[j - 1] if j > 0 else 0
                pa = abs(b - c)
                pb = abs(a - c)
                pc = abs(a + b - 2 * c)
                pr = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                row[j] = (row[j] + pr) & 0xFF
        else:
            raise ValueError(f"Unknown PNG filter: {filt}")

        prev_row = bytes(row)

        if bit_depth == 1:
            for b in row:
                for shift in (7, 6, 5, 4, 3, 2, 1, 0):
                    pixels.append((b >> shift) & 1)
        else:  # bit_depth == 2
            for b in row:
                for shift in (6, 4, 2, 0):
                    pixels.append((b >> shift) & 3)

    return width, height, pixels[: width * height], bit_depth


def pixels_to_gb2bpp(width, height, pixels, bit_depth, invert=True):
    """Convert full image to tile-ordered GB 2bpp bytes."""
    assert width % 8 == 0 and height % 8 == 0

    tiles_x = width // 8
    tiles_y = height // 8
    out = bytearray()

    for ty in range(tiles_y):
        for tx in range(tiles_x):
            for row in range(8):
                lo = 0
                hi = 0
                for col in range(8):
                    pv = pixels[(ty * 8 + row) * width + (tx * 8 + col)]

                    if bit_depth == 1:
                        gb = 3 if pv else 0
                    else:
                        gb = pv

                    if invert:
                        gb = 3 - gb

                    bit = 7 - col
                    if gb & 1:
                        lo |= 1 << bit
                    if gb & 2:
                        hi |= 1 << bit

                out.append(lo)
                out.append(hi)

    return bytes(out)


def tile_bytes_to_c_lines(blob, tiles_per_line=1):
    tile_count = len(blob) // 16
    lines = []
    for t in range(tile_count):
        tile = blob[t * 16 : (t + 1) * 16]
        lines.append("    { " + ", ".join(f"0x{b:02X}" for b in tile) + " },")
    return lines


def main():
    lw, lh, lp, lbd = decode_png_grayscale(POKEMON_LOGO_PNG)
    rw, rh, rp, rbd = decode_png_grayscale(RED_VERSION_PNG)
    pw, ph, pp, pbd = decode_png_grayscale(PLAYER_PNG)
    cw, ch, cp, cbd = decode_png_grayscale(COPYRIGHT_PNG)
    gw, gh, gp, gbd = decode_png_grayscale(GAMEFREAK_INC_PNG)

    assert (lw, lh) == (128, 56)
    assert (rw, rh) == (80, 8)
    assert (pw, ph) == (40, 56)
    assert (cw, ch) == (152, 8)
    assert (gw, gh) == (72, 8)

    logo = pixels_to_gb2bpp(lw, lh, lp, lbd, invert=True)
    version = pixels_to_gb2bpp(rw, rh, rp, rbd, invert=True)
    player = pixels_to_gb2bpp(pw, ph, pp, pbd, invert=True)
    copyright_all = pixels_to_gb2bpp(cw, ch, cp, cbd, invert=True)
    gamefreak_inc = pixels_to_gb2bpp(gw, gh, gp, gbd, invert=True)

    # title.asm loads NintendoCopyrightLogoGraphics and uses first 5 tiles
    # for ©'95.'96.'98 prefix; then 9 GAME FREAK inc. tiles.
    copyright_prefix = copyright_all[: 5 * 16]

    logo_tiles = len(logo) // 16
    version_tiles = len(version) // 16
    player_tiles = len(player) // 16
    copyright_tiles = len(copyright_prefix) // 16
    gamefreak_inc_tiles = len(gamefreak_inc) // 16
    title_copyright_tiles = copyright_tiles + gamefreak_inc_tiles

    assert logo_tiles == 112
    assert version_tiles == 10
    assert player_tiles == 35
    assert copyright_tiles == 5
    assert gamefreak_inc_tiles == 9

    h_lines = [
        "#pragma once",
        "/* title_screen_data.h -- AUTO-GENERATED by tools/extract_title_screen.py. */",
        "#include <stdint.h>",
        "",
        "#define TITLE_LOGO_TILES        112",
        "#define TITLE_RED_VERSION_TILES   10",
        "#define TITLE_PLAYER_TILES       35",
        "#define TITLE_COPYRIGHT_TILES    14",
        "",
        "extern const uint8_t gTitleLogoTiles[TITLE_LOGO_TILES][16];",
        "extern const uint8_t gTitleRedVersionTiles[TITLE_RED_VERSION_TILES][16];",
        "extern const uint8_t gTitlePlayerTiles[TITLE_PLAYER_TILES][16];",
        "extern const uint8_t gTitleCopyrightTiles[TITLE_COPYRIGHT_TILES][16];",
        "",
    ]

    c_lines = [
        "/* title_screen_data.c -- AUTO-GENERATED by tools/extract_title_screen.py. */",
        "#include \"title_screen_data.h\"",
        "",
        "const uint8_t gTitleLogoTiles[TITLE_LOGO_TILES][16] = {",
        *tile_bytes_to_c_lines(logo),
        "};",
        "",
        "const uint8_t gTitleRedVersionTiles[TITLE_RED_VERSION_TILES][16] = {",
        *tile_bytes_to_c_lines(version),
        "};",
        "",
        "const uint8_t gTitlePlayerTiles[TITLE_PLAYER_TILES][16] = {",
        *tile_bytes_to_c_lines(player),
        "};",
        "",
        "const uint8_t gTitleCopyrightTiles[TITLE_COPYRIGHT_TILES][16] = {",
        *tile_bytes_to_c_lines(copyright_prefix),
        *tile_bytes_to_c_lines(gamefreak_inc),
        "};",
        "",
    ]

    (OUT / "title_screen_data.h").write_text("\n".join(h_lines), encoding="ascii")
    (OUT / "title_screen_data.c").write_text("\n".join(c_lines), encoding="ascii")

    print(
        "Wrote title_screen_data.[ch]: "
        f"logo={logo_tiles}, version={version_tiles}, player={player_tiles}, copyright={title_copyright_tiles}"
    )


if __name__ == "__main__":
    main()
