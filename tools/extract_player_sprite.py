#!/usr/bin/env python3
"""
extract_player_sprite.py -- Convert red.png (16x96) to 6-frame player sprite.

Generates:
  src/data/player_sprite.h  -- gPlayerGfx[6][4][16]
  src/data/player_sprite.c  -- 6 animation frames, 4 tiles each

red.png is 16px wide x 96px tall = 6 frames of 16x16:
  Frame 0: DOWN walk A   (tiles at y=0)
  Frame 1: DOWN walk B   (tiles at y=16)
  Frame 2: LEFT walk A   (tiles at y=32)
  Frame 3: UP walk A     (tiles at y=48)
  Frame 4: UP walk B     (tiles at y=64)
  Frame 5: LEFT walk B   (tiles at y=80)

Each 16x16 frame = 4 tiles (2x2 grid):
  [0]=TL [1]=TR [2]=BL [3]=BR

RIGHT facing = LEFT frames (2,5) with OAM_FLAG_FLIP_X applied by the renderer.

2bpp convention (same as extract_sprites.py):
  PNG 2bpp grayscale: dark=0, light=3
  GB 2bpp:            color 0=transparent/lightest, 3=darkest
  Conversion:         gb_color = 3 - png_value  (invert)
"""

import struct, zlib
from pathlib import Path

POKERED = Path(r"C:\Users\Anthony\pokered\pokered-master")
RED_PNG = POKERED / "gfx" / "sprites" / "red.png"
OUT     = Path(r"C:\Users\Anthony\pokered\src\data")

FRAME_COUNT = 6
TILES_PER_FRAME = 4   # 2x2 grid of 8x8 tiles
TILE_BYTES = 16       # bytes per 2bpp 8x8 tile

# ---------------------------------------------------------------------------
# PNG decoder (2bpp grayscale, no PIL required) — copied from extract_sprites.py
# ---------------------------------------------------------------------------

def decode_png_2bpp_grayscale(path):
    with open(path, "rb") as f:
        data = f.read()
    assert data[:8] == b'\x89PNG\r\n\x1a\n', "Not a PNG"
    idat_chunks = []
    i = 8
    width = height = 0
    while i < len(data):
        length = struct.unpack('>I', data[i:i+4])[0]
        chunk_type = data[i+4:i+8]
        chunk_data = data[i+8:i+8+length]
        if chunk_type == b'IHDR':
            width, height = struct.unpack('>II', chunk_data[:8])
            bit_depth = chunk_data[8]
            color_type = chunk_data[9]
            assert bit_depth == 2 and color_type == 0, \
                f"Expected 2bpp grayscale, got bd={bit_depth} ct={color_type}"
        elif chunk_type == b'IDAT':
            idat_chunks.append(chunk_data)
        i += 12 + length
    raw = zlib.decompress(b''.join(idat_chunks))
    stride = (width * 2 + 7) // 8
    pixels = []
    prev_row = bytes(stride)
    pos = 0
    for _ in range(height):
        filt = raw[pos]; pos += 1
        row_raw = bytearray(raw[pos:pos+stride]); pos += stride
        if filt == 0:
            pass
        elif filt == 1:
            for j in range(1, stride):
                row_raw[j] = (row_raw[j] + row_raw[j-1]) & 0xFF
        elif filt == 2:
            for j in range(stride):
                row_raw[j] = (row_raw[j] + prev_row[j]) & 0xFF
        elif filt == 3:
            for j in range(stride):
                a = row_raw[j-1] if j > 0 else 0
                b = prev_row[j]
                row_raw[j] = (row_raw[j] + (a + b) // 2) & 0xFF
        elif filt == 4:
            for j in range(stride):
                a = row_raw[j-1] if j > 0 else 0
                b = prev_row[j]
                c = prev_row[j-1] if j > 0 else 0
                pa = abs(b - c); pb = abs(a - c); pc = abs(a + b - 2*c)
                pr = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                row_raw[j] = (row_raw[j] + pr) & 0xFF
        else:
            raise ValueError(f"Unknown filter {filt}")
        prev_row = bytes(row_raw)
        for byte in row_raw:
            for shift in (6, 4, 2, 0):
                pixels.append((byte >> shift) & 3)
    return width, height, pixels[:width * height]

def pixels_to_gb2bpp(width, height, pixels, tile_x, tile_y, invert=True):
    """Convert one 8x8 tile at grid position (tile_x, tile_y) to 16 GB 2bpp bytes."""
    out = bytearray()
    for row in range(8):
        lo = 0; hi = 0
        for col in range(8):
            pv = pixels[(tile_y * 8 + row) * width + (tile_x * 8 + col)]
            if invert:
                pv = 3 - pv
            bit = 7 - col
            if pv & 1: lo |= (1 << bit)
            if pv & 2: hi |= (1 << bit)
        out.append(lo)
        out.append(hi)
    return bytes(out)

def bytes_to_c_hex(data):
    return ", ".join(f"0x{b:02X}" for b in data)

FRAME_NAMES = [
    "DOWN walk A",
    "DOWN walk B",
    "LEFT walk A",
    "UP walk A",
    "UP walk B",
    "LEFT walk B",
]

def main():
    w, h, pixels = decode_png_2bpp_grayscale(RED_PNG)
    assert w == 16 and h == 96, f"Expected 16x96, got {w}x{h}"

    # Extract 6 frames × 4 tiles
    # Frame f is at y offset f*16 px = f*2 tile rows
    frames = []
    for f in range(FRAME_COUNT):
        frame_tiles = []
        frame_ty = f * 2  # top tile row of this frame
        for ty in range(2):
            for tx in range(2):
                tile = pixels_to_gb2bpp(w, h, pixels, tx, frame_ty + ty)
                frame_tiles.append(tile)
        frames.append(frame_tiles)  # [TL, TR, BL, BR]

    # Write header
    h_lines = [
        "#pragma once",
        "/* player_sprite.h -- Generated from pokered-master gfx/sprites/red.png */",
        "#include <stdint.h>",
        "",
        "/* 6 animation frames, 4 tiles each (2x2 = TL,TR,BL,BR), 16 bytes per tile.",
        " * Frame index: 0=DOWN_A 1=DOWN_B 2=LEFT_A 3=UP_A 4=UP_B 5=LEFT_B",
        " * RIGHT = LEFT frames (2,5) rendered with OAM_FLAG_FLIP_X. */",
        "#define PLAYER_FRAMES      6",
        "#define PLAYER_TILE_BYTES  16",
        "extern const uint8_t gPlayerGfx[PLAYER_FRAMES][4][PLAYER_TILE_BYTES];",
    ]

    # Write C source
    c_lines = [
        "/* player_sprite.c -- Generated from pokered-master gfx/sprites/red.png */",
        '#include "player_sprite.h"',
        "",
        "const uint8_t gPlayerGfx[PLAYER_FRAMES][4][PLAYER_TILE_BYTES] = {",
    ]
    tile_names = ["TL", "TR", "BL", "BR"]
    for f, frame_tiles in enumerate(frames):
        c_lines.append(f"    /* [{f}] {FRAME_NAMES[f]} */")
        c_lines.append("    {")
        for t, tile_data in enumerate(frame_tiles):
            c_lines.append(f"        /* {tile_names[t]} */ {{ {bytes_to_c_hex(tile_data)} }},")
        c_lines.append("    },")
    c_lines.append("};")
    c_lines.append("")

    h_out = "\n".join(h_lines) + "\n"
    c_out = "\n".join(c_lines) + "\n"

    with open(OUT / "player_sprite.h", "w") as f: f.write(h_out)
    with open(OUT / "player_sprite.c", "w") as f: f.write(c_out)

    print(f"player_sprite.h/c written: {FRAME_COUNT} frames x {TILES_PER_FRAME} tiles x {TILE_BYTES} bytes")

if __name__ == "__main__":
    main()
