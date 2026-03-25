#!/usr/bin/env python3
"""
extract_party_icons.py -- Extract Gen 1 party menu icon tile data.

Reads PNGs from pokered-master (same decode logic as extract_sprites.py) and
generates src/data/party_icon_data.h + party_icon_data.c containing:
  - gMonPartyIconType[152]  dex index 1-151 -> ICON_* constant
  - gIconTileGfx[256][16]   sprite_tile_gfx slot -> 16-byte GB 2bpp tile

Tile slot mapping mirrors LoadMonPartySpriteGfx / MonPartySpritePointers
(engine/gfx/mon_icons.asm, data/icon_pointers.asm):

  Frame 1 tiles (base = ICON_TYPE << 2, used by WriteMonPartySpriteOAM):
    ICON_MON (0)        -> slots  0, 2     from sprites/monster.png tiles 12, 14
    ICON_BALL (1)       -> slots  4, 6     from sprites/poke_ball.png tiles 0, 2
    ICON_HELIX (2)      -> slots  8-11     from sprites/poke_ball.png tiles 0-3
    ICON_FAIRY (3)      -> slots 12, 14    from sprites/fairy.png tiles 12, 14
    ICON_BIRD (4)       -> slots 16, 18    from sprites/bird.png tiles 12, 14
    ICON_WATER (5)      -> slots 20, 22    from sprites/seel.png tiles 0, 2
    ICON_BUG (6)        -> slots 24, 26    from icons/bug.png tiles 2, 3 (Frame2)
    ICON_GRASS (7)      -> slots 28, 30    from icons/plant.png tiles 2, 3 (Frame2)
    ICON_SNAKE (8)      -> slots 32, 34    from icons/snake.png tiles 0, 1
    ICON_QUADRUPED (9)  -> slots 36, 38    from icons/quadruped.png tiles 0, 1
    ICON_TRADEBUBBLE(14)-> slots 56, 58    from trade/bubble.png tiles 0, 2

  Frame 2 tiles (base = ICONOFFSET(64) + ICON_TYPE << 2, toggled by animation):
    ICON_MON        -> slots 64, 66     from monster.png tiles 0, 2
    ICON_BALL       -> slots 68, 70     from poke_ball.png tiles 0, 2 (same as frame1)
    ICON_HELIX      -> slots 72-75      from poke_ball.png tiles 0-3
    ICON_FAIRY      -> slots 76, 78     from fairy.png tiles 0, 2
    ICON_BIRD       -> slots 80, 82     from bird.png tiles 0, 2
    ICON_WATER      -> slots 84, 86     from seel.png tiles 12, 14
    ICON_BUG        -> slots 88, 90     from bug.png tiles 0, 1 (Frame1)
    ICON_GRASS      -> slots 92, 94     from plant.png tiles 0, 1 (Frame1)
    ICON_SNAKE      -> slots 96, 98     from snake.png tiles 2, 3
    ICON_QUADRUPED  -> slots 100, 102   from quadruped.png tiles 2, 3
    ICON_TRADEBUBBLE-> slots 120, 122   from bubble.png tiles 4, 6
"""

import struct, zlib
from pathlib import Path

POKERED = Path(r"C:\Users\Anthony\pokered\pokered-master")
OUT     = Path(r"C:\Users\Anthony\pokered\src\data")

# ---------------------------------------------------------------------------
# PNG 2bpp decoder (identical to extract_sprites.py)
# ---------------------------------------------------------------------------

def decode_png_2bpp_grayscale(path):
    with open(path, "rb") as f:
        data = f.read()
    assert data[:8] == b'\x89PNG\r\n\x1a\n'
    idat_chunks = []
    i = 8
    width = height = 0
    while i < len(data):
        length = struct.unpack('>I', data[i:i+4])[0]
        chunk_type = data[i+4:i+8]
        chunk_data = data[i+8:i+8+length]
        if chunk_type == b'IHDR':
            width, height = struct.unpack('>II', chunk_data[:8])
            bd, ct = chunk_data[8], chunk_data[9]
            assert bd == 2 and ct == 0, f"Expected 2bpp grayscale, got bd={bd} ct={ct}"
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
        if filt == 0: pass
        elif filt == 1:
            for j in range(1, stride): row_raw[j] = (row_raw[j] + row_raw[j-1]) & 0xFF
        elif filt == 2:
            for j in range(stride): row_raw[j] = (row_raw[j] + prev_row[j]) & 0xFF
        elif filt == 3:
            for j in range(stride):
                a = row_raw[j-1] if j > 0 else 0
                row_raw[j] = (row_raw[j] + (a + prev_row[j]) // 2) & 0xFF
        elif filt == 4:
            for j in range(stride):
                a = row_raw[j-1] if j > 0 else 0
                b = prev_row[j]
                c = prev_row[j-1] if j > 0 else 0
                pa = abs(b-c); pb = abs(a-c); pc = abs(a+b-2*c)
                pr = a if pa<=pb and pa<=pc else (b if pb<=pc else c)
                row_raw[j] = (row_raw[j] + pr) & 0xFF
        prev_row = bytes(row_raw)
        for byte in row_raw:
            for shift in (6, 4, 2, 0):
                pixels.append((byte >> shift) & 3)
    return width, height, pixels[:width * height]

def get_tile(path, tile_index):
    """Extract one 8x8 GB 2bpp tile from a PNG. Returns 16 bytes."""
    w, h, pixels = decode_png_2bpp_grayscale(path)
    tiles_x = w // 8
    tx = tile_index % tiles_x
    ty = tile_index // tiles_x
    out = bytearray(16)
    for row in range(8):
        lo = hi = 0
        for col in range(8):
            pv = pixels[(ty*8 + row) * w + (tx*8 + col)]
            pv = 3 - pv  # invert: dark PNG -> dark GB color
            bit = 7 - col
            if pv & 1: lo |= (1 << bit)
            if pv & 2: hi |= (1 << bit)
        out[row*2]   = lo
        out[row*2+1] = hi
    return bytes(out)

# ---------------------------------------------------------------------------
# Tile load table: (sprite_tile_gfx_slot, png_path, png_tile_index)
# ---------------------------------------------------------------------------

GFX = POKERED / "gfx"

TILE_MAP = [
    # --- Frame 1 (slot = ICON_TYPE << 2) ---
    # ICON_MON = 0, base = 0
    ( 0, GFX/"sprites"/"monster.png",    12),
    ( 2, GFX/"sprites"/"monster.png",    14),
    # ICON_BALL = 1, base = 4
    ( 4, GFX/"sprites"/"poke_ball.png",   0),
    ( 6, GFX/"sprites"/"poke_ball.png",   2),
    # ICON_HELIX = 2, base = 8  (asymmetric: 4 unique tiles from poke_ball)
    ( 8, GFX/"sprites"/"poke_ball.png",   0),
    ( 9, GFX/"sprites"/"poke_ball.png",   1),
    (10, GFX/"sprites"/"poke_ball.png",   2),
    (11, GFX/"sprites"/"poke_ball.png",   3),
    # ICON_FAIRY = 3, base = 12
    (12, GFX/"sprites"/"fairy.png",      12),
    (14, GFX/"sprites"/"fairy.png",      14),
    # ICON_BIRD = 4, base = 16
    (16, GFX/"sprites"/"bird.png",       12),
    (18, GFX/"sprites"/"bird.png",       14),
    # ICON_WATER = 5, base = 20
    (20, GFX/"sprites"/"seel.png",        0),
    (22, GFX/"sprites"/"seel.png",        2),
    # ICON_BUG = 6, base = 24  (frame1 uses BugIconFrame2 = bug.png tiles 2-3)
    (24, GFX/"icons"/"bug.png",           2),
    (26, GFX/"icons"/"bug.png",           3),
    # ICON_GRASS = 7, base = 28  (frame1 uses PlantIconFrame2 = plant.png tiles 2-3)
    (28, GFX/"icons"/"plant.png",         2),
    (30, GFX/"icons"/"plant.png",         3),
    # ICON_SNAKE = 8, base = 32
    (32, GFX/"icons"/"snake.png",         0),
    (34, GFX/"icons"/"snake.png",         1),
    # ICON_QUADRUPED = 9, base = 36
    (36, GFX/"icons"/"quadruped.png",     0),
    (38, GFX/"icons"/"quadruped.png",     1),
    # ICON_TRADEBUBBLE = 14, base = 56
    (56, GFX/"trade"/"bubble.png",        0),
    (58, GFX/"trade"/"bubble.png",        2),

    # --- Frame 2 (slot = ICONOFFSET(64) + ICON_TYPE << 2) ---
    # ICON_MON frame2 = base 64
    (64, GFX/"sprites"/"monster.png",     0),
    (66, GFX/"sprites"/"monster.png",     2),
    # ICON_BALL frame2 = base 68  (ball shakes, no tile change - same as frame1)
    (68, GFX/"sprites"/"poke_ball.png",   0),
    (70, GFX/"sprites"/"poke_ball.png",   2),
    # ICON_HELIX frame2 = base 72
    (72, GFX/"sprites"/"poke_ball.png",   0),
    (73, GFX/"sprites"/"poke_ball.png",   1),
    (74, GFX/"sprites"/"poke_ball.png",   2),
    (75, GFX/"sprites"/"poke_ball.png",   3),
    # ICON_FAIRY frame2 = base 76
    (76, GFX/"sprites"/"fairy.png",       0),
    (78, GFX/"sprites"/"fairy.png",       2),
    # ICON_BIRD frame2 = base 80
    (80, GFX/"sprites"/"bird.png",        0),
    (82, GFX/"sprites"/"bird.png",        2),
    # ICON_WATER frame2 = base 84  (seel.png tiles 12, 14)
    (84, GFX/"sprites"/"seel.png",       12),
    (86, GFX/"sprites"/"seel.png",       14),
    # ICON_BUG frame2 = base 88  (BugIconFrame1 = bug.png tiles 0-1)
    (88, GFX/"icons"/"bug.png",           0),
    (90, GFX/"icons"/"bug.png",           1),
    # ICON_GRASS frame2 = base 92  (PlantIconFrame1 = plant.png tiles 0-1)
    (92, GFX/"icons"/"plant.png",         0),
    (94, GFX/"icons"/"plant.png",         1),
    # ICON_SNAKE frame2 = base 96
    (96, GFX/"icons"/"snake.png",         2),
    (98, GFX/"icons"/"snake.png",         3),
    # ICON_QUADRUPED frame2 = base 100
   (100, GFX/"icons"/"quadruped.png",     2),
   (102, GFX/"icons"/"quadruped.png",     3),
    # ICON_TRADEBUBBLE frame2 = base 120
   (120, GFX/"trade"/"bubble.png",        4),
   (122, GFX/"trade"/"bubble.png",        6),
]

# ---------------------------------------------------------------------------
# MonPartyData: dex index 1-151 -> ICON_* type
# (from pokered-master/data/pokemon/menu_icons.asm, Pokedex order)
# ICON_MON=0 ICON_BALL=1 ICON_HELIX=2 ICON_FAIRY=3 ICON_BIRD=4
# ICON_WATER=5 ICON_BUG=6 ICON_GRASS=7 ICON_SNAKE=8 ICON_QUADRUPED=9
# ---------------------------------------------------------------------------

MON_PARTY_DATA = [
    0,  # [0] unused
    # dex 1-9: Bulbasaur line, Charmander line, Squirtle line
    7, 7, 7,   # 1 Bulbasaur, 2 Ivysaur, 3 Venusaur
    0, 0, 0,   # 4 Charmander, 5 Charmeleon, 6 Charizard
    5, 5, 5,   # 7 Squirtle, 8 Wartortle, 9 Blastoise
    # dex 10-15: Caterpie line, Weedle line
    6, 6, 6,   # 10 Caterpie, 11 Metapod, 12 Butterfree
    6, 6, 6,   # 13 Weedle, 14 Kakuna, 15 Beedrill
    # dex 16-22: Pidgey line, Rattata line, Spearow line
    4, 4, 4,   # 16 Pidgey, 17 Pidgeotto, 18 Pidgeot
    9, 9,      # 19 Rattata, 20 Raticate
    4, 4,      # 21 Spearow, 22 Fearow
    # dex 23-34: Ekans, Pikachu lines, Nido lines
    8, 8,      # 23 Ekans, 24 Arbok
    3, 3,      # 25 Pikachu, 26 Raichu
    0, 0,      # 27 Sandshrew, 28 Sandslash
    0, 0, 0,   # 29 NidoranF, 30 Nidorina, 31 Nidoqueen
    0, 0, 0,   # 32 NidoranM, 33 Nidorino, 34 Nidoking
    # dex 35-42: Clefairy, Vulpix, Jigglypuff, Zubat
    3, 3,      # 35 Clefairy, 36 Clefable
    9, 9,      # 37 Vulpix, 38 Ninetales
    3, 3,      # 39 Jigglypuff, 40 Wigglytuff
    0, 0,      # 41 Zubat, 42 Golbat
    # dex 43-57: Oddish, Paras, Venonat, Diglett, Meowth, Psyduck, Mankey
    7, 7, 7,   # 43 Oddish, 44 Gloom, 45 Vileplume
    6, 6,      # 46 Paras, 47 Parasect
    6, 6,      # 48 Venonat, 49 Venomoth
    0, 0,      # 50 Diglett, 51 Dugtrio
    0, 0,      # 52 Meowth, 53 Persian
    0, 0,      # 54 Psyduck, 55 Golduck
    0, 0,      # 56 Mankey, 57 Primeape
    # dex 58-68: Growlithe, Poliwag, Abra, Machop
    9, 9,      # 58 Growlithe, 59 Arcanine
    0, 0, 0,   # 60 Poliwag, 61 Poliwhirl, 62 Poliwrath
    0, 0, 0,   # 63 Abra, 64 Kadabra, 65 Alakazam
    0, 0, 0,   # 66 Machop, 67 Machoke, 68 Machamp
    # dex 69-76: Bellsprout, Tentacool, Geodude
    7, 7, 7,   # 69 Bellsprout, 70 Weepinbell, 71 Victreebel
    5, 5,      # 72 Tentacool, 73 Tentacruel
    0, 0, 0,   # 74 Geodude, 75 Graveler, 76 Golem
    # dex 77-89: Ponyta, Slowpoke, Magnemite, Farfetchd, Seel, Grimer
    9, 9, 9,   # 77 Ponyta, 78 Rapidash, 79 Slowpoke
    0,         # 80 Slowbro
    1, 1,      # 81 Magnemite, 82 Magneton
    4, 4, 4,   # 83 Farfetchd, 84 Doduo, 85 Dodrio
    5, 5,      # 86 Seel, 87 Dewgong
    0, 0,      # 88 Grimer, 89 Muk
    # dex 90-101: Shellder, Gastly, Onix, Drowzee, Krabby, Voltorb
    2, 2,      # 90 Shellder, 91 Cloyster
    0, 0, 0,   # 92 Gastly, 93 Haunter, 94 Gengar
    8,         # 95 Onix
    0, 0,      # 96 Drowzee, 97 Hypno
    5, 5,      # 98 Krabby, 99 Kingler
    1, 1,      # 100 Voltorb, 101 Electrode
    # dex 102-114: Exeggcute, Cubone, Hitmonlee, Lickitung, Koffing, Rhyhorn, Chansey
    7, 7,      # 102 Exeggcute, 103 Exeggutor
    0, 0,      # 104 Cubone, 105 Marowak
    0, 0,      # 106 Hitmonlee, 107 Hitmonchan
    0,         # 108 Lickitung
    0, 0,      # 109 Koffing, 110 Weezing
    9,         # 111 Rhyhorn
    0,         # 112 Rhydon
    3,         # 113 Chansey
    # dex 114-121: Tangela, Kangaskhan, Horsea, Goldeen, Staryu
    7,         # 114 Tangela
    0,         # 115 Kangaskhan
    5, 5,      # 116 Horsea, 117 Seadra
    5, 5,      # 118 Goldeen, 119 Seaking
    2, 2,      # 120 Staryu, 121 Starmie
    # dex 122-131: Mr.Mime, Scyther, Jynx, Electabuzz, Magmar, Pinsir, Tauros, Magikarp
    0,         # 122 Mr.Mime
    6,         # 123 Scyther
    0, 0, 0,   # 124 Jynx, 125 Electabuzz, 126 Magmar
    6,         # 127 Pinsir
    9,         # 128 Tauros
    5,         # 129 Magikarp
    8,         # 130 Gyarados
    5,         # 131 Lapras
    0,         # 132 Ditto
    # dex 133-143: Eevee line, Porygon, Omanyte line, Kabuto line, Aerodactyl
    9, 9, 9, 9, # 133 Eevee, 134 Vaporeon, 135 Jolteon, 136 Flareon
    0,          # 137 Porygon
    2, 2,       # 138 Omanyte, 139 Omastar
    2, 2,       # 140 Kabuto, 141 Kabutops
    4,          # 142 Aerodactyl
    # dex 143-151: Snorlax, birds, Dratini line, Mewtwo, Mew
    0,          # 143 Snorlax
    4, 4, 4,    # 144 Articuno, 145 Zapdos, 146 Moltres
    8, 8, 8,    # 147 Dratini, 148 Dragonair, 149 Dragonite
    0,          # 150 Mewtwo
    0,          # 151 Mew
]

assert len(MON_PARTY_DATA) == 152, f"Expected 152, got {len(MON_PARTY_DATA)}"

# ---------------------------------------------------------------------------
# Generate C files
# ---------------------------------------------------------------------------

def main():
    # Build gIconTileGfx[256][16]
    tile_gfx = [[0]*16 for _ in range(256)]
    errors = []
    for slot, png_path, tile_idx in TILE_MAP:
        try:
            tile_data = get_tile(png_path, tile_idx)
            tile_gfx[slot] = list(tile_data)
        except Exception as e:
            errors.append(f"slot {slot} {png_path.name} tile{tile_idx}: {e}")

    for e in errors:
        print(f"ERROR: {e}")

    used_slots = sorted(set(slot for slot, _, _ in TILE_MAP))

    # --- Header ---
    h_lines = [
        "#pragma once",
        "/* party_icon_data.h -- Generated by tools/extract_party_icons.py */",
        "#include <stdint.h>",
        "",
        "/* Icon type constants (mirror of pokered constants/icon_constants.asm) */",
        "#define ICON_MON         0",
        "#define ICON_BALL        1",
        "#define ICON_HELIX       2",
        "#define ICON_FAIRY       3",
        "#define ICON_BIRD        4",
        "#define ICON_WATER       5",
        "#define ICON_BUG         6",
        "#define ICON_GRASS       7",
        "#define ICON_SNAKE       8",
        "#define ICON_QUADRUPED   9",
        "#define ICON_TRADEBUBBLE 14",
        "#define ICON_ICONOFFSET  64   /* $40: frame2 tile slot = ICONOFFSET + ICON_TYPE<<2 */",
        "",
        "/* dex index 1-151 -> ICON_* type (index 0 unused) */",
        "extern const uint8_t gMonPartyIconType[152];",
        "",
        "/* Sprite tile data indexed by sprite_tile_gfx slot. */",
        "extern const uint8_t gIconTileGfx[256][16];",
        "",
        "/* Load party icon tile data into sprite_tile_gfx. */",
        "void PartyIcons_LoadTiles(void);",
    ]

    # --- Source ---
    c_lines = [
        "/* party_icon_data.c -- Generated by tools/extract_party_icons.py */",
        '#include "party_icon_data.h"',
        '#include "../platform/display.h"',
        "",
        "const uint8_t gMonPartyIconType[152] = {",
    ]

    # MonPartyData in rows of 10
    for i in range(0, 152, 10):
        chunk = MON_PARTY_DATA[i:i+10]
        row = ", ".join(str(v) for v in chunk)
        c_lines.append(f"    {row},  /* {i} */")
    c_lines.append("};")
    c_lines.append("")

    c_lines.append("const uint8_t gIconTileGfx[256][16] = {")
    for slot in range(256):
        b = tile_gfx[slot]
        row_str = ", ".join(f"0x{x:02X}" for x in b)
        c_lines.append(f"    {{ {row_str} }},  /* [{slot:3d}] */")
    c_lines.append("};")
    c_lines.append("")

    c_lines.append("void PartyIcons_LoadTiles(void) {")
    c_lines.append(f"    static const uint8_t kSlots[] = {{ {', '.join(str(s) for s in used_slots)} }};")
    c_lines.append(f"    for (int i = 0; i < {len(used_slots)}; i++)")
    c_lines.append( "        Display_LoadSpriteTile(kSlots[i], gIconTileGfx[kSlots[i]]);")
    c_lines.append("}")
    c_lines.append("")

    h_out = "\n".join(h_lines) + "\n"
    c_out = "\n".join(c_lines) + "\n"

    with open(OUT / "party_icon_data.h", "w") as f: f.write(h_out)
    with open(OUT / "party_icon_data.c", "w") as f: f.write(c_out)
    print(f"Written party_icon_data.h/c  ({len(used_slots)} tile slots, {len(MON_PARTY_DATA)} dex entries)")
    if errors:
        print(f"  {len(errors)} extraction errors")

if __name__ == "__main__":
    main()
