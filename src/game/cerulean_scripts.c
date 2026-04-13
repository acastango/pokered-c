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
#include "inventory.h"
#include "trainer_sight.h"
#include "../platform/hardware.h"
#include "../platform/audio.h"
#include "../data/event_constants.h"
#include "constants.h"
#include <stdio.h>

extern uint8_t wPlayerName[];

/* ---- Constants -------------------------------------------------------- */

#define MAP_CERULEAN_CITY  3

/* NPC index in kNpcs_CeruleanCity[] (CERULEANCITY_RIVAL = 0) */
#define RIVAL_NPC_IDX   0
#define ROCKET_NPC_IDX  1   /* Team Rocket thief at (30,8) */
#define GUARD1_NPC_IDX  5   /* CERULEANCITY_GUARD1 at (28,12) — hidden until Rocket beaten */
#define GUARD2_NPC_IDX  10  /* CERULEANCITY_GUARD2 at (27,12) — visible until Rocket beaten */

/* Trainer class 25 = RIVAL1 */
#define RIVAL1_CLASS    25

/* Trainer class 30 ($1E) = ROCKET; trainer #5 = Cerulean thief (Lv17 Machop+Drowzee) */
#define ROCKET_CLASS    30
#define ROCKET_TR_NO     5

/* Frames to wait after map reload before showing post-battle text (fade-in) */
#define ROCKET_POST_FADE_WAIT  17

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

    /* Rocket thief encounter states */
    CS_ROCKET_PRE_TEXT,      /* "Hey! Stay out!" pre-battle dialogue */
    CS_ROCKET_BATTLE_PENDING,/* text done; waiting for game.c to start battle */
    CS_ROCKET_POST_TIMER,    /* post-battle fade-in wait before showing text */
    CS_ROCKET_RETURN_TEXT,   /* "OK! I'll return the TM I stole!" */
    CS_ROCKET_RETURN_WAIT,   /* waiting for that text to be dismissed */
    CS_ROCKET_TM_GIVE,       /* give TM28, play SFX, show received text */
    CS_ROCKET_TM_WAIT,       /* waiting for "[PLAYER] recovered TM28!" text */
    CS_ROCKET_CLEANUP,       /* hide Rocket sprite, swap guards */
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
static int           g_rocket_battle_active = 0;
static int           g_rocket_post_timer  = 0;

/* ---- Text strings ----------------------------------------------------- */

/* Pre-battle — _CeruleanCityRivalPreBattleText */
static const char kPreBattle[] =
    "{RIVAL}: Yo!\n{PLAYER}!\f"
    "You're still\nstruggling along\nback here?\f"
    "I'm doing great!\nI caught a bunch\nof strong and\nsmart POKEMON!\f"
    "Here, let me see\nwhat you caught,\n{PLAYER}!";

/* Rocket pre-battle — _CeruleanCityRocketText */
static const char kRocketPreBattle[] =
    "Hey! Stay out!\nIt's not your\nyard! Huh? Me?\f"
    "I'm an innocent\nbystander! Don't\nyou believe me?";

/* Rocket post-battle defeat speech (shown by battle UI via gTrainerAfterText) */
static const char kRocketIGiveUp[] =
    "Stop!\nI give up! I'll\nleave quietly!";

/* In-battle defeat quote — _CeruleanCityRivalDefeatedText */
static const char kRivalDefeated[] =
    "Hey!\nTake it easy!\nYou won already!";

/* Rocket TM giving — _CeruleanCityRocketIllReturnTheTMText */
static const char kRocketIllReturn[] =
    "OK! I'll return\nthe TM I stole!";

/* "[PLAYER] recovered TM28!\fI better get moving! Bye!" — built at runtime */
static char kRocketReceived[64];

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
    printf("[cerulean] onload: state=%d beat_rival=%d pending=%d rival_active=%d rocket_active=%d pos=(%d,%d)\n",
           (int)g_state,
           CheckEvent(EVENT_BEAT_CERULEAN_RIVAL),
           g_pending_battle,
           g_rival_battle_active,
           g_rocket_battle_active,
           (int)wXCoord, (int)wYCoord);

    /* Preserve post-battle state across the map reload game.c does after
     * a battle ends — those states drive the walk-away / TM-giving sequence.
     * Re-show and reposition the rival since NPC_Load() reset him to (20,2). */
    if (g_state == CS_POST_BATTLE_TEXT ||
        g_state == CS_WALK_AWAY        ||
        g_state == CS_CLEANUP) {
        NPC_ShowSprite(RIVAL_NPC_IDX);
        NPC_SetTilePos(RIVAL_NPC_IDX, g_rival_stop_x, g_rival_stop_y);
        return;
    }

    /* Preserve Rocket post-battle states — don't reset g_state.
     * NPC_Load() restored all sprites, so re-apply guard visibility.
     * Rocket stays visible until CS_ROCKET_CLEANUP hides it. */
    if (g_state == CS_ROCKET_POST_TIMER  ||
        g_state == CS_ROCKET_RETURN_WAIT ||
        g_state == CS_ROCKET_TM_WAIT     ||
        g_state == CS_ROCKET_CLEANUP) {
        NPC_HideSprite(RIVAL_NPC_IDX);
        NPC_HideSprite(GUARD2_NPC_IDX);  /* SS Ticket already obtained */
        /* GUARD1 and ROCKET stay visible (defaults from NPC_Load) */
        return;
    }

    g_state = CS_IDLE;
    /* Rival is hidden until triggered.  After beating him the sprite walks
     * away and is hidden; on map re-enter he simply doesn't appear (flag set). */
    NPC_HideSprite(RIVAL_NPC_IDX);

    /* Guards and Rocket thief visibility.
     *
     * Original: BillsHouse.asm — when Bill gives the SS Ticket he calls
     * ShowObject GUARD1 / HideObject GUARD2.  The same transition also
     * happens via CeruleanHideRocket after defeating the overworld Rocket,
     * but that Rocket is only reachable once GUARD2 is already gone.
     * Trigger: EVENT_GOT_SS_TICKET (set when Bill hands over the ticket).
     *
     * Before: GUARD2 visible (blocks northeast path), GUARD1 hidden, Rocket visible.
     * After:  GUARD2 hidden, GUARD1 visible, Rocket hidden. */
    if (CheckEvent(EVENT_GOT_SS_TICKET)) {
        NPC_HideSprite(GUARD2_NPC_IDX);
        /* GUARD1 stays visible (default after NPC_Load) */
        if (CheckEvent(EVENT_BEAT_CERULEAN_ROCKET_THIEF)) {
            NPC_HideSprite(ROCKET_NPC_IDX);
        }
        /* Otherwise Rocket stays visible until defeated */
    } else {
        NPC_HideSprite(GUARD1_NPC_IDX);
        /* GUARD2 and ROCKET stay visible (default after NPC_Load) */
    }
}

void CeruleanScripts_StepCheck(void) {
    if (wCurMap != MAP_CERULEAN_CITY) return;
    if (g_state != CS_IDLE) return;

    /* Rocket thief trigger: (30,7) or (30,9), one tile N/S of NPC at (30,8).
     * Only active after SS Ticket (guards cleared) and before event is set. */
    if (!CheckEvent(EVENT_BEAT_CERULEAN_ROCKET_THIEF) &&
        CheckEvent(EVENT_GOT_SS_TICKET) &&
        (int)wXCoord == 30 &&
        ((int)wYCoord == 7 || (int)wYCoord == 9)) {

        printf("[cerulean] rocket trigger at (%d,%d)\n",
               (int)wXCoord, (int)wYCoord);
        gTrainerAfterText = kRocketIGiveUp;
        Text_ShowASCII(kRocketPreBattle);
        g_state = CS_ROCKET_PRE_TEXT;
        return;
    }

    if (CheckEvent(EVENT_BEAT_CERULEAN_RIVAL)) {
        printf("[cerulean] step check blocked: EVENT_BEAT_CERULEAN_RIVAL already set at (%d,%d), state=%d\n",
               (int)wXCoord, (int)wYCoord, (int)g_state);
        return;
    }

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
    if (g_state == CS_ROCKET_BATTLE_PENDING) {
        *class_out = ROCKET_CLASS;
        *no_out    = ROCKET_TR_NO;
        g_rocket_battle_active = 1;
        printf("[cerulean] pending battle consumed: rocket class=%u no=%u state=%d\n",
               (unsigned)*class_out, (unsigned)*no_out, (int)g_state);
    } else {
        *class_out = RIVAL1_CLASS;
        *no_out    = g_rival_tr_no;
        g_rival_battle_active = 1;
        printf("[cerulean] pending battle consumed: rival class=%u no=%u state=%d beat_rival=%d\n",
               (unsigned)*class_out, (unsigned)*no_out, (int)g_state,
               CheckEvent(EVENT_BEAT_CERULEAN_RIVAL));
    }
    return 1;
}

int CeruleanScripts_ConsumeRivalBattle(void) {
    if (!g_rival_battle_active) return 0;
    printf("[cerulean] consume rival battle: state=%d engaged=(class=%u no=%u) beat_rival=%d\n",
           (int)g_state,
           (unsigned)gEngagedTrainerClass,
           (unsigned)gEngagedTrainerNo,
           CheckEvent(EVENT_BEAT_CERULEAN_RIVAL));
    g_rival_battle_active = 0;
    return 1;
}

void CeruleanScripts_OnVictory(void) {
    printf("[cerulean] OnVictory: setting EVENT_BEAT_CERULEAN_RIVAL from %d, engaged=(class=%u no=%u), state=%d\n",
           CheckEvent(EVENT_BEAT_CERULEAN_RIVAL),
           (unsigned)gEngagedTrainerClass,
           (unsigned)gEngagedTrainerNo,
           (int)g_state);
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
    printf("[cerulean] OnDefeat: state=%d engaged=(class=%u no=%u)\n",
           (int)g_state,
           (unsigned)gEngagedTrainerClass,
           (unsigned)gEngagedTrainerNo);
    NPC_HideSprite(RIVAL_NPC_IDX);
    g_state = CS_IDLE;
}

int CeruleanScripts_ConsumeRocketBattle(void) {
    if (!g_rocket_battle_active) return 0;
    g_rocket_battle_active = 0;
    return 1;
}

void CeruleanScripts_OnRocketVictory(void) {
    SetEvent(EVENT_BEAT_CERULEAN_ROCKET_THIEF);
    g_rocket_post_timer = ROCKET_POST_FADE_WAIT;
    g_state = CS_ROCKET_POST_TIMER;
    printf("[cerulean] rocket defeated — post-battle sequence\n");
}

void CeruleanScripts_OnRocketDefeat(void) {
    /* Player blacked out — reset; Rocket stays in place */
    g_state = CS_IDLE;
}

void CeruleanScripts_Tick(void) {
    if (g_state == CS_IDLE ||
        g_state == CS_BATTLE_PENDING ||
        g_state == CS_ROCKET_BATTLE_PENDING) return;

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
        /* SaveEndBattleTextPointers in the original Cerulean script:
         * after "PLAYER defeated RIVAL!" the battle scene shows the short
         * defeat quote before the overworld "I went to Bill's" sequence. */
        gTrainerAfterText = kRivalDefeated;
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

    /* ---- Rocket thief states ------------------------------------------- */

    case CS_ROCKET_PRE_TEXT:
        if (Text_IsOpen()) { Text_Update(); return; }
        /* Text dismissed — arm battle */
        g_pending_battle = 1;
        g_state = CS_ROCKET_BATTLE_PENDING;
        return;

    case CS_ROCKET_POST_TIMER:
        if (g_rocket_post_timer > 0) { g_rocket_post_timer--; return; }
        /* Build received text with player name */
        {
            char player_ascii[12] = "RED";
            int i;
            for (i = 0; i < 11; i++) {
                uint8_t c = wPlayerName[i];
                if (c == 0x50) break;
                if (c >= 0x80 && c <= 0x99)      player_ascii[i] = (char)('A' + c - 0x80);
                else if (c >= 0xA0 && c <= 0xB9) player_ascii[i] = (char)('a' + c - 0xA0);
                else if (c >= 0xF6 && c <= 0xFF) player_ascii[i] = (char)('0' + c - 0xF6);
                else player_ascii[i] = '?';
                player_ascii[i + 1] = '\0';
            }
            snprintf(kRocketReceived, sizeof(kRocketReceived),
                     "%s recovered\nTM28!\fI better get\nmoving! Bye!",
                     player_ascii);
        }
        Text_ShowASCII(kRocketIllReturn);
        g_state = CS_ROCKET_RETURN_WAIT;
        return;

    case CS_ROCKET_RETURN_WAIT:
        if (Text_IsOpen()) { Text_Update(); return; }
        /* Give TM28 Dig, play SFX, show received text */
        Audio_PlaySFX_GetItem1();
        Inventory_Add(TM01 + 27, 1);   /* TM28 = TM01 + 27 */
        Text_ShowASCII(kRocketReceived);
        g_state = CS_ROCKET_TM_WAIT;
        return;

    case CS_ROCKET_TM_WAIT:
        if (Text_IsOpen()) { Text_Update(); return; }
        g_state = CS_ROCKET_CLEANUP;
        return;

    case CS_ROCKET_CLEANUP:
        /* Mirror CeruleanHideRocket: show GUARD1, hide GUARD2, hide ROCKET */
        NPC_HideSprite(ROCKET_NPC_IDX);
        NPC_HideSprite(GUARD2_NPC_IDX);
        /* GUARD1 stays visible (NPC_Load default, already visible after SS Ticket) */
        g_state = CS_IDLE;
        printf("[cerulean] rocket script complete\n");
        return;

    default:
        g_state = CS_IDLE;
        return;
    }
}
