#include "saffron_city_scripts.h"
#include "npc.h"
#include "../platform/hardware.h"
#include "../data/event_constants.h"

#define MAP_SAFFRON_CITY_A 0x0A
#define MAP_SAFFRON_CITY_B 0x0B

/* kNpcs_SaffronCity slots in event_data.c */
#define NPC_SAFFRON_ROCKET8 13 /* TOGGLE_SAFFRON_CITY_E */
#define NPC_SAFFRON_ROCKET9 14 /* TOGGLE_SAFFRON_CITY_F */

void SaffronCityScripts_OnMapLoad(void) {
    if (wCurMap != MAP_SAFFRON_CITY_A && wCurMap != MAP_SAFFRON_CITY_B) return;

    /* ASM parity:
     * PokemonTower7FMrFujiText -> HideObject(TOGGLE_SAFFRON_CITY_E),
     * ShowObject(TOGGLE_SAFFRON_CITY_F). */
    if (CheckEvent(EVENT_RESCUED_MR_FUJI)) {
        NPC_HideSprite(NPC_SAFFRON_ROCKET8);
        NPC_ShowSprite(NPC_SAFFRON_ROCKET9);
    } else {
        NPC_ShowSprite(NPC_SAFFRON_ROCKET8);
        NPC_HideSprite(NPC_SAFFRON_ROCKET9);
    }
}

