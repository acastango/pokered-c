#include "game_corner_scripts.h"
#include "npc.h"
#include "text.h"
#include "music.h"
#include "overworld.h"
#include "../platform/hardware.h"
#include "../platform/audio.h"
#include "../data/event_constants.h"

#define MAP_GAME_CORNER 0x87
#define GAMECORNER_ROCKET_NPC 10
#define ROCKET_CLASS 30
#define ROCKET_NO 7
/* Reserved/unused event slot in this port's generated table.
 * Used to persist "Game Corner rocket already left" state. */
#define EVENT_GAME_CORNER_ROCKET_LEFT EVENT_1BF

#define DIR_DOWN  0
#define DIR_UP    1
#define DIR_LEFT  2
#define DIR_RIGHT 3

typedef enum {
    GCS_IDLE = 0,
    GCS_BATTLE_PENDING,
    GCS_POST_TEXT,
    GCS_ROCKET_EXIT,
} GameCornerState;

static GameCornerState g_state = GCS_IDLE;
static int g_pending_battle = 0;
static int g_battle_active = 0;
static int g_exit_idx = 0;
static const int *g_exit_seq = 0;
static int g_post_text_shown = 0;
static int g_poster_open_pending = 0;

static const int kExitWalkAround[] = {
    DIR_DOWN, DIR_RIGHT, DIR_RIGHT, DIR_UP, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, -1
};
static const int kExitDirect[] = {
    DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, -1
};

static const char kRocketPreBattle[] =
    "I'm guarding this\nposter!\nGo away, or else!";

static const char kRocketAfterBattle[] =
    "Our hideout might\nbe discovered!\nI better tell\nBOSS!";

static const char kPosterSwitch[] =
    "Hey!\fA switch behind\nthe poster!?\nLet's push it!";

static void GameCorner_ApplyHideoutDoor(void) {
    /* ASM GameCornerSetRocketHideoutDoorTile:
     * - if EVENT_FOUND_ROCKET_HIDEOUT is clear, write $2A at (2,8) via ReplaceTileBlock
     * - if set, leave default open tile ($43)
     *
     * In this C port's extracted block map, the target staircase block is at
     * block coords (8,2), so we patch that cell directly. */
    if (CheckEvent(EVENT_FOUND_ROCKET_HIDEOUT))
        Map_SetBlock(8, 2, 0x43);
    else
        Map_SetBlock(8, 2, 0x2a);
}

void GameCornerScripts_OnMapLoad(void) {
    if (wCurMap != MAP_GAME_CORNER) return;

    /* Preserve post-battle scripted sequence across battle map reload. */
    if (g_state == GCS_POST_TEXT || g_state == GCS_ROCKET_EXIT) {
        g_pending_battle = 0;
        g_battle_active = 0;
        GameCorner_ApplyHideoutDoor();
        if (CheckEvent(EVENT_FOUND_ROCKET_HIDEOUT) || CheckEvent(EVENT_GAME_CORNER_ROCKET_LEFT))
            NPC_HideSprite(GAMECORNER_ROCKET_NPC);
        else
            NPC_ShowSprite(GAMECORNER_ROCKET_NPC);
        return;
    }

    g_state = GCS_IDLE;
    g_pending_battle = 0;
    g_battle_active = 0;
    g_exit_idx = 0;
    g_exit_seq = 0;
    g_post_text_shown = 0;
    g_poster_open_pending = 0;
    GameCorner_ApplyHideoutDoor();
    if (CheckEvent(EVENT_FOUND_ROCKET_HIDEOUT) || CheckEvent(EVENT_GAME_CORNER_ROCKET_LEFT))
        NPC_HideSprite(GAMECORNER_ROCKET_NPC);
    else
        NPC_ShowSprite(GAMECORNER_ROCKET_NPC);
}

int GameCornerScripts_IsActive(void) {
    return wCurMap == MAP_GAME_CORNER && g_state != GCS_IDLE;
}

int GameCornerScripts_GetPendingBattle(uint8_t *class_out, uint8_t *no_out) {
    if (!g_pending_battle) return 0;
    g_pending_battle = 0;
    g_battle_active = 1;
    *class_out = ROCKET_CLASS;
    *no_out = ROCKET_NO;
    return 1;
}

int GameCornerScripts_ConsumeRocketBattle(void) {
    return g_battle_active;
}

void GameCornerScripts_OnVictory(void) {
    g_battle_active = 0;
    g_post_text_shown = 0;
    g_state = GCS_POST_TEXT;
}

void GameCornerScripts_OnDefeat(void) {
    g_battle_active = 0;
    g_state = GCS_IDLE;
}

void GameCornerScripts_Tick(void) {
    if (wCurMap != MAP_GAME_CORNER) return;

    /* Keep map/object state synced to event flag while idle so save edits
     * (clearflag/setflag) are reflected immediately without reload. */
    if (g_state == GCS_IDLE) {
        if (g_poster_open_pending && !Text_IsOpen()) {
            Audio_PlaySFX_GoInside();
            SetEvent(EVENT_FOUND_ROCKET_HIDEOUT);
            GameCorner_ApplyHideoutDoor();
            g_poster_open_pending = 0;
        }
        GameCorner_ApplyHideoutDoor();
        if (CheckEvent(EVENT_FOUND_ROCKET_HIDEOUT) || CheckEvent(EVENT_GAME_CORNER_ROCKET_LEFT))
            NPC_HideSprite(GAMECORNER_ROCKET_NPC);
        else
            NPC_ShowSprite(GAMECORNER_ROCKET_NPC);
    }

    if (g_state == GCS_POST_TEXT) {
        if (!g_post_text_shown) {
            Text_ShowASCII(kRocketAfterBattle);
            g_post_text_shown = 1;
            return;
        }
        if (Text_IsOpen()) return;
        g_exit_seq = ((int)wYCoord == 6 || (int)wXCoord == 8) ? kExitDirect : kExitWalkAround;
        g_exit_idx = 0;
        g_state = GCS_ROCKET_EXIT;
        return;
    }
    if (g_state == GCS_ROCKET_EXIT) {
        if (NPC_IsWalking(GAMECORNER_ROCKET_NPC)) return;
        if (g_exit_seq[g_exit_idx] == -1) {
            SetEvent(EVENT_GAME_CORNER_ROCKET_LEFT);
            NPC_HideSprite(GAMECORNER_ROCKET_NPC);
            g_state = GCS_IDLE;
            return;
        }
        NPC_DoScriptedStep(GAMECORNER_ROCKET_NPC, g_exit_seq[g_exit_idx++]);
    }
}

void GameCorner_RocketScript(void) {
    if (wCurMap != MAP_GAME_CORNER) return;
    if (CheckEvent(EVENT_FOUND_ROCKET_HIDEOUT) || CheckEvent(EVENT_GAME_CORNER_ROCKET_LEFT)) return;
    if (g_state != GCS_IDLE) return;
    Text_ShowASCII(kRocketPreBattle);
    g_pending_battle = 1;
    g_state = GCS_BATTLE_PENDING;
}

void GameCorner_PosterScript(void) {
    if (wCurMap != MAP_GAME_CORNER) return;
    if (CheckEvent(EVENT_FOUND_ROCKET_HIDEOUT)) return;
    Text_ShowASCII(kPosterSwitch);
    Text_SetPendingSFX(Audio_PlaySFX_Switch);
    g_poster_open_pending = 1;
}
