#!/usr/bin/env python3
"""
extract_trainer_sprites.py -- Convert pokered trainer sprite PNGs to GB 2bpp C arrays.

Generates:
  src/data/trainer_sprites.h  -- extern declarations
  src/data/trainer_sprites.c  -- 47 trainer sprites, 49 tiles x 16 bytes each

Source PNGs: pokered-master/gfx/trainers/*.png  (56x56 2bpp grayscale)
Reference:   pokered-master/data/trainers/pic_pointers_money.asm  (1-indexed order)

Coordinate system:
  PNG 2bpp grayscale stores dark=0, light=3.
  GB 2bpp uses color 0=lightest/transparent, 3=darkest.
  Conversion: gb_color = 3 - png_value  (invert=True)

GB 2bpp tile format (8x8 tile = 16 bytes):
  For each row (8 pixels): lo_byte, hi_byte
  lo_byte bit(7-col) = bit0 of pixel at col
  hi_byte bit(7-col) = bit1 of pixel at col
"""

import struct
import zlib
from pathlib import Path

POKERED     = Path(r"C:\Users\Anthony\pokered\pokered-master")
OUT         = Path(r"C:\Users\Anthony\pokered\src\data")
TRAINERS_DIR = POKERED / "gfx" / "trainers"

# ---------------------------------------------------------------------------
# Trainer class -> PNG filename mapping
# Order matches data/trainers/pic_pointers_money.asm (1-indexed)
# ---------------------------------------------------------------------------

TRAINER_SPRITES = [
    'youngster',      # 1  YOUNGSTER
    'bugcatcher',     # 2  BUG CATCHER
    'lass',           # 3  LASS
    'sailor',         # 4  SAILOR
    'jr.trainerm',    # 5  JR.TRAINER (M)
    'jr.trainerf',    # 6  JR.TRAINER (F)
    'pokemaniac',     # 7  POKEMANIAC
    'supernerd',      # 8  SUPER NERD
    'hiker',          # 9  HIKER
    'biker',          # 10 BIKER
    'burglar',        # 11 BURGLAR
    'engineer',       # 12 ENGINEER
    'juggler',        # 13 JUGGLER
    'fisher',         # 14 FISHERMAN
    'swimmer',        # 15 SWIMMER
    'cueball',        # 16 CUE BALL
    'gambler',        # 17 GAMBLER
    'beauty',         # 18 BEAUTY
    'psychic',        # 19 PSYCHIC
    'rocker',         # 20 ROCKER
    'juggler',        # 21 JUGGLER (same pic as 13)
    'tamer',          # 22 TAMER
    'birdkeeper',     # 23 BIRD KEEPER
    'blackbelt',      # 24 BLACKBELT
    'rival1',         # 25 RIVAL1 (Blue)
    'prof.oak',       # 26 PROF.OAK
    'youngster',      # 27 CHIEF (no dedicated PNG; use youngster as fallback)
    'scientist',      # 28 SCIENTIST
    'giovanni',       # 29 GIOVANNI
    'rocket',         # 30 ROCKET
    'cooltrainerm',   # 31 COOLTRAINER (M)
    'cooltrainerf',   # 32 COOLTRAINER (F)
    'bruno',          # 33 BRUNO
    'brock',          # 34 BROCK
    'misty',          # 35 MISTY
    'lt.surge',       # 36 LT.SURGE
    'erika',          # 37 ERIKA
    'koga',           # 38 KOGA
    'blaine',         # 39 BLAINE
    'sabrina',        # 40 SABRINA
    'gentleman',      # 41 GENTLEMAN
    'rival2',         # 42 RIVAL2
    'rival3',         # 43 RIVAL3
    'lorelei',        # 44 LORELEI
    'channeler',      # 45 CHANNELER
    'agatha',         # 46 AGATHA
    'lance',          # 47 LANCE
]

# ASCII-safe names from data/trainers/names.asm
TRAINER_NAMES = [
    "YOUNGSTER",    # 1
    "BUG CATCHER",  # 2
    "LASS",         # 3
    "SAILOR",       # 4
    "JR.TRAINER",   # 5  (male glyph dropped)
    "JR.TRAINER",   # 6  (female glyph dropped)
    "POKEMANIAC",   # 7  (e->E)
    "SUPER NERD",   # 8
    "HIKER",        # 9
    "BIKER",        # 10
    "BURGLAR",      # 11
    "ENGINEER",     # 12
    "JUGGLER",      # 13
    "FISHERMAN",    # 14
    "SWIMMER",      # 15
    "CUE BALL",     # 16
    "GAMBLER",      # 17
    "BEAUTY",       # 18
    "PSYCHIC",      # 19
    "ROCKER",       # 20
    "JUGGLER",      # 21
    "TAMER",        # 22
    "BIRD KEEPER",  # 23
    "BLACKBELT",    # 24
    "RIVAL",        # 25 (RIVAL1)
    "PROF.OAK",     # 26
    "CHIEF",        # 27
    "SCIENTIST",    # 28
    "GIOVANNI",     # 29
    "ROCKET",       # 30
    "COOLTRAINER",  # 31 (male glyph dropped)
    "COOLTRAINER",  # 32 (female glyph dropped)
    "BRUNO",        # 33
    "BROCK",        # 34
    "MISTY",        # 35
    "LT.SURGE",     # 36
    "ERIKA",        # 37
    "KOGA",         # 38
    "BLAINE",       # 39
    "SABRINA",      # 40
    "GENTLEMAN",    # 41
    "RIVAL",        # 42 (RIVAL2)
    "RIVAL",        # 43 (RIVAL3)
    "LORELEI",      # 44
    "CHANNELER",    # 45
    "AGATHA",       # 46
    "LANCE",        # 47
]

assert len(TRAINER_SPRITES) == 47
assert len(TRAINER_NAMES)   == 47

NUM_TRAINERS       = 47
TRAINER_CANVAS_TILES = 49   # 7x7 tile canvas (56x56 px)
SPRITE_BYTES       = TRAINER_CANVAS_TILES * 16  # 784 bytes per trainer

# ---------------------------------------------------------------------------
# PNG decoder (2bpp grayscale, no PIL required)
# Copied from tools/extract_sprites.py
# ---------------------------------------------------------------------------

def decode_png_2bpp_grayscale(path):
    """
    Decode a 2bpp grayscale PNG.
    Returns (width, height, pixels) where pixels is a flat list of 2-bit values
    (raw PNG values, dark=0 light=3).
    """
    with open(path, "rb") as f:
        data = f.read()

    assert data[:8] == b'\x89PNG\r\n\x1a\n', "Not a PNG"

    idat_chunks = []
    i = 8
    width = height = bit_depth = 0
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

        if filt == 0:    # None
            pass
        elif filt == 1:  # Sub
            for j in range(1, stride):
                row_raw[j] = (row_raw[j] + row_raw[j-1]) & 0xFF
        elif filt == 2:  # Up
            for j in range(stride):
                row_raw[j] = (row_raw[j] + prev_row[j]) & 0xFF
        elif filt == 3:  # Average
            for j in range(stride):
                a = row_raw[j-1] if j > 0 else 0
                b = prev_row[j]
                row_raw[j] = (row_raw[j] + (a + b) // 2) & 0xFF
        elif filt == 4:  # Paeth
            for j in range(stride):
                a = row_raw[j-1] if j > 0 else 0
                b = prev_row[j]
                c = prev_row[j-1] if j > 0 else 0
                pa = abs(b - c); pb = abs(a - c); pc = abs(a + b - 2*c)
                pr = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                row_raw[j] = (row_raw[j] + pr) & 0xFF
        else:
            raise ValueError(f"Unknown PNG filter {filt}")

        prev_row = bytes(row_raw)

        for byte in row_raw:
            for shift in (6, 4, 2, 0):
                pixels.append((byte >> shift) & 3)

    return width, height, pixels[:width * height]


def pixels_to_gb2bpp(width, height, pixels, invert=True):
    """
    Convert flat pixel list (row-major, 2bpp PNG values) to GB 2bpp tile bytes.
    invert=True: gb_color = 3 - png_value (dark PNG pixel -> dark GB color 3).
    Returns bytes covering (width//8) * (height//8) tiles, row-major tile order.
    """
    tiles_x = width  // 8
    tiles_y = height // 8
    out = bytearray()

    for ty in range(tiles_y):
        for tx in range(tiles_x):
            for row in range(8):
                lo = 0; hi = 0
                for col in range(8):
                    pv = pixels[(ty*8 + row) * width + (tx*8 + col)]
                    if invert:
                        pv = 3 - pv
                    bit = 7 - col
                    if pv & 1: lo |= (1 << bit)
                    if pv & 2: hi |= (1 << bit)
                out.append(lo)
                out.append(hi)
    return bytes(out)

# ---------------------------------------------------------------------------
# C formatting helpers
# ---------------------------------------------------------------------------

def tile_to_c_array(tile_bytes):
    """Format 16 bytes of one tile as a hex initializer list."""
    assert len(tile_bytes) == 16
    return "{ " + ", ".join(f"0x{b:02X}" for b in tile_bytes) + " }"


def sprite_to_c_array(sprite_bytes, trainer_idx, trainer_name, png_stem):
    """Format 784-byte sprite as a 49-element array of 16-byte tile initializers."""
    assert len(sprite_bytes) == SPRITE_BYTES
    lines = []
    lines.append(f"    /* [{trainer_idx}] {trainer_name} ({png_stem}.png) */")
    lines.append("    {")
    for t in range(TRAINER_CANVAS_TILES):
        tile = sprite_bytes[t*16 : t*16+16]
        comma = "," if t < TRAINER_CANVAS_TILES - 1 else ""
        lines.append(f"        {tile_to_c_array(tile)}{comma}")
    lines.append("    },")
    return "\n".join(lines)

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    # -- Header file ----------------------------------------------------------
    h_content = """\
#pragma once
/* trainer_sprites.h -- Trainer battle sprite tile data.
 * Auto-generated by tools/extract_trainer_sprites.py
 * 47 trainer classes, each 49 tiles x 16 bytes = 784 bytes.
 * Index: [trainer_class - 1]  (trainer_class is 1-based)
 * ALWAYS refer to pokered-master data/trainers/pic_pointers_money.asm. */
#include <stdint.h>
#define NUM_TRAINERS 47
#define TRAINER_CANVAS_TILES 49   /* 7x7 tile canvas, same as pokemon front sprites */
extern const uint8_t gTrainerFrontSprite[NUM_TRAINERS][TRAINER_CANVAS_TILES][16];
extern const char * const gTrainerClassNames[NUM_TRAINERS];
"""

    # -- C file ---------------------------------------------------------------
    c_lines = [
        "/* trainer_sprites.c -- Trainer battle sprite tile data.",
        " * Auto-generated by tools/extract_trainer_sprites.py",
        " * Source: pokered-master/gfx/trainers/*.png  (56x56 2bpp grayscale)",
        " * Reference: pokered-master/data/trainers/pic_pointers_money.asm */",
        '#include "trainer_sprites.h"',
        "",
        "/* ---------------------------------------------------------------------------",
        " * gTrainerFrontSprite[trainer_class - 1][tile_index][16 bytes]",
        " * 49 tiles in row-major order (row 0-6, col 0-6 within 56x56 bitmap).",
        " * GB 2bpp: each tile row = lo_byte, hi_byte.",
        " * --------------------------------------------------------------------------- */",
        f"const uint8_t gTrainerFrontSprite[NUM_TRAINERS][TRAINER_CANVAS_TILES][16] = {{",
    ]

    for idx, png_stem in enumerate(TRAINER_SPRITES):
        trainer_class = idx + 1
        trainer_name  = TRAINER_NAMES[idx]
        png_path = TRAINERS_DIR / f"{png_stem}.png"

        assert png_path.exists(), \
            f"Missing trainer PNG for class {trainer_class} ({trainer_name}): {png_path}"

        w, h, pixels = decode_png_2bpp_grayscale(png_path)
        assert w == 56 and h == 56, \
            f"Trainer {trainer_name} PNG is {w}x{h}, expected 56x56: {png_path}"

        sprite_bytes = pixels_to_gb2bpp(w, h, pixels, invert=True)
        assert len(sprite_bytes) == SPRITE_BYTES, \
            f"Sprite byte count mismatch for {trainer_name}: {len(sprite_bytes)} != {SPRITE_BYTES}"

        c_lines.append(sprite_to_c_array(sprite_bytes, trainer_class, trainer_name, png_stem))

    c_lines.append("};")
    c_lines.append("")

    # -- Name table -----------------------------------------------------------
    c_lines.append("/* gTrainerClassNames[trainer_class - 1] */")
    c_lines.append("const char * const gTrainerClassNames[NUM_TRAINERS] = {")
    for idx, name in enumerate(TRAINER_NAMES):
        comma = "," if idx < NUM_TRAINERS - 1 else ""
        c_lines.append(f'    "{name}"{comma}')
    c_lines.append("};")
    c_lines.append("")

    c_content = "\n".join(c_lines)

    # -- Write output files ---------------------------------------------------
    OUT.mkdir(parents=True, exist_ok=True)
    with open(OUT / "trainer_sprites.h", "w") as f:
        f.write(h_content)
    with open(OUT / "trainer_sprites.c", "w") as f:
        f.write(c_content)

    print("Generated trainer_sprites.h and trainer_sprites.c")


if __name__ == "__main__":
    main()
