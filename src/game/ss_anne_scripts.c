/* ss_anne_scripts.c — SS Anne story scripts.
 *
 * Ports scripts/SSAnne2F.asm (rival battle) and
 * scripts/SSAnneCaptainsRoom.asm (HM01 Cut sequence).
 *
 * Rival trigger: player steps to (33-35, 4) on SS_ANNE_2F (map 0x60)
 * while EVENT_BEAT_SS_ANNE_RIVAL is not set.
 *
 * Captain trigger: player presses A on captain NPC at (4,2) in
 * SS_ANNE_CAPTAINS_ROOM (map 0x65) — fires SSAnne_CaptainScript().
 */
#include "ss_anne_scripts.h"
#include "text.h"
#include "npc.h"
#include "player.h"
#include "overworld.h"
#include "music.h"
#include "inventory.h"
#include "../platform/hardware.h"
#include "../data/event_constants.h"
#include "constants.h"
#include <stdio.h>

/* ---- Map IDs ---------------------------------------------------------- */
#define MAP_SS_ANNE_2F          0x60
#define MAP_SS_ANNE_CAPTAINS_ROOM 0x65

/* ---- NPC indices ------------------------------------------------------- */
/* kNpcs_SSAnne2F: 0=Waiter, 1=Rival */
#define RIVAL_NPC_IDX   1

/* ---- Trainer class ----------------------------------------------------- */
/* Rival2 = class 42 in gTrainerPartyData */
#define RIVAL2_CLASS    42

/* ---- Direction codes --------------------------------------------------- */
#define DIR_DOWN  0
#define DIR_UP    1
#define DIR_LEFT  2
#define DIR_RIGHT 3

/* ---- HM item ID -------------------------------------------------------- */
#define HM01  0xC4

/* ============================================================
 * RIVAL STATE MACHINE
 * ============================================================ */

typedef enum {
    RS_IDLE = 0,
    RS_PRE_BATTLE_TEXT,   /* showing pre-battle dialogue */
    RS_BATTLE_PENDING,    /* text done; waiting for game.c to start battle */
    RS_POST_BATTLE_TEXT,  /* battle over; showing post-battle text */
    RS_WALK_AWAY,         /* rival walking off screen */
    RS_CLEANUP,           /* hide rival, restore music */
} RivalState;

static RivalState g_rival_state         = RS_IDLE;
static int        g_pending_battle      = 0;
static int        g_rival_battle_active = 0;
static uint8_t    g_rival_tr_no         = 0;
static int        g_walk_steps_left     = 0;
static int        g_post_text_shown     = 0;

/* ---- Rival text ------------------------------------------------------- */

/* SSAnne2FRivalText (pre-battle) */
static const char kRivalPreBattle[] =
    "{RIVAL}: Hey!\n{PLAYER}!\f"
    "Weren't you\ninvited here?\f"
    "Let me check\nyour POKEMON\nout!";

/* SSAnne2FRivalDefeatedText (post-battle) */
static const char kRivalPostBattle[] =
    "{RIVAL}: Hah!\nI won't lose\nnext time!\f"
    "I'm going to\nsee the CAPTAIN\nnow!\f"
    "Later, {PLAYER}!";

/* ============================================================
 * CAPTAIN STATE MACHINE
 * ============================================================ */

typedef enum {
    CS_IDLE = 0,
    CS_CAPTAIN_PRE_TEXT,   /* showing "captain is sick" text */
    CS_CAPTAIN_HM_TEXT,    /* showing "take this HM" text */
    CS_CAPTAIN_DONE,       /* item given, clean up */
} CaptainState;

static CaptainState g_captain_state    = CS_IDLE;
static int          g_captain_text_seq = 0;

/* ---- Captain text ----------------------------------------------------- */

static const char kCaptainSick[] =
    "CAPTAIN: Ohhh...\nI'm so seasick.\f"
    "Rub my back\nwill ya?";

static const char kCaptainBetter[] =
    "CAPTAIN: Ohhhh!\nThat feels so\ngood!\f"
    "I needed that!\nTake this as\nthanks!";

static const char kCaptainAlreadyBetter[] =
    "CAPTAIN: I feel\nmuch better\nnow. Thanks!";

/* ============================================================
 * PUBLIC API
 * ============================================================ */

void SSAnneScripts_OnMapLoad(void) {
    if (wCurMap == MAP_SS_ANNE_2F) {
        /* Rival is hidden until triggered (or gone after defeat). */
        if (g_rival_state == RS_POST_BATTLE_TEXT ||
            g_rival_state == RS_WALK_AWAY        ||
            g_rival_state == RS_CLEANUP) {
            /* Map reload during post-battle walk sequence — keep rival shown */
            NPC_ShowSprite(RIVAL_NPC_IDX);
            return;
        }
        g_rival_state = RS_IDLE;
        if (!CheckEvent(EVENT_BEAT_SS_ANNE_RIVAL))
            NPC_ShowSprite(RIVAL_NPC_IDX);
        else
            NPC_HideSprite(RIVAL_NPC_IDX);
    }
}

void SSAnneScripts_StepCheck(void) {
    /* Rival battle trigger on SS_ANNE_2F */
    if (wCurMap != MAP_SS_ANNE_2F) return;
    if (g_rival_state != RS_IDLE) return;
    if (CheckEvent(EVENT_BEAT_SS_ANNE_RIVAL)) return;

    /* Rival stands at (36, 4); trigger when player approaches within 3 tiles */
    if ((int)wYCoord != 4) return;
    if ((int)wXCoord < 33 || (int)wXCoord > 35) return;

    printf("[ss_anne] rival trigger at (%d,%d)\n", (int)wXCoord, (int)wYCoord);

    NPC_ShowSprite(RIVAL_NPC_IDX);
    Music_Play(MUSIC_MEET_RIVAL);
    g_rival_state = RS_PRE_BATTLE_TEXT;
    Text_ShowASCII(kRivalPreBattle);
}

int SSAnneScripts_IsActive(void) {
    return g_rival_state != RS_IDLE || g_captain_state != CS_IDLE;
}

int SSAnneScripts_GetPendingBattle(uint8_t *class_out, uint8_t *no_out) {
    if (!g_pending_battle) return 0;
    g_pending_battle = 0;
    *class_out = RIVAL2_CLASS;
    *no_out    = g_rival_tr_no;
    g_rival_battle_active = 1;
    return 1;
}

int SSAnneScripts_ConsumeRivalBattle(void) {
    if (!g_rival_battle_active) return 0;
    g_rival_battle_active = 0;
    return 1;
}

void SSAnneScripts_OnVictory(void) {
    SetEvent(EVENT_BEAT_SS_ANNE_RIVAL);
    Music_PlayFromLoop(Music_GetMapID(wCurMap));
    g_post_text_shown = 0;
    g_rival_state = RS_POST_BATTLE_TEXT;
    printf("[ss_anne] rival defeated\n");
}

void SSAnneScripts_OnDefeat(void) {
    g_rival_state = RS_IDLE;
}

/* Captain A-press callback — wired into event_data.c */
void SSAnne_CaptainScript(void) {
    if (g_captain_state != CS_IDLE) return;  /* already running */
    if (CheckEvent(EVENT_GOT_HM01)) {
        Text_ShowASCII(kCaptainAlreadyBetter);
        return;
    }
    g_captain_state    = CS_CAPTAIN_PRE_TEXT;
    g_captain_text_seq = 0;
    Text_ShowASCII(kCaptainSick);
}

void SSAnneScripts_Tick(void) {
    /* ---- Rival state machine ---- */
    switch (g_rival_state) {

    case RS_IDLE:
        break;

    case RS_PRE_BATTLE_TEXT:
        if (Text_IsOpen()) { Text_Update(); return; }
        /* Text dismissed — select party based on rival's starter */
        if      (wRivalStarter == STARTER2) g_rival_tr_no = 2;
        else if (wRivalStarter == STARTER3) g_rival_tr_no = 3;
        else                                g_rival_tr_no = 1;
        g_pending_battle  = 1;
        g_rival_state     = RS_BATTLE_PENDING;
        return;

    case RS_BATTLE_PENDING:
        /* game.c polls GetPendingBattle() and starts the battle */
        return;

    case RS_POST_BATTLE_TEXT:
        if (!g_post_text_shown) {
            Text_ShowASCII(kRivalPostBattle);
            g_post_text_shown = 1;
            return;
        }
        if (Text_IsOpen()) { Text_Update(); return; }
        /* Walk rival UP and off-screen (toward captain's room) */
        NPC_DoScriptedStep(RIVAL_NPC_IDX, DIR_UP);
        g_walk_steps_left = 5;
        g_rival_state = RS_WALK_AWAY;
        return;

    case RS_WALK_AWAY:
        if (NPC_IsWalking(RIVAL_NPC_IDX)) return;
        if (g_walk_steps_left > 0) {
            NPC_DoScriptedStep(RIVAL_NPC_IDX, DIR_UP);
            g_walk_steps_left--;
            return;
        }
        g_rival_state = RS_CLEANUP;
        return;

    case RS_CLEANUP:
        NPC_HideSprite(RIVAL_NPC_IDX);
        Music_Play(Music_GetMapID(wCurMap));
        g_rival_state = RS_IDLE;
        printf("[ss_anne] rival script complete\n");
        return;
    }

    /* ---- Captain state machine ---- */
    switch (g_captain_state) {

    case CS_IDLE:
        break;

    case CS_CAPTAIN_PRE_TEXT:
        if (Text_IsOpen()) { Text_Update(); return; }
        /* Pre-text done — play healing jingle, then give item text */
        Music_Play(MUSIC_PKMN_HEALED);
        Text_ShowASCII(kCaptainBetter);
        g_captain_state = CS_CAPTAIN_HM_TEXT;
        return;

    case CS_CAPTAIN_HM_TEXT:
        /* Wait for jingle to finish and text to close */
        if (Text_IsOpen()) { Text_Update(); return; }
        if (Music_IsPlaying()) return;
        /* Give HM01 */
        Inventory_Add(HM01, 1);
        SetEvent(EVENT_GOT_HM01);
        SetEvent(EVENT_RUBBED_CAPTAINS_BACK);
        /* Restore map music */
        Music_Play(Music_GetMapID(wCurMap));
        g_captain_state = CS_CAPTAIN_DONE;
        printf("[ss_anne] HM01 Cut received\n");
        return;

    case CS_CAPTAIN_DONE:
        g_captain_state = CS_IDLE;
        return;
    }
}
