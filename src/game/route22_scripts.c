/* route22_scripts.c — Route 22 rival 1 encounter.
 *
 * Ports scripts/Route22.asm:
 *   Route22DefaultScript / Route22FirstRivalBattleScript /
 *   Route22Rival1StartBattleScript / Route22Rival1AfterBattleScript /
 *   Route22Rival1ExitScript
 *
 * Trigger tiles (from Route22DefaultScript.Route22RivalBattleCoords):
 *   (29,4) → coord_index=0   (29,5) → coord_index=1
 *
 * Walk approach (Route22MoveRivalRightScript / Route22RivalMovementData):
 *   coord_index=0: 3 RIGHT steps  → rival ends at (28,5), faces RIGHT
 *   coord_index=1: 4 RIGHT steps  → rival ends at (29,5), faces UP
 *
 * Exit sequences (Route22Rival1ExitMovementData1/2):
 *   coord_index=0: RIGHT RIGHT DOWN×5
 *   coord_index=1: UP RIGHT×3 DOWN×6
 *
 * Trainer numbers (OPP_RIVAL1 = class 25):
 *   STARTER2 (Squirtle)   → no 4
 *   STARTER3 (Bulbasaur)  → no 5
 *   STARTER1 (Charmander) → no 6
 */
#include "route22_scripts.h"
#include "text.h"
#include "npc.h"
#include "music.h"
#include "overworld.h"
#include "trainer_sight.h"
#include "../platform/hardware.h"
#include "../data/event_constants.h"
#include "constants.h"
#include <stdio.h>

/* ---- Constants ---------------------------------------------------- */

#define MAP_ROUTE22    0x21
#define RIVAL_NPC_IDX  0        /* ROUTE22_RIVAL1 — object_event index 0 */
#define RIVAL1_CLASS   25       /* OPP_RIVAL1 */

#define DIR_DOWN   0
#define DIR_UP     1
#define DIR_LEFT   2
#define DIR_RIGHT  3

/* ---- Text (from text/Route22.asm) --------------------------------- */

static const char kPreBattle[] =
    "{RIVAL}: Hey!\n{PLAYER}!\f"
    "You're going to\n#MON LEAGUE?\f"
    "Forget it! You\nprobably don't\nhave any BADGEs!\f"
    "The guard won't\nlet you through!\f"
    "By the way, did\nyour #MON\nget any stronger?";

static const char kAfterBattle[] =
    "I heard #MON\nLEAGUE has many\ntough trainers!\f"
    "I have to figure\nout how to get\npast them!\f"
    "You should quit\ndawdling and get\na move on!";

/* In-battle defeat quote — _Route22Rival1DefeatedText */
static const char kDefeatedText[] =
    "Awww!\nYou just lucked\nout!";

/* ---- Exit movement sequences -------------------------------------- */
/* Direction codes: 0=DOWN 1=UP 2=LEFT 3=RIGHT.  -1 = end. */

/* Route22Rival1ExitMovementData1 (coord_index=0): RIGHT×2 DOWN×5 */
static const int kExitSeq0[] = { 3, 3, 0, 0, 0, 0, 0, -1 };

/* Route22Rival1ExitMovementData2 (coord_index=1): UP RIGHT×3 DOWN×6 */
static const int kExitSeq1[] = { 1, 3, 3, 3, 0, 0, 0, 0, 0, 0, -1 };

/* ---- State -------------------------------------------------------- */

typedef enum {
    R22S_IDLE = 0,
    R22S_WALK,           /* rival walks RIGHT into view toward player */
    R22S_PRE_BATTLE,     /* pre-battle dialogue */
    R22S_BATTLE_PENDING, /* waiting for game.c to start battle */
    R22S_POST_BATTLE,    /* after-battle text + rival victory music */
    R22S_EXIT_WALK,      /* execute exit movement sequence */
    R22S_CLEANUP,        /* hide rival, reset events, restore music */
} Route22State;

static Route22State g_state          = R22S_IDLE;
static int          g_pending_battle = 0;
static int          g_battle_active  = 0;
static uint8_t      g_rival_tr_no    = 0;
static int          g_coord_index    = 0;   /* 0=player at y=4, 1=player at y=5 */
static int          g_walk_steps     = 0;
static int          g_rival_stop_x   = 0;  /* tile pos saved after walk completes */
static int          g_rival_stop_y   = 0;
static int          g_post_shown     = 0;
static const int   *g_exit_seq       = 0;
static int          g_exit_step      = 0;

/* ---- Map load ----------------------------------------------------- */

void Route22Scripts_OnMapLoad(void) {
    if (wCurMap != MAP_ROUTE22) return;

    /* Preserve post-battle walk state across the map reload after battle.
     * NPC_Load() resets sprites to default positions, so we must re-show
     * and reposition RIVAL1 to where he stood when battle started.
     * Always hide RIVAL2 — not used in this encounter. */
    if (g_state == R22S_POST_BATTLE ||
        g_state == R22S_EXIT_WALK   ||
        g_state == R22S_CLEANUP) {
        NPC_HideSprite(1);
        NPC_ShowSprite(RIVAL_NPC_IDX);
        NPC_SetTilePos(RIVAL_NPC_IDX, g_rival_stop_x, g_rival_stop_y);
        NPC_SetFacing(RIVAL_NPC_IDX, DIR_RIGHT);
        return;
    }

    /* Reset to idle — any leftover in-progress state is stale after a
     * fresh map load (e.g. from a debug checkpoint). */
    g_state = R22S_IDLE;

    /* Both rival sprites start hidden; RIVAL1 is shown only when the
     * scripted encounter triggers.  RIVAL2 is not used yet. */
    NPC_HideSprite(0);
    NPC_HideSprite(1);
}

/* ---- Step check --------------------------------------------------- */

void Route22Scripts_StepCheck(void) {
    if (wCurMap != MAP_ROUTE22) return;
    if (g_state != R22S_IDLE) return;

    if (!CheckEvent(EVENT_ROUTE22_RIVAL_WANTS_BATTLE)) return;
    if (!CheckEvent(EVENT_1ST_ROUTE22_RIVAL_BATTLE)) return;
    if (CheckEvent(EVENT_BEAT_ROUTE22_RIVAL_1ST_BATTLE)) return;

    if ((int)wXCoord != 29) return;
    int py = (int)wYCoord;
    if (py != 4 && py != 5) return;

    g_coord_index = (py == 5) ? 1 : 0;

    printf("[route22] rival 1 triggered at (%d,%d) coord_index=%d\n",
           29, py, g_coord_index);

    /* Place rival just off the left edge of the screen (x=18 when player
     * is at x=29, screen spans ~x=19-37), then show and walk him RIGHT
     * into view.  Mirrors the spirit of the original: rival appears from
     * the west and walks toward the player. */
    NPC_SetTilePos(RIVAL_NPC_IDX, 18, 5);
    NPC_ShowSprite(RIVAL_NPC_IDX);
    Music_Play(MUSIC_MEET_RIVAL);
    g_walk_steps = 10;   /* x=18 → x=28, stopping 1 tile in front of player */
    NPC_DoScriptedStep(RIVAL_NPC_IDX, DIR_RIGHT);
    g_walk_steps--;
    g_state = R22S_WALK;
}

/* ---- Public accessors --------------------------------------------- */

int Route22Scripts_IsActive(void) {
    /* Route 22's state machine should only suppress overworld processing
     * while the player is actually on Route 22. Otherwise stale post-battle
     * state can block unrelated map scripts that run later in game.c. */
    return wCurMap == MAP_ROUTE22 && g_state != R22S_IDLE;
}

int Route22Scripts_GetPendingBattle(uint8_t *class_out, uint8_t *no_out) {
    if (wCurMap != MAP_ROUTE22) return 0;
    if (!g_pending_battle) return 0;
    g_pending_battle = 0;
    *class_out = RIVAL1_CLASS;
    *no_out    = g_rival_tr_no;
    g_battle_active = 1;
    return 1;
}

int Route22Scripts_ConsumeRivalBattle(void) {
    if (!g_battle_active) return 0;
    g_battle_active = 0;
    return 1;
}

void Route22Scripts_OnVictory(void) {
    SetEvent(EVENT_BEAT_ROUTE22_RIVAL_1ST_BATTLE);
    /* ASM: farcall Music_RivalAlternateStart */
    Music_PlayFromLoop(Music_GetMapID(wCurMap));
    g_post_shown = 0;
    g_state = R22S_POST_BATTLE;
    printf("[route22] rival 1 defeated — post-battle\n");
}

void Route22Scripts_OnDefeat(void) {
    /* Player blacked out — hide rival, reset to idle */
    NPC_HideSprite(RIVAL_NPC_IDX);
    g_state = R22S_IDLE;
}

/* ---- Per-frame tick ----------------------------------------------- */

void Route22Scripts_Tick(void) {
    if (g_state == R22S_IDLE || g_state == R22S_BATTLE_PENDING) return;

    switch (g_state) {

    case R22S_WALK:
        if (NPC_IsWalking(RIVAL_NPC_IDX)) return;
        if (g_walk_steps > 0) {
            NPC_DoScriptedStep(RIVAL_NPC_IDX, DIR_RIGHT);
            g_walk_steps--;
            return;
        }
        /* Save position so OnMapLoad can restore it after the battle reload. */
        NPC_GetTilePos(RIVAL_NPC_IDX, &g_rival_stop_x, &g_rival_stop_y);
        /* Rival walked to x=28; player is at x=29 to the right — always face RIGHT. */
        NPC_SetFacing(RIVAL_NPC_IDX, DIR_RIGHT);
        Text_ShowASCII(kPreBattle);
        g_state = R22S_PRE_BATTLE;
        return;

    case R22S_PRE_BATTLE:
        if (Text_IsOpen()) return;
        /* Pick trainer party by rival's starter (Route22Rival1StartBattleScript.StarterTable) */
        if      (wRivalStarter == STARTER2) g_rival_tr_no = 4;
        else if (wRivalStarter == STARTER3) g_rival_tr_no = 5;
        else                                g_rival_tr_no = 6;
        /* SaveEndBattleTextPointers in Route22.asm: show the short defeat
         * quote in the battle scene before the overworld after-battle text. */
        gTrainerAfterText = kDefeatedText;
        g_pending_battle = 1;
        g_state = R22S_BATTLE_PENDING;
        return;

    case R22S_POST_BATTLE:
        /* Route22Rival1AfterBattleScript: show after-battle text */
        if (!g_post_shown) {
            Text_ShowASCII(kAfterBattle);
            g_post_shown = 1;
            return;
        }
        if (Text_IsOpen()) return;
        /* Begin exit walk */
        g_exit_seq  = (g_coord_index == 1) ? kExitSeq1 : kExitSeq0;
        g_exit_step = 0;
        NPC_DoScriptedStep(RIVAL_NPC_IDX, g_exit_seq[g_exit_step++]);
        g_state = R22S_EXIT_WALK;
        return;

    case R22S_EXIT_WALK:
        if (NPC_IsWalking(RIVAL_NPC_IDX)) return;
        if (g_exit_seq[g_exit_step] != -1) {
            NPC_DoScriptedStep(RIVAL_NPC_IDX, g_exit_seq[g_exit_step++]);
            return;
        }
        g_state = R22S_CLEANUP;
        return;

    case R22S_CLEANUP:
        /* Route22Rival1ExitScript: hide rival, restore music, reset events */
        NPC_HideSprite(RIVAL_NPC_IDX);
        Music_Play(Music_GetMapID(wCurMap));
        ClearEvent(EVENT_1ST_ROUTE22_RIVAL_BATTLE);
        ClearEvent(EVENT_ROUTE22_RIVAL_WANTS_BATTLE);
        g_state = R22S_IDLE;
        printf("[route22] rival 1 exit complete\n");
        return;

    default:
        g_state = R22S_IDLE;
        return;
    }
}
