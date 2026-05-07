#include "saffron_city_scripts.h"
#include "constants.h"
#include "inventory.h"
#include "npc.h"
#include "text.h"
#include "../platform/audio.h"
#include "../platform/hardware.h"
#include "../data/event_constants.h"

#define MAP_SAFFRON_CITY_A 0x0A
#define MAP_SAFFRON_CITY_B 0x0B
#define ITEM_TM01 (ITEM_HM01 + 5)
#define ITEM_TM29 (ITEM_TM01 + 28)

/* kNpcs_SaffronCity slots in event_data.c */
#define NPC_SAFFRON_ROCKET8 13 /* TOGGLE_SAFFRON_CITY_E */
#define NPC_SAFFRON_ROCKET9 14 /* TOGGLE_SAFFRON_CITY_F */

typedef enum {
    SC_IDLE = 0,
    SC_TM29_WAIT_PRETEXT_CLOSE,
} SaffronCityScriptState;

static SaffronCityScriptState sState = SC_IDLE;

static const char kMrPsychicPretext[] =
    "...Wait! Don't\nsay a word!\f"
    "You wanted this!";
static const char kMrPsychicReceived[] =
    "{PLAYER} received\nTM29!@";
static const char kMrPsychicExplain[] =
    "TM29 is PSYCHIC!\f"
    "It can lower the\ntarget's SPECIAL\nabilities.";
static const char kMrPsychicNoRoom[] =
    "Your pack is\njammed full!";

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

void MrPsychicsHouse_MrPsychicScript(void) {
    if (sState != SC_IDLE) return;

    if (CheckEvent(EVENT_GOT_TM29)) {
        Text_ShowASCII(kMrPsychicExplain);
        return;
    }

    Text_ShowASCII(kMrPsychicPretext);
    sState = SC_TM29_WAIT_PRETEXT_CLOSE;
}

void SaffronCityScripts_Tick(void) {
    switch (sState) {
    case SC_IDLE:
        return;
    case SC_TM29_WAIT_PRETEXT_CLOSE:
        if (Text_IsOpen()) return;
        if (Inventory_Add(ITEM_TM29, 1) != 0) {
            Text_ShowASCII(kMrPsychicNoRoom);
            sState = SC_IDLE;
            return;
        }
        SetEvent(EVENT_GOT_TM29);
        Audio_PlaySFX_GetItem1();
        Text_ShowASCII(kMrPsychicReceived);
        sState = SC_IDLE;
        return;
    }
}

int SaffronCityScripts_IsActive(void) {
    return sState != SC_IDLE;
}
