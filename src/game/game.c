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
#include "battle/battle_trainer.h"
#include "battle/battle_driver.h"
#include "battle/battle_ui.h"
#include "battle_transition.h"
#include "trainer_sight.h"
#include "party_menu.h"
#include "pokecenter.h"
#include "pokemart.h"
#include "music.h"
#include "../platform/audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Scene state ------------------------------------------------- */
typedef enum {
    SCENE_OVERWORLD = 0,
    SCENE_BTRANS,       /* battle transition animation */
    SCENE_BATTLE,
    SCENE_MENU,         /* future */
} GameScene;

static GameScene gScene = SCENE_OVERWORLD;

/* Debug spawn: Viridian City (map 1), in front of the mart. */
#define MAP_PALLET_TOWN     0x01

#define PLAYER_START_X      58
#define PLAYER_START_Y      41

void GameInit(void) {
    extern void WRAMClear(void);
    WRAMClear();

    int save_ok = (Save_Load() == 0);
    if (save_ok)
        printf("[save] Save loaded OK — map %d @ (%d,%d)\n", wCurMap, wXCoord, wYCoord);
    else {
        printf("[save] No save, starting new game\n");
        wCurMap = MAP_PALLET_TOWN;
        wXCoord = PLAYER_START_X;
        wYCoord = PLAYER_START_Y;
    }

    Map_Load(wCurMap);
    Font_Load();
    Player_Init(wXCoord, wYCoord);
    NPC_Load();
    Trainer_LoadMap();
    Map_BuildScrollView();
    NPC_BuildView(0, 0);

    /* Give player starters if no save was loaded (new game only). */
    if (!save_ok && wPartyCount == 0) {
        wPlayerID = 12345;   /* test trainer ID — would be randomised in a full game */

        Pokemon_InitMon(&wPartyMons[0], SPECIES_BULBASAUR,  6);
        /* Give Bulbasaur Absorb (move 71) for drain testing — replaces Growl slot */
        wPartyMons[0].base.moves[1] = 71;
        wPartyMons[0].base.pp[1]    = 20;
        Pokemon_InitMon(&wPartyMons[1], SPECIES_CHARMANDER, 6);
        Pokemon_InitMon(&wPartyMons[2], SPECIES_SQUIRTLE,   6);
        wPartyCount = 3;

        /* Debug: max money for testing */
        wPlayerMoney[0] = 0x99;
        wPlayerMoney[1] = 0x99;
        wPlayerMoney[2] = 0x99;

        /* Copy player name to OT field for each starter
         * (mirrors SetPartyMonOT / CopyPlayerNameToStringBuffer in pokered) */
        for (int i = 0; i < 3; i++)
            memcpy(wPartyMonOT[i], wPlayerName, NAME_LENGTH);

        printf("[party] Started with Bulbasaur (Tackle/Absorb), Charmander, Squirtle (all Lv.6)\n");
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

/* Set to 1 before BattleTransition_Start when the pending battle is a trainer
 * battle.  Checked when transition completes to call Battle_StartTrainer
 * instead of Battle_Start. */
static int gPendingTrainerBattle = 0;

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

    BattleTransition_Start(0, wCurEnemyLevel, player_level, 0);
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

    /* Search within ±2 coords of the facing tile.
     * Our tile coords are doubled (asm_x*2), so ±2 = 1 asm tile in each direction.
     * This range is needed for counter NPCs (e.g. nurse): the player stands at the
     * counter tile (1 asm tile away from the nurse), so the facing tile is the
     * counter, not the nurse — ±2 bridges that extra tile. */
    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            int i = NPC_FindAtTile(fx + dx, fy + dy);
            if (i < 0 || i >= ev->num_npcs) continue;
            NPC_FacePlayer(i);

            /* Trainer NPC: bypass normal text dispatch.
             * Defeated → show after_text.  Undefeated → engage immediately.
             * Mirrors CheckInteractionWithMapTrainer (home/trainers.asm). */
            if (ev->trainers) {
                int is_trainer = 0;
                for (int ti = 0; ti < ev->num_trainers; ti++) {
                    const map_trainer_t *tr = &ev->trainers[ti];
                    if (tr->npc_idx != i) continue;
                    is_trainer = 1;
                    if (CheckEvent(tr->flag_bit)) {
                        if (tr->after_text) Text_ShowASCII(tr->after_text);
                    } else {
                        Trainer_EngageImmediate(i);
                    }
                    break;
                }
                if (is_trainer) return;
            }

            if (ev->npcs[i].script)
                ev->npcs[i].script();
            else if (ev->npcs[i].text)
                Text_ShowASCII(ev->npcs[i].text);
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
        /* Save battle type before Tick() resets wIsInBattle to 0 (BUI_END case). */
        int was_trainer_battle = (wIsInBattle == 2);
        BattleUI_Tick();
        if (!BattleUI_IsActive()) {
            /* Battle ended — restore overworld GFX and state.
             * Font_LoadHudTiles() overwrote tile_gfx slots 2-24 during battle;
             * Map_ReloadGfx() restores the tileset so the map renders correctly.
             * Map_ResetScrollState() ensures the next Map_BuildScrollView does a
             * full rebuild even if gScrollViewReady was set when battle triggered.
             * gStepJustCompleted cleared so check_wild_encounter doesn't fire
             * immediately on the first overworld frame after battle. */

            /* Mark trainer as defeated if the player won a trainer battle.
             * "Player won" = all enemy mons fainted. */
            if (was_trainer_battle && !Battle_AnyEnemyPokemonAliveCheck())
                Trainer_MarkCurrentDefeated();

            gScene = SCENE_OVERWORLD;
            gStepJustCompleted = 0;
            memset(wShadowOAM, 0, sizeof(wShadowOAM));
            Display_SetPalette(0xE4, 0xD0, 0xE0);
            Map_ReloadGfx();
            Map_ResetScrollState();
            Font_Load();   /* restore font/box tiles (120-126, 128-255) */
            Music_Play(Music_GetMapID(wCurMap));
            NPC_Load();
            Trainer_LoadMap();
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
                Trainer_LoadMap();
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
            Music_Play(MUSIC_TRAINER_BATTLE);
            gPendingTrainerBattle = 1;
            BattleTransition_Start(0, 0, player_level, 0);
            gScene = SCENE_BTRANS;
        }
        return;
    }

    /* START: open pause menu (only when player is idle, no warp in progress) */
    if ((hJoyPressed & PAD_START) && !Player_IsMoving() && gWarpPhase == WARP_NONE) {
        Audio_PlaySFX_StartMenu();
        Menu_Open();
        return;
    }

    check_item_pickup();
    if (Text_IsOpen()) return;
    check_npc_interact();
    /* If the NPC script just activated the mart or pokecenter, stop overworld
     * processing immediately.  They will call NPC_HideInRect before any
     * NPC_BuildView runs, so stopping here prevents the end-of-tick
     * NPC_BuildView from undoing the selective hide on this same frame. */
    if (Pokemart_IsActive() || Pokecenter_IsActive()) return;
    /* Flush OAM positions so NPC_FacePlayer's tile/flag changes are applied
     * before the dialog box renders. Player is idle so scroll is 0. */
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
    /* Encounter check mirrors CheckForBattleAfterStep: only fires on step completion,
     * not every frame.  This prevents immediate re-trigger after a battle ends. */
    if (gStepJustCompleted) {
        gStepJustCompleted = 0;
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
