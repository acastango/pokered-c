/* overworld.c — Map loading and tile view expansion
 *
 * Block expansion (mirrors LoadCurrentMapView in engine/overworld.asm):
 *   Each map block = 4x4 tiles.  The blockset lookup is:
 *     tile_id = blocks[block_id * 16 + local_y * 4 + local_x]
 *
 * Camera: player is centered at screen tile (8, 9).
 *   view origin = (wXCoord - 8, wYCoord - 9)
 *   Matches original pokered: player reference tile at lda_coord 8,9.
 *
 * Out-of-bounds block IDs use cur_map->border_block.
 */
#include "overworld.h"
#include "warp.h"
#include "../data/map_data.h"
#include "../data/tileset_data.h"
#include "anim.h"
#include "music.h"
#include "../data/connection_data.h"
#include "../data/event_data.h"
#include "../platform/hardware.h"
#include "../platform/display.h"
#include "../game/constants.h"

static const tileset_info_t *cur_tileset    = NULL;
static const map_info_t     *cur_map        = NULL;
static const map_connections_t *cur_conns   = NULL;  /* connections for current map */
static int cur_map_w = 0;  /* cur_map->width  * 4, in tile units */
static int cur_map_h = 0;  /* cur_map->height * 4, in tile units */

int gCamX = 0;
int gCamY = 0;

/* Scroll tile buffer: 22x20 (one extra tile on each edge for sub-pixel panning) */
uint8_t gScrollTileMap[SCROLL_MAP_W * SCROLL_MAP_H];

/* ---- Internal helpers ------------------------------------ */

static uint8_t get_block_id(int bx, int by) {
    if (!cur_map || !cur_map->blocks) return 0;
    if (bx < 0 || by < 0 || bx >= cur_map->width || by >= cur_map->height) {
        if (wCurMap < NUM_MAPS) return gMapEvents[wCurMap].border_block;
        return 0;
    }
    return cur_map->blocks[by * cur_map->width + bx];
}

static int clamp_cam(int player_tile, int half, int map_tiles, int screen_tiles) {
    (void)map_tiles; (void)screen_tiles;
    /* Player stays centred at screen tile `half` regardless of map size.
     *
     * The original GB game uses unsigned SCX/SCY with wrap-around: "negative"
     * camera values are simply (256 - |offset|) and the BG tilemap is filled
     * with border tiles in the wrap region.  Both outdoor and indoor maps use
     * this model.  For outdoor maps, connected_tile() fills OOB positions with
     * neighbour-map content; for indoor maps, Map_GetTile returns border_block
     * for negative coords — matching the original's border-tile fill. */
    return player_tile - half;
}

void Map_UpdateCamera(void) {
    if (!cur_map) return;
    int map_w = cur_map->width  * 4;
    int map_h = cur_map->height * 4;
    gCamX = clamp_cam((int)wXCoord, 8,  map_w, SCREEN_WIDTH);
    gCamY = clamp_cam((int)wYCoord, 9, map_h, SCREEN_HEIGHT);
}

/* ---- Public API ------------------------------------------ */

void Map_Load(uint8_t map_id) {
    if (map_id >= NUM_MAPS) return;
    Warp_Reset();   /* clear post-warp cooldown on every map load */
    const map_info_t *m = &gMapTable[map_id];

    cur_map     = m;
    cur_tileset = &gTilesets[m->tileset_id];
    cur_map_w   = m->width  * 4;
    cur_map_h   = m->height * 4;
    cur_conns   = (map_id < NUM_MAP_CONNECTIONS) ? &gMapConnections[map_id] : NULL;

    wCurMap        = map_id;
    wCurMapWidth   = m->width;
    wCurMapHeight  = m->height;
    wCurMapTileset = m->tileset_id;
    wGrassTile     = cur_tileset->grass_tile;  /* 0xFF = no grass for this tileset */

    if (m->blocks)
        Display_LoadTileset(cur_tileset->gfx, cur_tileset->gfx_tiles);

    Anim_SetTileset(cur_tileset->anim_type);
    Music_Play(Music_GetMapID(map_id));
}

/* Read one tile directly from a connected map's block data.
 * Used for out-of-bounds rendering so adjacent outdoor maps flow seamlessly.
 * No recursion: if conn_tx/conn_ty are also out of bounds, returns 0. */
static uint8_t connected_tile(const map_conn_t *conn, int ctx, int cty) {
    uint8_t dest = conn->dest_map;
    if (dest == 0xFF || dest >= NUM_MAPS) return 0;
    const map_info_t     *cm = &gMapTable[dest];
    const tileset_info_t *ct = &gTilesets[cm->tileset_id];
    int cw = cm->width * 4,  ch = cm->height * 4;
    if (ctx < 0 || cty < 0 || ctx >= cw || cty >= ch) return 0;
    int bx = ctx >> 2, by = cty >> 2;
    int lx = ctx & 3,  ly = cty & 3;
    uint8_t bid = cm->blocks[by * cm->width + bx];
    return ct->blocks[bid * 16 + ly * 4 + lx];
}

uint8_t Map_GetTile(int tx, int ty) {
    if (!cur_map || !cur_tileset) return 0;

    /* Out-of-bounds: render tiles from the connected outdoor map so that
     * the scroll buffer shows neighbor content before/during the transition.
     * Mirrors the connection strip pre-load in the original (LoadNorth/
     * SouthConnectionsTileMap, LoadEastWestConnectionsTileMap).
     * Only applies to outdoor maps (tileset 0) that have a connection. */
    if (cur_conns && wCurMapTileset == 0) {
        if (ty < 0 && cur_conns->north.dest_map != 0xFF)
            return connected_tile(&cur_conns->north,
                                  tx + cur_conns->north.adjust,
                                  cur_conns->north.player_coord + ty + 1);
        if (ty >= cur_map_h && cur_conns->south.dest_map != 0xFF)
            /* south player_coord=1 (ODD half-block origin) but tile 0 is
             * physically adjacent to the south boundary — subtract 1. */
            return connected_tile(&cur_conns->south,
                                  tx + cur_conns->south.adjust,
                                  cur_conns->south.player_coord - 1 + (ty - cur_map_h));
        if (tx < 0 && cur_conns->west.dest_map != 0xFF)
            /* west player_coord=dw*4-2 (EVEN) but tile dw*4-1 is physically
             * adjacent to the west boundary — use +2 instead of +1. */
            return connected_tile(&cur_conns->west,
                                  cur_conns->west.player_coord + tx + 2,
                                  ty + cur_conns->west.adjust);
        if (tx >= cur_map_w && cur_conns->east.dest_map != 0xFF)
            return connected_tile(&cur_conns->east,
                                  cur_conns->east.player_coord + (tx - cur_map_w),
                                  ty + cur_conns->east.adjust);
    }

    /* Default out-of-bounds: border block — expand block ID → tile ID like in-bounds. */
    if (tx < 0 || ty < 0 || tx >= cur_map_w || ty >= cur_map_h) {
        if (wCurMap < NUM_MAPS) {
            uint8_t bid = gMapEvents[wCurMap].border_block;
            return cur_tileset->blocks[bid * 16 + (ty & 3) * 4 + (tx & 3)];
        }
        return 0;
    }

    int bx = tx >> 2,  by = ty >> 2;
    int lx = tx & 3,   ly = ty & 3;
    uint8_t bid = get_block_id(bx, by);
    return cur_tileset->blocks[bid * 16 + ly * 4 + lx];
}

void Map_BuildView(void) {
    if (!cur_map || !cur_tileset) return;

    /* Camera: player tile (wXCoord, wYCoord) appears at screen tile (8, 9) */
    int ox = (int)wXCoord - 8;
    int oy = (int)wYCoord - 9;

    for (int sy = 0; sy < SCREEN_HEIGHT; sy++)
        for (int sx = 0; sx < SCREEN_WIDTH; sx++)
            wTileMap[sy * SCREEN_WIDTH + sx] = Map_GetTile(ox + sx, oy + sy);
}

/* Connection transition state.
 *
 * When the player crosses a map boundary, the scroll buffer must continue
 * to be built from the OLD map for the full walk animation (WALK_FRAMES
 * frames).  This mirrors the original's circular VRAM buffer: SCX/SCY kept
 * scrolling while the connection strip (already in VRAM) provided seamless
 * tile content across the boundary.
 *
 * gScrollViewReady: set for the crossing frame itself (frame 0 of the walk).
 *   Map_BuildScrollView skips that one rebuild because Map_PreBuildScrollStep
 *   already filled the buffer at the correct stepped-camera position.
 *
 * gConnTransRemaining: countdown for the remaining WALK_FRAMES-1 animated
 *   frames.  Map_BuildScrollView rebuilds from the saved old-map context at
 *   the same stepped camera (conn_cam_x/y) so the buffer content is identical
 *   to frame 0's pre-built buffer, and gScrollPxX/Y advance smoothly to 0. */
static int gScrollViewReady    = 0;
static int gConnTransRemaining = 0;

/* Saved old-map state for the connection transition rebuild. */
static const map_info_t       *conn_save_map      = NULL;
static const tileset_info_t   *conn_save_tileset  = NULL;
static const map_connections_t *conn_save_conns   = NULL;
static int   conn_save_map_w = 0, conn_save_map_h = 0;
static uint8_t conn_save_tileset_id = 0;
static int   conn_cam_x = 0, conn_cam_y = 0;

/* Number of animation frames per step — must match WALK_FRAMES in player.c. */
#define CONN_WALK_FRAMES 8

void Map_BuildScrollView(void) {
    if (!cur_map || !cur_tileset) return;

    /* Frame 0 of connection crossing: buffer already filled by
     * Map_PreBuildScrollStep; just update camera and skip the rebuild. */
    if (gScrollViewReady) {
        gScrollViewReady = 0;
        Map_UpdateCamera();
        return;
    }

    /* Frames 1..(WALK_FRAMES-1): rebuild from old-map context at the same
     * stepped camera used for frame 0.  The old map's connected_tile()
     * provides seamless neighbor-map content in the out-of-bounds region,
     * and its in-bounds region shows the tiles that were visible before
     * crossing — eliminating all per-frame visual chop during the walk. */
    if (gConnTransRemaining > 0) {
        gConnTransRemaining--;

        /* Temporarily install old-map state so Map_GetTile uses the right
         * block/tile data and connection pointers. */
        const map_info_t       *sv_map      = cur_map;
        const tileset_info_t   *sv_tileset  = cur_tileset;
        const map_connections_t *sv_conns   = cur_conns;
        int    sv_map_w = cur_map_w, sv_map_h = cur_map_h;
        uint8_t sv_ts   = wCurMapTileset;

        cur_map         = conn_save_map;
        cur_tileset     = conn_save_tileset;
        cur_conns       = conn_save_conns;
        cur_map_w       = conn_save_map_w;
        cur_map_h       = conn_save_map_h;
        wCurMapTileset  = conn_save_tileset_id;

        int ox = conn_cam_x - 2;
        int oy = conn_cam_y - 2;
        for (int sy = 0; sy < SCROLL_MAP_H; sy++)
            for (int sx = 0; sx < SCROLL_MAP_W; sx++)
                gScrollTileMap[sy * SCROLL_MAP_W + sx] = Map_GetTile(ox + sx, oy + sy);

        /* Restore new-map state before updating camera for OAM positioning. */
        cur_map        = sv_map;
        cur_tileset    = sv_tileset;
        cur_conns      = sv_conns;
        cur_map_w      = sv_map_w;
        cur_map_h      = sv_map_h;
        wCurMapTileset = sv_ts;

        Map_UpdateCamera();
        return;
    }

    /* Normal case: rebuild from current map. */
    Map_UpdateCamera();

    /* Two extra tiles on each edge for sub-pixel panning.
     * Buffer origin: (gCamX-2, gCamY-2).
     * At scroll_px=0, buffer tile (2,2) renders at screen (0,0) — normal view.
     * Two tiles are needed because each step moves the camera by 2 tiles (16px),
     * so gScrollPxX starts at 16 and the buffer must cover the old position. */
    int ox = gCamX - 2;
    int oy = gCamY - 2;

    for (int sy = 0; sy < SCROLL_MAP_H; sy++)
        for (int sx = 0; sx < SCROLL_MAP_W; sx++)
            gScrollTileMap[sy * SCROLL_MAP_W + sx] = Map_GetTile(ox + sx, oy + sy);
}

/* Fill gScrollTileMap from the CURRENT (old) map at the camera position that
 * results from stepping (dx, dy), and save old-map context so
 * Map_BuildScrollView can continue using it for the remaining walk frames.
 *
 * Called just before Connection_Check switches cur_map to the new map. */
void Map_PreBuildScrollStep(int dx, int dy) {
    if (!cur_map || !cur_tileset) return;
    /* Only activate for real connections. */
    if (!cur_conns || wCurMapTileset != 0) return;
    if (dy < 0 && cur_conns->north.dest_map == 0xFF) return;
    if (dy > 0 && cur_conns->south.dest_map == 0xFF) return;
    if (dx < 0 && cur_conns->west.dest_map  == 0xFF) return;
    if (dx > 0 && cur_conns->east.dest_map  == 0xFF) return;

    /* Save old-map context for the animated walk frames. */
    conn_save_map        = cur_map;
    conn_save_tileset    = cur_tileset;
    conn_save_conns      = cur_conns;
    conn_save_map_w      = cur_map_w;
    conn_save_map_h      = cur_map_h;
    conn_save_tileset_id = wCurMapTileset;

    int new_px = (int)wXCoord + dx;
    int new_py = (int)wYCoord + dy;
    conn_cam_x = clamp_cam(new_px, 8,  cur_map_w, SCREEN_WIDTH);
    conn_cam_y = clamp_cam(new_py, 9, cur_map_h, SCREEN_HEIGHT);

    /* Build frame 0's buffer immediately (before Connection_Check switches map). */
    int ox = conn_cam_x - 2;
    int oy = conn_cam_y - 2;
    for (int sy = 0; sy < SCROLL_MAP_H; sy++)
        for (int sx = 0; sx < SCROLL_MAP_W; sx++)
            gScrollTileMap[sy * SCROLL_MAP_W + sx] = Map_GetTile(ox + sx, oy + sy);

    gScrollViewReady    = 1;
    gConnTransRemaining = 0;  /* single-frame pre-build only; R1's south connected_tile fills PT */
}

/* Connection_Check — mirrors CheckMapConnections in home/overworld.asm.
 * dx/dy: the step direction that took the player off the map edge (-1, 0, +1).
 * Coords are in tile units (4 tiles per block); wXCoord/wYCoord are uint16_t
 * so they can represent large routes (e.g. Route 23 = 72 blocks = 288 tiles).
 */
int Connection_Check(int dx, int dy) {
    if (wCurMap >= NUM_MAP_CONNECTIONS) return 0;
    const map_connections_t *c = &gMapConnections[wCurMap];
    const map_conn_t *conn = NULL;
    int is_north_south = 0;

    if      (dy < 0 && c->north.dest_map != 0xFF) { conn = &c->north; is_north_south = 1; }
    else if (dy > 0 && c->south.dest_map != 0xFF) { conn = &c->south; is_north_south = 1; }
    else if (dx < 0 && c->west.dest_map  != 0xFF) { conn = &c->west;  is_north_south = 0; }
    else if (dx > 0 && c->east.dest_map  != 0xFF) { conn = &c->east;  is_north_south = 0; }

    if (!conn) return 0;

    if (is_north_south) {
        wXCoord = (uint16_t)((int)wXCoord + conn->adjust);
        wYCoord = (uint16_t)conn->player_coord;
    } else {
        wYCoord = (uint16_t)((int)wYCoord + conn->adjust);
        wXCoord = (uint16_t)conn->player_coord;
    }
    Map_Load(conn->dest_map);
    return 1;
}

int Tile_IsPassable(uint8_t tile_id) {
    if (!cur_tileset) return 1;
    const uint8_t *p = cur_tileset->coll_tiles;
    while (*p != 0xFF) {
        if (*p++ == tile_id) return 1;
    }
    return 0;
}
