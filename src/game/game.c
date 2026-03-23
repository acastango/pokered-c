/* game.c — Strong definitions of GameInit / GameTick.
 *
 * These override the __attribute__((weak)) stubs in main.c.
 *
 * GameInit:  load Pallet Town, spawn player near Red's house.
 * GameTick:  scene dispatcher — routes each 60 Hz VBlank to the correct
 *            scene handler (overworld, battle, menu, …).
 *
 * Frame-rate model:
 *   main.c runs at 60 Hz (one call per VBlank).  Each scene controls its own
 *   pace via gScene and the idle gate below:
 *     SCENE_OVERWORLD idle → 30 Hz (gate skips every other frame, matching the
 *                            original's double-DelayFrame between input polls)
 *     SCENE_OVERWORLD mid-step/dialog/warp → 60 Hz (no gate)
 *     SCENE_BATTLE / SCENE_MENU (future) → 60 Hz (no gate)
 */
#include "../platform/hardware.h"
#include "../platform/display.h"
#include "../platform/save.h"
#include "overworld.h"
#include "anim.h"
#include "player.h"
#include "warp.h"
#include "npc.h"
#include "text.h"
#include "menu.h"
#include "bag_menu.h"
#include "inventory.h"
#include "pokemon.h"
#include "constants.h"
#include "../data/font_data.h"
#include "../data/event_data.h"
#include "../data/wild_data.h"
#include "battle/battle_init.h"
#include "battle/battle_driver.h"

#include <stdio.h>
#include <stdlib.h>

/* ---- Scene state ------------------------------------------------- */
typedef enum {
    SCENE_OVERWORLD = 0,
    SCENE_BATTLE,       /* future */
    SCENE_MENU,         /* future */
} GameScene;

static GameScene gScene = SCENE_OVERWORLD;

/* Route 22 (0x21) for battle smoke testing — has rate=25 wild encounters.
 * Revert to MAP_PALLET_TOWN (0x00) once battles are confirmed working. */
#define MAP_PALLET_TOWN     0x21

/* Starting tile position in Pallet Town.
 * Red's House warp is at step (5,5) -> tile x=5*2=10, y=5*2+1=11.
 * Player exits 2 tiles south = (10, 13) — keeps Y odd. */
#define PLAYER_START_X      10
#define PLAYER_START_Y      13

void GameInit(void) {
    extern void WRAMClear(void);
    WRAMClear();

    if (Save_Load() == 0)
        printf("[save] Save loaded OK\n");
    else
        printf("[save] No save, starting new game\n");

    Map_Load(MAP_PALLET_TOWN);
    Font_Load();
    Player_Init(PLAYER_START_X, PLAYER_START_Y);
    NPC_Load();
    Map_BuildScrollView();
    NPC_BuildView(0, 0);

    /* Give player a starter Bulbasaur (species 0x99, level 5) if no save was loaded. */
    if (wPartyCount == 0) {
        Pokemon_InitMon(&wPartyMons[0], SPECIES_BULBASAUR, 5);
        wPartyCount = 1;
        printf("[party] Started with %s Lv.%d (HP %d/%d, ATK %d, DEF %d, SPD %d, SPC %d)\n",
               Pokemon_GetName(1), wPartyMons[0].level,
               wPartyMons[0].base.hp, wPartyMons[0].max_hp,
               wPartyMons[0].atk, wPartyMons[0].def,
               wPartyMons[0].spd, wPartyMons[0].spc);
    }
}

/* Wild encounter message: "Wild <POKEMON>!" placeholder */
static const uint8_t kWildText[] = {
    /* "Wild  POKeMON!" — abbreviated */
    0x96,0x88,0x8B,0x8B,0x7F,0x8F,0x8E,0x8A,0x84,0x8C,0x8E,0x8D,0x9B,0x50
};

/* No hardcoded tile ID — use wGrassTile, which is loaded from the tileset
 * header by Map_Load (overworld.c:86).  0xFF means "no grass" for this tileset. */

/* Warp fade transition — mirrors GBFadeOutToBlack + GBFadeInFromWhite.
 *
 * Palette values derived from fade.asm FadePal1-8 tables.
 * Each fade step is held for FADE_TICKS_PER_STEP overworld ticks
 * (= 4 ticks × 2 VBlanks/tick = 8 VBlanks, matching DelayFrames(8)).
 *
 * BGP, OBP0, OBP1 for GBFadeOutToBlack (4 steps: normal → black):
 *   Step 0: FadePal4 — {0xE4,0xD0,0xE0}  normal-ish
 *   Step 1: FadePal3 — {0xF9,0xE4,0xE4}  BGP darker
 *   Step 2: FadePal2 — {0xFE,0xFE,0xF8}  near black
 *   Step 3: FadePal1 — {0xFF,0xFF,0xFF}  all black
 *
 * No fade-in: normal door warps snap directly to normal palette (LoadGBPal on
 * first OverworldLoop), matching the original GBFadeOutToBlack path.
 * GBFadeInFromWhite is only used for battle/fly warps (not yet implemented).
 */
#define FADE_TICKS_PER_STEP  4   /* 4 overworld ticks = 8 VBlanks at 60 Hz */
#define FADE_OUT_STEPS       4

static const uint8_t kFadeOut[FADE_OUT_STEPS][3] = {
    {0xE4, 0xD0, 0xE0},
    {0xF9, 0xE4, 0xE4},
    {0xFE, 0xFE, 0xF8},
    {0xFF, 0xFF, 0xFF},
};


typedef enum { WARP_NONE = 0, WARP_FADE_OUT } WarpPhase;
static WarpPhase gWarpPhase    = WARP_NONE;
static int       gWarpStep     = 0;
static int       gWarpStepTimer = 0;

/* Wild encounter: roll against rate and trigger text if hit */
static void check_wild_encounter(void) {
    if (wCurMap >= NUM_MAPS) return;
    const wild_mons_t *w = &gWildGrass[wCurMap];
    if (!w->rate) return;

    /* Grass tile check: wGrassTile is set by Map_Load from the tileset header.
     * 0xFF = no grass for this tileset (towns, indoor maps, etc.). */
    if (wGrassTile == 0xFF) return;
    uint8_t cur_tile = Map_GetTile((int)wXCoord, (int)wYCoord);
    if (cur_tile != wGrassTile) return;

    /* Encounter probability: rate/256 per step (simplified) */
    uint8_t roll = (uint8_t)(hRandomAdd ^ hRandomSub ^ hFrameCounter);
    if (roll >= w->rate) return;

    /* Pick a random slot from the 10 encounter slots */
    uint8_t slot_idx = roll % 10;
    wCurEnemyLevel   = w->slots[slot_idx].level;
    wCurPartySpecies = w->slots[slot_idx].species;

    Battle_Start();
    Battle_RunLoop();
}

/* A-button interaction: check item ball at the tile in front of the player.
 * Mirrors PickUpItem (engine/events/pick_up_item.asm): reads sprite extra data,
 * calls GiveItem, shows "Found ITEM!" or "No More Room". */
static void check_item_pickup(void) {
    if (!(hJoyPressed & PAD_A)) return;

    int fx, fy;
    Player_GetFacingTile(&fx, &fy);

    if (wCurMap >= NUM_MAPS) return;
    const map_events_t *ev = &gMapEvents[wCurMap];
    if (!ev->items) return;

    for (int i = 0; i < ev->num_items; i++) {
        const item_event_t *it = &ev->items[i];
        if ((int)it->x != fx || (int)it->y != fy) continue;

        /* Already picked up on a previous visit */
        if (wPickedUpItems[wCurMap] & (1u << i)) return;

        static char pickup_msg[48];
        if (Inventory_Add(it->item_id, 1) == 0) {
            /* Mark picked up and hide the sprite */
            wPickedUpItems[wCurMap] |= (uint16_t)(1u << i);
            NPC_HideSprite(ev->num_npcs + i);

            char name[16];
            Inventory_DecodeASCII(it->item_id, name, sizeof(name));
            if (name[0] == '\0') {
                snprintf(pickup_msg, sizeof(pickup_msg), "Found an item!");
            } else {
                snprintf(pickup_msg, sizeof(pickup_msg), "Found %s!", name);
            }
        } else {
            snprintf(pickup_msg, sizeof(pickup_msg), "No room for\nmore items!");
        }
        Text_ShowASCII(pickup_msg);
        return;
    }
}

/* A-button interaction: check NPC at the tile in front of the player */
static void check_npc_interact(void) {
    if (!(hJoyPressed & PAD_A)) return;

    int fx, fy;
    Player_GetFacingTile(&fx, &fy);

    if (wCurMap >= NUM_MAPS) return;
    const map_events_t *ev = &gMapEvents[wCurMap];
    if (!ev->npcs) return;

    /* Search within ±1 tile of the facing tile.
     * Use NPC_FindAtTile so WALK NPCs are found at their current position. */
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int i = NPC_FindAtTile(fx + dx, fy + dy);
            if (i < 0 || i >= ev->num_npcs) continue;
            NPC_FacePlayer(i);
            if (ev->npcs[i].text) Text_ShowASCII(ev->npcs[i].text);
            return;
        }
    }
}

/* A-button interaction: check sign at the tile in front of the player */
static void check_sign_interact(void) {
    if (!(hJoyPressed & PAD_A)) return;

    int fx, fy;
    Player_GetFacingTile(&fx, &fy);

    if (wCurMap >= NUM_MAPS) return;
    const map_events_t *ev = &gMapEvents[wCurMap];
    if (!ev->signs) return;

    for (int i = 0; i < ev->num_signs; i++) {
        const sign_event_t *s = &ev->signs[i];
        if ((int)s->x == fx && (int)s->y == fy) {
            if (s->text) Text_ShowASCII(s->text);
            return;
        }
    }
}

void GameTick(void) {
    /* ---- Text dialog: always runs at 60 Hz --------------------------- *
     * hJoyPressed is an edge signal valid for exactly one VBlank frame.  *
     * Polling it after the 30 Hz gate would silently discard presses on  *
     * odd frames.  The original WaitForTextScrollButtonPress uses a       *
     * single DelayFrame (not two), so text input is natively 60 Hz.     */
    if (Text_IsOpen()) {
        Text_Update();
        return;
    }

    if (Menu_IsOpen()) {
        Menu_Tick();
        return;
    }

    if (BagMenu_IsOpen()) {
        BagMenu_Tick();
        return;
    }

    /* ---- Tile animation: runs at 60 Hz (every VBlank), before the gate.
     * Mirrors UpdateMovingBgTiles called from the VBlank ISR in the original. */
    Anim_UpdateTiles();

    /* ---- Scene-aware frame gate ----------------------------------- *
     * main.c calls GameTick at 60 Hz (every VBlank).                 *
     * SCENE_OVERWORLD always runs at 30 Hz — both idle AND mid-step  *
     * use two DelayFrames per OverworldLoop iteration in the original.*
     * (AdvancePlayerSprite → CheckMapConnections → OverworldLoop,    *
     *  which calls DelayFrame twice, so steps are 8×2=16 VBlanks.)   *
     * Future scenes (SCENE_BATTLE, SCENE_MENU) run at full 60 Hz.   */
    if (gScene == SCENE_OVERWORLD) {
        static int ow_phase = 0;
        if (ow_phase ^= 1) return;
    }

    /* Warp fade transition: GBFadeOutToBlack → swap map → GBFadeInFromWhite */
    if (gWarpPhase == WARP_FADE_OUT) {
        Display_SetPalette(kFadeOut[gWarpStep][0],
                           kFadeOut[gWarpStep][1],
                           kFadeOut[gWarpStep][2]);
        if (--gWarpStepTimer == 0) {
            gWarpStep++;
            gWarpStepTimer = FADE_TICKS_PER_STEP;
            if (gWarpStep >= FADE_OUT_STEPS) {
                /* Fully black — execute the deferred map load, then rebuild views.
                 * Warp_Execute loads the new tileset GFX + sets player position.
                 * This is the only frame where tile_gfx[] changes, matching the
                 * original's "VRAM update during LCD-off at peak black".
                 *
                 * No fade-in: the original GBFadeOutToBlack path snaps directly
                 * to normal palette via LoadGBPal on the first OverworldLoop
                 * iteration.  GBFadeInFromWhite is only used for battle recovery
                 * and fly/dungeon warps. */
                Warp_Execute();
                Map_BuildScrollView();
                NPC_BuildView(0, 0);
                Player_SyncOAM();
                Display_SetPalette(0xE4, 0xD0, 0xE0);  /* snap to normal */
                gWarpPhase = WARP_NONE;
            }
        }
        return;
    }


    /* START: open pause menu (only when player is idle, no warp in progress) */
    if ((hJoyPressed & PAD_START) && !Player_IsMoving() && gWarpPhase == WARP_NONE) {
        Menu_Open();
        return;
    }

    check_item_pickup();
    if (Text_IsOpen()) return;
    check_npc_interact();
    /* Flush OAM positions immediately so NPC_FacePlayer's tile/flag changes
     * (including FlippedOAM column-swap for right-facing) are applied before
     * the dialog box renders.  Player must be idle to press A, so scroll is 0. */
    NPC_BuildView(0, 0);
    if (Text_IsOpen()) return;
    check_sign_interact();
    if (Text_IsOpen()) return;

    /* Door step-out: fire once on the first normal frame after arriving on a
     * door tile (mirrors PlayerStepOutFromDoor / BIT_STANDING_ON_DOOR). */
    if (Warp_HasDoorStep()) {
        Player_ForceStepDown();
    }

    Player_Update();

    /* If a warp just fired: begin GBFadeOutToBlack.
     * Map_BuildScrollView is deferred until fade-out completes (fully black),
     * so the old map visually fades out before the new one appears. */
    if (Warp_JustHappened()) {
        gWarpPhase    = WARP_FADE_OUT;
        gWarpStep     = 0;
        gWarpStepTimer = FADE_TICKS_PER_STEP;
        return;
    }

    NPC_Update();
    check_wild_encounter();
    Map_BuildScrollView();
    NPC_BuildView(gScrollPxX, gScrollPxY);
}
