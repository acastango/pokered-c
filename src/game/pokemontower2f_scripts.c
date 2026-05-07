#include "pokemontower2f_scripts.h"
#include "text.h"
#include "npc.h"
#include "music.h"
#include "player.h"
#include "rival_starter.h"
#include "trainer_sight.h"
#include "../platform/hardware.h"
#include "../data/event_constants.h"

#define MAP_POKEMON_TOWER_2F 0x8f
#define RIVAL_NPC_IDX 0
#define RIVAL2_CLASS 42

#define DIR_DOWN  0
#define DIR_UP    1
#define DIR_LEFT  2
#define DIR_RIGHT 3

typedef enum {
    PT2_IDLE = 0,
    PT2_PRE_BATTLE_TEXT,
    PT2_BATTLE_PENDING,
    PT2_POST_BATTLE_TEXT,
    PT2_RIVAL_EXIT,
    PT2_CLEANUP,
} PT2State;

static PT2State g_state = PT2_IDLE;
static int g_pending_battle = 0;
static int g_battle_active = 0;
static uint8_t g_rival_tr_no = 4;
static int g_post_shown = 0;
static int g_exit_idx = 0;
static const int *g_exit_seq = 0;
static int g_rival_stop_x = 14;
static int g_rival_stop_y = 5;
static int g_rival_face_after_battle = DIR_DOWN;

static const char kWhatBringsYouHere[] =
    "{RIVAL}: Hey,\n{PLAYER}! What\nbrings you here?\nYour POKEMON\n"
    "don't look dead!\fI can at least\nmake them faint!\nLet's go, pal!";

static const char kDefeatedText[] =
    "What!?\fI was just\ncareless!";

static const char kHowsYourDex[] =
    "{RIVAL}: How's your\nPOKEDEX coming?\fI just caught a\nCUBONE!\fI can't find the\n"
    "grown-up MAROWAK\nyet!\fI doubt there are\nany left!\fWell, I better get\ngoing!\fI've got a lot to\naccomplish, pal!\f"
    "Smell ya later!";

/* coord_index=0 -> down then right */
static const int kExitDownThenRight[] = { DIR_DOWN, DIR_DOWN, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_DOWN, DIR_DOWN, -1 };
/* coord_index=1 -> right then down */
static const int kExitRightThenDown[] = { DIR_RIGHT, DIR_DOWN, DIR_DOWN, DIR_RIGHT, DIR_DOWN, DIR_DOWN, DIR_RIGHT, DIR_RIGHT, -1 };

static int choose_coord_index(void) {
    if ((int)wXCoord == 15 && (int)wYCoord == 5) return 0;
    if ((int)wXCoord == 14 && (int)wYCoord == 6) return 1;
    return -1;
}

static void face_rival_toward_player(void) {
    int rx, ry;
    NPC_GetTilePos(RIVAL_NPC_IDX, &rx, &ry);
    if ((int)wXCoord > rx) NPC_SetFacing(RIVAL_NPC_IDX, DIR_RIGHT);
    else if ((int)wXCoord < rx) NPC_SetFacing(RIVAL_NPC_IDX, DIR_LEFT);
    else if ((int)wYCoord > ry) NPC_SetFacing(RIVAL_NPC_IDX, DIR_DOWN);
    else NPC_SetFacing(RIVAL_NPC_IDX, DIR_UP);
}

static uint8_t choose_trainer_no(void) {
    uint8_t rival_starter = RivalStarter_Get();
    if (rival_starter == STARTER2) return 4;
    if (rival_starter == STARTER3) return 5;
    return 6;
}

static void begin_rival_encounter(int from_coord_trigger) {
    if (CheckEvent(EVENT_BEAT_POKEMON_TOWER_RIVAL)) {
        Text_ShowASCII(kHowsYourDex);
        return;
    }
    if (g_state != PT2_IDLE) return;

    int ci = choose_coord_index();
    if (from_coord_trigger) {
        if (ci == 1) {
            SetEvent(EVENT_POKEMON_TOWER_RIVAL_ON_LEFT);
            NPC_SetFacing(RIVAL_NPC_IDX, DIR_RIGHT);
            gPlayerFacing = DIR_LEFT;
            g_rival_face_after_battle = DIR_RIGHT;
        } else {
            ClearEvent(EVENT_POKEMON_TOWER_RIVAL_ON_LEFT);
            NPC_SetFacing(RIVAL_NPC_IDX, DIR_DOWN);
            gPlayerFacing = DIR_UP;
            g_rival_face_after_battle = DIR_DOWN;
        }
    } else {
        if ((int)wXCoord >= 15 && (int)wYCoord <= 5)
            ClearEvent(EVENT_POKEMON_TOWER_RIVAL_ON_LEFT);
        else
            SetEvent(EVENT_POKEMON_TOWER_RIVAL_ON_LEFT);
        face_rival_toward_player();
        g_rival_face_after_battle = NPC_GetFacing(RIVAL_NPC_IDX);
    }

    NPC_GetTilePos(RIVAL_NPC_IDX, &g_rival_stop_x, &g_rival_stop_y);
    Music_Play(MUSIC_MEET_RIVAL);
    Text_ShowASCII(kWhatBringsYouHere);
    g_state = PT2_PRE_BATTLE_TEXT;
}

void PokemonTower2FScripts_OnMapLoad(void) {
    if (wCurMap != MAP_POKEMON_TOWER_2F) return;

    if (g_state == PT2_POST_BATTLE_TEXT || g_state == PT2_RIVAL_EXIT || g_state == PT2_CLEANUP) {
        NPC_ShowSprite(RIVAL_NPC_IDX);
        NPC_SetTilePos(RIVAL_NPC_IDX, g_rival_stop_x, g_rival_stop_y);
        NPC_SetFacing(RIVAL_NPC_IDX, g_rival_face_after_battle);
        return;
    }

    g_state = PT2_IDLE;
    g_pending_battle = 0;
    g_battle_active = 0;
    g_post_shown = 0;
    g_exit_idx = 0;
    g_exit_seq = 0;

    if (CheckEvent(EVENT_BEAT_POKEMON_TOWER_RIVAL))
        NPC_HideSprite(RIVAL_NPC_IDX);
}

void PokemonTower2FScripts_StepCheck(void) {
    if (wCurMap != MAP_POKEMON_TOWER_2F) return;
    if (g_state != PT2_IDLE) return;
    if (CheckEvent(EVENT_BEAT_POKEMON_TOWER_RIVAL)) return;
    if (choose_coord_index() < 0) return;
    begin_rival_encounter(1);
}

void PokemonTower2FScripts_Tick(void) {
    if (g_state == PT2_IDLE || g_state == PT2_BATTLE_PENDING) return;

    switch (g_state) {
    case PT2_PRE_BATTLE_TEXT:
        if (Text_IsOpen()) return;
        gTrainerAfterText = kDefeatedText;
        g_rival_tr_no = choose_trainer_no();
        g_pending_battle = 1;
        g_state = PT2_BATTLE_PENDING;
        return;
    case PT2_POST_BATTLE_TEXT:
        if (!g_post_shown) {
            NPC_SetFacing(RIVAL_NPC_IDX, g_rival_face_after_battle);
            Text_ShowASCII(kHowsYourDex);
            g_post_shown = 1;
            return;
        }
        if (Text_IsOpen()) return;
        g_exit_seq = CheckEvent(EVENT_POKEMON_TOWER_RIVAL_ON_LEFT) ? kExitRightThenDown : kExitDownThenRight;
        g_exit_idx = 0;
        NPC_DoScriptedStep(RIVAL_NPC_IDX, g_exit_seq[g_exit_idx++]);
        g_state = PT2_RIVAL_EXIT;
        return;
    case PT2_RIVAL_EXIT:
        if (NPC_IsWalking(RIVAL_NPC_IDX)) return;
        if (g_exit_seq && g_exit_seq[g_exit_idx] != -1) {
            NPC_DoScriptedStep(RIVAL_NPC_IDX, g_exit_seq[g_exit_idx++]);
            return;
        }
        g_state = PT2_CLEANUP;
        return;
    case PT2_CLEANUP:
        NPC_HideSprite(RIVAL_NPC_IDX);
        Music_Play(Music_GetMapID(wCurMap));
        g_state = PT2_IDLE;
        return;
    default:
        g_state = PT2_IDLE;
        return;
    }
}

int PokemonTower2FScripts_IsActive(void) {
    return wCurMap == MAP_POKEMON_TOWER_2F && g_state != PT2_IDLE;
}

int PokemonTower2FScripts_GetPendingBattle(uint8_t *class_out, uint8_t *no_out) {
    if (wCurMap != MAP_POKEMON_TOWER_2F) return 0;
    if (!g_pending_battle) return 0;
    g_pending_battle = 0;
    *class_out = RIVAL2_CLASS;
    *no_out = g_rival_tr_no;
    g_battle_active = 1;
    return 1;
}

int PokemonTower2FScripts_ConsumeRivalBattle(void) {
    if (!g_battle_active) return 0;
    g_battle_active = 0;
    return 1;
}

void PokemonTower2FScripts_OnVictory(void) {
    SetEvent(EVENT_BEAT_POKEMON_TOWER_RIVAL);
    Music_PlayFromLoop(Music_GetMapID(wCurMap));
    g_post_shown = 0;
    g_state = PT2_POST_BATTLE_TEXT;
}

void PokemonTower2FScripts_OnDefeat(void) {
    g_state = PT2_IDLE;
}

void PokemonTower2F_RivalScript(void) {
    if (wCurMap != MAP_POKEMON_TOWER_2F) return;
    begin_rival_encounter(0);
}
