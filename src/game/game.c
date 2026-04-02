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
#include "../data/event_constants.h"
#include "../data/wild_data.h"
#include "battle/battle_init.h"
#include "battle/battle_trainer.h"
#include "battle/battle_driver.h"
#include "battle/battle_ui.h"
#include "battle/battle_loop.h"
#include "battle_transition.h"
#include "trainer_sight.h"
#include "party_menu.h"
#include "pokecenter.h"
#include "pokemart.h"
#include "music.h"
#include "../platform/audio.h"
#include "debug_cli.h"
#include "gate_scripts.h"
#include "pokedex.h"
#include "main_menu.h"
#include "intro.h"
#include "pallet_scripts.h"
#include "oakslab_scripts.h"
#include "gym_scripts.h"
#include "viridian_mart_scripts.h"
#include "mtmoon_scripts.h"
#include "cerulean_scripts.h"
#include "route24_scripts.h"
#include "bills_house_scripts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Scene state ------------------------------------------------- */
typedef enum {
    SCENE_OVERWORLD = 0,
    SCENE_BTRANS,        /* battle transition animation   */
    SCENE_BATTLE,
    SCENE_MENU,          /* future */
    SCENE_MAIN_MENU,     /* startup: CONTINUE / NEW GAME */
    SCENE_INTRO,         /* Oak's speech, new-game setup  */
} GameScene;

static GameScene gScene = SCENE_OVERWORLD;

int  Game_GetScene(void)   { return (int)gScene; }
void Game_SetScene(int s)  { gScene = (GameScene)s; }

/* Set by --skip flag in main.c: bypass main menu when a save exists */
int gSkipMenu = 0;

/* ---- Enter overworld from loaded wCurMap/wXCoord/wYCoord ---------- */
/* Fire all per-map OnMapLoad callbacks.  Called any time the current map
 * changes: enter_overworld, battle-end reload, warp fade-out, and debug
 * teleport.  NPC_Load() must have already been called before this. */
static void fire_map_onload_callbacks(void) {
    PalletScripts_OnMapLoad();
    OaksLabScripts_OnMapLoad();
    ViridianMartScripts_OnMapLoad();
    MtMoon_OnMapLoad();
    CeruleanScripts_OnMapLoad();
    Route24Scripts_OnMapLoad();
    BillsHouseScripts_OnMapLoad();
    Trainer_LoadMap();
    Gate_LoadMap();
}

static void enter_overworld(void) {
    Map_Load(wCurMap);
    Font_Load();
    Player_Init((uint8_t)wXCoord, (uint8_t)wYCoord);
    NPC_Load();
    fire_map_onload_callbacks();
    Map_BuildScrollView();
    NPC_BuildView(0, 0);
    gScene = SCENE_OVERWORLD;
}

void GameInit(void) {
    extern void WRAMClear(void);
    WRAMClear();

    int save_ok = (Save_Load() == 0);
    if (save_ok)
        printf("[save] Save loaded OK — map %d @ (%d,%d)\n", wCurMap, wXCoord, wYCoord);
    else
        printf("[save] No save found — showing main menu\n");

    Font_Load();
    /* --skip flag: jump straight to overworld when a save is available */
    if (gSkipMenu && save_ok) {
        enter_overworld();
        return;
    }
    /* Always show main menu first (CONTINUE if save found, NEW GAME otherwise).
     * Mirrors MainMenu in engine/menus/main_menu.asm. */
    MainMenu_Open(save_ok);
    gScene = SCENE_MAIN_MENU;
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
#define FADE_IN_STEPS        4

static const uint8_t kFadeOut[FADE_OUT_STEPS][3] = {
    {0xE4, 0xD0, 0xE0},
    {0xF9, 0xE4, 0xE4},
    {0xFE, 0xFE, 0xF8},
    {0xFF, 0xFF, 0xFF},
};

/* GBFadeInFromWhite — FadePalWhite4→1 from fade.asm (white → normal). */
static const uint8_t kFadeInFromWhite[FADE_IN_STEPS][3] = {
    {0x00, 0x00, 0x00},  /* all white */
    {0x0A, 0x01, 0x08},  /* near white */
    {0x28, 0x04, 0x20},  /* mid */
    {0xE4, 0xD0, 0xE0},  /* normal */
};

typedef enum { WARP_NONE = 0, WARP_FADE_OUT, WARP_FADE_IN } WarpPhase;
static WarpPhase gWarpPhase    = WARP_NONE;
static int       gWarpStep     = 0;
static int       gWarpStepTimer = 0;

/* Set to 1 before BattleTransition_Start when the pending battle is a trainer
 * battle.  Checked when transition completes to call Battle_StartTrainer
 * instead of Battle_Start. */
static int gPendingTrainerBattle = 0;

/* Debug: set to 1 to suppress wild encounters. */
int gNoWilds = 0;

/* Wild encounter: roll against rate and trigger text if hit */
static void check_wild_encounter(void) {
    if (gNoWilds) return;
    if (wCurMap >= NUM_MAPS) return;
    const wild_mons_t *w = &gWildGrass[wCurMap];
    if (!w->rate) return;

    /* Grass tile check: wGrassTile is set by Map_Load from the tileset header.
     * 0xFF = no grass for this tileset (towns, indoor maps, etc.). */
    if (wGrassTile == 0xFF) return;
    uint8_t cur_tile = Map_GetGameTile((int)wXCoord, (int)wYCoord);
    if (cur_tile != wGrassTile) return;

    /* Encounter probability: rate/256 per step (simplified) */
    uint8_t roll = (uint8_t)(hRandomAdd ^ hRandomSub ^ hFrameCounter);
    if (roll >= w->rate) return;

    /* Pick a random slot from the 10 encounter slots */
    uint8_t slot_idx = roll % 10;
    wCurEnemyLevel   = w->slots[slot_idx].level;
    wCurPartySpecies = w->slots[slot_idx].species;

    /* Start battle music immediately (mirrors pokered: PlaySound before BattleTransition). */
    Music_Play(MUSIC_WILD_BATTLE);

    /* Find first non-fainted party mon level for transition type selection. */
    int player_level = 5;
    for (int i = 0; i < wPartyCount; i++) {
        if (wPartyMons[i].base.hp > 0) {
            player_level = wPartyMons[i].level;
            break;
        }
    }

    /* Clear NPC sprites only — preserve player OAM (slots 0-3).
     * Mirrors the original: wShadowOAMSprite04 onward is cleared but
     * wShadowOAMSprite00 (player) is never touched, so the player sprite
     * stays visible throughout the flash and animation phases. */
    memset(wShadowOAM + 4, 0, (MAX_SPRITES - 4) * sizeof(wShadowOAM[0]));

    BattleTransition_Start(0, wCurEnemyLevel, player_level);
    gScene = SCENE_BTRANS;
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

    /* Search for an NPC along the player's line of sight only — no diagonals.
     * Mirrors the original's CheckInteractionWithNPC: checks the tile in front,
     * then steps further along the same axis (for counter-style NPCs like the
     * nurse across a desk).  Perpendicular offset stays 0 so a nearby NPC in a
     * different row/column never accidentally fires (e.g. gym guide beside a statue). */
    int facing = gPlayerFacing & 3;  /* 0=down 1=up 2=left 3=right */
    int best_i = -1;
    int best_score = 9999;
    for (int dist = 0; dist <= 2; dist++) {
        int tx = fx, ty = fy;
        switch (facing) {
            case 0: ty = fy + dist; break;  /* down  — search further south */
            case 1: ty = fy - dist; break;  /* up    — search further north */
            case 2: tx = fx - dist; break;  /* left  — search further west  */
            case 3: tx = fx + dist; break;  /* right — search further east  */
        }
        int i = NPC_FindAtTile(tx, ty);
        if (i < 0 || i >= ev->num_npcs) continue;
        if (dist < best_score) { best_score = dist; best_i = i; }
        break;  /* take the closest hit along the axis */
    }

    if (best_i < 0) return;

    NPC_FacePlayer(best_i);

    /* Trainer NPC: bypass normal text dispatch.
     * Defeated → show after_text.  Undefeated → engage immediately.
     * Mirrors CheckInteractionWithMapTrainer (home/trainers.asm). */
    if (ev->trainers) {
        int is_trainer = 0;
        for (int ti = 0; ti < ev->num_trainers; ti++) {
            const map_trainer_t *tr = &ev->trainers[ti];
            if (tr->npc_idx != best_i) continue;
            is_trainer = 1;
            if (CheckEvent(tr->flag_bit)) {
                if (tr->after_text) Text_ShowASCII(tr->after_text);
            } else {
                Trainer_EngageImmediate(best_i);
            }
            break;
        }
        if (is_trainer) return;
    }

    if (ev->npcs[best_i].script)
        ev->npcs[best_i].script();
    else if (ev->npcs[best_i].text)
        Text_ShowASCII(ev->npcs[best_i].text);
}

/* A-button interaction: check hidden events at the tile in front of the player.
 * Mirrors CheckForHiddenEvent / CheckIfCoordsInFrontOfPlayerMatch from
 * engine/overworld/hidden_events.asm.  Fires when the player faces a tile
 * that has a registered hidden event for the current map (bench guys, PCs,
 * posters, bookcases, etc.).  No sprite is involved — the trigger is purely
 * positional. */
static void check_hidden_event(void) {
    if (!(hJoyPressed & PAD_A)) return;

    int fx, fy;
    Player_GetFacingTile(&fx, &fy);

    if (wCurMap >= NUM_MAPS) return;
    const map_events_t *ev = &gMapEvents[wCurMap];
    if (!ev->hidden_events) return;

    for (int i = 0; i < ev->num_hidden_events; i++) {
        const hidden_event_t *h = &ev->hidden_events[i];
        if ((int)h->x != fx || (int)h->y != fy) continue;
        if (h->script)
            h->script();
        else if (h->text)
            Text_ShowASCII(h->text);
        return;
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
    DebugCLI_Tick();

    /* ---- Debug: file-based teleport (bugs/teleport.txt) -------------- *
     * Format:  map_id [tile_x tile_y]                                    *
     * Written by Claude Code; consumed and deleted on detection.         *
     * Checked once per second (every 60 frames) to avoid fs overhead.   */
    static int s_tele_timer = 0;
    if (++s_tele_timer >= 60) {
        s_tele_timer = 0;
        FILE *tf = fopen("bugs/teleport.txt", "r");
        if (tf) {
            int map_id = -1, tx = -1, ty = -1;
            fscanf(tf, "%d %d %d", &map_id, &tx, &ty);
            fclose(tf);
            remove("bugs/teleport.txt");
            if (map_id >= 0 && map_id < NUM_MAPS) {
                Warp_ForceTeleport((uint8_t)map_id, tx, ty);
                fire_map_onload_callbacks();
                Map_BuildScrollView();
                NPC_BuildView(0, 0);
            } else
                printf("[debug] teleport: bad map_id %d (max %d)\n", map_id, NUM_MAPS - 1);
        }
    }

    /* ---- Text dialog: always runs at 60 Hz --------------------------- *
     * hJoyPressed is an edge signal valid for exactly one VBlank frame.  *
     * Polling it after the 30 Hz gate would silently discard presses on  *
     * odd frames.  The original WaitForTextScrollButtonPress uses a       *
     * single DelayFrame (not two), so text input is natively 60 Hz.     */
    /* ---- Main menu (startup) -------------------------------------- */
    if (gScene == SCENE_MAIN_MENU) {
        if (MainMenu_IsOpen()) {
            MainMenu_Tick();
        } else {
            int result = MainMenu_GetResult();
            if (result == MAIN_MENU_CONTINUE) {
                enter_overworld();
            } else if (result == MAIN_MENU_NEW_GAME) {
                Intro_Start();
                gScene = SCENE_INTRO;
            }
        }
        return;
    }

    /* ---- Intro (Oak's speech / new-game setup) -------------------- */
    if (gScene == SCENE_INTRO) {
        if (Text_IsOpen()) { Text_Update(); return; }
        if (Intro_IsActive()) {
            Intro_Tick();
        } else {
            /* Intro done — wCurMap/wXCoord/wYCoord set to bedroom */
            enter_overworld();
        }
        return;
    }

    if (Text_IsOpen()) {
        Text_Update();
        /* If text is still open, stop here.  If text just closed while in
         * battle, fall through so BattleUI_Tick runs in the SAME frame —
         * avoids a 1-frame blank text box caused by Text_Close() erasing
         * rows 12-17 one frame before the battle state machine redraws.
         * Draw the empty box border immediately so rows 12-17 show a
         * proper bordered box even if the next BUI state doesn't redraw
         * the box this frame (e.g. BUI_TURN_END transitions to BUI_DRAW_HUD
         * without calling bui_draw_box). */
        if (Text_IsOpen() || gScene != SCENE_BATTLE) return;
        /* ASM: JoypadLowSensitivity suppresses held A/B after text closes,
         * preventing the A press that dismissed text from also triggering a
         * menu selection in BattleUI_Tick on the same frame.  Mirror this by
         * consuming the input that closed the text. */
        hJoyPressed = 0;
    }

    if (Menu_IsOpen()) {
        Menu_Tick();
        return;
    }

    if (BagMenu_IsOpen()) {
        BagMenu_Tick();
        return;
    }

    if (Pokedex_IsOpen()) {
        Pokedex_Tick();
        return;
    }

    /* ---- Party menu (overworld context) --------------------------- *
     * Opened via START → POKéMON.  After close, restore map GFX.   *
     * Skip during battle: BattleUI_Tick handles the menu from       *
     * BUI_PARTY_SELECT so the overworld GFX restore doesn't fire.  */
    if (PartyMenu_IsOpen() && gScene != SCENE_BATTLE) {
        PartyMenu_Tick();
        if (!PartyMenu_IsOpen()) {
            /* Party menu closed — restore overworld graphics */
            Display_SetPalette(0xE4, 0xD0, 0xE0);
            Map_ReloadGfx();
            Font_Load();
            Map_BuildScrollView();
            NPC_BuildView(gScrollPxX, gScrollPxY);
        }
        return;
    }

    /* ---- Pokémart buy/sell sequence ------------------------------- *
     * Runs at 60 Hz (before overworld gate).  Text_IsOpen() check at  *
     * the top already handles frames where dialog is open.             */
    if (Pokemart_IsActive()) {
        Pokemart_Tick();
        return;
    }

    /* ---- Pokémon Center healing sequence -------------------------- *
     * Runs at 60 Hz (before overworld gate).  Text_IsOpen() check at  *
     * the top already handles frames where dialog is open.             */
    if (Pokecenter_IsActive()) {
        Pokecenter_Tick();
        return;
    }

    /* ---- Battle transition (non-blocking) ------------------------- */
    if (gScene == SCENE_BTRANS) {
        if (BattleTransition_Tick()) {
            /* Transition complete — launch battle. */
            if (gPendingTrainerBattle) {
                gPendingTrainerBattle = 0;
                Battle_StartTrainer(gEngagedTrainerClass, gEngagedTrainerNo);
            } else {
                Battle_Start();
            }
            BattleUI_Enter();
            gScene = SCENE_BATTLE;
        }
        return;
    }

    /* ---- Battle scene (non-blocking) ------------------------------ */
    if (gScene == SCENE_BATTLE) {
        /* Save before BattleUI_Tick(): BUI_END clears both wIsInBattle and
         * wBattleResult in the same tick as setting bui_state=BUI_INACTIVE,
         * so we must snapshot them here to know the outcome after the tick. */
        uint8_t saved_battle_result = wBattleResult;
        BattleUI_Tick();
        if (!BattleUI_IsActive()) {
            /* Battle ended — restore overworld GFX and state.
             * Font_LoadHudTiles() overwrote tile_gfx slots 2-24 during battle;
             * Map_ReloadGfx() restores the tileset so the map renders correctly.
             * Map_ResetScrollState() ensures the next Map_BuildScrollView does a
             * full rebuild even if gScrollViewReady was set when battle triggered.
             * gStepJustCompleted cleared so check_wild_encounter doesn't fire
             * immediately on the first overworld frame after battle. */

            {
                int was_gym_trainer    = GymScripts_ConsumeGymTrainer();
                int was_cerulean_rival = CeruleanScripts_ConsumeRivalBattle();
                int was_rocket_r24     = Route24Scripts_ConsumeRocketBattle();
                if (saved_battle_result == BATTLE_OUTCOME_TRAINER_VICTORY) {
                    if (wGymLeaderNo)
                        GymScripts_OnVictory();
                    else if (was_gym_trainer)
                        GymScripts_OnGymTrainerVictory();
                    else if (was_cerulean_rival)
                        CeruleanScripts_OnVictory();
                    else if (was_rocket_r24)
                        Route24Scripts_OnVictory();
                    else
                        Trainer_MarkCurrentDefeated();
                } else if (was_cerulean_rival) {
                    CeruleanScripts_OnDefeat();
                } else if (was_rocket_r24) {
                    Route24Scripts_OnDefeat();
                }
            }

            gScene = SCENE_OVERWORLD;
            gStepJustCompleted = 0;
            memset(wShadowOAM, 0, sizeof(wShadowOAM));
            if (saved_battle_result == BATTLE_OUTCOME_TRAINER_VICTORY) {
                /* Palette is already white from BUI_FADE_WHITE — fade in from white. */
                gWarpPhase     = WARP_FADE_IN;
                gWarpStep      = 0;
                gWarpStepTimer = FADE_TICKS_PER_STEP;
            } else {
                Display_SetPalette(0xE4, 0xD0, 0xE0);
            }
            Map_ReloadGfx();
            Map_ResetScrollState();
            Font_Load();   /* restore font/box tiles (120-126, 128-255) */
            Music_Play(Music_GetMapID(wCurMap));
            NPC_Load();
            fire_map_onload_callbacks();
            Player_SyncOAM();
            Map_BuildScrollView();
            NPC_BuildView(gScrollPxX, gScrollPxY);
        }
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
        static int     ow_phase          = 0;
        static uint8_t ow_pressed_latch  = 0;
        if (ow_phase ^= 1) {
            /* Skip frame: latch any presses so the next active frame sees them.
             * Mirrors original: Joypad was read once per 30 Hz OverworldLoop
             * iteration, so presses between iterations were never lost. */
            ow_pressed_latch |= hJoyPressed;
            return;
        }
        /* Active frame: fold latched presses in so interactions aren't missed. */
        hJoyPressed      |= ow_pressed_latch;
        ow_pressed_latch  = 0;
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
                fire_map_onload_callbacks();
                Map_BuildScrollView();
                NPC_BuildView(0, 0);
                Player_SyncOAM();
                Display_SetPalette(0xE4, 0xD0, 0xE0);  /* snap to normal */
                Music_Play(Music_GetMapID(wCurMap));     /* start new map's music */
                gWarpPhase = WARP_NONE;
            }
        }
        return;
    }

    /* GBFadeInFromWhite — used after trainer battle victory. */
    if (gWarpPhase == WARP_FADE_IN) {
        Display_SetPalette(kFadeInFromWhite[gWarpStep][0],
                           kFadeInFromWhite[gWarpStep][1],
                           kFadeInFromWhite[gWarpStep][2]);
        if (--gWarpStepTimer == 0) {
            gWarpStep++;
            gWarpStepTimer = FADE_TICKS_PER_STEP;
            if (gWarpStep >= FADE_IN_STEPS) {
                Display_SetPalette(0xE4, 0xD0, 0xE0);
                gWarpPhase = WARP_NONE;
            }
        }
        return;
    }

    /* ---- Trainer engagement (non-blocking) ------------------------ *
     * While a trainer is walking toward the player or showing text,   *
     * block all normal overworld input and update the state machine.   *
     * Mirrors CheckFightingMapTrainers / TrainerWalkUpToPlayer flow.  */
    if (Trainer_IsEngaging()) {
        int r = Trainer_SightTick();
        NPC_Update();
        /* Don't rebuild the map while text is open — Text_ShowASCII already drew
         * the text box on the tilemap; Map_BuildScrollView would overwrite it. */
        if (!Text_IsOpen()) {
            Map_BuildScrollView();
            NPC_BuildView(gScrollPxX, gScrollPxY);
        }
        if (r) {
            /* Battle ready — find first non-fainted party mon for transition */
            int player_level = 5;
            for (int i = 0; i < wPartyCount; i++) {
                if (wPartyMons[i].base.hp > 0) {
                    player_level = wPartyMons[i].level;
                    break;
                }
            }
            memset(wShadowOAM + 4, 0, (MAX_SPRITES - 4) * sizeof(wShadowOAM[0]));
            Music_Play(wGymLeaderNo ? MUSIC_GYM_LEADER_BATTLE : MUSIC_TRAINER_BATTLE);
            gPendingTrainerBattle = 1;
            BattleTransition_Start(1, 0, player_level);
            gScene = SCENE_BTRANS;
        }
        return;
    }

    /* ---- Pallet Town map scripts ---------------------------------- *
     * Run before normal overworld input/movement so Oak can interrupt
     * the player at the north exit before an actual step/connection
     * begins. This matches the original script timing more closely.   */
    {
        int pallet_was_active = PalletScripts_IsActive();
        PalletScripts_Tick();
        if (PalletScripts_IsActive()) {
            /* If Oak just interrupted the player at the town edge, do not
             * advance the player's pre-trigger movement on that same frame.
             * On subsequent frames, keep scripted movement progressing. */
            if (pallet_was_active) {
                Player_Update();
                if (Warp_JustHappened()) {
                    gWarpPhase    = WARP_FADE_OUT;
                    gWarpStep     = 0;
                    gWarpStepTimer = FADE_TICKS_PER_STEP;
                    return;
                }
            }
            NPC_Update();
            if (!Text_IsOpen()) {
                Map_BuildScrollView();
                NPC_BuildView(gScrollPxX, gScrollPxY);
            }
            return;
        }
    }

    {
        /* OaksLabScripts_Tick may set gPendingBattle via OLS_BATTLE_TRIGGER;
         * check for it after each tick and start the battle transition. */
        int oakslab_was_active = OaksLabScripts_IsActive();
        OaksLabScripts_Tick();
        {
            uint8_t tr_class, tr_no;
            if (OaksLabScripts_GetPendingBattle(&tr_class, &tr_no)) {
                int player_level = 5;
                for (int i = 0; i < wPartyCount; i++) {
                    if (wPartyMons[i].base.hp > 0) {
                        player_level = wPartyMons[i].level;
                        break;
                    }
                }
                gEngagedTrainerClass = tr_class;
                gEngagedTrainerNo    = tr_no;
                memset(wShadowOAM + 4, 0, (MAX_SPRITES - 4) * sizeof(wShadowOAM[0]));
                Music_Play(wGymLeaderNo ? MUSIC_GYM_LEADER_BATTLE : MUSIC_TRAINER_BATTLE);
                gPendingTrainerBattle = 1;
                BattleTransition_Start(1, 0, player_level);
                gScene = SCENE_BTRANS;
                return;
            }
        }
        if (OaksLabScripts_IsActive()) {
            /* ShowPokedexData takes over the screen — skip overworld rendering.
             * OaksLabScripts_Tick already ran above, advancing the state machine. */
            if (Pokedex_IsShowingData()) {
                Pokedex_ShowDataTick();
                return;
            }
            if (oakslab_was_active) {
                Player_Update();
                if (Warp_JustHappened()) {
                    gWarpPhase    = WARP_FADE_OUT;
                    gWarpStep     = 0;
                    gWarpStepTimer = FADE_TICKS_PER_STEP;
                    return;
                }
            }
            NPC_Update();
            if (!Text_IsOpen()) {
                Map_BuildScrollView();
                OaksLabScripts_PostRender();
            }
            /* Always rebuild NPC OAM — the window layer masks sprites under the
             * text box, so calling this while text is open is safe and ensures
             * the rival sprite is visible above the box throughout the sequence. */
            NPC_BuildView(gScrollPxX, gScrollPxY);
            return;
        }
    }

    /* ---- Viridian Mart parcel-giving sequence ----------------------- *
     * Mirrors ViridianMartDefaultScript: entry text → auto-walk to      *
     * counter → parcel quest text → give OAK's PARCEL.                  */
    {
        ViridianMartScripts_Tick();
        if (ViridianMartScripts_IsActive()) {
            Player_Update();
            NPC_Update();
            if (!Text_IsOpen()) {
                Map_BuildScrollView();
                NPC_BuildView(gScrollPxX, gScrollPxY);
            }
            return;
        }
    }

    /* ---- Gym scripts (gym leader pre/post-battle text) ------------- */
    {
        GymScripts_Tick();
        {
            uint8_t tr_class, tr_no;
            if (GymScripts_GetPendingBattle(&tr_class, &tr_no)) {
                int player_level = 5;
                for (int i = 0; i < wPartyCount; i++) {
                    if (wPartyMons[i].base.hp > 0) {
                        player_level = wPartyMons[i].level;
                        break;
                    }
                }
                gEngagedTrainerClass = tr_class;
                gEngagedTrainerNo    = tr_no;
                memset(wShadowOAM + 4, 0, (MAX_SPRITES - 4) * sizeof(wShadowOAM[0]));
                Music_Play(wGymLeaderNo ? MUSIC_GYM_LEADER_BATTLE : MUSIC_TRAINER_BATTLE);
                gPendingTrainerBattle = 1;
                BattleTransition_Start(1, 0, player_level);
                gScene = SCENE_BTRANS;
                return;
            }
        }
        if (GymScripts_IsActive()) {
            NPC_Update();
            if (!Text_IsOpen()) {
                Map_BuildScrollView();
                NPC_BuildView(gScrollPxX, gScrollPxY);
            }
            return;
        }
    }

    /* ---- Cerulean City rival battle script ----------------------------- */
    {
        CeruleanScripts_Tick();
        {
            uint8_t tr_class, tr_no;
            if (CeruleanScripts_GetPendingBattle(&tr_class, &tr_no)) {
                int player_level = 5;
                for (int i = 0; i < wPartyCount; i++) {
                    if (wPartyMons[i].base.hp > 0) {
                        player_level = wPartyMons[i].level;
                        break;
                    }
                }
                gEngagedTrainerClass = tr_class;
                gEngagedTrainerNo    = tr_no;
                memset(wShadowOAM + 4, 0, (MAX_SPRITES - 4) * sizeof(wShadowOAM[0]));
                Music_Play(MUSIC_TRAINER_BATTLE);
                gPendingTrainerBattle = 1;
                BattleTransition_Start(1, 0, player_level);
                gScene = SCENE_BTRANS;
                return;
            }
        }
        if (CeruleanScripts_IsActive()) {
            NPC_Update();
            if (!Text_IsOpen()) {
                Map_BuildScrollView();
                NPC_BuildView(gScrollPxX, gScrollPxY);
            }
            return;
        }
    }

    /* ---- Route 24 Rocket scripted encounter ----------------------------- */
    {
        Route24Scripts_Tick();
        {
            uint8_t tr_class, tr_no;
            if (Route24Scripts_GetPendingBattle(&tr_class, &tr_no)) {
                int player_level = 5;
                for (int i = 0; i < wPartyCount; i++) {
                    if (wPartyMons[i].base.hp > 0) {
                        player_level = wPartyMons[i].level;
                        break;
                    }
                }
                gEngagedTrainerClass = tr_class;
                gEngagedTrainerNo    = tr_no;
                memset(wShadowOAM + 4, 0, (MAX_SPRITES - 4) * sizeof(wShadowOAM[0]));
                Music_Play(MUSIC_TRAINER_BATTLE);
                gPendingTrainerBattle = 1;
                BattleTransition_Start(1, 0, player_level);
                gScene = SCENE_BTRANS;
                return;
            }
        }
        if (Route24Scripts_IsActive()) {
            NPC_Update();
            if (!Text_IsOpen()) {
                Map_BuildScrollView();
                NPC_BuildView(gScrollPxX, gScrollPxY);
            }
            return;
        }
    }

    /* ---- Bill's House transformation script ---------------------------- */
    {
        BillsHouseScripts_Tick();
        if (BillsHouseScripts_IsActive()) {
            NPC_Update();
            if (!Text_IsOpen()) {
                Map_BuildScrollView();
                NPC_BuildView(gScrollPxX, gScrollPxY);
            }
            return;
        }
    }

    /* START: open pause menu (only when player is idle, no warp in progress) */
    if ((hJoyPressed & PAD_START) && !Player_IsMoving() && gWarpPhase == WARP_NONE) {
        Audio_PlaySFX_StartMenu();
        Menu_Open();
        return;
    }

    check_item_pickup();
    /* Mt. Moon fossil sequence: tick runs before text check so yes/no
     * state machine can process input.  PostRender redraws the box
     * after Map_BuildScrollView overwrites the tilemap (same as oakslab). */
    if (MtMoon_IsActive()) {
        MtMoon_Tick();
        NPC_Update();           /* needed for Super Nerd scripted walk */
        Map_BuildScrollView();
        MtMoon_PostRender();
        NPC_BuildView(gScrollPxX, gScrollPxY);
        return;
    }
    if (Text_IsOpen()) return;
    if (!Gate_PewterIsActive())
        check_npc_interact();
    /* If the NPC script just activated the mart, pokecenter, or oakslab
     * script (e.g. ball callback), stop overworld processing immediately. */
    if (Pokemart_IsActive() || Pokecenter_IsActive() || OaksLabScripts_IsActive() || ViridianMartScripts_IsActive()) return;
    /* Flush OAM positions so NPC_FacePlayer's tile/flag changes are applied
     * before the dialog box renders. Player is idle so scroll is 0. */
    NPC_BuildView(0, 0);
    if (Text_IsOpen()) return;
    check_hidden_event();
    if (Text_IsOpen()) return;
    check_sign_interact();
    if (Text_IsOpen()) return;

    /* Door step-out: fire once on the first normal frame after arriving on a
     * door tile (mirrors PlayerStepOutFromDoor / BIT_STANDING_ON_DOOR). */
    if (Warp_HasDoorStep()) {
        Player_ForceStepDown();
    }

    /* Deferred south push from Viridian old man block — runs here so
     * begin_step's camera update is in sync with Map_BuildScrollView below,
     * preventing the scroll snap that occurs when text is open. */
    Gate_ViridianDoPush();
    Gate_PewterTick();

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

    /* Encounter check mirrors CheckForBattleAfterStep: only fires on step completion,
     * not every frame.  This prevents immediate re-trigger after a battle ends. */
    if (gStepJustCompleted) {
        gStepJustCompleted = 0;
        /* Position triggers that fire on step arrival (mirrors map scripts
         * that check wXCoord/wYCoord every frame in the original). */
        Gate_ViridianStepCheck();
        Gate_PewterEastCheck();
        MtMoon_StepCheck();
        CeruleanScripts_StepCheck();
        Route24Scripts_StepCheck();
        if (Text_IsOpen()) return;
        /* Trainer sight check has priority over wild encounters (mirrors
         * CheckFightingMapTrainers running before CheckForBattleAfterStep). */
        Trainer_CheckSight();
        if (!Trainer_IsEngaging())
            check_wild_encounter();
    }
    /* check_wild_encounter may have flipped gScene to SCENE_BATTLE.
     * Skip the map rebuild in that case — the battle UI owns the tilemap now. */
    if (gScene == SCENE_OVERWORLD) {
        Map_BuildScrollView();
        NPC_BuildView(gScrollPxX, gScrollPxY);
    }
}
