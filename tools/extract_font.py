#!/usr/bin/env python3
"""
extract_font.py -- Convert pokered font PNGs to GB 2bpp C arrays.

Generates:
  src/data/font_data.h / .c

Tile slot layout (must match text.c, npc.c, player.c):
  0-95   : tileset BG tiles
  96-119 : NPC sprite tiles (6 NPCs × 4 tiles)
  120-125: box-drawing chars ┌─┐│└┘  (from font_extra.png chars $79-$7E)
  126    : blank/space tile
  127    : player sprite
  128-255: main font tiles (font.png, chars $80-$FF)

Sources:
  font.png        128×64px  1bpp grayscale → 128 tiles (char $80–$FF)
  font_extra.png  128×16px  2bpp grayscale → 32 tiles (chars $60–$7F)
    We extract tiles 25–31 from font_extra (chars $79–$7F = ┌─┐│└┘ and space)
"""

import os, struct, zlib
from pathlib import Path

POKERED    = Path(r"C:\Users\Anthony\pokered\pokered-master")
OUT        = Path(r"C:\Users\Anthony\pokered\src\data")
FONT_PNG   = POKERED / "gfx" / "font" / "font.png"
EXTRA_PNG  = POKERED / "gfx" / "font" / "font_extra.png"

# ---------------------------------------------------------------------------
# PNG decoder (2bpp or 1bpp grayscale, stdlib only)
# ---------------------------------------------------------------------------

def decode_png_grayscale(path):
    """Returns (width, height, pixels, bit_depth) for grayscale PNG."""
    with open(path, "rb") as f:
        data = f.read()
    assert data[:8] == b'\x89PNG\r\n\x1a\n'

    idat = []
    width = height = bd = 0
    i = 8
    while i < len(data):
        length = struct.unpack('>I', data[i:i+4])[0]
        typ = data[i+4:i+8]
        chunk = data[i+8:i+8+length]
        if typ == b'IHDR':
            width, height = struct.unpack('>II', chunk[:8])
            bd = chunk[8]
            assert chunk[9] == 0, "Expected grayscale"
        elif typ == b'IDAT':
            idat.append(chunk)
        i += 12 + length

    raw = zlib.decompress(b''.join(idat))
    stride = (width * bd + 7) // 8
    prev = bytes(stride)
    pixels = []
    pos = 0

    for _ in range(height):
        filt = raw[pos]; pos += 1
        row = bytearray(raw[pos:pos+stride]); pos += stride
        if filt == 0: pass
        elif filt == 1:
            for j in range(1, stride): row[j] = (row[j] + row[j-1]) & 0xFF
        elif filt == 2:
            for j in range(stride): row[j] = (row[j] + prev[j]) & 0xFF
        elif filt == 3:
            for j in range(stride):
                a = row[j-1] if j else 0
                row[j] = (row[j] + (a + prev[j]) // 2) & 0xFF
        elif filt == 4:
            for j in range(stride):
                a = row[j-1] if j else 0
                b = prev[j]; c = prev[j-1] if j else 0
                pa, pb, pc = abs(b-c), abs(a-c), abs(a+b-2*c)
                pr = a if pa<=pb and pa<=pc else (b if pb<=pc else c)
                row[j] = (row[j] + pr) & 0xFF
        prev = bytes(row)

        if bd == 2:
            for byte in row:
                for sh in (6, 4, 2, 0):
                    pixels.append((byte >> sh) & 3)
        elif bd == 1:
            for byte in row:
                for sh in (7, 6, 5, 4, 3, 2, 1, 0):
                    pixels.append((byte >> sh) & 1)

    return width, height, pixels[:width*height], bd

# ---------------------------------------------------------------------------
# Convert pixel grid to GB 2bpp tile bytes
# ---------------------------------------------------------------------------

def pixels_to_gb2bpp(width, height, pixels, bd, invert=True):
    """Tile-order GB 2bpp bytes for all (width//8) × (height//8) tiles."""
    tx_count = width  // 8
    ty_count = height // 8
    out = bytearray()
    for ty in range(ty_count):
        for tx in range(tx_count):
            for row in range(8):
                lo = hi = 0
                for col in range(8):
                    pv = pixels[(ty*8+row)*width + tx*8+col]
                    if bd == 1:
                        # 1bpp: 0→color 0, 1→color 3 (double both planes)
                        # In font.png: 0=white (bg), 1=black (ink)
                        # After inversion: 0=bg=color0, 1=ink=color3
                        gb = 3 if (pv ^ (1 if invert else 0)) else 0
                    else:
                        # 2bpp: invert so dark PNG pixel → dark GB color
                        gb = (3 - pv) if invert else pv
                    bit = 7 - col
                    if gb & 1: lo |= 1 << bit
                    if gb & 2: hi |= 1 << bit
                out.append(lo); out.append(hi)
    return bytes(out)

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def bytes_to_c_array(data, cols=16):
    lines = []
    for i in range(0, len(data), cols):
        lines.append("    " + ", ".join(f"0x{b:02X}" for b in data[i:i+cols]) + ",")
    return "\n".join(lines)

def main():
    # --- Main font (font.png, 1bpp, 128×64, 128 tiles, char $80-$FF) ---
    w, h, px, bd = decode_png_grayscale(FONT_PNG)
    print(f"font.png: {w}×{h} bd={bd} → {w//8}×{h//8} = {(w//8)*(h//8)} tiles")
    font_tiles = pixels_to_gb2bpp(w, h, px, bd, invert=True)
    assert len(font_tiles) == 128 * 16, len(font_tiles)

    # --- font_extra (2bpp, 128×16, 32 tiles, char $60-$7F) ---
    # Tiles 25-31 in row order = chars $79-$7F (┌─┐│└┘ space)
    # Layout: 16 tiles wide × 2 tiles tall = 32 tiles
    # Tile index = row*16 + col; tiles 25-30 = row1, col9-14 = ┌─┐│└┘
    # and tile 31 = space ($7F)
    w2, h2, px2, bd2 = decode_png_grayscale(EXTRA_PNG)
    print(f"font_extra.png: {w2}×{h2} bd={bd2} → {w2//8}×{h2//8} = {(w2//8)*(h2//8)} tiles")
    extra_tiles = pixels_to_gb2bpp(w2, h2, px2, bd2, invert=True)

    # Extract the 6 box-drawing chars (tiles 25-30 = ┌─┐│└┘)
    # Tile 31 = space ($7F) → we'll use as blank
    def get_tile(tiles_bytes, idx):
        return tiles_bytes[idx*16:(idx+1)*16]

    box_chars = [get_tile(extra_tiles, i) for i in range(25, 32)]  # 7 tiles (25-31)
    # box_chars[0]=┌ [1]=─ [2]=┐ [3]=│ [4]=└ [5]=┘ [6]=space

    # Build C arrays
    h_lines = [
        "#pragma once",
        "/* font_data.h -- Generated from pokered-master gfx/font/*.png */",
        "#include <stdint.h>",
        "",
        "/* Tile slot indices (must match npc.c / player.c / text.c) */",
        "#define FONT_TILE_BASE    128   /* char $80 = tile 128 */",
        "#define BOX_TILE_BASE     120   /* ┌─┐│└┘ at slots 120-125 */",
        "#define BLANK_TILE_SLOT   126   /* space / blank tile */",
        "",
        "/* char_to_tile: pokered char code → tile cache slot */",
        "static inline int Font_CharToTile(unsigned char c) {",
        "    if (c >= 0x80) return FONT_TILE_BASE + (c - 0x80);",
        "    if (c == 0x7F) return BLANK_TILE_SLOT;  /* space */",
        "    if (c >= 0x79 && c <= 0x7E) return BOX_TILE_BASE + (c - 0x79);",
        "    return BLANK_TILE_SLOT;  /* control chars / unknown */",
        "}",
        "",
        "/* 128 main font tiles (char $80–$FF), 16 bytes each */",
        "extern const uint8_t gFontTiles[128][16];",
        "",
        "/* Box-drawing + blank tiles (7 tiles: ┌─┐│└┘ + space) */",
        "extern const uint8_t gBoxTiles[7][16];",
        "",
        "/* Load all font tiles into the display tile cache.  Call once at startup. */",
        "void Font_Load(void);",
    ]

    c_lines = [
        "/* font_data.c -- Generated from pokered-master gfx/font/*.png */",
        '#include "font_data.h"',
        '#include "../platform/display.h"',
        "",
        "const uint8_t gFontTiles[128][16] = {",
    ]
    for i in range(128):
        t = font_tiles[i*16:(i+1)*16]
        c_lines.append(f"    /* char ${0x80+i:02X} */ {{ {', '.join(f'0x{b:02X}' for b in t)} }},")
    c_lines += [
        "};",
        "",
        "const uint8_t gBoxTiles[7][16] = {",
        "    /* ┌ */ { " + ", ".join(f"0x{b:02X}" for b in box_chars[0]) + " },",
        "    /* ─ */ { " + ", ".join(f"0x{b:02X}" for b in box_chars[1]) + " },",
        "    /* ┐ */ { " + ", ".join(f"0x{b:02X}" for b in box_chars[2]) + " },",
        "    /* │ */ { " + ", ".join(f"0x{b:02X}" for b in box_chars[3]) + " },",
        "    /* └ */ { " + ", ".join(f"0x{b:02X}" for b in box_chars[4]) + " },",
        "    /* ┘ */ { " + ", ".join(f"0x{b:02X}" for b in box_chars[5]) + " },",
        "    /* ' '*/ { " + ", ".join(f"0x{b:02X}" for b in box_chars[6]) + " },",
        "};",
        "",
        "void Font_Load(void) {",
        "    /* Load 128 main font tiles at slots 128–255 */",
        "    for (int i = 0; i < 128; i++)",
        "        Display_LoadTile((uint8_t)(FONT_TILE_BASE + i), gFontTiles[i]);",
        "    /* Load 6 box-drawing tiles at slots 120–125, blank at 126 */",
        "    for (int i = 0; i < 7; i++)",
        "        Display_LoadTile((uint8_t)(BOX_TILE_BASE + i), gBoxTiles[i]);",
        "}",
        "",
    ]

    with open(OUT / "font_data.h", "w") as f: f.write("\n".join(h_lines) + "\n")
    with open(OUT / "font_data.c", "w") as f: f.write("\n".join(c_lines) + "\n")
    print(f"font_data.h: {(OUT/'font_data.h').stat().st_size} bytes")
    print(f"font_data.c: {(OUT/'font_data.c').stat().st_size} bytes")

if __name__ == "__main__":
    main()
