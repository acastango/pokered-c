#pragma once
#include <stdint.h>
#include "../game/constants.h"

/* overworld.h — Map loading and tile-view expansion
 *
 * Map_Load()           loads the map's tileset into the display and sets WRAM state.
 * Map_BuildView()      fills wTileMap[20x18] from the current player position.
 * Map_BuildScrollView() fills gScrollTileMap[22x20] with one extra border tile on
 *                       each side, for sub-tile smooth scrolling via Display_RenderScrolled.
 * Map_GetTile()        returns the tile ID at map tile coordinates (tx, ty).
 * Tile_IsPassable()    returns 1 if tile_id is in the current tileset's pass list.
 */
void    Map_Load(uint8_t map_id);
/* Re-upload the current tileset GFX to tile_gfx[].  Call after battle to
 * restore slots 0-N overwritten by Font_LoadHudTiles during battle. */
void    Map_ReloadGfx(void);
void    Map_BuildView(void);
void    Map_BuildScrollView(void);
void    Map_UpdateCamera(void);
uint8_t Map_GetTile(int tx, int ty);
/* Override a single map block at block coords (bx, by).  Survives until the
 * next Map_Load.  Used by gym door scripts (ReplaceTileBlock equivalent). */
void    Map_SetBlock(int bx, int by, uint8_t block_id);
/* Convenience: look up tile at game coordinates (ASM wXCoord/wYCoord units).
 * Converts to tile coords internally: tile_x = gx*2, tile_y = gy*2+1. */
uint8_t Map_GetGameTile(int gx, int gy);
/* Return the block ID (tileset metatile index) at game coordinates.
 * This is the value stored in cur_map->blocks[], after any runtime overrides. */
uint8_t Map_GetBlockAt(int gx, int gy);
/* Set (override) the block ID at game coordinates.  Same as Map_SetBlock but
 * accepts game coords so callers don't need to do the coordinate conversion. */
void    Map_SetBlockAt(int gx, int gy, uint8_t block_id);
int     Tile_IsPassable(uint8_t tile_id);
/* Query block ID at (bx, by) including runtime Map_SetBlock overrides.
 * Used as the Display_SetBlockIDQueryFn callback for the debug overlay. */
int     Map_GetBlockIdRaw(int bx, int by);
int     Tile_IsPairBlocked(uint8_t a, uint8_t b);
int     Connection_Check(int dx, int dy);  /* dx/dy: step direction (-1,0,+1) */
void    Map_PreBuildScrollStep(int dx, int dy); /* pre-fill scroll buffer from old map before a connection transition */
void    Map_ResetScrollState(void);  /* clear scroll-transition flags; call on battle return to force full rebuild */

/* 24x22 scroll tile buffer (SCREEN_WIDTH+4 wide, SCREEN_HEIGHT+4 tall).
 * Origin = (gCamX-2, gCamY-2) — two extra tiles on each edge.
 * Two extra tiles are required because each step moves the camera 2 tiles (16px),
 * so at step-start the buffer must reach back to the old camera position. */
#define SCROLL_MAP_W  (SCREEN_WIDTH  + 4)   /* 24 */
#define SCROLL_MAP_H  (SCREEN_HEIGHT + 4)   /* 22 */
extern uint8_t gScrollTileMap[SCROLL_MAP_W * SCROLL_MAP_H];

/* Camera tile origin (top-left tile visible on screen).
 * gCamX = wXCoord*2 - 8,  gCamY = wYCoord*2 + 1 - 9.
 * Coordinates are in 8px tile units for rendering. */
extern int gCamX;
extern int gCamY;
