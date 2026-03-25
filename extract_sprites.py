#!/usr/bin/env python
"""extract_sprites.py — Convert Gen 1 Pokemon PNG sprites to GB 2bpp C data.

Reads front + back PNGs from pokered-master/gfx/pokemon/{front,back}/
Outputs src/data/pokemon_sprites.c and src/data/pokemon_sprites.h

Pixel value → GB color:
  255 (white)  → 0  (transparent / background)
  170 (light)  → 1
   85 (dark)   → 2
    0 (black)  → 3

GB 2bpp per tile row (8 px → 2 bytes):
  byte0 = plane0 (bit0 of each color), MSB = leftmost pixel
  byte1 = plane1 (bit1 of each color), MSB = leftmost pixel
"""

import os, sys
from PIL import Image

FRONT_DIR = r"C:\Users\Anthony\pokered\pokered-master\gfx\pokemon\front"
BACK_DIR  = r"C:\Users\Anthony\pokered\pokered-master\gfx\pokemon\back"
OUT_C     = r"C:\Users\Anthony\pokered\src\data\pokemon_sprites.c"
OUT_H     = r"C:\Users\Anthony\pokered\src\data\pokemon_sprites.h"

# dex number (1-151) -> base filename (without .png)
DEX_TO_FRONT = {
     1:'bulbasaur',  2:'ivysaur',    3:'venusaur',
     4:'charmander', 5:'charmeleon', 6:'charizard',
     7:'squirtle',   8:'wartortle',  9:'blastoise',
    10:'caterpie',  11:'metapod',   12:'butterfree',
    13:'weedle',    14:'kakuna',    15:'beedrill',
    16:'pidgey',    17:'pidgeotto', 18:'pidgeot',
    19:'rattata',   20:'raticate',
    21:'spearow',   22:'fearow',
    23:'ekans',     24:'arbok',
    25:'pikachu',   26:'raichu',
    27:'sandshrew', 28:'sandslash',
    29:'nidoranf',  30:'nidorina',  31:'nidoqueen',
    32:'nidoranm',  33:'nidorino',  34:'nidoking',
    35:'clefairy',  36:'clefable',
    37:'vulpix',    38:'ninetales',
    39:'jigglypuff',40:'wigglytuff',
    41:'zubat',     42:'golbat',
    43:'oddish',    44:'gloom',     45:'vileplume',
    46:'paras',     47:'parasect',
    48:'venonat',   49:'venomoth',
    50:'diglett',   51:'dugtrio',
    52:'meowth',    53:'persian',
    54:'psyduck',   55:'golduck',
    56:'mankey',    57:'primeape',
    58:'growlithe', 59:'arcanine',
    60:'poliwag',   61:'poliwhirl', 62:'poliwrath',
    63:'abra',      64:'kadabra',   65:'alakazam',
    66:'machop',    67:'machoke',   68:'machamp',
    69:'bellsprout',70:'weepinbell',71:'victreebel',
    72:'tentacool', 73:'tentacruel',
    74:'geodude',   75:'graveler',  76:'golem',
    77:'ponyta',    78:'rapidash',
    79:'slowpoke',  80:'slowbro',
    81:'magnemite', 82:'magneton',
    83:'farfetchd',
    84:'doduo',     85:'dodrio',
    86:'seel',      87:'dewgong',
    88:'grimer',    89:'muk',
    90:'shellder',  91:'cloyster',
    92:'gastly',    93:'haunter',   94:'gengar',
    95:'onix',
    96:'drowzee',   97:'hypno',
    98:'krabby',    99:'kingler',
   100:'voltorb',  101:'electrode',
   102:'exeggcute',103:'exeggutor',
   104:'cubone',   105:'marowak',
   106:'hitmonlee',107:'hitmonchan',
   108:'lickitung',
   109:'koffing',  110:'weezing',
   111:'rhyhorn',  112:'rhydon',
   113:'chansey',
   114:'tangela',
   115:'kangaskhan',
   116:'horsea',   117:'seadra',
   118:'goldeen',  119:'seaking',
   120:'staryu',   121:'starmie',
   122:'mr.mime',
   123:'scyther',
   124:'jynx',
   125:'electabuzz',
   126:'magmar',
   127:'pinsir',
   128:'tauros',
   129:'magikarp', 130:'gyarados',
   131:'lapras',
   132:'ditto',
   133:'eevee',    134:'vaporeon', 135:'jolteon',  136:'flareon',
   137:'porygon',
   138:'omanyte',  139:'omastar',
   140:'kabuto',   141:'kabutops',
   142:'aerodactyl',
   143:'snorlax',
   144:'articuno', 145:'zapdos',   146:'moltres',
   147:'dratini',  148:'dragonair',149:'dragonite',
   150:'mewtwo',
   151:'mew',
}

# back sprites: same base name + 'b' suffix
DEX_TO_BACK = {k: v + 'b' for k, v in DEX_TO_FRONT.items()}


def px_to_color(v):
    """Grayscale 0/85/170/255 → GB color 0-3."""
    if v >= 200: return 0   # white   → color 0 (transparent)
    if v >= 120: return 1   # light   → color 1
    if v >= 40:  return 2   # dark    → color 2
    return 3                # black   → color 3


def img_to_tiles(img, canvas_w_tiles, canvas_h_tiles):
    """Convert a PIL image to a list of canvas_w*canvas_h GB 2bpp tiles (16 bytes each).

    The image is centered in the canvas (padding with color-0 / white).
    canvas_w_tiles and canvas_h_tiles are in 8-pixel tile units.
    Returns a flat list of (canvas_w_tiles * canvas_h_tiles) tile byte lists.
    """
    canvas_px_w = canvas_w_tiles * 8
    canvas_px_h = canvas_h_tiles * 8

    img_w, img_h = img.size
    ox = (canvas_px_w - img_w) // 2   # x offset to center image
    oy = (canvas_px_h - img_h) // 2   # y offset to center image

    # Build canvas as 2D array of GB colors (0-3)
    canvas = [[0] * canvas_px_w for _ in range(canvas_px_h)]
    pixels = img.load()
    for y in range(img_h):
        for x in range(img_w):
            c = px_to_color(pixels[x, y])
            canvas[oy + y][ox + x] = c

    # Convert each 8x8 tile to 16 bytes (GB 2bpp)
    tiles = []
    for ty in range(canvas_h_tiles):
        for tx in range(canvas_w_tiles):
            tile_bytes = []
            for row in range(8):
                py = ty * 8 + row
                b0, b1 = 0, 0
                for col in range(8):
                    px_val = canvas[py][tx * 8 + col]
                    bit_pos = 7 - col
                    b0 |= (px_val & 1)       << bit_pos
                    b1 |= ((px_val >> 1) & 1) << bit_pos
                tile_bytes.append(b0)
                tile_bytes.append(b1)
            tiles.append(tile_bytes)
    return tiles


def load_front(dex):
    """Load front sprite, return (w_tiles, h_tiles, tiles_list)."""
    fname = DEX_TO_FRONT[dex] + '.png'
    path  = os.path.join(FRONT_DIR, fname)
    if not os.path.exists(path):
        print(f"  WARNING: missing front sprite dex={dex}: {path}", file=sys.stderr)
        return 7, 7, [([0]*16)] * 49
    img = Image.open(path).convert('L')
    w_tiles = img.width  // 8
    h_tiles = img.height // 8
    # All front sprites fit in 7x7 canvas
    tiles = img_to_tiles(img, 7, 7)
    return w_tiles, h_tiles, tiles


def load_back(dex):
    """Load back sprite scaled 2x, return tiles_list (7x7=49 tiles).

    Matches ScaleSpriteByTwo (engine/battle/scale_sprites.asm):
      - Input: 4x4 tile (32x32 px) sprite
      - Crop: last 4 rows and last 4 cols are ignored → 28x28 effective pixels
      - Scale: each pixel becomes 2x2 → 56x56 output (7x7 tiles)
    """
    fname = DEX_TO_BACK[dex] + '.png'
    path  = os.path.join(BACK_DIR, fname)
    if not os.path.exists(path):
        print(f"  WARNING: missing back sprite dex={dex}: {path}", file=sys.stderr)
        return [([0]*16)] * 49
    img = Image.open(path).convert('L')
    # Crop to 28x28 (drop last 4 rows and last 4 cols as the ASM does)
    img_crop = img.crop((0, 0, 28, 28))
    # Scale 2x nearest-neighbor → 56x56
    img_2x = img_crop.resize((56, 56), Image.NEAREST)
    # Encode as 7x7 tile canvas (image fills it exactly, no centering needed)
    tiles = img_to_tiles(img_2x, 7, 7)
    return tiles


def tiles_to_c(tiles):
    """Render a list of 16-byte tiles as a C initializer string."""
    rows = []
    for t in tiles:
        hex_bytes = ','.join(f'0x{b:02X}' for b in t)
        rows.append(f'        {{{hex_bytes}}}')
    return ',\n'.join(rows)


def main():
    print("Extracting front sprites...")
    front_w   = [0] * 152
    front_h   = [0] * 152
    front_tiles = [None] * 152  # each is list of 49 tiles

    for dex in range(1, 152):
        w, h, tiles = load_front(dex)
        front_w[dex]     = w
        front_h[dex]     = h
        front_tiles[dex] = tiles
        print(f"  dex {dex:3d}: {DEX_TO_FRONT[dex]:<15s} {w}x{h} tiles")

    print("Extracting back sprites...")
    back_tiles = [None] * 152  # each is list of 16 tiles

    for dex in range(1, 152):
        tiles = load_back(dex)
        back_tiles[dex] = tiles
        print(f"  dex {dex:3d}: {DEX_TO_BACK[dex]:<15s} 4x4 tiles")

    # --- Write .h ---
    h_src = """\
#pragma once
/* pokemon_sprites.h — GB 2bpp sprite data for all 151 Gen 1 Pokemon.
 *
 * Front sprites: stored as 7x7 tile canvas (56x56 px), centered.
 *   Actual sprite size (w x h tiles) stored in gPokemonFrontSpriteW/H.
 *   Index = Pokedex number (1-151). Index 0 is unused (all zeros).
 *
 * Back sprites: always 4x4 tiles (32x32 px), centered.
 *   Index = Pokedex number (1-151). Index 0 is unused (all zeros).
 *
 * GB 2bpp encoding: for each row in a tile (8px → 2 bytes):
 *   byte[2*row+0] = plane0 (LSB of color index), MSB = leftmost pixel
 *   byte[2*row+1] = plane1 (MSB of color index), MSB = leftmost pixel
 *   color 0 = white/transparent, 3 = black
 */
#include <stdint.h>

#define POKEMON_FRONT_CANVAS_TILES  49  /* 7x7 tile canvas */
#define POKEMON_BACK_TILES          49  /* 7x7 after 2x scale (ScaleSpriteByTwo) */

/* Natural size of each front sprite (may be 5, 6, or 7). */
extern const uint8_t gPokemonFrontSpriteW[152];
extern const uint8_t gPokemonFrontSpriteH[152];

/* Front sprite tile data: [dex][tile_index][16 bytes] */
extern const uint8_t gPokemonFrontSprite[152][POKEMON_FRONT_CANVAS_TILES][16];

/* Back sprite tile data: [dex][tile_index][16 bytes] */
extern const uint8_t gPokemonBackSprite[152][POKEMON_BACK_TILES][16];
"""
    with open(OUT_H, 'w', encoding='utf-8') as f:
        f.write(h_src)
    print(f"Wrote {OUT_H}")

    # --- Write .c ---
    lines = []
    lines.append('/* pokemon_sprites.c — AUTO-GENERATED by extract_sprites.py. DO NOT EDIT. */')
    lines.append('#include "pokemon_sprites.h"')
    lines.append('')

    # Width/height arrays
    w_vals = ','.join(str(front_w[i]) for i in range(152))
    h_vals = ','.join(str(front_h[i]) for i in range(152))
    lines.append(f'const uint8_t gPokemonFrontSpriteW[152] = {{{w_vals}}};')
    lines.append(f'const uint8_t gPokemonFrontSpriteH[152] = {{{h_vals}}};')
    lines.append('')

    # Front sprites
    lines.append('const uint8_t gPokemonFrontSprite[152][POKEMON_FRONT_CANVAS_TILES][16] = {')
    for dex in range(152):
        if dex == 0:
            lines.append('    /* dex 0 unused */ {')
            for _ in range(49):
                lines.append('        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},')
            lines.append('    },')
        else:
            name = DEX_TO_FRONT[dex]
            lines.append(f'    /* dex {dex} {name} */ {{')
            lines.append(tiles_to_c(front_tiles[dex]))
            lines.append('    },')
    lines.append('};')
    lines.append('')

    # Back sprites
    lines.append('const uint8_t gPokemonBackSprite[152][POKEMON_BACK_TILES][16] = {')
    for dex in range(152):
        if dex == 0:
            lines.append('    /* dex 0 unused */ {')
            for _ in range(16):
                lines.append('        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},')
            lines.append('    },')
        else:
            name = DEX_TO_BACK[dex]
            lines.append(f'    /* dex {dex} {name} */ {{')
            lines.append(tiles_to_c(back_tiles[dex]))
            lines.append('    },')
    lines.append('};')

    with open(OUT_C, 'w') as f:
        f.write('\n'.join(lines) + '\n')
    print(f"Wrote {OUT_C}")


if __name__ == '__main__':
    main()
