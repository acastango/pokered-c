/* route2gate_scripts.c — Route 2 Gate Oak's aide: HM05 FLASH event.
 *
 * ASM reference:
 *   scripts/Route2Gate.asm         — map script entry point
 *   engine/events/oaks_aide.asm    — OaksAideScript routine
 *   home/count_set_bits.asm        — CountSetBits used for Pokedex check
 *
 * Flow (mirrors OaksAideScript):
 *   Player talks to aide (NPC 0, pos 1,4 on map 0x31)
 *   → Already gave Flash (EVENT_GOT_HM05 set): show already-given text
 *   → Count bits in wPokedexOwned (19 bytes, 151 Pokémon)
 *   → < 10 owned: show "need 10 kinds" text
 *   → ≥ 10 owned, bag full: show "no room" text
 *   → ≥ 10 owned, bag OK: give HM05, set EVENT_GOT_HM05, jingle + text
 *
 * Map ID: ROUTE_2_GATE = 0x31
 */
#include "route2gate_scripts.h"
#include "text.h"
#include "inventory.h"
#include "constants.h"
#include "../data/event_constants.h"
#include "../platform/hardware.h"
#include "../platform/audio.h"
#include <stdio.h>

#define MAP_ROUTE_2_GATE  0x31
#define ITEM_HM05         0xC8   /* HM01=0xC4 .. HM05=0xC8 */
#define OAK_AIDE_REQUIRED 10

/* ---- State machine --------------------------------------------------- */
typedef enum {
    RG_IDLE = 0,
    RG_TEXT_WAIT,    /* waiting for any text to close → IDLE */
    RG_JINGLE_WAIT,  /* GetKeyItem jingle playing → show received text */
    RG_GIVE_WAIT,    /* "received HM05" text open → IDLE on close */
} rg_state_t;

static rg_state_t s_state = RG_IDLE;

/* ---- Text strings (mirrors Route2Gate.asm / OaksAideScript text) ----- */
static const char kText_AlreadyGiven[] =
    "FLASH can light\nup even the\ndarkest cave!\nUse it well!";

static const char kText_NeedMore[] =
    "Hi! I'm one of\nPROF. OAK's\naides!\nIf you've caught\n10 kinds of\nPOKeMON, I'll\ngive you\nsomething!";

static const char kText_BagFull[] =
    "Your BAG is full.\nCome back when\nyou have room.";

static const char kText_Received[] =
    "You've caught 10\nkinds of POKeMON!\nAs promised,\nhere's HM 05!\nFLASH will light\nup dark caves.";

/* ---- Helpers --------------------------------------------------------- */
static int count_owned(void) {
    int n = 0;
    for (int i = 0; i < 19; i++) {
        uint8_t b = wPokedexOwned[i];
        while (b) { n += b & 1; b >>= 1; }
    }
    return n;
}

/* ---- Public API ------------------------------------------------------ */

void Route2GateScripts_OnMapLoad(void) {
    /* No auto-trigger needed; interaction is NPC-driven. */
    (void)0;
}

int Route2GateScripts_IsActive(void) {
    return s_state != RG_IDLE;
}

void Route2GateScripts_AideInteract(void) {
    if (s_state != RG_IDLE) return;
    if (wCurMap != MAP_ROUTE_2_GATE) return;

    /* Already gave Flash */
    if (CheckEvent(EVENT_GOT_HM05)) {
        Text_ShowASCII(kText_AlreadyGiven);
        s_state = RG_TEXT_WAIT;
        return;
    }

    /* Count owned Pokémon (mirrors CountSetBits on wPokedexOwned) */
    int owned = count_owned();
    printf("[r2gate] Oak's aide: player owns %d kinds.\n", owned);

    if (owned < OAK_AIDE_REQUIRED) {
        Text_ShowASCII(kText_NeedMore);
        s_state = RG_TEXT_WAIT;
        return;
    }

    /* Try to give HM05 */
    if (Inventory_Add(ITEM_HM05, 1) != 0) {
        Text_ShowASCII(kText_BagFull);
        s_state = RG_TEXT_WAIT;
        return;
    }

    /* Success: set flag, play jingle, show text */
    SetEvent(EVENT_GOT_HM05);
    Audio_PlaySFX_GetKeyItem();
    s_state = RG_JINGLE_WAIT;
}

void Route2GateScripts_Tick(void) {
    switch (s_state) {
        case RG_IDLE:
            break;

        case RG_TEXT_WAIT:
            if (!Text_IsOpen())
                s_state = RG_IDLE;
            break;

        case RG_JINGLE_WAIT:
            if (!Audio_IsSFXPlaying_GetKeyItem()) {
                Text_ShowASCII(kText_Received);
                s_state = RG_GIVE_WAIT;
            }
            break;

        case RG_GIVE_WAIT:
            if (!Text_IsOpen())
                s_state = RG_IDLE;
            break;
    }
}
