#pragma once
/* font_data.h -- Generated from pokered-master gfx/font/ PNGs */
#include <stdint.h>

/* Tile slot indices (must match npc.c / player.c / text.c) */
#define FONT_TILE_BASE    128   /* char $80 = tile 128 */
#define BOX_TILE_BASE     120   /* ┌─┐│└┘ at slots 120-125 */
#define BLANK_TILE_SLOT   126   /* space / blank tile */
/* HUD tiles: chars $62–$78 (HP bar, level, HUD border).
 * Loaded from font_battle_extra.2bpp + battle_hud_*.1bpp at startup. */
#define HUD_TILE_BASE     2     /* slots 2–24  (23 tiles) */

/* char_to_tile: pokered char code → tile cache slot */
static inline int Font_CharToTile(unsigned char c) {
    if (c >= 0x80) return FONT_TILE_BASE + (c - 0x80);
    if (c == 0x7F) return BLANK_TILE_SLOT;  /* space */
    if (c >= 0x79 && c <= 0x7E) return BOX_TILE_BASE + (c - 0x79);
    /* HP bar + HUD border tiles (font_battle_extra / battle_hud) */
    if (c >= 0x62 && c <= 0x78) return HUD_TILE_BASE + (c - 0x62);
    return BLANK_TILE_SLOT;  /* control chars / unknown */
}

/* 128 main font tiles (char $80–$FF), 16 bytes each */
extern const uint8_t gFontTiles[128][16];

/* Box-drawing + blank tiles (7 tiles: ┌─┐│└┘ + space) */
extern const uint8_t gBoxTiles[7][16];

/* HUD tiles: chars $62–$78 (23 tiles).
 * Final merged data: font_battle_extra for $62-$72, battle_hud_1 for $6D-$6F,
 * battle_hud_2 for $73-$75, battle_hud_3 for $76-$78. */
extern const uint8_t gHudTiles[23][16];

/* Load all font tiles into the display tile cache.  Call once at startup. */
void Font_Load(void);

/* Load HP bar + HUD border tiles ($62–$78) into display tile cache.
 * Call once when entering battle (matches LoadHudAndHpBarAndStatusTilePatterns). */
void Font_LoadHudTiles(void);
