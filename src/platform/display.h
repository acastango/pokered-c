#pragma once
#include <stdint.h>

#define DISPLAY_SCALE   3   /* 160x144 → 480x432 */

int  Display_Init(void);
void Display_Render(void);  /* wTileMap + wShadowOAM → screen (no scroll) */
/* Display_RenderScrolled: render tile_map (stride columns wide) with sub-tile
 * pixel offset (px, py).  Replaces Display_Render for the overworld walk system.
 * tile_map must be (SCREEN_WIDTH+2) x (SCREEN_HEIGHT+2) = 22x20 tiles.
 * At px=0,py=0 the result is identical to a normal 20x18 view (center 20x18 tiles). */
void Display_RenderScrolled(int px, int py, const uint8_t *tile_map, int stride);
void Display_LoadTileset(const uint8_t *gfx, int num_tiles);
void Display_LoadTile(uint8_t tile_id, const uint8_t *gfx);       /* load BG tile */
void Display_GetTile(uint8_t tile_id, uint8_t out[16]);           /* read BG tile GFX */
void Display_LoadSpriteTile(uint8_t tile_id, const uint8_t *gfx); /* load sprite (OBJ) tile */
void Display_SetPalette(uint8_t bgp, uint8_t obp0, uint8_t obp1);
void Display_SetBGP(uint8_t bgp);  /* change background palette only — sprites unaffected */
/* Screen-shake offset: shifts the rendered framebuffer by (ox, oy) pixels.
 * Matches the GB window-register offset used by PlayApplyingAttackAnimation.
 * Call Display_SetShakeOffset(0,0) to clear. */
void Display_SetShakeOffset(int ox, int oy);
void Display_Quit(void);
