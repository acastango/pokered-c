#!/usr/bin/env python3
"""
extract_connections.py -- Extract map connection data from pokered-master headers.

Reads:  data/maps/headers/*.asm  (connection declarations)
        constants/map_constants.asm  (map W/H/ID)

Writes: src/data/connection_data.h
        src/data/connection_data.c

Connection alignment formulas (from macros/scripts/maps.asm):
  north: player_y = dest_height*2-1,  x_adjust = -offset*2
  south: player_y = 0,                x_adjust = -offset*2
  west:  player_x = dest_width*2-1,   y_adjust = -offset*2
  east:  player_x = 0,                y_adjust = -offset*2

On the GB, wXCoord/wYCoord are uint8 so wrapping at 0 gives 0xFF (255).
In our C port we use the same sentinel: 255 = one step beyond map edge.
"""

import glob
import re
from pathlib import Path

SRC  = Path(r"C:\Users\Anthony\pokered\pokered-master")
OUT  = Path(r"C:\Users\Anthony\pokered\src\data")

# ---------------------------------------------------------------------------
# 1. Parse map constants → {CONST_NAME: (width, height, map_id)}
# ---------------------------------------------------------------------------

def parse_map_constants(path):
    """Returns dict: CONST_NAME -> (width_blocks, height_blocks, map_id)"""
    consts = {}
    map_id = 0
    with open(path) as f:
        for line in f:
            m = re.match(r'\s*map_const\s+(\w+)\s*,\s*(\d+)\s*,\s*(\d+)', line)
            if m:
                name = m.group(1)          # e.g. PALLET_TOWN
                w    = int(m.group(2))
                h    = int(m.group(3))
                consts[name] = (w, h, map_id)
                map_id += 1
    return consts

# ---------------------------------------------------------------------------
# 2. Parse map order (name -> map_id) from map_header_pointers.asm
# ---------------------------------------------------------------------------

def parse_map_order(path):
    """Returns list of CamelCase map names in map_id order (from 'dw FooBar_h' lines)."""
    order = []
    with open(path) as f:
        for line in f:
            m = re.match(r'\s*dw\s+(\w+)_h\b', line)
            if m:
                order.append(m.group(1))
    return order

# ---------------------------------------------------------------------------
# 3. Parse one header file for connections
# ---------------------------------------------------------------------------

def parse_header(path):
    """Returns list of (direction, dest_const, offset) tuples."""
    conns = []
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            s = line.strip()
            # connection north, ViridianCity, VIRIDIAN_CITY, 5
            m = re.match(r'connection\s+(north|south|west|east)\s*,\s*\w+\s*,\s*(\w+)\s*,\s*(-?\d+)', s)
            if m:
                direction = m.group(1)
                dest_const = m.group(2)   # e.g. VIRIDIAN_CITY
                offset = int(m.group(3))
                conns.append((direction, dest_const, offset))
    return conns

# ---------------------------------------------------------------------------
# 4. Compute alignment values
# ---------------------------------------------------------------------------

def compute_alignment(direction, dest_const, offset, map_consts):
    """
    Returns (player_coord, adjust, dest_map_id) for a connection.

    Our C port uses TILE coordinates (4 tiles per block), not the original
    half-block coords (2 per block).  Scale accordingly:
      original formula:  *2,  offset adjustment: -offset*2
      our port:          *4,  offset adjustment: -offset*4

    player_coord: the fixed axis coord when entering the destination map
    adjust:       signed delta applied to the free axis
    """
    if dest_const not in map_consts:
        return None
    dw, dh, dest_id = map_consts[dest_const]
    adjust = -offset * 4
    if direction == 'north':
        player_coord = dh * 4 - 1   # last tile row (step dh*2-1 → tile (dh*2-1)*2+1 = dh*4-1, odd)
    elif direction == 'south':
        player_coord = 1             # first tile row (step 0 → tile 0*2+1 = 1, odd)
    elif direction == 'west':
        player_coord = dw * 4 - 2   # last tile col (step dw*2-1 → tile (dw*2-1)*2 = dw*4-2, even)
    elif direction == 'east':
        player_coord = 0             # first tile col (step 0 → tile 0*2 = 0, even)
    return (player_coord, adjust, dest_id)

# ---------------------------------------------------------------------------
# 5. Main
# ---------------------------------------------------------------------------

def main():
    map_consts = parse_map_constants(SRC / 'constants/map_constants.asm')
    map_order  = parse_map_order(SRC / 'data/maps/map_header_pointers.asm')

    # Build CamelCase name -> map_id from the order list
    name_to_id = {name: idx for idx, name in enumerate(map_order)}
    num_maps   = len(map_order)

    # Build map_id -> {direction: (player_coord, adjust, dest_id)}
    connections = [{} for _ in range(num_maps)]

    headers_dir = SRC / 'data/maps/headers'
    for hpath in sorted(headers_dir.glob('*.asm')):
        map_name = hpath.stem   # CamelCase
        if map_name not in name_to_id:
            continue
        src_id = name_to_id[map_name]
        for (direction, dest_const, offset) in parse_header(hpath):
            result = compute_alignment(direction, dest_const, offset, map_consts)
            if result is None:
                print(f"  WARN: unknown dest const {dest_const} in {map_name}")
                continue
            player_coord, adjust, dest_id = result
            connections[src_id][direction] = (player_coord, adjust, dest_id)

    # ---- Emit .h -------------------------------------------------------
    h_lines = [
        "/* connection_data.h -- Generated by tools/extract_connections.py */",
        "#pragma once",
        "#include <stdint.h>",
        "",
        "/* One directional connection. dest_map==0xFF means no connection. */",
        "typedef struct {",
        "    uint8_t  dest_map;     /* destination map ID, 0xFF = none */",
        "    int16_t  player_coord; /* fixed-axis coord when entering dest (tile units) */",
        "    int16_t  adjust;       /* signed delta applied to the free axis (tile units) */",
        "} map_conn_t;",
        "",
        "typedef struct {",
        "    map_conn_t north;",
        "    map_conn_t south;",
        "    map_conn_t west;",
        "    map_conn_t east;",
        "} map_connections_t;",
        "",
        f"#define NUM_MAP_CONNECTIONS {num_maps}",
        "extern const map_connections_t gMapConnections[NUM_MAP_CONNECTIONS];",
        "",
    ]
    (OUT / 'connection_data.h').write_text('\n'.join(h_lines), encoding='utf-8')
    print(f"Wrote {OUT / 'connection_data.h'}")

    # ---- Emit .c -------------------------------------------------------
    def conn_entry(d):
        if d:
            pc, adj, did = d
            return f"{{0x{did:02X}, {pc:3d}, {adj:4d}}}"
        else:
            return "{0xFF,   0,    0}"

    c_lines = [
        "/* connection_data.c -- Generated by tools/extract_connections.py */",
        '#include "connection_data.h"',
        "",
        "const map_connections_t gMapConnections[NUM_MAP_CONNECTIONS] = {",
    ]
    for i, name in enumerate(map_order):
        conns = connections[i]
        n = conn_entry(conns.get('north'))
        s = conn_entry(conns.get('south'))
        w = conn_entry(conns.get('west'))
        e = conn_entry(conns.get('east'))
        c_lines.append(
            f"    [{i:3d}] = {{ .north={n}, .south={s}, .west={w}, .east={e} }},"
            f"  /* {name} */"
        )
    c_lines += ["};", ""]

    (OUT / 'connection_data.c').write_text('\n'.join(c_lines), encoding='utf-8')
    print(f"Wrote {OUT / 'connection_data.c'}")

    # ---- Summary -------------------------------------------------------
    total = sum(len(c) for c in connections)
    print(f"Total connections: {total} across {num_maps} maps")

if __name__ == '__main__':
    main()
