/* npc.c — NPC overworld sprite loading, movement, and OAM management.
 *
 * Sprite GFX tile layout (12 tiles per sprite, 2×6 grid, from facings.asm):
 *   Tiles  0-3:  facing down  standing (StandingDown)
 *   Tiles  4-7:  facing up   standing (StandingUp)
 *   Tiles  8-11: facing left standing (StandingLeft; right reuses + OAM_XFLIP)
 *   Tiles 12-15: facing down  walking (WalkingDown)
 *   Tiles 16-19: facing up   walking (WalkingUp)
 *   Tiles 20-23: facing left walking (WalkingLeft; right reuses + OAM_XFLIP)
 *
 * OAM layout (NormalOAM / FlippedOAM from facings.asm):
 *   NormalOAM  (down/up/left): [0]@(px,   py), [1]@(px+8, py),
 *                               [2]@(px,   py+8), [3]@(px+8, py+8)
 *   FlippedOAM (right):        [0]@(px+8, py), [1]@(px,   py),
 *                               [2]@(px+8, py+8), [3]@(px,   py+8)
 *   All FlippedOAM entries have OAM_XFLIP set.
 *
 * NPC facing when talked to (MakeNPCFacePlayer in movement.asm):
 *   Player facing up    → NPC faces down (toward player)
 *   Player facing down  → NPC faces up
 *   Player facing left  → NPC faces right
 *   Player facing right → NPC faces left
 *
 * Random movement (UpdateNPCSprite in movement.asm):
 *   WALK NPCs attempt a random step every NPC_MOVE_DELAY_MIN..MAX frames.
 *   The destination tile must be passable and not occupied by another NPC.
 *   Walk animation: NPC_WALK_FRAMES frames, pixel offset from old→new position.
 */
#include "npc.h"
#include "overworld.h"
#include "player.h"
#include "../platform/hardware.h"
#include "../platform/display.h"
#include "../game/constants.h"
#include "../data/event_data.h"
#include "../data/sprite_data.h"

/* OAM attribute bit 5: horizontal flip (GB standard). */
#define OAM_XFLIP  0x20

/* Animation frames per NPC step — matches original wWalkCounter = 8 VBlanks.
 * Main loop runs at 60 Hz; mid-step fires every frame, so 8 frames = 8 VBlanks. */
#define NPC_WALK_FRAMES   8
/* Pixels advanced per frame: 2 tiles * TILE_PX / NPC_WALK_FRAMES = 2. */
#define NPC_WALK_STEP_PX  2

/* Frame delay between random-walk move attempts. */
#define NPC_MOVE_DELAY_MIN  24
#define NPC_MOVE_DELAY_MAX  56   /* MIN + up to 32 random extra frames */

#define MAX_NPCS       16   /* 16 NPCs × 4 OAM slots = 64, stored in sprite tile space */
#define NPC_TILE_BASE   0   /* sprite tile slots 0-63 (16 NPCs × 4) */
#define NPC_OAM_BASE    4   /* OAM slots 4-67 (player is 0-3) */

/* ---- Per-NPC runtime state --------------------------------------- */

static int      npc_count = 0;
static uint8_t  npc_sprite[MAX_NPCS];      /* sprite_id */
static uint8_t  npc_x[MAX_NPCS];           /* tile X coordinate */
static uint8_t  npc_y[MAX_NPCS];           /* tile Y coordinate */
static uint8_t  npc_facing[MAX_NPCS];      /* 0=down 1=up 2=left 3=right */
static uint8_t  npc_move_type[MAX_NPCS];   /* 0=STAY 1=WALK */
static int      npc_move_timer[MAX_NPCS];  /* frames until next move attempt */
static int      npc_walk_frames[MAX_NPCS]; /* animation frames remaining */
static int      npc_px_off[MAX_NPCS];      /* pixel X offset for walk anim */
static int      npc_py_off[MAX_NPCS];      /* pixel Y offset for walk anim */
static uint8_t  npc_hidden[MAX_NPCS];      /* 1 = picked up / hidden, skip OAM */

/* ---- Tile GFX ---------------------------------------------------- */

/* Return tile index into gSpriteGfx[sprite_id] for the given facing.
 * Right facing reuses left-facing tiles; OAM_XFLIP mirrors them. */
static int facing_tile_base(int facing) {
    if (facing == 1) return 4;   /* up */
    if (facing >= 2) return 8;   /* left or right */
    return 0;                    /* down */
}

/* Load stand or walk tiles for NPC i into sprite tile slots.
 * walking=0: standing frame; walking=1: walking frame (tiles 12+ in sprite sheet).
 * Sprites with only 12 tiles (SPRITE_TILES/2) have zeros for walk tiles — acceptable. */
static void reload_npc_tiles_ex(int i, int walking) {
    uint8_t base  = (uint8_t)(NPC_TILE_BASE + i * 4);
    int     sid   = npc_sprite[i] < NUM_SPRITES ? npc_sprite[i] : 0;
    int     tb    = facing_tile_base(npc_facing[i]) + (walking ? SPRITE_TILES / 2 : 0);
    const uint8_t *gfx = gSpriteGfx[sid];
    Display_LoadSpriteTile(base + 0, gfx + (tb + 0) * 16);
    Display_LoadSpriteTile(base + 1, gfx + (tb + 1) * 16);
    Display_LoadSpriteTile(base + 2, gfx + (tb + 2) * 16);
    Display_LoadSpriteTile(base + 3, gfx + (tb + 3) * 16);
}

static void reload_npc_tiles(int i) { reload_npc_tiles_ex(i, 0); }

/* Set OAM tile IDs and flags for NPC i based on its current facing. */
static void apply_npc_oam_facing(int i) {
    uint8_t tile_base = (uint8_t)(NPC_TILE_BASE + i * 4);
    int     oam       = NPC_OAM_BASE + i * 4;
    uint8_t flg       = (npc_facing[i] == 3) ? OAM_XFLIP : 0;
    for (int s = 0; s < 4; s++) {
        wShadowOAM[oam + s].tile  = tile_base + s;
        wShadowOAM[oam + s].flags = flg;
    }
}

/* ---- Public API -------------------------------------------------- */

void NPC_Load(void) {
    for (int i = 0; i < MAX_NPCS * 4; i++) {
        wShadowOAM[NPC_OAM_BASE + i].y = 0;
        wShadowOAM[NPC_OAM_BASE + i].x = 0;
    }
    for (int i = 0; i < MAX_NPCS; i++) npc_hidden[i] = 0;
    npc_count = 0;

    if (wCurMap >= NUM_MAPS) return;
    const map_events_t *ev = &gMapEvents[wCurMap];

    /* Load regular NPCs */
    int n = 0;
    if (ev->npcs) {
        n = ev->num_npcs < MAX_NPCS ? ev->num_npcs : MAX_NPCS;
        for (int i = 0; i < n; i++) {
            const npc_event_t *npc = &ev->npcs[i];
            npc_sprite[i]      = npc->sprite_id;
            npc_x[i]           = (uint8_t)npc->x;
            npc_y[i]           = (uint8_t)npc->y;
            npc_facing[i]      = 0;   /* default: face down */
            npc_move_type[i]   = npc->movement;
            npc_move_timer[i]  = NPC_MOVE_DELAY_MIN;
            npc_walk_frames[i] = 0;
            npc_px_off[i]      = 0;
            npc_py_off[i]      = 0;
            reload_npc_tiles(i);
            apply_npc_oam_facing(i);
        }
    }

    /* Load item ball sprites after NPCs, skipping already-picked-up ones.
     * Each item ball occupies one NPC slot so it gets OAM + tile space. */
    if (ev->items) {
        int ni = ev->num_items < (MAX_NPCS - n) ? ev->num_items : (MAX_NPCS - n);
        for (int i = 0; i < ni; i++) {
            int slot = n + i;
            if (wCurMap < 248 && (wPickedUpItems[wCurMap] & (1u << i))) {
                /* Already picked up: mark hidden, leave OAM Y=0 */
                npc_hidden[slot] = 1;
                continue;
            }
            const item_event_t *it = &ev->items[i];
            npc_sprite[slot]      = 0x3D; /* SPRITE_POKE_BALL */
            npc_x[slot]           = (uint8_t)it->x;
            npc_y[slot]           = (uint8_t)it->y;
            npc_facing[slot]      = 0;
            npc_move_type[slot]   = 0;    /* STAY */
            npc_move_timer[slot]  = 0;
            npc_walk_frames[slot] = 0;
            npc_px_off[slot]      = 0;
            npc_py_off[slot]      = 0;
            reload_npc_tiles(slot);
            apply_npc_oam_facing(slot);
        }
        n += ni;
    }

    npc_count = n;
}

void NPC_HideSprite(int npc_slot_idx) {
    if (npc_slot_idx < 0 || npc_slot_idx >= MAX_NPCS) return;
    npc_hidden[npc_slot_idx] = 1;
    int oam = NPC_OAM_BASE + npc_slot_idx * 4;
    for (int s = 0; s < 4; s++)
        wShadowOAM[oam + s].y = 0;
}

/* Make NPC i face the player (opposite of gPlayerFacing).
 * Mirrors MakeNPCFacePlayer in engine/overworld/movement.asm. */
void NPC_FacePlayer(int i) {
    if (i < 0 || i >= npc_count) return;
    /* Player up→NPC down, player down→NPC up, left→right, right→left */
    static const uint8_t flip[4] = {1, 0, 3, 2};
    npc_facing[i] = flip[gPlayerFacing & 3];
    reload_npc_tiles(i);
    apply_npc_oam_facing(i);
}

/* Returns 1 if any NPC other than skip_idx is at (nx, ny). */
static int npc_blocked_except(int skip_idx, int nx, int ny) {
    for (int i = 0; i < npc_count; i++) {
        if (i == skip_idx) continue;
        if (nx == (int)npc_x[i] && ny == (int)npc_y[i]) return 1;
    }
    return 0;
}

int NPC_IsBlocked(int nx, int ny) {
    return npc_blocked_except(-1, nx, ny);
}

/* Return the index of the NPC at runtime position (tx, ty), or -1 if none.
 * Uses the live npc_x/npc_y (not spawn coords) so WALK NPCs are always found. */
int NPC_FindAtTile(int tx, int ty) {
    for (int i = 0; i < npc_count; i++) {
        if ((int)npc_x[i] == tx && (int)npc_y[i] == ty) return i;
    }
    return -1;
}

/* Per-frame update: advance walk animations and drive random movement.
 * Mirrors UpdateNPCSprite / .randomMovement in engine/overworld/movement.asm. */
void NPC_Update(void) {
    /* Step direction table: down, up, left, right (2 tiles per step). */
    static const int8_t ddx[4] = { 0,  0, -2,  2};
    static const int8_t ddy[4] = { 2, -2,  0,  0};

    for (int i = 0; i < npc_count; i++) {
        /* Advance walk animation: pixel offset slides toward 0.
         * Tile frame alternates Stand→Walk→Stand→Walk over the 8-frame step,
         * mirroring UpdateSpriteInWalkingAnimation: AnimFrameCounter advances
         * every 2 ticks (original: every 4 VBlanks / 2 game-ticks).
         * counter 0,2 = stand; counter 1,3 = walk. */
        if (npc_walk_frames[i] > 0) {
            npc_walk_frames[i]--;
            if (npc_px_off[i] > 0) npc_px_off[i] -= NPC_WALK_STEP_PX;
            if (npc_px_off[i] < 0) npc_px_off[i] += NPC_WALK_STEP_PX;
            if (npc_py_off[i] > 0) npc_py_off[i] -= NPC_WALK_STEP_PX;
            if (npc_py_off[i] < 0) npc_py_off[i] += NPC_WALK_STEP_PX;

            int elapsed  = NPC_WALK_FRAMES - npc_walk_frames[i]; /* 1..8 */
            int counter  = (elapsed - 1) / 2;                    /* 0,0,1,1,2,2,3,3 */
            int walking  = (npc_walk_frames[i] > 0) ? (counter & 1) : 0;
            reload_npc_tiles_ex(i, walking);                      /* odd=walk, even/done=stand */
            continue;
        }
        npc_px_off[i] = 0;
        npc_py_off[i] = 0;

        if (npc_move_type[i] != 1) continue; /* STAY: nothing to do */

        if (npc_move_timer[i] > 0) {
            npc_move_timer[i]--;
            continue;
        }

        /* Pick a random direction and try to walk there. */
        uint8_t r   = hRandomAdd ^ hRandomSub ^ hFrameCounter;
        int     dir = r & 3;
        int     ndx = ddx[dir];
        int     ndy = ddy[dir];
        int     nx  = (int)npc_x[i] + ndx;
        int     ny  = (int)npc_y[i] + ndy;

        /* Never walk off the map — NPCs must stay in-bounds. */
        int map_w = (int)wCurMapWidth  * 4;
        int map_h = (int)wCurMapHeight * 4;
        if (nx < 0 || ny < 0 || nx >= map_w || ny >= map_h) {
            npc_move_timer[i] = NPC_MOVE_DELAY_MIN + (int)((r >> 2) & 31);
            continue;
        }

        if (Tile_IsPassable(Map_GetTile(nx, ny))   &&
            !npc_blocked_except(i, nx, ny)          &&
            (nx != (int)wXCoord || ny != (int)wYCoord)) {

            npc_facing[i] = (uint8_t)dir;
            reload_npc_tiles(i);
            apply_npc_oam_facing(i);

            /* Commit destination, start pixel-offset animation from old pos. */
            npc_x[i]           = (uint8_t)nx;
            npc_y[i]           = (uint8_t)ny;
            npc_px_off[i]      = -ndx * TILE_PX;
            npc_py_off[i]      = -ndy * TILE_PX;
            npc_walk_frames[i] = NPC_WALK_FRAMES;
        }

        /* Reset timer with random jitter (24–55 frames). */
        npc_move_timer[i] = NPC_MOVE_DELAY_MIN + (int)((r >> 2) & 31);
    }
}

void NPC_BuildView(int scroll_px_x, int scroll_px_y) {
    int ox = gCamX;
    int oy = gCamY;

    for (int i = 0; i < npc_count; i++) {
        if (npc_hidden[i]) continue;

        int nx = (int)npc_x[i] - ox;
        int ny = (int)npc_y[i] - oy;

        /* Pixel top-left of the 16×16 sprite.
         * Walk animation offset displaces relative to the committed tile coord.
         * Camera scroll offset (scroll_px_*) shifts all sprites together. */
        int px = nx       * TILE_PX + scroll_px_x + npc_px_off[i];
        int py = (ny - 1) * TILE_PX + scroll_px_y + npc_py_off[i] - 4;

        int oam = NPC_OAM_BASE + i * 4;

        if (px + 16 <= 0 || px >= SCREEN_WIDTH_PX ||
            py + 16 <= 0 || py >= SCREEN_HEIGHT_PX) {
            wShadowOAM[oam + 0].y = 0;
            wShadowOAM[oam + 1].y = 0;
            wShadowOAM[oam + 2].y = 0;
            wShadowOAM[oam + 3].y = 0;
            continue;
        }

        if (npc_facing[i] == 3) {
            /* FlippedOAM: left/right column positions swapped, OAM_XFLIP set. */
            wShadowOAM[oam + 0].y = (uint8_t)(py     + OAM_Y_OFS);
            wShadowOAM[oam + 0].x = (uint8_t)(px + 8 + OAM_X_OFS);
            wShadowOAM[oam + 1].y = (uint8_t)(py     + OAM_Y_OFS);
            wShadowOAM[oam + 1].x = (uint8_t)(px     + OAM_X_OFS);
            wShadowOAM[oam + 2].y = (uint8_t)(py + 8 + OAM_Y_OFS);
            wShadowOAM[oam + 2].x = (uint8_t)(px + 8 + OAM_X_OFS);
            wShadowOAM[oam + 3].y = (uint8_t)(py + 8 + OAM_Y_OFS);
            wShadowOAM[oam + 3].x = (uint8_t)(px     + OAM_X_OFS);
        } else {
            /* NormalOAM: left column at px, right column at px+8. */
            wShadowOAM[oam + 0].y = (uint8_t)(py     + OAM_Y_OFS);
            wShadowOAM[oam + 0].x = (uint8_t)(px     + OAM_X_OFS);
            wShadowOAM[oam + 1].y = (uint8_t)(py     + OAM_Y_OFS);
            wShadowOAM[oam + 1].x = (uint8_t)(px + 8 + OAM_X_OFS);
            wShadowOAM[oam + 2].y = (uint8_t)(py + 8 + OAM_Y_OFS);
            wShadowOAM[oam + 2].x = (uint8_t)(px     + OAM_X_OFS);
            wShadowOAM[oam + 3].y = (uint8_t)(py + 8 + OAM_Y_OFS);
            wShadowOAM[oam + 3].x = (uint8_t)(px + 8 + OAM_X_OFS);
        }

        /* Grass priority: bottom two OAM entries render behind BG colors 1-3
         * when the NPC stands on the grass tile (mirrors GrassPriority /
         * update_player_oam grass check in player.c). */
        if (wGrassTile != 0xFF &&
            Map_GetTile((int)npc_x[i], (int)npc_y[i]) == wGrassTile) {
            wShadowOAM[oam + 2].flags |=  OAM_FLAG_PRIORITY;
            wShadowOAM[oam + 3].flags |=  OAM_FLAG_PRIORITY;
        } else {
            wShadowOAM[oam + 2].flags &= ~OAM_FLAG_PRIORITY;
            wShadowOAM[oam + 3].flags &= ~OAM_FLAG_PRIORITY;
        }
    }

    /* Hide unused NPC OAM slots. */
    for (int i = npc_count; i < MAX_NPCS; i++) {
        int oam = NPC_OAM_BASE + i * 4;
        wShadowOAM[oam + 0].y = 0;
        wShadowOAM[oam + 1].y = 0;
        wShadowOAM[oam + 2].y = 0;
        wShadowOAM[oam + 3].y = 0;
    }
}
