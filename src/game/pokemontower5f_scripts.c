#include "pokemontower5f_scripts.h"
#include "text.h"
#include "pokemon.h"
#include "../platform/hardware.h"
#include "../data/event_constants.h"

#define MAP_POKEMON_TOWER_5F 0x92

typedef enum {
    PT5_IDLE = 0,
    PT5_SHOW_TEXT,
} PT5State;

static PT5State g_state = PT5_IDLE;

static const char kPurifiedZoneText[] =
    "Entered purified,\nprotected zone!\f"
    "{PLAYER}'s POKEMON\nare fully healed!";

static int in_purified_zone(void) {
    int x = (int)wXCoord;
    int y = (int)wYCoord;
    return (x == 10 || x == 11) && (y == 8 || y == 9);
}

void PokemonTower5FScripts_OnMapLoad(void) {
    if (wCurMap != MAP_POKEMON_TOWER_5F) {
        g_state = PT5_IDLE;
        return;
    }

    if (in_purified_zone()) {
        SetEvent(EVENT_IN_PURIFIED_ZONE);
    } else {
        ClearEvent(EVENT_IN_PURIFIED_ZONE);
    }
    g_state = PT5_IDLE;
}

void PokemonTower5FScripts_StepCheck(void) {
    if (wCurMap != MAP_POKEMON_TOWER_5F) return;
    if (g_state != PT5_IDLE) return;

    if (!in_purified_zone()) {
        ClearEvent(EVENT_IN_PURIFIED_ZONE);
        return;
    }

    if (CheckEvent(EVENT_IN_PURIFIED_ZONE)) return;

    SetEvent(EVENT_IN_PURIFIED_ZONE);
    Pokemon_HealParty();
    Text_ShowASCII(kPurifiedZoneText);
    g_state = PT5_SHOW_TEXT;
}

void PokemonTower5FScripts_Tick(void) {
    if (g_state != PT5_SHOW_TEXT) return;
    if (Text_IsOpen()) return;
    g_state = PT5_IDLE;
}

int PokemonTower5FScripts_IsActive(void) {
    return wCurMap == MAP_POKEMON_TOWER_5F && g_state != PT5_IDLE;
}
