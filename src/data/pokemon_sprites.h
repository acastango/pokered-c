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
