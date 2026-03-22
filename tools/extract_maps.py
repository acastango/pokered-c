#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
extract_maps.py -- Extract pokered map/tileset data into C source files.

Generates:
  src/data/tileset_data.h / .c  -- block tables, collision lists, 2bpp tile GFX
  src/data/map_data.h / .c      -- map info table + embedded block data
"""

import os, re, sys, struct
from pathlib import Path

SRC  = Path(r"C:\Users\Anthony\pokered\pokered-master")
OUT  = Path(r"C:\Users\Anthony\pokered\src\data")

def wfile(name, content):
    path = OUT / name
    with open(path, "w", encoding="utf-8") as f:
        f.write(content)
    print(f"  {path}  ({len(content)} chars)")

def rbin(path):
    with open(path, "rb") as f:
        return f.read()

def bytes_to_c_array(data, indent="    "):
    """Format bytes as a C hex array with 16 bytes per line."""
    lines = []
    for i in range(0, len(data), 16):
        row = data[i:i+16]
        lines.append(indent + ", ".join(f"0x{b:02X}" for b in row) + ",")
    return "\n".join(lines)

# ---------------------------------------------------------------------------
# Tileset name <-> ID mapping (from constants/tileset_constants.asm)
# ---------------------------------------------------------------------------

TILESET_IDS = {
    "OVERWORLD": 0, "REDS_HOUSE_1": 1, "MART": 2, "FOREST": 3,
    "REDS_HOUSE_2": 4, "DOJO": 5, "POKECENTER": 6, "GYM": 7,
    "HOUSE": 8, "FOREST_GATE": 9, "MUSEUM": 10, "UNDERGROUND": 11,
    "GATE": 12, "SHIP": 13, "SHIP_PORT": 14, "CEMETERY": 15,
    "INTERIOR": 16, "CAVERN": 17, "LOBBY": 18, "MANSION": 19,
    "LAB": 20, "CLUB": 21, "FACILITY": 22, "PLATEAU": 23,
}
NUM_TILESETS = 24

# Tileset ID -> blockset file stem (many share block tables)
BLOCKSET_FILES = {
    0:  "overworld",   1: "reds_house",  2: "pokecenter", 3: "forest",
    4:  "reds_house",  5: "gym",         6: "pokecenter", 7: "gym",
    8:  "house",       9: "gate",        10: "gate",      11: "underground",
    12: "gate",        13: "ship",       14: "ship_port", 15: "cemetery",
    16: "interior",    17: "cavern",     18: "lobby",     19: "mansion",
    20: "lab",         21: "club",       22: "facility",  23: "plateau",
}

# Tileset ID -> PNG file stem
GFX_FILES = {
    0:  "overworld",   1: "reds_house",  2: "pokecenter", 3: "forest",
    4:  "reds_house",  5: "gym",         6: "pokecenter", 7: "gym",
    8:  "house",       9: "gate",        10: "gate",      11: "underground",
    12: "gate",        13: "ship",       14: "ship_port", 15: "cemetery",
    16: "interior",    17: "cavern",     18: "lobby",     19: "mansion",
    20: "lab",         21: "club",       22: "facility",  23: "plateau",
}

# ---------------------------------------------------------------------------
# PNG -> 2bpp conversion
# ---------------------------------------------------------------------------

def png_to_2bpp(png_path):
    """Convert a grayscale 4-shade PNG to GB 2bpp tile data."""
    try:
        from PIL import Image
        import numpy as np
    except ImportError:
        print("WARNING: Pillow not installed; using empty tile data", file=sys.stderr)
        return bytes(16 * 256)  # 256 blank tiles

    img = Image.open(png_path).convert("L")
    arr = __import__("numpy").array(img, dtype=__import__("numpy").uint8)
    h, w = arr.shape

    # Gray -> GB color: 255=0(white), 170=1, 85=2, 0=3(black)
    gb = (255 - arr.astype(int)) // 85  # values 0-3

    tiles_x = w // 8
    tiles_y = h // 8
    result = bytearray()

    for ty in range(tiles_y):
        for tx in range(tiles_x):
            tile = gb[ty*8:(ty+1)*8, tx*8:(tx+1)*8]
            for row in range(8):
                p0 = p1 = 0
                for col in range(8):
                    c = int(tile[row, col])
                    p0 |= (c & 1) << (7 - col)
                    p1 |= ((c >> 1) & 1) << (7 - col)
                result.append(p0)
                result.append(p1)
    return bytes(result)

# ---------------------------------------------------------------------------
# Parse collision tile IDs
# ---------------------------------------------------------------------------

def parse_collisions(coll_asm):
    """Returns dict: label -> list of passable tile IDs (0xFF-terminated list)."""
    text = coll_asm.read_text(encoding="utf-8")
    # Remove macro definition
    text = re.sub(r"MACRO.*?ENDM", "", text, flags=re.DOTALL)
    text = re.sub(r";[^\n]*", "", text)

    coll = {}
    cur_labels = []

    for line in text.splitlines():
        line = line.strip()
        if not line:
            continue
        # Label line: "Overworld_Coll::" or "RedsHouse1_Coll::"
        m = re.match(r"^(\w+_Coll)::$", line)
        if m:
            cur_labels.append(m.group(1))
            continue
        # Data line: "coll_tiles $xx, $yy, ..."
        m = re.match(r"coll_tiles(.*)", line)
        if m and cur_labels:
            hex_vals = re.findall(r"\$([0-9A-Fa-f]+)", m.group(1))
            tiles = [int(v, 16) for v in hex_vals]
            for lbl in cur_labels:
                coll[lbl] = tiles
            cur_labels = []

    return coll

# ---------------------------------------------------------------------------
# Parse map header pointer list -> ordered list of map names
# ---------------------------------------------------------------------------

def parse_map_order(ptrs_asm):
    text = ptrs_asm.read_text(encoding="utf-8")
    text = re.sub(r";[^\n]*", "", text)
    names = []
    for m in re.finditer(r"dw\s+(\w+)_h\b", text):
        names.append(m.group(1))
    return names

# ---------------------------------------------------------------------------
# Parse map constants -> dict of name -> (width, height)
# ---------------------------------------------------------------------------

def parse_map_constants(const_asm):
    text = const_asm.read_text(encoding="utf-8")
    text = re.sub(r";[^\n]*", "", text)
    result = {}
    for m in re.finditer(r"map_const\s+(\w+)\s*,\s*(\d+)\s*,\s*(\d+)", text):
        result[m.group(1)] = (int(m.group(2)), int(m.group(3)))  # width, height
    return result

# ---------------------------------------------------------------------------
# Parse map headers -> dict of map_name -> tileset_name
# ---------------------------------------------------------------------------

def parse_map_headers(headers_dir):
    """Returns dict: map_name -> (tileset_name, const_name).
    Reads 'map_header MapName, CONST_NAME, TILESET, flags' from each .asm file.
    Using the constant name directly avoids camel_to_upper conversion bugs."""
    result = {}
    for f in headers_dir.glob("*.asm"):
        text = f.read_text(encoding="utf-8")
        text = re.sub(r";[^\n]*", "", text)
        m = re.search(r"map_header\s+\w+\s*,\s*(\w+)\s*,\s*(\w+)\s*,", text)
        if m:
            const_name   = m.group(1)   # e.g. ROUTE_1
            tileset_name = m.group(2)   # e.g. OVERWORLD
            result[f.stem] = (tileset_name, const_name)
    return result

def parse_map_tilesets(headers_dir):
    result = {}
    for name, (ts, _) in parse_map_headers(headers_dir).items():
        result[name] = ts
    return result

# ---------------------------------------------------------------------------
# Main extraction
# ---------------------------------------------------------------------------

print("Parsing map order...")
map_order = parse_map_order(SRC / "data/maps/map_header_pointers.asm")
print(f"  {len(map_order)} maps in pointer table")

print("Parsing map constants...")
map_dims = parse_map_constants(SRC / "constants/map_constants.asm")

print("Parsing map tilesets and constants from headers...")
map_headers  = parse_map_headers(SRC / "data/maps/headers")
map_tilesets = {name: ts for name, (ts, _) in map_headers.items()}

print("Parsing collision lists...")
coll_data = parse_collisions(SRC / "data/tilesets/collision_tile_ids.asm")

# ---------------------------------------------------------------------------
# Load blockset binary data
# ---------------------------------------------------------------------------

print("Loading blockset .bst files...")
blocksets = {}  # file_stem -> bytes
for ts_id in range(NUM_TILESETS):
    stem = BLOCKSET_FILES[ts_id]
    bst_path = SRC / "gfx/blocksets" / f"{stem}.bst"
    if stem not in blocksets:
        if bst_path.exists():
            blocksets[stem] = rbin(bst_path)
        else:
            print(f"  WARNING: missing {bst_path}", file=sys.stderr)
            blocksets[stem] = bytes(2048)

# ---------------------------------------------------------------------------
# Convert tileset PNGs to 2bpp
# ---------------------------------------------------------------------------

print("Converting tileset PNGs to 2bpp...")
gfx_data = {}  # file_stem -> bytes (2bpp)
for ts_id in range(NUM_TILESETS):
    stem = GFX_FILES[ts_id]
    png_path = SRC / "gfx/tilesets" / f"{stem}.png"
    if stem not in gfx_data:
        if png_path.exists():
            data = png_to_2bpp(png_path)
            gfx_data[stem] = data
            print(f"  {stem}.png -> {len(data)} bytes = {len(data)//16} tiles")
        else:
            print(f"  WARNING: missing {png_path}", file=sys.stderr)
            gfx_data[stem] = bytes(16 * 128)  # 128 blank tiles

# ---------------------------------------------------------------------------
# Build per-tileset collision list (label name -> array of tile IDs)
# ---------------------------------------------------------------------------

TILESET_COLL_LABEL = {
    0: "Overworld_Coll", 1: "RedsHouse2_Coll", 2: "Pokecenter_Coll",
    3: "Forest_Coll",    4: "RedsHouse2_Coll", 5: "Gym_Coll",
    6: "Pokecenter_Coll",7: "Gym_Coll",        8: "House_Coll",
    9: "Gate_Coll",      10: "Gate_Coll",      11: "Underground_Coll",
    12: "Gate_Coll",     13: "Ship_Coll",      14: "ShipPort_Coll",
    15: "Cemetery_Coll", 16: "Interior_Coll",  17: "Cavern_Coll",
    18: "Lobby_Coll",    19: "Mansion_Coll",   20: "Lab_Coll",
    21: "Club_Coll",     22: "Facility_Coll",  23: "Plateau_Coll",
}

# ---------------------------------------------------------------------------
# Generate tileset_data.h
# ---------------------------------------------------------------------------

tileset_h = """\
#pragma once
/* tileset_data.h -- Generated from pokered-master tileset binaries.
 *
 * Each tileset entry has:
 *   blocks[]     -- block table: 16 tile IDs per block (4x4), indexed 0..N-1
 *   gfx[]        -- 2bpp tile graphics: 16 bytes per tile
 *   gfx_tiles    -- number of tiles in gfx[]
 *   num_blocks   -- number of blocks
 *   coll_tiles[] -- passable tile ID list ($FF-terminated)
 */
#include <stdint.h>

#define NUM_TILESETS 24

typedef struct {
    const uint8_t *blocks;      /* block table (16 bytes per block) */
    uint16_t       num_blocks;
    const uint8_t *gfx;         /* 2bpp tile graphics (16 bytes per tile) */
    uint16_t       gfx_tiles;
    const uint8_t *coll_tiles;  /* passable tile IDs, 0xFF-terminated */
} tileset_info_t;

extern const tileset_info_t gTilesets[NUM_TILESETS];
"""
wfile("tileset_data.h", tileset_h)

# ---------------------------------------------------------------------------
# Generate tileset_data.c
# ---------------------------------------------------------------------------

lines = [
    "/* tileset_data.c -- Generated from pokered-master tileset binaries. */",
    '#include "tileset_data.h"',
    "",
]

# Emit block tables (one per unique stem)
written_stems = set()
for ts_id in range(NUM_TILESETS):
    stem = BLOCKSET_FILES[ts_id]
    if stem in written_stems:
        continue
    written_stems.add(stem)
    data = blocksets[stem]
    lines.append(f"static const uint8_t kBlocks_{stem}[{len(data)}] = {{")
    lines.append(bytes_to_c_array(data))
    lines.append("};")
    lines.append("")

# Emit GFX data (one per unique stem)
written_stems = set()
for ts_id in range(NUM_TILESETS):
    stem = GFX_FILES[ts_id]
    if stem in written_stems:
        continue
    written_stems.add(stem)
    data = gfx_data[stem]
    lines.append(f"static const uint8_t kGfx_{stem}[{len(data)}] = {{")
    lines.append(bytes_to_c_array(data))
    lines.append("};")
    lines.append("")

# Emit collision lists (one per unique label)
written_labels = set()
for ts_id in range(NUM_TILESETS):
    label = TILESET_COLL_LABEL.get(ts_id, "")
    if label in written_labels:
        continue
    written_labels.add(label)
    tiles = coll_data.get(label, [])
    tile_strs = ", ".join(f"0x{t:02X}" for t in tiles) + ", 0xFF"
    lines.append(f"static const uint8_t kColl_{label}[] = {{ {tile_strs} }};")

lines.append("")

# Emit the gTilesets table
lines.append("const tileset_info_t gTilesets[NUM_TILESETS] = {")
for ts_id in range(NUM_TILESETS):
    bstem = BLOCKSET_FILES[ts_id]
    gstem = GFX_FILES[ts_id]
    label = TILESET_COLL_LABEL.get(ts_id, "Overworld_Coll")
    bdata = blocksets[bstem]
    gdata = gfx_data[gstem]
    num_blocks = len(bdata) // 16
    num_tiles  = len(gdata) // 16
    lines.append(
        f"    [{ts_id}] = {{ "
        f".blocks=kBlocks_{bstem}, .num_blocks={num_blocks}, "
        f".gfx=kGfx_{gstem}, .gfx_tiles={num_tiles}, "
        f".coll_tiles=kColl_{label} }},"
    )
lines.append("};")
lines.append("")

wfile("tileset_data.c", "\n".join(lines))

# ---------------------------------------------------------------------------
# Generate map_data.h
# ---------------------------------------------------------------------------

map_h = """\
#pragma once
/* map_data.h -- Generated from pokered-master map data.
 *
 * gMapTable is indexed by map ID (0x00 = PALLET_TOWN ... 0xF7 = AGATHAS_ROOM).
 * For unused/invalid map IDs, blocks==NULL.
 */
#include <stdint.h>

typedef struct {
    uint8_t        width;       /* map width in blocks */
    uint8_t        height;      /* map height in blocks */
    uint8_t        tileset_id;  /* index into gTilesets[] */
    uint8_t        border_block;/* block ID to use for out-of-bounds */
    const uint8_t *blocks;      /* raw block data (width*height bytes) */
    const char    *name;        /* map name for debug */
} map_info_t;

#define NUM_MAPS 248

extern const map_info_t gMapTable[NUM_MAPS];
"""
wfile("map_data.h", map_h)

# ---------------------------------------------------------------------------
# Generate map_data.c
# ---------------------------------------------------------------------------

# Build ID -> info for all 248 map IDs
# map_order list is in map_id order starting from 0

TILESET_NAME_TO_ID = {v: k for k, v in
    {v: k for k, v in {
        "OVERWORLD": 0, "REDS_HOUSE_1": 1, "MART": 2, "FOREST": 3,
        "REDS_HOUSE_2": 4, "DOJO": 5, "POKECENTER": 6, "GYM": 7,
        "HOUSE": 8, "FOREST_GATE": 9, "MUSEUM": 10, "UNDERGROUND": 11,
        "GATE": 12, "SHIP": 13, "SHIP_PORT": 14, "CEMETERY": 15,
        "INTERIOR": 16, "CAVERN": 17, "LOBBY": 18, "MANSION": 19,
        "LAB": 20, "CLUB": 21, "FACILITY": 22, "PLATEAU": 23,
    }.items()}.items()}
TILESET_NAME_TO_ID = {
    "OVERWORLD": 0, "REDS_HOUSE_1": 1, "MART": 2, "FOREST": 3,
    "REDS_HOUSE_2": 4, "DOJO": 5, "POKECENTER": 6, "GYM": 7,
    "HOUSE": 8, "FOREST_GATE": 9, "MUSEUM": 10, "UNDERGROUND": 11,
    "GATE": 12, "SHIP": 13, "SHIP_PORT": 14, "CEMETERY": 15,
    "INTERIOR": 16, "CAVERN": 17, "LOBBY": 18, "MANSION": 19,
    "LAB": 20, "CLUB": 21, "FACILITY": 22, "PLATEAU": 23,
}

# Also need map_name -> map_id (for const names in map_constants)
# The map_order list maps index -> map header name (CamelCase from the pointer file)
# The map_constants use UPPER_SNAKE names like PALLET_TOWN
# Derive UPPER_SNAKE from CamelCase map name:
def camel_to_upper(s):
    return re.sub(r'(?<=[a-z0-9])(?=[A-Z])', '_', s).upper()

map_data_lines = [
    "/* map_data.c -- Generated from pokered-master map data. */",
    '#include "map_data.h"',
    "",
]

# Embed block data for each map.
# Multiple map IDs can share the same map_name (duplicate pointer entries in pokered).
# Only emit the C array once per unique name; subsequent entries reference the first.
map_entries = []  # (map_id, name, width, height, ts_id, has_blk, border)
emitted_blk_names = set()

for map_id, map_name in enumerate(map_order):
    blk_path = SRC / "maps" / f"{map_name}.blk"
    # Use the constant name from the header file directly — avoids camel_to_upper bugs
    _, const_name = map_headers.get(map_name, (None, camel_to_upper(map_name)))
    dims = map_dims.get(const_name, (0, 0))
    w, h = dims

    tileset_name = map_tilesets.get(map_name, "OVERWORLD")
    ts_id = TILESET_NAME_TO_ID.get(tileset_name, 0)

    if blk_path.exists() and w > 0 and h > 0:
        blk = rbin(blk_path)
        expected = w * h
        if len(blk) != expected:
            print(f"  WARNING: {map_name}.blk size {len(blk)} != {w}*{h}={expected}", file=sys.stderr)
            blk = blk[:expected] if len(blk) > expected else blk + bytes(expected - len(blk))

        if map_name not in emitted_blk_names:
            map_data_lines.append(f"static const uint8_t kBlk_{map_name}[{len(blk)}] = {{")
            map_data_lines.append(bytes_to_c_array(blk))
            map_data_lines.append("};")
            emitted_blk_names.add(map_name)
        map_entries.append((map_id, map_name, w, h, ts_id, True, 0))
    else:
        map_entries.append((map_id, map_name, w, h, ts_id, False, 0))

map_data_lines.append("")

# Emit gMapTable
map_data_lines.append("const map_info_t gMapTable[NUM_MAPS] = {")

for i in range(248):
    if i < len(map_entries):
        map_id, name, w, h, ts_id, has_blk, border = map_entries[i]
        blk_ref = f"kBlk_{name}" if has_blk else "NULL"
        map_data_lines.append(
            f'    [{map_id}] = {{ .width={w:3d}, .height={h:3d}, '
            f'.tileset_id={ts_id:2d}, .border_block=0, '
            f'.blocks={blk_ref}, .name="{name}" }},'
        )
    else:
        map_data_lines.append(f"    [{i}] = {{ 0, 0, 0, 0, NULL, NULL }},")

map_data_lines.append("};")
map_data_lines.append("")

wfile("map_data.c", "\n".join(map_data_lines))

print("Done.")
