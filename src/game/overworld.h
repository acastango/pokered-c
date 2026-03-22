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
void    Map_BuildView(void);
void    Map_BuildScrollView(void);
void    Map_UpdateCamera(void);
uint8_t Map_GetTile(int tx, int ty);
int     Tile_IsPassable(uint8_t tile_id);
int     Connection_Check(int dx, int dy);  /* dx/dy: step direction (-1,0,+1) */
void    Map_PreBuildScrollStep(int dx, int dy); /* pre-fill scroll buffer from old map before a connection transition */

/* 24x22 scroll tile buffer (SCREEN_WIDTH+4 wide, SCREEN_HEIGHT+4 tall).
 * Origin = (gCamX-2, gCamY-2) — two extra tiles on each edge.
 * Two extra tiles are required because each step moves the camera 2 tiles (16px),
 * so at step-start the buffer must reach back to the old camera position. */
#define SCROLL_MAP_W  (SCREEN_WIDTH  + 4)   /* 24 */
#define SCROLL_MAP_H  (SCREEN_HEIGHT + 4)   /* 22 */
extern uint8_t gScrollTileMap[SCROLL_MAP_W * SCROLL_MAP_H];

/* Clamped camera tile origin (top-left tile visible on screen).
 * gCamX = clamp(wXCoord-9, 0, map_w*4-SCREEN_WIDTH), negative for small maps (centered). */
extern int gCamX;
extern int gCamY;
