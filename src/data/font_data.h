#pragma once
/* font_data.h -- Generated from pokered-master gfx/font/ PNGs */
#include <stdint.h>

/* Tile slot indices (must match npc.c / player.c / text.c) */
#define FONT_TILE_BASE    128   /* char $80 = tile 128 */
#define BOX_TILE_BASE     120   /* ┌─┐│└┘ at slots 120-125 */
#define BLANK_TILE_SLOT   126   /* space / blank tile */

/* char_to_tile: pokered char code → tile cache slot */
static inline int Font_CharToTile(unsigned char c) {
    if (c >= 0x80) return FONT_TILE_BASE + (c - 0x80);
    if (c == 0x7F) return BLANK_TILE_SLOT;  /* space */
    if (c >= 0x79 && c <= 0x7E) return BOX_TILE_BASE + (c - 0x79);
    return BLANK_TILE_SLOT;  /* control chars / unknown */
}

/* 128 main font tiles (char $80–$FF), 16 bytes each */
extern const uint8_t gFontTiles[128][16];

/* Box-drawing + blank tiles (7 tiles: ┌─┐│└┘ + space) */
extern const uint8_t gBoxTiles[7][16];

/* Load all font tiles into the display tile cache.  Call once at startup. */
void Font_Load(void);
