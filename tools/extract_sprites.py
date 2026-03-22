#!/usr/bin/env python3
"""
extract_sprites.py -- Convert pokered sprite PNGs to GB 2bpp C arrays.

Generates:
  src/data/sprite_data.h  -- NUM_SPRITES constant + extern array
  src/data/sprite_data.c  -- per-sprite 2bpp tile GFX (12 tiles each = 192 bytes)

Coordinate system:
  PNG 2bpp grayscale stores dark=0, light=3.
  GB 2bpp uses color 0=lightest/transparent, 3=darkest.
  Conversion: gb_color = 3 - png_value  (invert)

GB 2bpp tile format (8x8 tile = 16 bytes):
  For each row (8 pixels): lo_byte, hi_byte
  lo_byte bit(7-col) = bit0 of pixel at col
  hi_byte bit(7-col) = bit1 of pixel at col
"""

import os, re, struct, zlib
from pathlib import Path

POKERED = Path(r"C:\Users\Anthony\pokered\pokered-master")
OUT     = Path(r"C:\Users\Anthony\pokered\src\data")
SPRITES_DIR = POKERED / "gfx" / "sprites"

# ---------------------------------------------------------------------------
# Build SPRITE_ID -> PNG stem mapping from sprites.asm
# ---------------------------------------------------------------------------

# Each `overworld_sprite XxxSprite, N` line at position i (0-based after SPRITE_RED)
# corresponds to SPRITE_ID = i+1  (SPRITE_NONE=0 has no entry)
SPRITE_NONE = 0

def parse_sprites_asm():
    """Returns list of (sprite_name_stem, num_tiles) indexed by sprite_id-1."""
    path = POKERED / "data" / "sprites" / "sprites.asm"
    entries = []
    with open(path) as f:
        for line in f:
            m = re.search(r"overworld_sprite\s+(\w+)Sprite,\s*(\d+)", line)
            if m:
                name = m.group(1)
                tiles = int(m.group(2))
                entries.append((name, tiles))
    return entries  # index 0 = SPRITE_RED (sprite_id=1)

# ---------------------------------------------------------------------------
# PNG decoder (2bpp grayscale, no PIL required)
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

    # Parse chunks
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

    # Each scanline: 1 filter byte + ceil(width * 2 / 8) data bytes
    stride = (width * 2 + 7) // 8   # bytes per row of pixel data
    pixels = []
    prev_row = bytes(stride)
    pos = 0

    for _ in range(height):
        filt = raw[pos]; pos += 1
        row_raw = bytearray(raw[pos:pos+stride]); pos += stride

        # Apply filter
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
            raise ValueError(f"Unknown filter {filt}")

        prev_row = bytes(row_raw)

        # Extract 2bpp pixel values (4 pixels per byte, MSB = leftmost)
        for byte in row_raw:
            for shift in (6, 4, 2, 0):
                pixels.append((byte >> shift) & 3)

    return width, height, pixels[:width * height]

# ---------------------------------------------------------------------------
# Convert pixel grid to GB 2bpp tiles
# ---------------------------------------------------------------------------

def pixels_to_gb2bpp(width, height, pixels, invert=True):
    """
    Convert flat pixel list (row-major, 2bpp PNG values) to GB 2bpp tile bytes.
    invert=True: gb_color = 3 - png_value (dark PNG pixel -> dark GB color 3).
    Returns bytes covering (width//8) * (height//8) tiles.
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
# CamelCase sprite name -> snake_case PNG stem heuristic
# ---------------------------------------------------------------------------

SPRITE_NAME_TO_PNG = {
    # Manual overrides for names that don't snake_case cleanly
    "Red":              "red",
    "Blue":             "blue",
    "Oak":              "oak",
    "Youngster":        "youngster",
    "Monster":          "monster",
    "CooltrainerF":     "cooltrainer_f",
    "CooltrainerM":     "cooltrainer_m",
    "LittleGirl":       "little_girl",
    "Bird":             "bird",
    "MiddleAgedMan":    "middle_aged_man",
    "Gambler":          "gambler",
    "SuperNerd":        "super_nerd",
    "Girl":             "girl",
    "Hiker":            "hiker",
    "Beauty":           "beauty",
    "Gentleman":        "gentleman",
    "Daisy":            "daisy",
    "Biker":            "biker",
    "Sailor":           "sailor",
    "Cook":             "cook",
    "BikeShopClerk":    "bike_shop_clerk",
    "MrFuji":           "mr_fuji",
    "Giovanni":         "giovanni",
    "Rocket":           "rocket",
    "Channeler":        "channeler",
    "Waiter":           "waiter",
    "SilphWorkerF":     "silph_worker_f",
    "MiddleAgedWoman":  "middle_aged_woman",
    "BrunetteGirl":     "brunette_girl",
    "Lance":            "lance",
    "Scientist":        "scientist",
    "Rocker":           "rocker",
    "Swimmer":          "swimmer",
    "SafariZoneWorker": "safari_zone_worker",
    "GymGuide":         "gym_guide",
    "Gramps":           "gramps",
    "Clerk":            "clerk",
    "FishingGuru":      "fishing_guru",
    "Granny":           "granny",
    "Nurse":            "nurse",
    "LinkReceptionist": "link_receptionist",
    "SilphPresident":   "silph_president",
    "SilphWorkerM":     "silph_worker_m",
    "Warden":           "warden",
    "Captain":          "captain",
    "Fisher":           "fisher",
    "Koga":             "koga",
    "Guard":            "guard",
    "Mom":              "mom",
    "BaldingGuy":       "balding_guy",
    "LittleBoy":        "little_boy",
    "GameboyKid":       "gameboy_kid",
    "Fairy":            "fairy",
    "Agatha":           "agatha",
    "Bruno":            "bruno",
    "Lorelei":          "lorelei",
    "Seel":             "seel",
    "PokeBall":         "poke_ball",
    "Fossil":           "fossil",
    "Boulder":          "boulder",
    "Paper":            "paper",
    "Pokedex":          "pokedex",
    "Clipboard":        "clipboard",
    "Snorlax":          "snorlax",
    "OldAmber":         "old_amber",
    "GamblerAsleep":    "gambler_asleep",
}

def bytes_to_c_array(data):
    lines = []
    for i in range(0, len(data), 16):
        row = data[i:i+16]
        lines.append("    " + ", ".join(f"0x{b:02X}" for b in row) + ",")
    return "\n".join(lines)

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    entries = parse_sprites_asm()
    # entries[i] = (CamelName, num_tiles), sprite_id = i+1

    # Total sprites = entries count + 1 (for SPRITE_NONE at index 0)
    num_sprites = len(entries) + 1  # includes SPRITE_NONE

    h_lines = [
        "#pragma once",
        "/* sprite_data.h -- Generated from pokered-master gfx/sprites/ PNGs. */",
        "#include <stdint.h>",
        "",
        f"#define NUM_SPRITES  {num_sprites}",
        "#define SPRITE_TILES 24      /* tiles per overworld sprite (2x12): 12 stand + 12 walk */",
        "#define SPRITE_GFX_SIZE (SPRITE_TILES * 16)  /* bytes */",
        "",
        "/* gSpriteGfx[sprite_id][tile][16 bytes].                         */",
        "/* sprite_id 0 = SPRITE_NONE (empty).  Use first SPRITE_TILES tiles */",
        "/* (front-facing animation frame) for overworld rendering.         */",
        "extern const uint8_t gSpriteGfx[NUM_SPRITES][SPRITE_GFX_SIZE];",
    ]

    c_lines = [
        "/* sprite_data.c -- Generated from pokered-master gfx/sprites/ PNGs. */",
        '#include "sprite_data.h"',
        "",
        f"const uint8_t gSpriteGfx[NUM_SPRITES][SPRITE_GFX_SIZE] = {{",
    ]

    # Entry 0: SPRITE_NONE — all zeros
    c_lines.append("    /* [0] SPRITE_NONE */")
    c_lines.append("    { " + ", ".join(["0x00"] * (24 * 16)) + " },")

    missing = []
    for idx, (name, tile_count) in enumerate(entries):
        sprite_id = idx + 1
        png_stem = SPRITE_NAME_TO_PNG.get(name, "")
        png_path = SPRITES_DIR / f"{png_stem}.png" if png_stem else None

        if not png_path or not png_path.exists():
            # Try to find it by listing available files
            for fn in os.listdir(SPRITES_DIR):
                if fn.endswith(".png"):
                    stem = fn[:-4]
                    # rough match
                    if stem.replace("_","").lower() == name.lower():
                        png_path = SPRITES_DIR / fn
                        break
            else:
                missing.append(name)
                # Emit zeros placeholder
                c_lines.append(f"    /* [{sprite_id}] {name} (missing) */")
                c_lines.append("    { " + ", ".join(["0x00"] * (24 * 16)) + " },")
                continue

        try:
            w, h, pixels = decode_png_2bpp_grayscale(png_path)
        except Exception as e:
            missing.append(f"{name}: {e}")
            c_lines.append(f"    /* [{sprite_id}] {name} (error: {e}) */")
            c_lines.append("    { " + ", ".join(["0x00"] * (24 * 16)) + " },")
            continue

        # Convert entire PNG to GB 2bpp
        all_tiles = pixels_to_gb2bpp(w, h, pixels, invert=True)

        # Take the first SPRITE_TILES (24) tiles = 384 bytes.
        # Layout: tiles 0-11 = standing frames (down/up/left), tiles 12-23 = walking frames.
        # Sprites with only 12 tiles (16x48px sheets) get zeros for walking tiles.
        sprite_bytes = all_tiles[:24 * 16]
        if len(sprite_bytes) < 24 * 16:
            sprite_bytes += b'\x00' * (24 * 16 - len(sprite_bytes))

        c_lines.append(f"    /* [{sprite_id}] {name} ({w}x{h}px, {tile_count} tiles) */")
        c_lines.append("    {")
        c_lines.append(bytes_to_c_array(sprite_bytes))
        c_lines.append("    },")

    c_lines.append("};")
    c_lines.append("")

    h_out = "\n".join(h_lines) + "\n"
    c_out = "\n".join(c_lines) + "\n"

    with open(OUT / "sprite_data.h", "w") as f: f.write(h_out)
    with open(OUT / "sprite_data.c", "w") as f: f.write(c_out)

    print(f"sprite_data.h: {len(h_out)} chars")
    print(f"sprite_data.c: {len(c_out)} chars ({num_sprites} sprites)")
    if missing:
        print(f"WARNING: missing/failed {len(missing)} sprites: {missing[:5]}{'...' if len(missing)>5 else ''}")

if __name__ == "__main__":
    main()
