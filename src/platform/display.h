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
void Display_SetBGP(uint8_t bgp);   /* change background palette only — sprites unaffected */
void Display_SetOBP1(uint8_t obp1); /* change OBP1 sprite palette only (used by SS Anne smoke: 0x00 = all white) */
/* Apply gMapPalOffset to the palette (mirrors LoadGBPal in home/fade.asm).
 * Call after any map load or warp. offset=0→normal, offset=6→dark cave. */
void Display_LoadMapPalette(void);
/* Screen-shake offset: shifts the rendered framebuffer by (ox, oy) pixels.
 * Matches the GB window-register offset used by PlayApplyingAttackAnimation.
 * Call Display_SetShakeOffset(0,0) to clear. */
void Display_SetShakeOffset(int ox, int oy);
/* Per-band horizontal pixel offset: shift a contiguous range of screen rows
 * by px pixels in X during Display_RenderScrolled.  Mirrors the GB raster
 * trick (mid-frame SCX change) used by VermilionDock_SyncScrollWithLY to make
 * rows 10-15 scroll independently of the rest of the screen during SS Anne
 * departure.  Negative px shifts the band LEFT (ship departs left).
 * Call with row_start=-1 to clear. */
void Display_SetBandXPx(int row_start, int num_rows, int px);
/* Save current framebuffer as a BMP file.  Returns 0 on success, -1 on error. */
int  Display_SaveScreenshot(const char *path);

/* Per-tile debug overlay: blends a solid RGBA color over screen tiles.
 * Enable/disable with Display_SetOverlayEnabled.  Set per-tile colors with
 * Display_SetOverlayTile (tx in [0,SCREEN_WIDTH), ty in [0,SCREEN_HEIGHT)).
 * rgba = 0xRRGGBBAA; AA = blend alpha (0=transparent, 255=opaque); 0 = no tint. */
void Display_SetOverlayEnabled(int on);
void Display_ClearOverlay(void);
void Display_SetOverlayTile(int tx, int ty, uint32_t rgba);

/* Block-ID debug overlay: draws 2-char hex block IDs over each map block.
 * Toggle with Display_SetBlockIDOverlay(1/0).
 * Register a query callback once (e.g. Map_GetBlockIdRaw from overworld.c).
 * Update camera each frame with Display_SetBlockIDCam so positions track correctly. */
void Display_SetBlockIDOverlay(int enabled);
int  Display_GetBlockIDOverlay(void);
void Display_SetBlockIDQueryFn(int (*fn)(int bx, int by));
void Display_SetBlockIDCam(int cam_tx, int cam_ty);

void Display_Quit(void);
