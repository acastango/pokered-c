#!/usr/bin/env python3
"""extract_town_map.py -- Generate Town Map gfx/tilemap data from pokered-master ASM assets.

Sources:
  gfx/town_map/town_map.png        (32x32, 2bpp grayscale; 16 tiles)
  gfx/town_map/town_map.rle        (RLE tilemap stream, nibble-packed)
  gfx/town_map/town_map_cursor.png (16x16, 1bpp grayscale; 4 tiles)

Outputs:
  src/data/town_map_data.h
  src/data/town_map_data.c
"""

import struct
import zlib
from pathlib import Path

ROOT = Path(r"C:/Users/Anthony/pokered")
POKERED = ROOT / "pokered-master"
OUT = ROOT / "src" / "data"

TOWN_MAP_PNG = POKERED / "gfx" / "town_map" / "town_map.png"
TOWN_MAP_RLE = POKERED / "gfx" / "town_map" / "town_map.rle"
CURSOR_PNG = POKERED / "gfx" / "town_map" / "town_map_cursor.png"
MAP_CONSTANTS_ASM = POKERED / "constants" / "map_constants.asm"
TOWN_MAP_ENTRIES_ASM = POKERED / "data" / "maps" / "town_map_entries.asm"
TOWN_MAP_ORDER_ASM = POKERED / "data" / "maps" / "town_map_order.asm"


def decode_png_grayscale(path: Path):
    data = path.read_bytes()
    assert data[:8] == b"\x89PNG\r\n\x1a\n", f"{path}: not PNG"

    width = height = bit_depth = color_type = 0
    idat_chunks = []
    i = 8
    while i < len(data):
        length = struct.unpack(">I", data[i:i+4])[0]
        chunk_type = data[i+4:i+8]
        chunk_data = data[i+8:i+8+length]
        if chunk_type == b"IHDR":
            width, height = struct.unpack(">II", chunk_data[:8])
            bit_depth = chunk_data[8]
            color_type = chunk_data[9]
            assert color_type == 0, f"{path}: expected grayscale PNG"
            assert bit_depth in (1, 2), f"{path}: expected 1bpp/2bpp grayscale"
        elif chunk_type == b"IDAT":
            idat_chunks.append(chunk_data)
        i += length + 12

    raw = zlib.decompress(b"".join(idat_chunks))
    stride = (width * bit_depth + 7) // 8

    pixels = []
    prev = bytes(stride)
    pos = 0
    for _ in range(height):
        filt = raw[pos]
        pos += 1
        row = bytearray(raw[pos:pos+stride])
        pos += stride

        if filt == 0:
            pass
        elif filt == 1:
            for j in range(1, stride):
                row[j] = (row[j] + row[j - 1]) & 0xFF
        elif filt == 2:
            for j in range(stride):
                row[j] = (row[j] + prev[j]) & 0xFF
        elif filt == 3:
            for j in range(stride):
                a = row[j - 1] if j > 0 else 0
                row[j] = (row[j] + ((a + prev[j]) // 2)) & 0xFF
        elif filt == 4:
            for j in range(stride):
                a = row[j - 1] if j > 0 else 0
                b = prev[j]
                c = prev[j - 1] if j > 0 else 0
                pa = abs(b - c)
                pb = abs(a - c)
                pc = abs(a + b - 2 * c)
                pr = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                row[j] = (row[j] + pr) & 0xFF
        else:
            raise ValueError(f"{path}: unsupported PNG filter {filt}")

        prev = bytes(row)

        if bit_depth == 2:
            for byte in row:
                for shift in (6, 4, 2, 0):
                    pixels.append((byte >> shift) & 0x03)
        else:
            for byte in row:
                for shift in (7, 6, 5, 4, 3, 2, 1, 0):
                    pixels.append((byte >> shift) & 0x01)

    return width, height, pixels[: width * height], bit_depth


def png_to_gb_2bpp_tiles(path: Path):
    w, h, pixels, bd = decode_png_grayscale(path)
    assert w % 8 == 0 and h % 8 == 0, f"{path}: dimensions must be multiples of 8"

    tiles_x = w // 8
    tiles_y = h // 8
    out = []

    for ty in range(tiles_y):
        for tx in range(tiles_x):
            tile = [0] * 16
            for row in range(8):
                lo = 0
                hi = 0
                for col in range(8):
                    pv = pixels[(ty * 8 + row) * w + (tx * 8 + col)]
                    if bd == 1:
                        # PNG grayscale 1bpp: 0=black, 1=white.
                        # GB color index: 3=black, 0=white.
                        gb = 3 if pv == 0 else 0
                    else:
                        gb = 3 - pv
                    bit = 7 - col
                    if gb & 1:
                        lo |= (1 << bit)
                    if gb & 2:
                        hi |= (1 << bit)
                tile[row * 2 + 0] = lo
                tile[row * 2 + 1] = hi
            out.append(tile)

    return out


def to_c_rows(data, row_len=16, indent="    "):
    rows = []
    for i in range(0, len(data), row_len):
        chunk = data[i:i + row_len]
        rows.append(indent + ", ".join(f"0x{b:02X}" for b in chunk) + ",")
    return "\n".join(rows)


def parse_map_constants():
    map_ids = {}
    indoor_group_end = {}
    const_value = 0

    for raw in MAP_CONSTANTS_ASM.read_text(encoding="utf-8").splitlines():
        line = raw.split(";", 1)[0].strip()
        if not line:
            continue
        if line.startswith("map_const "):
            rest = line[len("map_const "):]
            name = rest.split(",", 1)[0].strip()
            map_ids[name] = const_value
            const_value += 1
            continue
        if line.startswith("end_indoor_group "):
            grp = line[len("end_indoor_group "):].strip()
            indoor_group_end[grp] = const_value

    return map_ids, indoor_group_end


def parse_town_map_entries(indoor_group_end):
    external = [0] * 0x25  # FIRST_INDOOR_MAP in pokered
    ext_i = 0
    internal = []

    for raw in TOWN_MAP_ENTRIES_ASM.read_text(encoding="utf-8").splitlines():
        line = raw.split(";", 1)[0].strip()
        if not line:
            continue

        if line.startswith("outdoor_map "):
            parts = [p.strip() for p in line[len("outdoor_map "):].split(",")]
            if len(parts) >= 3:
                x = int(parts[0], 0)
                y = int(parts[1], 0)
                packed = ((y & 0x0F) << 4) | (x & 0x0F)
                if ext_i < len(external):
                    external[ext_i] = packed
                ext_i += 1
            continue

        if line.startswith("indoor_map "):
            parts = [p.strip() for p in line[len("indoor_map "):].split(",")]
            if len(parts) >= 4:
                group = parts[0]
                x = int(parts[1], 0)
                y = int(parts[2], 0)
                group_end = indoor_group_end[group]
                packed = ((y & 0x0F) << 4) | (x & 0x0F)
                internal.append((group_end, packed))

    return external, internal


def parse_town_map_order(map_ids):
    order = []
    for raw in TOWN_MAP_ORDER_ASM.read_text(encoding="utf-8").splitlines():
        line = raw.split(";", 1)[0].strip()
        if not line or not line.startswith("db "):
            continue
        sym = line[3:].strip()
        if sym in map_ids:
            order.append(map_ids[sym])
    return order


def main():
    world_tiles = png_to_gb_2bpp_tiles(TOWN_MAP_PNG)
    cursor_tiles = png_to_gb_2bpp_tiles(CURSOR_PNG)
    rle = list(TOWN_MAP_RLE.read_bytes())

    assert len(world_tiles) == 16, f"Expected 16 map tiles, got {len(world_tiles)}"
    assert len(cursor_tiles) == 4, f"Expected 4 cursor tiles, got {len(cursor_tiles)}"
    assert 0 in rle, "town_map.rle must end with 0 terminator"
    map_ids, indoor_group_end = parse_map_constants()
    external_coords, internal_coords = parse_town_map_entries(indoor_group_end)
    order_map_ids = parse_town_map_order(map_ids)

    h_text = """#pragma once

#include <stdint.h>

/* Generated by tools/extract_town_map.py from pokered-master/gfx/town_map assets. */

#define TOWN_MAP_WORLD_TILE_BASE 0x60
#define TOWN_MAP_WORLD_TILE_COUNT 16
#define TOWN_MAP_FIRST_INDOOR_MAP 0x25
#define TOWN_MAP_SCREEN_TILE_COUNT (20 * 18)

extern const uint8_t gTownMapWorldTiles[TOWN_MAP_WORLD_TILE_COUNT][16];
extern const uint8_t gTownMapCursorTiles[4][16];
extern const uint8_t gTownMapCompressedMap[];
extern const uint8_t gTownMapExternalCoords[TOWN_MAP_FIRST_INDOOR_MAP];
extern const uint8_t gTownMapOrderMapIds[];
extern const int gTownMapOrderCount;

typedef struct {
    uint8_t group_end;
    uint8_t coords;
} town_map_internal_entry_t;
extern const town_map_internal_entry_t gTownMapInternalCoords[];
extern const int gTownMapInternalCoordsCount;

void TownMapData_LoadTiles(void);
"""

    c_lines = []
    c_lines.append("#include \"town_map_data.h\"")
    c_lines.append("")
    c_lines.append("#include \"../platform/display.h\"")
    c_lines.append("")
    c_lines.append("const uint8_t gTownMapWorldTiles[TOWN_MAP_WORLD_TILE_COUNT][16] = {")
    for i, t in enumerate(world_tiles):
        c_lines.append(f"    /* ${0x60 + i:02X} */ {{ " + ", ".join(f"0x{b:02X}" for b in t) + " },")
    c_lines.append("};")
    c_lines.append("")
    c_lines.append("const uint8_t gTownMapCursorTiles[4][16] = {")
    for i, t in enumerate(cursor_tiles):
        c_lines.append(f"    /* {i} */ {{ " + ", ".join(f"0x{b:02X}" for b in t) + " },")
    c_lines.append("};")
    c_lines.append("")
    c_lines.append("const uint8_t gTownMapCompressedMap[] = {")
    c_lines.append(to_c_rows(rle, row_len=16))
    c_lines.append("};")
    c_lines.append("")
    c_lines.append("const uint8_t gTownMapExternalCoords[TOWN_MAP_FIRST_INDOOR_MAP] = {")
    c_lines.append(to_c_rows(external_coords, row_len=16))
    c_lines.append("};")
    c_lines.append("")
    c_lines.append("const town_map_internal_entry_t gTownMapInternalCoords[] = {")
    for group_end, packed in internal_coords:
        c_lines.append(f"    {{ 0x{group_end:02X}, 0x{packed:02X} }},")
    c_lines.append("};")
    c_lines.append(f"const int gTownMapInternalCoordsCount = {len(internal_coords)};")
    c_lines.append("")
    c_lines.append("const uint8_t gTownMapOrderMapIds[] = {")
    c_lines.append(to_c_rows(order_map_ids, row_len=16))
    c_lines.append("};")
    c_lines.append(f"const int gTownMapOrderCount = {len(order_map_ids)};")
    c_lines.append("")
    c_lines.append("void TownMapData_LoadTiles(void) {")
    c_lines.append("    for (int i = 0; i < TOWN_MAP_WORLD_TILE_COUNT; i++)")
    c_lines.append("        Display_LoadTile((uint8_t)(TOWN_MAP_WORLD_TILE_BASE + i), gTownMapWorldTiles[i]);")
    c_lines.append("    for (int i = 0; i < 4; i++)")
    c_lines.append("        Display_LoadSpriteTile((uint8_t)(4 + i), gTownMapCursorTiles[i]);")
    c_lines.append("}")
    c_lines.append("")

    (OUT / "town_map_data.h").write_text(h_text, encoding="utf-8")
    (OUT / "town_map_data.c").write_text("\n".join(c_lines) + "\n", encoding="utf-8")

    print("Wrote src/data/town_map_data.h")
    print("Wrote src/data/town_map_data.c")


if __name__ == "__main__":
    main()
