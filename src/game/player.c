/* player.c — Player movement, walk animation, and directional OAM sprite.
 *
 * Original reference: engine/overworld/movement.asm, home/overworld.asm,
 *                     data/sprites/facings.asm
 *
 * Walk cycle (home/overworld.asm OverworldLoop / AdvancePlayerSprite):
 *   wWalkCounter = 8 when a step starts; decremented each frame.
 *   Background scrolls 2px/frame × 8 frames = 16px = 1 original step.
 *   wXCoord/wYCoord update at the last frame (wWalkCounter→0) in the
 *   original.  In this port we update them at step-start (equivalent
 *   in our single-context model since no code reads them mid-walk).
 *
 * Collision (home/overworld.asm CollisionCheckOnLand → CheckTilePassable
 *            → GetTileAndCoordsInFrontOfPlayer):
 *   The original reads the tile ONE step ahead from wTileMap:
 *     DOWN  → lda_coord 8,11  (player lower-left row 9, +2 rows = +1 step)
 *     UP    → lda_coord 8,7
 *     LEFT  → lda_coord 6,9
 *     RIGHT → lda_coord 10,9
 *   The "+2 screen rows" equals one step because original steps move 2
 *   screen tiles.  Our steps move 1 tile, so the target tile is simply
 *   Map_GetTile(nx, ny).
 *
 * Animation (UpdatePlayerSprite, SpriteFacingAndAnimationTable):
 *   IntraAnimFrameCounter: increments every frame while walking, wraps @ 4.
 *   AnimFrameCounter:      advances every 4 intra-frames, cycles 0-3.
 *   ImageIndex = FacingDirection + AnimFrameCounter.
 *   Both reset to 0 when idle (.notMoving branch).
 *
 * red.png layout (16×96 = 6 frames of 16×16, top-to-bottom):
 *   [0] StandDown  [1] StandUp  [2] StandLeft
 *   [3] WalkDown   [4] WalkUp   [5] WalkLeft
 *
 * SpriteFacingAndAnimationTable [facing][anim_counter 0-3]:
 *   DOWN:  Stand, Walk, Stand, Walk+flipX
 *   UP:    Stand, Walk, Stand, Walk+flipX
 *   LEFT:  Stand, Walk, Stand, Walk
 *   RIGHT: all LEFT frames with flipX
 *
 * OAM layout:
 *   Slots 0-3:   player (this file)
 *   Slots 4+:    NPCs   (npc.c)
 *   Sprite tile slots 64-67: active player frame (PLAYER_TILE_BASE in sprite space)
 */
#include "player.h"
#include "overworld.h"
#include "warp.h"
#include "npc.h"
#include "../platform/hardware.h"
#include "../platform/display.h"
#include "../game/constants.h"
#include "../data/player_sprite.h"
#include <stdio.h>

#define PLAYER_TILE_BASE   64   /* sprite tile slots 64-67 (after 16 NPCs × 4 = 64) */
#define WALK_FRAMES         8   /* frames per step = original wWalkCounter (8 VBlanks) */

/* ---- Ledge jump shadow OAM ---------------------------------------- */
/* Mirrors LoadHoppingShadowOAM from engine/overworld/ledges.asm.
 *
 * The original loads shadow.1bpp (quarter-circle 8×8, 1BPP) into VRAM tile $7F
 * via CopyVideoDataDouble (each 1BPP byte → lo=byte, hi=byte in 2BPP → color 3
 * = black where bit set, transparent where clear).  Four OAM entries 2×2 with
 * XY flip variants form a 16×16 oval ground shadow.
 *
 * Original position: Y_OAM=$54 (screen Y=68), X_OAM=$48 (screen X=64).
 * Relative to player TL (screen Y=64, X=64): shadow TL is 4px below player TL,
 * sitting at the lower half of the player sprite (the feet / ground plane). */
#define SHADOW_TILE_IDX   127   /* sprite tile $7F — matches vChars1 tile $7f */
#define SHADOW_OAM_BASE   68    /* OAM slots 68-71: after player(0-3) + NPCs(4-67) */
#define SHADOW_Y_OFFSET     8   /* shadow TL is 8px below player TL (original: $54-$4C = 84-76 = 8px) */

/* 2BPP tile data for shadow quarter-circle (bottom-right quadrant).
 * Derived from gfx/overworld/shadow.png (8×8, 1-bit):
 *   rows 0-3: blank; row 4: 3 right pixels; row 5: 5; row 6: 6; row 7: 7.
 * CopyVideoDataDouble equivalent: each 1BPP row byte b → {b, b} in 2BPP. */
static const uint8_t kShadowTile[16] = {
    0x00, 0x00,   /* row 0: ........ */
    0x00, 0x00,   /* row 1: ........ */
    0x00, 0x00,   /* row 2: ........ */
    0x00, 0x00,   /* row 3: ........ */
    0x07, 0x07,   /* row 4: .....### */
    0x1F, 0x1F,   /* row 5: ...##### */
    0x3F, 0x3F,   /* row 6: ..###### */
    0x7F, 0x7F,   /* row 7: .####### */
};

/* ---- Ledge jump ------------------------------------------ */
/* Mirrors HandleLedges + _HandleMidJump from engine/overworld/ledges.asm
 * and engine/overworld/player_animations.asm.
 *
 * A ledge jump spans 2 normal walk steps (16 frames total).
 * Step 1: player moves onto the ledge tile (normally impassable, bypassed).
 * Step 2: player moves onto the landing tile below/beside the ledge.
 * Arc: sprite Y is offset each frame per PlayerJumpingYScreenCoords.
 *      Relative to normal position: rises ~8px, lands 4px below, resets.
 */

/* gLedgeStep: 0=not jumping, 1=first step, 2=second step */
static int gLedgeStep = 0;
static int gLedgeDX   = 0;
static int gLedgeDY   = 0;
/* gArcFrame: 0-15, incremented once per tick while gLedgeStep > 0.
 * Decoupled from gWalkTimer so arc[0] is shown on the very first frame
 * and arc[15] on the very last, exactly matching the 16-entry original
 * PlayerJumpingYScreenCoords table. */
static int gArcFrame  = 0;

/* Mirrors wJoyIgnore / IgnoreInputForHalfSecond in EnterMap.
 * While > 0, Player_Update skips joypad reads (decrements each overworld tick).
 * Set by Warp_Execute after every map transition so a held direction button
 * can't immediately re-trigger the warp tile just arrived on. */
static int gInputIgnoreFrames = 0;

void Player_IgnoreInputFrames(int n) {
    gInputIgnoreFrames = n;
}

/* Signed Y-pixel offsets relative to the player's normal sprite Y,
 * indexed by arc frame 0-15 (frames 0-7 = step 1, frames 8-15 = step 2).
 * Derived from PlayerJumpingYScreenCoords: $38=base, values relative to $38.
 * Negative = upward (visual rise), positive = downward (visual drop/bounce). */
static const int kLedgeArc[16] = {
     0, -2, -4, -6, -7, -8, -8, -8,   /* step 1: rising */
    -7, -6, -5, -4, -2,  0, +4, +4    /* step 2: falling + bounce */
};

/* LedgeTiles table — mirrors data/tilesets/ledge_tiles.asm.
 * Entries: { gPlayerFacing, tile_player_on, ledge_tile_ahead }.
 * Only fires for OVERWORLD tileset (tileset_id=0). */
typedef struct { int facing; uint8_t cur_tile; uint8_t ledge_tile; } ledge_entry_t;
static const ledge_entry_t kLedgeTiles[] = {
    { 0, 0x2C, 0x37 },  /* DOWN, grass,  south-ledge type A */
    { 0, 0x39, 0x36 },  /* DOWN, path,   south-ledge type B */
    { 0, 0x39, 0x37 },  /* DOWN, path,   south-ledge type A */
    { 2, 0x2C, 0x27 },  /* LEFT, grass,  left-ledge */
    { 2, 0x39, 0x27 },  /* LEFT, path,   left-ledge */
    { 3, 0x2C, 0x0D },  /* RIGHT, grass, right-ledge type A */
    { 3, 0x2C, 0x1D },  /* RIGHT, grass, right-ledge type B */
    { 3, 0x39, 0x0D },  /* RIGHT, path,  right-ledge type A */
};
#define NUM_LEDGE_ENTRIES ((int)(sizeof(kLedgeTiles)/sizeof(kLedgeTiles[0])))

static int is_ledge_jump(int nx, int ny) {
    if (wCurMapTileset != 0) return 0;   /* OVERWORLD only */
    uint8_t cur  = Map_GetTile((int)wXCoord, (int)wYCoord);
    uint8_t next = Map_GetTile(nx, ny);
    for (int i = 0; i < NUM_LEDGE_ENTRIES; i++) {
        if (kLedgeTiles[i].facing    == gPlayerFacing &&
            kLedgeTiles[i].cur_tile  == cur           &&
            kLedgeTiles[i].ledge_tile == next) return 1;
    }
    return 0;
}

/* Animation entry: which gPlayerGfx frame to show, and whether to flip it. */
typedef struct { int8_t frame; uint8_t flip; } anim_entry_t;

/* SpriteFacingAndAnimationTable — C port of data/sprites/facings.asm.
 * Indexed by [facing 0-3][AnimFrameCounter 0-3].
 * Frames: 0=StandDown 1=StandUp 2=StandLeft 3=WalkDown 4=WalkUp 5=WalkLeft */
static const anim_entry_t kAnimTable[4][4] = {
    /* DOWN  */ { {0,0}, {3,0}, {0,0}, {3,1} },
    /* UP    */ { {1,0}, {4,0}, {1,0}, {4,1} },
    /* LEFT  */ { {2,0}, {5,0}, {2,0}, {5,0} },
    /* RIGHT */ { {2,1}, {5,1}, {2,1}, {5,1} },
};

int gPlayerFacing = 0;
int gScrollPxX    = 0;
int gScrollPxY    = 0;

static int gWalkTimer        = 0;  /* counts WALK_FRAMES..1 while animating */
static int gWalkDX           = 0;  /* step direction of current/last step */
static int gWalkDY           = 0;
static int gBgScrollDX       = 0;  /* camera tile delta this step (0 at map edge) */
static int gBgScrollDY       = 0;
static int gPlayerOffPxX     = 0;  /* sprite pixel offset from its resting position */
static int gPlayerOffPxY     = 0;
static int gIntraAnimFrame   = 0;  /* 0-3; advances AnimFrameCounter every 4 frames */
static int gAnimFrameCounter = 0;  /* 0-3; cycles through walk animation states */

/* ---- Sprite helpers -------------------------------------- */

/* Show/hide 2×2 shadow OAM (slots SHADOW_OAM_BASE..+3) at the player's
 * ground position.  Arc offset is intentionally NOT applied — the shadow
 * stays at ground level while the player sprite arcs above it. */
static void update_shadow_oam(void) {
    if (gLedgeStep == 0 || gWalkTimer == 0) {
        wShadowOAM[SHADOW_OAM_BASE + 0].y = 0;
        wShadowOAM[SHADOW_OAM_BASE + 1].y = 0;
        wShadowOAM[SHADOW_OAM_BASE + 2].y = 0;
        wShadowOAM[SHADOW_OAM_BASE + 3].y = 0;
        return;
    }
    int sx = ((int)wXCoord - gCamX)     * TILE_PX + gPlayerOffPxX;
    int sy = ((int)wYCoord - gCamY - 1) * TILE_PX + gPlayerOffPxY + SHADOW_Y_OFFSET;

    wShadowOAM[SHADOW_OAM_BASE + 0].y     = (uint8_t)(sy     + OAM_Y_OFS);
    wShadowOAM[SHADOW_OAM_BASE + 0].x     = (uint8_t)(sx     + OAM_X_OFS);
    wShadowOAM[SHADOW_OAM_BASE + 0].tile  = SHADOW_TILE_IDX;
    wShadowOAM[SHADOW_OAM_BASE + 0].flags = OAM_FLAG_PALETTE;

    wShadowOAM[SHADOW_OAM_BASE + 1].y     = (uint8_t)(sy     + OAM_Y_OFS);
    wShadowOAM[SHADOW_OAM_BASE + 1].x     = (uint8_t)(sx + 8 + OAM_X_OFS);
    wShadowOAM[SHADOW_OAM_BASE + 1].tile  = SHADOW_TILE_IDX;
    wShadowOAM[SHADOW_OAM_BASE + 1].flags = OAM_FLAG_PALETTE | OAM_FLAG_FLIP_X;

    wShadowOAM[SHADOW_OAM_BASE + 2].y     = (uint8_t)(sy + 8 + OAM_Y_OFS);
    wShadowOAM[SHADOW_OAM_BASE + 2].x     = (uint8_t)(sx     + OAM_X_OFS);
    wShadowOAM[SHADOW_OAM_BASE + 2].tile  = SHADOW_TILE_IDX;
    wShadowOAM[SHADOW_OAM_BASE + 2].flags = OAM_FLAG_PALETTE | OAM_FLAG_FLIP_Y;

    wShadowOAM[SHADOW_OAM_BASE + 3].y     = (uint8_t)(sy + 8 + OAM_Y_OFS);
    wShadowOAM[SHADOW_OAM_BASE + 3].x     = (uint8_t)(sx + 8 + OAM_X_OFS);
    wShadowOAM[SHADOW_OAM_BASE + 3].tile  = SHADOW_TILE_IDX;
    wShadowOAM[SHADOW_OAM_BASE + 3].flags = OAM_FLAG_PALETTE | OAM_FLAG_FLIP_X | OAM_FLAG_FLIP_Y;
}

static void load_player_frame(int frame_idx) {
    const uint8_t (*fr)[PLAYER_TILE_BYTES] = gPlayerGfx[frame_idx];
    Display_LoadSpriteTile(PLAYER_TILE_BASE + 0, fr[0]);
    Display_LoadSpriteTile(PLAYER_TILE_BASE + 1, fr[1]);
    Display_LoadSpriteTile(PLAYER_TILE_BASE + 2, fr[2]);
    Display_LoadSpriteTile(PLAYER_TILE_BASE + 3, fr[3]);
}

/* Write OAM slots 0-3 for the current facing + animation state.
 * Mirrors UpdatePlayerSprite + FlippedOAM layout in facings.asm. */
static void update_player_oam(void) {
    int anim_idx = (gWalkTimer > 0) ? gAnimFrameCounter : 0;
    const anim_entry_t *e = &kAnimTable[gPlayerFacing & 3][anim_idx];
    load_player_frame(e->frame);

    /* FlippedOAM: swap TL<->TR and BL<->BR tile IDs when flipping. */
    uint8_t tl, tr, bl, br;
    uint8_t flags = e->flip ? OAM_FLAG_FLIP_X : 0;
    if (e->flip) {
        tl = PLAYER_TILE_BASE + 1;  tr = PLAYER_TILE_BASE + 0;
        bl = PLAYER_TILE_BASE + 3;  br = PLAYER_TILE_BASE + 2;
    } else {
        tl = PLAYER_TILE_BASE + 0;  tr = PLAYER_TILE_BASE + 1;
        bl = PLAYER_TILE_BASE + 2;  br = PLAYER_TILE_BASE + 3;
    }

    /* Player map tile (wXCoord, wYCoord) is the sprite's lower-left reference.
     * Camera places the player at screen tile (8, 9), so sprite TL is at
     * screen tile (8, 8) — one tile above the player reference tile.
     * Sprite top-left pixel = (player_tile - cam_origin - {0,1}) * 8px
     * plus a per-frame sliding offset for smooth animation. */
    int sx = ((int)wXCoord - gCamX)     * TILE_PX + gPlayerOffPxX;
    /* Sub-tile Y offset: original wSpritePlayerStateData1YPixels = $3C = 60px.
     * Tile row 8 starts at 64px, so the sprite TL is 4px above the grid boundary.
     * This 4px overlap makes the player's head visually touch the tile row above
     * (e.g. ledge tiles), matching the original's lda_coord 8,9 positioning. */
    int sy = ((int)wYCoord - gCamY - 1) * TILE_PX + gPlayerOffPxY - 4;

    /* Ledge jump arc: apply signed Y-pixel offset from kLedgeArc.
     * gArcFrame 0-15 spans the full 16-frame jump (8 frames × 2 steps).
     * gArcFrame is advanced once per tick in Player_Update, decoupled from
     * gWalkTimer so arc[0] (=0, no offset) is shown on the very first frame.
     * Guard gArcFrame < 16 handles the terminal tick where gLedgeStep is
     * still set but the final arc frame has already been rendered.
     * Mirrors _HandleMidJump / PlayerJumpingYScreenCoords. */
    if (gLedgeStep > 0 && gArcFrame < 16) {
        sy += kLedgeArc[gArcFrame];
    }

    wShadowOAM[0].y = (uint8_t)(sy     + OAM_Y_OFS);
    wShadowOAM[0].x = (uint8_t)(sx     + OAM_X_OFS);
    wShadowOAM[0].tile = tl;  wShadowOAM[0].flags = flags;

    wShadowOAM[1].y = (uint8_t)(sy     + OAM_Y_OFS);
    wShadowOAM[1].x = (uint8_t)(sx + 8 + OAM_X_OFS);
    wShadowOAM[1].tile = tr;  wShadowOAM[1].flags = flags;

    /* Grass priority: when player stands on the tileset's grass tile, set
     * OAM_FLAG_PRIORITY (bit 7) on the bottom two OAM entries so they render
     * behind non-transparent BG pixels (the grass blades cover the lower body).
     * Mirrors AdvancePlayerSprite's wSpritePlayerStateData2GrassPriority logic
     * and facings.asm UNDER_GRASS flag on the bottom-half entries only. */
    if (wGrassTile != 0xFF && Map_GetTile((int)wXCoord, (int)wYCoord) == wGrassTile)
        flags |= OAM_FLAG_PRIORITY;

    wShadowOAM[2].y = (uint8_t)(sy + 8 + OAM_Y_OFS);
    wShadowOAM[2].x = (uint8_t)(sx     + OAM_X_OFS);
    wShadowOAM[2].tile = bl;  wShadowOAM[2].flags = flags;

    wShadowOAM[3].y = (uint8_t)(sy + 8 + OAM_Y_OFS);
    wShadowOAM[3].x = (uint8_t)(sx + 8 + OAM_X_OFS);
    wShadowOAM[3].tile = br;  wShadowOAM[3].flags = flags;

    update_shadow_oam();
}

/* Advance IntraAnimFrameCounter; rolls AnimFrameCounter every 4 ticks.
 * Mirrors UpdateSpriteInWalkingAnimation in movement.asm.
 * At 60 Hz (one call per VBlank), wrapping at 4 matches the original exactly. */
static void advance_anim(void) {
    if (++gIntraAnimFrame >= 4) {
        gIntraAnimFrame = 0;
        gAnimFrameCounter = (gAnimFrameCounter + 1) & 3;
    }
}

/* Commit (nx, ny) as the new player position and start an 8-frame walk
 * animation.  Computes background scroll and sprite slide deltas from the
 * camera movement so both mid-map and edge-of-map cases work correctly:
 *
 *   Mid-map  (camera tracks player): BG scrolls 1px/frame, sprite stays centred.
 *   Map edge (camera clamped):       BG static, sprite slides 1px/frame across it.
 */
static void begin_step(int nx, int ny, int dx, int dy) {
    int old_cam_x = gCamX;
    int old_cam_y = gCamY;

    wXCoord = (uint16_t)nx;
    wYCoord = (uint16_t)ny;
    Map_UpdateCamera();

    gWalkDX     = dx;
    gWalkDY     = dy;
    gBgScrollDX = gCamX - old_cam_x;   /* ±1 mid-map, 0 at map edges */
    gBgScrollDY = gCamY - old_cam_y;

    /* BG starts offset one tile in the step direction, scrolls toward 0. */
    gScrollPxX = gBgScrollDX * TILE_PX;
    gScrollPxY = gBgScrollDY * TILE_PX;

    /* Sprite starts at old screen position, slides to new.
     * offset = (cam_delta - step_delta) * 8:
     *   mid-map  → 0         (sprite stays at screen centre)
     *   map edge → -step * 8 (sprite begins at old pixel, slides +1px/frame) */
    gPlayerOffPxX = (gBgScrollDX - dx) * TILE_PX;
    gPlayerOffPxY = (gBgScrollDY - dy) * TILE_PX;

    gWalkTimer = WALK_FRAMES;
}

/* ---- Public API ------------------------------------------ */

/* Automatically step one tile south from a door (PlayerStepOutFromDoor).
 * Called after a warp if the arrival tile is a door tile.  Mirrors
 * PlayerStepOutFromDoor in engine/overworld/auto_movement.asm:
 *   - always steps DOWN regardless of current facing
 *   - no collision check (door exits are always passable)
 * Step size is 2 tiles to match the doubled coordinate system. */
void Player_ForceStepDown(void) {
    gPlayerFacing = 0;  /* face down (image index 0: StandingDown) */
    begin_step((int)wXCoord, (int)wYCoord + 2, 0, 2);
}

void Player_SetPos(uint16_t x, uint16_t y) {
    wXCoord       = x;
    wYCoord       = y;
    gWalkTimer    = 0;
    gScrollPxX    = 0;
    gScrollPxY    = 0;
    gPlayerOffPxX = 0;
    gPlayerOffPxY = 0;
    gLedgeStep    = 0;
    gArcFrame     = 0;
    Map_UpdateCamera();
}

void Player_Init(uint8_t x, uint8_t y) {
    wXCoord           = x;
    wYCoord           = y;
    gWalkTimer        = 0;
    gScrollPxX        = 0;
    gScrollPxY        = 0;
    gPlayerOffPxX     = 0;
    gPlayerOffPxY     = 0;
    gIntraAnimFrame   = 0;
    gAnimFrameCounter = 0;
    gLedgeStep        = 0;
    gArcFrame         = 0;
    Map_UpdateCamera();
    Display_LoadSpriteTile(SHADOW_TILE_IDX, kShadowTile);
    update_player_oam();
}

void Player_Update(void) {
    /* ---- Mid-step: advance BG scroll and sprite slide ---- */
    if (gWalkTimer > 0) {
        /* Background scrolls from initial offset toward 0 (1 px per frame = 1px/VBlank). */
        gScrollPxX    -= gBgScrollDX;
        gScrollPxY    -= gBgScrollDY;
        /* Sprite slides from old screen position toward new (1 px per frame). */
        gPlayerOffPxX += (gWalkDX - gBgScrollDX);
        gPlayerOffPxY += (gWalkDY - gBgScrollDY);
        advance_anim();
        if (--gWalkTimer == 0) {
            /* Snap to zero at step completion to absorb any rounding drift. */
            gScrollPxX = gScrollPxY = gPlayerOffPxX = gPlayerOffPxY = 0;
            if (gLedgeStep == 1) {
                /* Auto-advance to step 2: move onto the landing tile.
                 * No collision check — the landing tile is always passable. */
                gLedgeStep = 2;
                begin_step((int)wXCoord + gLedgeDX, (int)wYCoord + gLedgeDY,
                           gLedgeDX, gLedgeDY);
            } else {
                /* Don't clear gLedgeStep here — advance gArcFrame below
                 * so arc[15] (+4 bounce) renders on this terminal frame,
                 * then clear gLedgeStep after update_player_oam. */
                Warp_Check();
            }
        }
        /* Advance gArcFrame once per tick while a ledge jump is active.
         * On the step-1→2 transition gWalkTimer was just reset by begin_step
         * so gLedgeStep=2 is set; gArcFrame correctly advances to 8 here. */
        if (gLedgeStep > 0) gArcFrame++;
        update_player_oam();
        /* Now safe to retire the ledge jump: arc[15] has been rendered. */
        if (gWalkTimer == 0 && gLedgeStep == 2) gLedgeStep = 0;
        return;
    }

    /* ---- Input suppression (mirrors wJoyIgnore / IgnoreInputForHalfSecond) -
     * While gInputIgnoreFrames > 0 the joypad is masked for this tick.
     * This prevents a held direction from immediately re-triggering a warp
     * tile after arrival, matching the original EnterMap behaviour. */
    if (gInputIgnoreFrames > 0) {
        static int last_printed = -1;
        if (gInputIgnoreFrames != last_printed) {
            printf("[input] suppressed, %d frames remaining\n", gInputIgnoreFrames);
            last_printed = gInputIgnoreFrames;
        }
        gInputIgnoreFrames--;
        update_player_oam();
        return;
    }

    /* ---- Idle: read d-pad for movement ------------------- */
    /* Step size = 2 tiles, matching the original's wXCoord/wYCoord units
     * (1 ASM step = 2 tiles).  The collision list was built for even-tile
     * positions only; 1-tile steps would check intermediate block positions
     * that the original never reaches, causing false blocks and ghost walls. */
    int dx = 0, dy = 0;
    if      (hJoyHeld & PAD_RIGHT) { dx =  2; gPlayerFacing = 3; }
    else if (hJoyHeld & PAD_LEFT)  { dx = -2; gPlayerFacing = 2; }
    else if (hJoyHeld & PAD_DOWN)  { dy =  2; gPlayerFacing = 0; }
    else if (hJoyHeld & PAD_UP)    { dy = -2; gPlayerFacing = 1; }

    if (!dx && !dy) {
        /* .notMoving: reset anim counters so sprite returns to standing frame. */
        gIntraAnimFrame = gAnimFrameCounter = 0;
        update_player_oam();
        return;
    }

    int nx = (int)wXCoord + dx;
    int ny = (int)wYCoord + dy;

    /* ---- Off-map edge: attempt connected-map transition -- */
    if (nx < 0 || ny < 0 ||
        nx >= (int)wCurMapWidth  * 4 ||
        ny >= (int)wCurMapHeight * 4)
    {
        /* Mirror the original's collision-strip check (CollisionCheckOnLand):
         * before allowing a boundary crossing, verify the destination tile is
         * passable.  Map_GetTile returns the adjacent map's tile for OOB
         * positions on outdoor maps (via connected_tile), or the border_block
         * tile when there is no connection.  Both must be passable to proceed.
         *
         * This prevents the player from crossing into a connected map when
         * they are in a border block column whose tiles are impassable in the
         * destination map (e.g. west border column x=0-3 of Pallet Town leads
         * into Route 21's water-border row which is impassable without Surf).
         *
         * Skip this check for indoor maps (tileset != 0): they rely on
         * Warp_Check for exits, and their border_block is always impassable
         * which would falsely block the exit warp from firing. */
        if (wCurMapTileset == 0 && !Tile_IsPassable(Map_GetTile(nx, ny))) {
            update_player_oam();
            return;
        }
        /* Pre-build the scroll buffer from the OLD map at the stepped camera
         * position before Connection_Check switches cur_map.  This keeps the
         * old map's tile content visible in the portion of the screen that is
         * still inside the old map boundary (the bottom rows when going north,
         * top rows when going south, etc.), preventing a 1-frame visual chop.
         * The out-of-bounds portion is already filled by connected_tile() which
         * reads from the neighbour map — so the top-of-screen rows are seamless
         * in both the pre-built and post-switch buffers.  Map_BuildScrollView
         * will skip its rebuild this frame because gScrollViewReady is set. */
        Map_PreBuildScrollStep(dx, dy);
        if (Connection_Check(dx, dy)) {
            /* Smooth outdoor connection: outdoor maps all share tileset 0 so
             * no tile GFX reload is needed.  Do NOT call begin_step here —
             * it computes gBgScrollDX from (new_cam - old_cam), but the two
             * maps live in different coordinate spaces so the delta is huge
             * and the BG would jump visually.  Instead, force a single-tile
             * scroll exactly as if the player took a normal mid-map step,
             * matching the seamless view-pointer shift in the original. */
            Map_UpdateCamera();
            NPC_Load();
            gWalkDX       = dx;
            gWalkDY       = dy;
            gBgScrollDX   = dx;
            gBgScrollDY   = dy;
            gScrollPxX    = dx * TILE_PX;
            gScrollPxY    = dy * TILE_PX;
            gPlayerOffPxX = 0;
            gPlayerOffPxY = 0;
            gWalkTimer    = WALK_FRAMES;
        } else {
            /* No outdoor connection — player is at the edge of an indoor map
             * (or an outdoor map boundary with no neighbour).  Check for an
             * exit warp: e.g. pressing DOWN on the exit mat at the south wall
             * of Red's House triggers a return to Pallet Town.
             *
             * This mirrors the original OverworldLoop path where
             * CheckWarpsCollision is reached even when TryDoStep is blocked
             * by the map boundary.  Without this call, gWarpCooldown is never
             * consumed and the exit warp never fires. */
            printf("[oob] map=%d pos=(%d,%d) dir=(%d,%d) → Warp_Check\n",
                   wCurMap, wXCoord, wYCoord, dx, dy);
            Warp_Check();
        }
        update_player_oam();
        return;
    }

    /* ---- NPC collision -------------------------------------------- */
    if (NPC_IsBlocked(nx, ny)) {
        update_player_oam();
        return;
    }

    /* ---- Ledge jump: bypass passability, force two-step arc ----------
     * Mirrors HandleLedges in engine/overworld/ledges.asm.
     * is_ledge_jump checks cur tile + next tile + facing against kLedgeTiles.
     * Only fires on OVERWORLD tileset (id=0). */
    if (is_ledge_jump(nx, ny)) {
        gLedgeStep = 1;
        gLedgeDX   = dx;
        gLedgeDY   = dy;
        gArcFrame  = 0;   /* show arc[0]=0 on this very first frame */
        begin_step(nx, ny, dx, dy);
        update_player_oam();
        return;
    }

    /* ---- Tile collision (CheckTilePassable in original) --
     * GetTileAndCoordsInFrontOfPlayer reads the tile ONE step ahead — in our
     * 1-tile/step model that is the target tile (nx, ny). */
    if (!Tile_IsPassable(Map_GetTile(nx, ny))) {
        update_player_oam();
        return;
    }

    /* ---- Valid step: commit and animate ------------------ */
    begin_step(nx, ny, dx, dy);
    update_player_oam();
}

void Player_GetFacingTile(int *out_x, int *out_y) {
    static const int ddx[4] = { 0,  0, -2, 2 };
    static const int ddy[4] = { 2, -2,  0, 0 };
    *out_x = (int)wXCoord + ddx[gPlayerFacing & 3];
    *out_y = (int)wYCoord + ddy[gPlayerFacing & 3];
}

int Player_IsMoving(void) {
    return gWalkTimer > 0;
}

/* Refresh wShadowOAM[0-3] to match current position/facing.
 * Call after any out-of-band position change (e.g. warp map swap)
 * to prevent the stale sprite being visible during fade-in. */
void Player_SyncOAM(void) {
    update_player_oam();
}
