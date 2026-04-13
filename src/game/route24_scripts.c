/* route24_scripts.c — Route 24 Nugget Bridge Rocket encounter.
 *
 * Ports scripts/Route24.asm / Route24CooltrainerM1Text.
 *
 * Trigger: player steps on (10,15) in Route 24 (map 0x23)
 * while EVENT_GOT_NUGGET is not set.
 *
 * Sequence (matches Route24CooltrainerM1Text in ASM):
 *   1. "Congratulations! / You just earned a fabulous prize!"
 *   2. Give NUGGET → "{PLAYER} received a NUGGET!" → SetEvent(EVENT_GOT_NUGGET)
 *   3. "By the way, would you like to join TEAM ROCKET?" speech
 *   4. Battle: Rocket class 30, party 6
 *   5. Post-battle (Route24AfterRocketBattleScript):
 *      "Arrgh! / With your ability, you could become a top leader..."
 *
 * Note: the Rocket NPC has no sprite-hide event in the ASM — he stays
 * visible after the encounter and shows "With your ability..." on re-talk.
 */
#include "route24_scripts.h"
#include "text.h"
#include "player.h"
#include "overworld.h"
#include "music.h"
#include "inventory.h"
#include "../platform/audio.h"
#include "../platform/hardware.h"
#include "../data/event_constants.h"
#include <stdio.h>

/* ---- Constants -------------------------------------------------------- */

#define MAP_ROUTE24   0x23

#define TRIGGER_X  10
#define TRIGGER_Y  15

/* Rocket trainer class/party (object_event: OPP_ROCKET, 6) */
#define ROCKET_CLASS  30
#define ROCKET_NO      6

/* ---- State ------------------------------------------------------------- */

typedef enum {
    RS_IDLE = 0,
    RS_PRE_TEXT_CONGRATS,  /* "Congratulations! You beat our 5 contest trainers!" */
    RS_PRE_TEXT_PRIZE,     /* "You just earned a fabulous prize!" */
    RS_PRE_TEXT_NUGGET,    /* give nugget + show receive message */
    RS_PRE_TEXT_ROCKET,    /* "By the way, join TEAM ROCKET?" speech */
    RS_BATTLE_PENDING,     /* waiting for game.c to start battle */
    RS_POST_TEXT,          /* "Arrgh! / With your ability..." */
    RS_CLEANUP,
} Route24State;

static Route24State g_state          = RS_IDLE;
static int          g_pending_battle = 0;
static int          g_battle_active  = 0;
static int          g_post_step      = 0;

/* ---- Text strings ----------------------------------------------------- */

/* _Route24CooltrainerM1YouBeatOurContestText */
static const char kCongratsText[] =
    "Congratulations!\nYou beat our 5\ncontest trainers!";

/* _Route24CooltrainerM1YouJustEarnedAPrizeText */
static const char kPrizeText[] =
    "You just earned a\nfabulous prize!";

/* _Route24CooltrainerM1ReceivedNuggetText */
static const char kNuggetText[] =
    "{PLAYER} received\na NUGGET!";

/* _Route24CooltrainerM1JoinTeamRocketText */
static const char kJoinRocketText[] =
    "By the way, would\nyou like to join\nTEAM ROCKET?\f"
    "We're a group\ndedicated to evil\nusing POKEMON!\f"
    "Want to join?\f"
    "Are you sure?\f"
    "Come on, join us!\f"
    "I'm telling you\nto join!\f"
    "OK, you need\nconvincing!\f"
    "I'll make you an\noffer you can't\nrefuse!";

/* _Route24CooltrainerM1DefeatedText + YouCouldBecomeATopLeaderText
 * (DefeatedText is saved as EndBattleText in ASM — shown by battle engine;
 *  we show both here together in the post-battle script phase) */
static const char kDefeatedText[] =
    "Arrgh!\nYou are good!\f"
    "With your ability,\nyou could become\na top leader in\nTEAM ROCKET!";

/* ---- Public API ------------------------------------------------------- */

void Route24Scripts_OnMapLoad(void) {
    /* Rocket NPC has no sprite-hide event in ASM — always visible. */
}

void Route24Scripts_StepCheck(void) {
    if (wCurMap != MAP_ROUTE24) return;
    if (g_state != RS_IDLE) return;
    if (CheckEvent(EVENT_GOT_NUGGET)) return;

    if ((int)wYCoord != TRIGGER_Y) return;
    if ((int)wXCoord != TRIGGER_X) return;

    printf("[route24] step trigger: Rocket encounter at (%d,%d)\n",
           (int)wXCoord, (int)wYCoord);

    Text_ShowASCII(kCongratsText);
    g_state = RS_PRE_TEXT_CONGRATS;
}

int Route24Scripts_IsActive(void) { return g_state != RS_IDLE; }

int Route24Scripts_GetPendingBattle(uint8_t *class_out, uint8_t *no_out) {
    if (!g_pending_battle) return 0;
    g_pending_battle = 0;
    *class_out = ROCKET_CLASS;
    *no_out    = ROCKET_NO;
    g_battle_active = 1;
    return 1;
}

int Route24Scripts_ConsumeRocketBattle(void) {
    if (!g_battle_active) return 0;
    g_battle_active = 0;
    return 1;
}

void Route24Scripts_OnVictory(void) {
    SetEvent(EVENT_BEAT_ROUTE24_ROCKET);
    g_post_step = 0;
    g_state = RS_POST_TEXT;
    printf("[route24] Rocket defeated — post-battle sequence\n");
}

void Route24Scripts_OnDefeat(void) {
    /* EVENT_GOT_NUGGET already set before battle — trigger won't re-fire. */
    g_state = RS_IDLE;
}

void Route24Scripts_Tick(void) {
    if (g_state == RS_IDLE || g_state == RS_BATTLE_PENDING) return;

    switch (g_state) {

    case RS_PRE_TEXT_CONGRATS:
        if (Text_IsOpen()) { Text_Update(); return; }
        Audio_PlaySFX_GetItem1();
        Text_ShowASCII(kPrizeText);
        g_state = RS_PRE_TEXT_PRIZE;
        return;

    case RS_PRE_TEXT_PRIZE:
        if (Text_IsOpen()) { Text_Update(); return; }
        /* Give nugget and show receive message */
        Inventory_Add(ITEM_NUGGET, 1);
        SetEvent(EVENT_GOT_NUGGET);
        Audio_PlaySFX_GetItem1();
        Text_ShowASCII(kNuggetText);
        g_state = RS_PRE_TEXT_NUGGET;
        return;

    case RS_PRE_TEXT_NUGGET:
        if (Text_IsOpen()) { Text_Update(); return; }
        Text_ShowASCII(kJoinRocketText);
        g_state = RS_PRE_TEXT_ROCKET;
        return;

    case RS_PRE_TEXT_ROCKET:
        if (Text_IsOpen()) { Text_Update(); return; }
        g_pending_battle = 1;
        g_state = RS_BATTLE_PENDING;
        return;

    case RS_POST_TEXT:
        if (!g_post_step) {
            Text_ShowASCII(kDefeatedText);
            g_post_step = 1;
            return;
        }
        if (Text_IsOpen()) { Text_Update(); return; }
        g_state = RS_CLEANUP;
        return;

    case RS_CLEANUP:
        g_state = RS_IDLE;
        printf("[route24] Rocket script complete\n");
        return;

    default:
        g_state = RS_IDLE;
        return;
    }
}
