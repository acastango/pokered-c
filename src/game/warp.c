/* warp.c — Warp trigger and map transition system.
 *
 * Original reference: home/overworld.asm CheckWarpsCollision + EnterMap +
 * engine/overworld/tilesets.asm LoadTilesetHeader / LoadDestinationWarpPosition.
 *
 * Arrival position:
 *   The player is placed exactly at the destination warp tile (no +2 offset).
 *   The original pokered also places the player at the warp tile and then
 *   plays a door-step-out animation to move them off it.  We replicate the
 *   no-re-trigger effect via a one-step cooldown (gWarpCooldown) instead.
 *
 * wLastMap update rule (mirrors CheckIfInOutsideMap in home/overworld.asm):
 *   Only update wLastMap when the current map is an outdoor map.
 *   CheckIfInOutsideMap checks wCurMapTileset == OVERWORLD (0) or PLATEAU (23).
 *   Indoor maps (gates, buildings, forests, caves) must NOT overwrite wLastMap,
 *   so that LAST_MAP (0xFF) destination warps always return to the correct
 *   outdoor map rather than bouncing back through the last indoor map visited.
 *
 * Input suppression (mirrors IgnoreInputForHalfSecond / wJoyIgnore in EnterMap):
 *   After each successful warp, Player_IgnoreInputFrames(8) is called.  This
 *   suppresses joypad reads in Player_Update for 8 overworld ticks, preventing
 *   a held direction from immediately re-triggering the arrival warp tile.
 *   Warp_Check itself is NOT gated — the first deliberate step fires immediately.
 */
#include "warp.h"
#include "overworld.h"
#include "player.h"
#include "npc.h"
#include "../platform/hardware.h"
#include "../data/event_data.h"
#include "../data/map_data.h"
#include "../game/constants.h"
#include <stdio.h>

/* LAST_MAP sentinel (0xFF in the original — "return to previous outdoor map") */
#define LAST_MAP 0xFF

/* Outdoor tileset IDs — matches CheckIfInOutsideMap in home/overworld.asm.
 * OVERWORLD=0 covers all cities and routes; PLATEAU=23 covers Route 23 /
 * Indigo Plateau (these are the only two tilesets that set the Z flag). */
#define TILESET_OVERWORLD  0
#define TILESET_PLATEAU   23

/* Per-tileset door tile ID lists.
 * Mirrors DoorTileIDPointers in data/tilesets/door_tile_ids.asm.
 *
 * When the player arrives on one of these tiles after a warp, a one-step
 * south walk is automatically triggered (PlayerStepOutFromDoor).
 * Tilesets not in the original table get an empty list (no step-out). */
static const uint8_t kDoorTiles_Overworld[]   = {0x1B, 0x58, 0xFF};
static const uint8_t kDoorTiles_Mart[]        = {0x5E, 0xFF};
static const uint8_t kDoorTiles_Forest[]      = {0x3A, 0xFF};
static const uint8_t kDoorTiles_House[]       = {0x54, 0xFF};
static const uint8_t kDoorTiles_Museum[]      = {0x3B, 0xFF};  /* FOREST_GATE/MUSEUM/GATE */
static const uint8_t kDoorTiles_Ship[]        = {0x1E, 0xFF};
static const uint8_t kDoorTiles_Lobby[]       = {0x1C, 0x38, 0x1A, 0xFF};
static const uint8_t kDoorTiles_Mansion[]     = {0x1A, 0x1C, 0x53, 0xFF};
static const uint8_t kDoorTiles_Lab[]         = {0x34, 0xFF};
static const uint8_t kDoorTiles_Facility[]    = {0x43, 0x58, 0x1B, 0xFF};
static const uint8_t kDoorTiles_Plateau[]     = {0x3B, 0x1B, 0xFF};
static const uint8_t kDoorTiles_Empty[]       = {0xFF};

static const uint8_t * const kDoorTilesByTileset[NUM_TILESETS] = {
    /* 0  OVERWORLD   */ kDoorTiles_Overworld,
    /* 1  REDS_HOUSE_1*/ kDoorTiles_Empty,
    /* 2  MART        */ kDoorTiles_Mart,
    /* 3  FOREST      */ kDoorTiles_Forest,
    /* 4  REDS_HOUSE_2*/ kDoorTiles_Empty,
    /* 5  DOJO        */ kDoorTiles_Empty,
    /* 6  POKECENTER  */ kDoorTiles_Empty,
    /* 7  GYM         */ kDoorTiles_Empty,
    /* 8  HOUSE       */ kDoorTiles_House,
    /* 9  FOREST_GATE */ kDoorTiles_Museum,
    /* 10 MUSEUM      */ kDoorTiles_Museum,
    /* 11 UNDERGROUND */ kDoorTiles_Empty,
    /* 12 GATE        */ kDoorTiles_Museum,
    /* 13 SHIP        */ kDoorTiles_Ship,
    /* 14 SHIP_PORT   */ kDoorTiles_Empty,
    /* 15 CEMETERY    */ kDoorTiles_Empty,
    /* 16 INTERIOR    */ kDoorTiles_Empty,
    /* 17 CAVERN      */ kDoorTiles_Empty,
    /* 18 LOBBY       */ kDoorTiles_Lobby,
    /* 19 MANSION     */ kDoorTiles_Mansion,
    /* 20 LAB         */ kDoorTiles_Lab,
    /* 21 CLUB        */ kDoorTiles_Empty,
    /* 22 FACILITY    */ kDoorTiles_Facility,
    /* 23 PLATEAU     */ kDoorTiles_Plateau,
};

/* Returns 1 if the player is standing on a door tile for the current tileset.
 * Mirrors IsPlayerStandingOnDoorTile in engine/overworld/doors.asm. */
static int is_door_tile(uint8_t tile) {
    if (wCurMapTileset >= NUM_TILESETS) return 0;
    const uint8_t *p = kDoorTilesByTileset[wCurMapTileset];
    for (; *p != 0xFF; p++) {
        if (*p == tile) return 1;
    }
    return 0;
}

/* Per-tileset warp-trigger tile ID lists.
 * Mirrors WarpTileIDPointers in data/tilesets/warp_tile_ids.asm.
 *
 * Tiles in a tileset's list fire the warp from ANY approach direction
 * (e.g. staircase tile 0x1C, cave entrance 0x1A, overworld door 0x1B).
 *
 * Tiles NOT in the list (e.g. exit mat 0x14 in Red's House) require
 * IsPlayerFacingEdgeOfMap to pass — warp only fires when the player is
 * at the map boundary and walking directly through the exit. */
static const uint8_t kWarpTiles_Overworld[]   = {0x1B, 0x58, 0xFF};
static const uint8_t kWarpTiles_RedsHouse[]   = {0x1A, 0x1C, 0xFF};
static const uint8_t kWarpTiles_Mart[]        = {0x5E, 0xFF};
static const uint8_t kWarpTiles_Forest[]      = {0x5A, 0x5C, 0x3A, 0xFF};
static const uint8_t kWarpTiles_Dojo[]        = {0x4A, 0xFF};
static const uint8_t kWarpTiles_House[]       = {0x54, 0x5C, 0x32, 0xFF};
static const uint8_t kWarpTiles_ForestGate[]  = {0x3B, 0x1A, 0x1C, 0xFF};
static const uint8_t kWarpTiles_Underground[] = {0x13, 0xFF};
static const uint8_t kWarpTiles_Ship[]        = {0x37, 0x39, 0x1E, 0x4A, 0xFF};
static const uint8_t kWarpTiles_Cemetery[]    = {0x1B, 0x13, 0xFF};
static const uint8_t kWarpTiles_Interior[]    = {0x15, 0x55, 0x04, 0xFF};
static const uint8_t kWarpTiles_Cavern[]      = {0x18, 0x1A, 0x22, 0xFF};
static const uint8_t kWarpTiles_Lobby[]       = {0x1A, 0x1C, 0x38, 0xFF};
static const uint8_t kWarpTiles_Mansion[]     = {0x1A, 0x1C, 0x53, 0xFF};
static const uint8_t kWarpTiles_Lab[]         = {0x34, 0xFF};
static const uint8_t kWarpTiles_Facility[]    = {0x43, 0x58, 0x20, 0x1B, 0x13, 0xFF};
static const uint8_t kWarpTiles_Plateau[]     = {0x1B, 0x3B, 0xFF};
static const uint8_t kWarpTiles_Empty[]       = {0xFF};

static const uint8_t * const kWarpTilesByTileset[NUM_TILESETS] = {
    /* 0  OVERWORLD   */ kWarpTiles_Overworld,
    /* 1  REDS_HOUSE_1*/ kWarpTiles_RedsHouse,
    /* 2  MART        */ kWarpTiles_Mart,
    /* 3  FOREST      */ kWarpTiles_Forest,
    /* 4  REDS_HOUSE_2*/ kWarpTiles_RedsHouse,
    /* 5  DOJO        */ kWarpTiles_Dojo,
    /* 6  POKECENTER  */ kWarpTiles_Mart,
    /* 7  GYM         */ kWarpTiles_Dojo,
    /* 8  HOUSE       */ kWarpTiles_House,
    /* 9  FOREST_GATE */ kWarpTiles_ForestGate,
    /* 10 MUSEUM      */ kWarpTiles_ForestGate,
    /* 11 UNDERGROUND */ kWarpTiles_Underground,
    /* 12 GATE        */ kWarpTiles_ForestGate,
    /* 13 SHIP        */ kWarpTiles_Ship,
    /* 14 SHIP_PORT   */ kWarpTiles_Empty,
    /* 15 CEMETERY    */ kWarpTiles_Cemetery,
    /* 16 INTERIOR    */ kWarpTiles_Interior,
    /* 17 CAVERN      */ kWarpTiles_Cavern,
    /* 18 LOBBY       */ kWarpTiles_Lobby,
    /* 19 MANSION     */ kWarpTiles_Mansion,
    /* 20 LAB         */ kWarpTiles_Lab,
    /* 21 CLUB        */ kWarpTiles_Empty,
    /* 22 FACILITY    */ kWarpTiles_Facility,
    /* 23 PLATEAU     */ kWarpTiles_Plateau,
};

/* Returns 1 if the tile at the warp position fires from any direction.
 * Returns 0 if IsPlayerFacingEdgeOfMap gating is required. */
static int is_warp_trigger_tile(uint8_t tile) {
    if (wCurMapTileset >= NUM_TILESETS) return 1;
    const uint8_t *p = kWarpTilesByTileset[wCurMapTileset];
    for (; *p != 0xFF; p++) {
        if (*p == tile) return 1;
    }
    return 0;
}

/* Mirrors IsPlayerFacingEdgeOfMap (engine/overworld/player_state.asm:86).
 * Original check (block coords): facing==DOWN → wCurMapHeight*2-1 == wYCoord.
 * Our doubled coords: wYCoord_orig = (wYCoord-1)/2, so the boundary maps to
 *   DOWN  → wYCoord == wCurMapHeight*4 - 1
 *   UP    → wYCoord == 1
 *   LEFT  → wXCoord == 0
 *   RIGHT → wXCoord == wCurMapWidth*4 - 2   (last even x position) */
static int is_facing_edge_of_map(void) {
    switch (gPlayerFacing & 3) {
        case 0: return (int)wYCoord == (int)wCurMapHeight * 4 - 1;
        case 1: return (int)wYCoord == 1;
        case 2: return (int)wXCoord == 0;
        case 3: return (int)wXCoord == (int)wCurMapWidth  * 4 - 2;
    }
    return 0;
}

/* Set to 1 by Warp_Check when a warp fires; consumed by Warp_JustHappened(). */
static int gWarpJustHappened = 0;

/* Set to 1 by Warp_Execute when the player arrives on a door tile; consumed by
 * Warp_HasDoorStep().  Triggers the step-out walk (PlayerStepOutFromDoor). */
static int gWarpDoorStep = 0;

/* Pending warp destination — set by Warp_Check, consumed by Warp_Execute(). */
static int     gWarpPending     = 0;
static uint8_t gPendingDestMap  = 0;
static uint8_t gPendingDestIdx  = 0;

int Warp_JustHappened(void) {
    int v = gWarpJustHappened;
    gWarpJustHappened = 0;
    return v;
}

int Warp_HasDoorStep(void) {
    int v = gWarpDoorStep;
    gWarpDoorStep = 0;
    return v;
}

void Warp_Reset(void) {
    Player_IgnoreInputFrames(0);
}

static int is_outdoor_map(void) {
    return wCurMapTileset == TILESET_OVERWORLD ||
           wCurMapTileset == TILESET_PLATEAU;
}

int Warp_Check(void) {
    if (wCurMap >= NUM_MAPS) return 0;

    const map_events_t *ev = &gMapEvents[wCurMap];
    if (!ev->warps || ev->num_warps == 0) return 0;

    printf("[warp_check] map=%d pos=(%d,%d) facing=%d edge=%d warps=%d\n",
           wCurMap, wXCoord, wYCoord, gPlayerFacing,
           (int)((wCurMapHeight > 0) ? (wYCoord == (uint16_t)(wCurMapHeight * 4 - 1)) : 0),
           ev->num_warps);

    for (int i = 0; i < ev->num_warps; i++) {
        const map_warp_t *w = &ev->warps[i];
        printf("[warp_check]   warp[%d]=(%d,%d) match=%d\n",
               i, w->x, w->y, (w->x == wXCoord && w->y == wYCoord));
        if (w->x != wXCoord || w->y != wYCoord) continue;

        /* Save source map for the debug log before anything is updated. */
        uint8_t from_map = wCurMap;

        /* Directional gate (mirrors IsPlayerStandingOnDoorTileOrWarpTile +
         * ExtraWarpCheck/IsPlayerFacingEdgeOfMap in home/overworld.asm).
         *
         * If the tile at the warp position is in the tileset's warp-trigger
         * list (e.g. staircase 0x1C, cave entrance 0x1A) → fire from any
         * direction.  Otherwise (e.g. exit mat 0x14 in Red's House) → only
         * fire when the player is at the map boundary facing outward, i.e.
         * they walked directly through the exit, not sideways across the mat. */
        uint8_t warp_tile = Map_GetTile((int)w->x, (int)w->y);
        if (!is_warp_trigger_tile(warp_tile) && !is_facing_edge_of_map()) {
            continue;
        }

        /* Warp match — determine destination map */
        uint8_t dest_map = w->dest_map;
        if (dest_map == LAST_MAP) {
            dest_map = wLastMap;   /* return to previous outdoor map */
        }
        if (dest_map >= NUM_MAPS) {
            printf("[warp] dest_map 0x%02X out of range\n", dest_map);
            return 0;
        }

        uint8_t dest_idx = w->dest_warp_idx;

        /* Only update wLastMap when leaving an outdoor map.
         * Do this NOW (before Map_Load changes wCurMap/wCurMapTileset).
         * Indoor maps must not overwrite it (mirrors CheckIfInOutsideMap). */
        if (is_outdoor_map()) {
            wLastMap = wCurMap;
        }

        /* Store destination — actual map load is deferred to Warp_Execute()
         * (called at peak black) so the fade-out still uses the old tileset. */
        gPendingDestMap  = dest_map;
        gPendingDestIdx  = dest_idx;
        gWarpPending      = 1;
        gWarpJustHappened = 1;
        printf("[warp] map %d -> map %d (warp %d) queued\n",
               from_map, dest_map, dest_idx);
        return 1;
    }
    return 0;
}

void Warp_Execute(void) {
    if (!gWarpPending) return;
    gWarpPending = 0;

    uint8_t dest_map = gPendingDestMap;
    uint8_t dest_idx = gPendingDestIdx;

    Map_Load(dest_map);   /* loads new tileset GFX into tile_gfx[] */
    /* Suppress joypad for 8 overworld ticks (~16 VBlanks) after arrival.
     * Mirrors IgnoreInputForHalfSecond / wJoyIgnore set in EnterMap.
     * Prevents a held direction from immediately re-triggering the warp
     * tile the player just landed on.  Does NOT block Warp_Check itself,
     * so the first deliberate step fires the exit warp immediately. */
    Player_IgnoreInputFrames(8);

    const map_events_t *dev = &gMapEvents[dest_map];
    if (dev->warps && dest_idx < dev->num_warps) {
        const map_warp_t *dw = &dev->warps[dest_idx];
        int max_x = (int)wCurMapWidth  * 4 - 1;
        int max_y = (int)wCurMapHeight * 4 - 1;
        int nx = (int)dw->x;
        int ny = (int)dw->y;
        if (nx > max_x) nx = max_x;
        if (ny > max_y) ny = max_y;
        Player_SetPos((uint16_t)nx, (uint16_t)ny);
    } else {
        Player_SetPos(
            (uint16_t)((wCurMapWidth  * 4) / 2),
            (uint16_t)((wCurMapHeight * 4) / 2)
        );
    }

    /* Check for door step-out: if the arrival tile is a door tile for this
     * tileset, schedule an automatic step south (PlayerStepOutFromDoor).
     * Mirrors the BIT_STANDING_ON_DOOR flag set in WarpFound2 → EnterMap. */
    gWarpDoorStep = is_door_tile(Map_GetTile((int)wXCoord, (int)wYCoord));

    NPC_Load();
    printf("[warp] executed -> map %d @ (%d,%d)\n",
           dest_map, wXCoord, wYCoord);
}
