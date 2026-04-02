/* cerulean_scripts.c — Cerulean City rival battle sequence.
 *
 * Ports scripts/CeruleanCity.asm:
 *   CeruleanCityDefaultScript        — step trigger + rival walk-in
 *   CeruleanCityRivalBattleScript    — pre-battle text + battle init
 *   CeruleanCityRivalDefeatedScript  — post-battle text + walk-away
 *   CeruleanCityRivalCleanupScript   — hide rival + restore music
 *
 * Trigger: player steps on (20,6) or (21,6) in Cerulean City (map 3)
 * while EVENT_BEAT_CERULEAN_RIVAL is not set.
 */
#include "cerulean_scripts.h"
#include "text.h"
#include "npc.h"
#include "player.h"
#include "overworld.h"
#include "music.h"
#include "../platform/hardware.h"
#include "../data/event_constants.h"
#include "constants.h"
#include <stdio.h>

/* ---- Constants -------------------------------------------------------- */

#define MAP_CERULEAN_CITY  3

/* NPC index in kNpcs_CeruleanCity[] (CERULEANCITY_RIVAL = 0) */
#define RIVAL_NPC_IDX   0

/* Trainer class 25 = RIVAL1 */
#define RIVAL1_CLASS    25

/* Walk direction codes matching NPC_DoScriptedStep:
 * 0=DOWN, 1=UP, 2=LEFT, 3=RIGHT */
#define DIR_DOWN  0
#define DIR_UP    1
#define DIR_LEFT  2
#define DIR_RIGHT 3

/* Y coord rival reaches before facing player (one tile above player at y=6) */
#define RIVAL_STOP_Y  5

/* ---- State ------------------------------------------------------------- */

typedef enum {
    CS_IDLE = 0,
    CS_WALK_IN,           /* rival walking 3 steps DOWN toward player */
    CS_PRE_BATTLE_TEXT,   /* showing pre-battle dialogue */
    CS_BATTLE_PENDING,    /* text done; waiting for game.c to start battle */
    CS_POST_BATTLE_TEXT,  /* battle over; showing "I went to Bill's" text */
    CS_WALK_AWAY,         /* rival walking off screen */
    CS_CLEANUP,           /* walk done; hide rival, restore music */
} CeruleanState;

static CeruleanState g_state              = CS_IDLE;
static int           g_pending_battle     = 0;  /* battle ready for game.c */
static int           g_rival_battle_active = 0; /* set while battle in progress */
static uint8_t       g_rival_tr_no        = 0;
static int           g_player_x_at_trigger = 20; /* saved at trigger time */
static int           g_walk_steps_left    = 0;
static int           g_post_text_shown    = 0;
static int           g_rival_stop_x       = 20; /* position rival reached before battle */
static int           g_rival_stop_y       = RIVAL_STOP_Y;

/* ---- Text strings ----------------------------------------------------- */

/* Pre-battle — _CeruleanCityRivalPreBattleText */
static const char kPreBattle[] =
    "{RIVAL}: Yo!\n{PLAYER}!\f"
    "You're still\nstruggling along\nback here?\f"
    "I'm doing great!\nI caught a bunch\nof strong and\nsmart POKEMON!\f"
    "Here, let me see\nwhat you caught,\n{PLAYER}!";

/* Post-battle — _CeruleanCityRivalIWentToBillsText */
static const char kIWentToBills[] =
    "{RIVAL}: Hey,\nguess what?\f"
    "I went to BILL's\nand got him to\nshow me his rare\nPOKEMON!\f"
    "That added a lot\nof pages to my\nPOKEDEX!\f"
    "After all, BILL's\nworld famous as a\nPOKEMANIAC!\f"
    "He invented the\nPOKEMON Storage\nSystem on PC!\f"
    "Since you're using\nhis system, go\nthank him!\f"
    "Well, I better\nget rolling!\nSmell ya later!";

/* ---- Public API ------------------------------------------------------- */

void CeruleanScripts_OnMapLoad(void) {
    if (wCurMap != MAP_CERULEAN_CITY) return;

    /* Preserve post-battle state across the map reload game.c does after
     * a battle ends — those states drive the walk-away sequence.
     * Re-show and reposition the rival since NPC_Load() reset him to (20,2). */
    if (g_state == CS_POST_BATTLE_TEXT ||
        g_state == CS_WALK_AWAY        ||
        g_state == CS_CLEANUP) {
        NPC_ShowSprite(RIVAL_NPC_IDX);
        NPC_SetTilePos(RIVAL_NPC_IDX, g_rival_stop_x, g_rival_stop_y);
        return;
    }

    g_state = CS_IDLE;
    /* Rival is hidden until triggered.  After beating him the sprite walks
     * away and is hidden; on map re-enter he simply doesn't appear (flag set). */
    NPC_HideSprite(RIVAL_NPC_IDX);
}

void CeruleanScripts_StepCheck(void) {
    if (wCurMap != MAP_CERULEAN_CITY) return;
    if (g_state != CS_IDLE) return;
    if (CheckEvent(EVENT_BEAT_CERULEAN_RIVAL)) return;

    /* Trigger coords: (20,6) or (21,6) — the two bridge tiles */
    if ((int)wYCoord != 6) return;
    if ((int)wXCoord != 20 && (int)wXCoord != 21) return;

    printf("[cerulean] step trigger: rival encounter at (%d,%d)\n",
           (int)wXCoord, (int)wYCoord);

    g_player_x_at_trigger = (int)wXCoord;

    /* ASM: if player X != 20, shift rival's X to 21 (other bridge lane).
     * Mirrors the SPRITESTATEDATA2_MAPX = 25 adjustment in the original. */
    if (g_player_x_at_trigger != 20) {
        NPC_SetTilePos(RIVAL_NPC_IDX, 21, 2);
    }

    /* Show rival sprite and play meet-rival music */
    NPC_ShowSprite(RIVAL_NPC_IDX);
    Music_Play(MUSIC_MEET_RIVAL);

    /* Start first step DOWN — remaining steps driven in CS_WALK_IN */
    NPC_DoScriptedStep(RIVAL_NPC_IDX, DIR_DOWN);
    g_state = CS_WALK_IN;
}

int CeruleanScripts_IsActive(void) { return g_state != CS_IDLE; }

int CeruleanScripts_GetPendingBattle(uint8_t *class_out, uint8_t *no_out) {
    if (!g_pending_battle) return 0;
    g_pending_battle = 0;
    *class_out = RIVAL1_CLASS;
    *no_out    = g_rival_tr_no;
    g_rival_battle_active = 1;
    return 1;
}

int CeruleanScripts_ConsumeRivalBattle(void) {
    if (!g_rival_battle_active) return 0;
    g_rival_battle_active = 0;
    return 1;
}

void CeruleanScripts_OnVictory(void) {
    SetEvent(EVENT_BEAT_CERULEAN_RIVAL);
    /* Mirrors CeruleanCityRivalDefeatedScript: stop music then play
     * Music_RivalAlternateStart (map music from loop, skipping intro). */
    Music_PlayFromLoop(Music_GetMapID(wCurMap));
    g_post_text_shown = 0;
    g_state = CS_POST_BATTLE_TEXT;
    printf("[cerulean] rival defeated — post-battle sequence\n");
}

void CeruleanScripts_OnDefeat(void) {
    /* Player lost — hide rival, reset (normal overworld handles loss flow) */
    NPC_HideSprite(RIVAL_NPC_IDX);
    g_state = CS_IDLE;
}

void CeruleanScripts_Tick(void) {
    if (g_state == CS_IDLE || g_state == CS_BATTLE_PENDING) return;

    switch (g_state) {

    case CS_WALK_IN: {
        if (NPC_IsWalking(RIVAL_NPC_IDX)) return;
        int rx, ry;
        NPC_GetTilePos(RIVAL_NPC_IDX, &rx, &ry);
        if (ry < RIVAL_STOP_Y) {
            NPC_DoScriptedStep(RIVAL_NPC_IDX, DIR_DOWN);
            return;
        }
        /* Arrived — save stop position (needed to restore after NPC_Load) */
        g_rival_stop_x = rx;
        g_rival_stop_y = ry;
        /* Show pre-battle text */
        Text_ShowASCII(kPreBattle);
        g_state = CS_PRE_BATTLE_TEXT;
        return;
    }

    case CS_PRE_BATTLE_TEXT:
        if (Text_IsOpen()) { Text_Update(); return; }
        /* Text dismissed — select party based on rival's starter */
        if      (wRivalStarter == STARTER2) g_rival_tr_no = 7;  /* rival has Squirtle */
        else if (wRivalStarter == STARTER3) g_rival_tr_no = 8;  /* rival has Bulbasaur */
        else                                g_rival_tr_no = 9;  /* rival has Charmander */
        g_pending_battle = 1;
        g_state = CS_BATTLE_PENDING;
        return;

    case CS_POST_BATTLE_TEXT:
        if (!g_post_text_shown) {
            Text_ShowASCII(kIWentToBills);
            g_post_text_shown = 1;
            return;
        }
        if (Text_IsOpen()) { Text_Update(); return; }
        /* Text done — rival walks away.
         * ASM Movement3: LEFT + 6xDOWN (player on right lane, X=21)
         * ASM Movement4: RIGHT + 6xDOWN (player on left lane, X=20) */
        {
            int side_dir = (g_player_x_at_trigger == 20) ? DIR_RIGHT : DIR_LEFT;
            NPC_DoScriptedStep(RIVAL_NPC_IDX, side_dir);
            g_walk_steps_left = 6;
        }
        g_state = CS_WALK_AWAY;
        return;

    case CS_WALK_AWAY:
        if (NPC_IsWalking(RIVAL_NPC_IDX)) return;
        if (g_walk_steps_left > 0) {
            NPC_DoScriptedStep(RIVAL_NPC_IDX, DIR_DOWN);
            g_walk_steps_left--;
            return;
        }
        g_state = CS_CLEANUP;
        return;

    case CS_CLEANUP:
        NPC_HideSprite(RIVAL_NPC_IDX);
        /* ASM CeruleanCityRivalCleanupScript calls PlayDefaultMusic here.
         * Music_RivalAlternateStart already started in OnVictory, so just
         * restart map music normally to match PlayDefaultMusic behaviour. */
        Music_Play(Music_GetMapID(wCurMap));
        g_state = CS_IDLE;
        printf("[cerulean] rival script complete\n");
        return;

    default:
        g_state = CS_IDLE;
        return;
    }
}
