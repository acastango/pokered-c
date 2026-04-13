/* pokeflute.c — POKe FLUTE overworld field item effect.
 *
 * Mirrors ItemUsePokeFlute (engine/items/item_effects.asm lines 1671+) for
 * the overworld SNORLAX-wake path, and Route12DefaultScript / Route16DefaultScript
 * (scripts/Route12.asm, Route16.asm) for the post-battle calmed-down text.
 *
 * Flow:
 *   1. PokeFlute_Use(): coord check → show woke-up text, arm s_fight_pending.
 *   2. game.c polls PokeFlute_ConsumeSnorlaxBattle() after Text_IsOpen() guard:
 *      fires when text is dismissed → sets wCurPartySpecies/Level, starts SCENE_BTRANS.
 *   3. Battle ends: game.c polls PokeFlute_ConsumeSnorlaxPostBattle(), calls
 *      PokeFlute_OnSnorlaxVictory() or PokeFlute_OnSnorlaxCaught().
 *   4. Callbacks: set EVENT_BEAT, show calmed-down text (victory only).
 *   5. PokeFlute_LoadMap(): hides SNORLAX NPC if EVENT_BEAT is set.
 */
#include "pokeflute.h"
#include "npc.h"
#include "text.h"
#include "../platform/hardware.h"
#include "../data/event_constants.h"

#define MAP_ROUTE12  0x17
#define MAP_ROUTE16  0x1b

/* Coord arrays: one-tile cardinal neighbors of each SNORLAX object.
 * Mirrors Route12SnorlaxFluteCoords / Route16SnorlaxFluteCoords (item_effects.asm).
 * SNORLAX R12 object is at (10,62); R16 object is at (26,10). */
static const int kR12Coords[][2] = {
    {  9, 62 }, /* one tile W  */
    { 10, 61 }, /* one tile N  */
    { 10, 63 }, /* one tile S  */
    { 11, 62 }, /* one tile E  */
    { -1, -1 }  /* sentinel    */
};
static const int kR16Coords[][2] = {
    { 27, 10 }, /* one tile E  */
    { 25, 10 }, /* one tile W  */
    { -1, -1 }
};

static int s_fight_map     = 0; /* MAP_ROUTE12 or MAP_ROUTE16; 0 = no active encounter */
static int s_fight_pending = 0; /* armed by Use(), consumed by ConsumeSnorlaxBattle()  */
static int s_in_battle     = 0; /* set by Consume, cleared by ConsumeSnorlaxPostBattle */

static int coords_match(const int c[][2]) {
    for (int i = 0; c[i][0] != -1; i++) {
        if ((int)wXCoord == c[i][0] && (int)wYCoord == c[i][1])
            return 1;
    }
    return 0;
}

void PokeFlute_Use(void) {
    /* Route 12 SNORLAX — mirrors the Route_12 branch in ItemUsePokeFlute */
    if (wCurMap == MAP_ROUTE12 && !CheckEvent(EVENT_BEAT_ROUTE12_SNORLAX)) {
        if (coords_match(kR12Coords)) {
            /* Mirrors Route12SnorlaxWokeUpText */
            Text_ShowASCII("SNORLAX woke up!\nIt attacked in a\ngrumpy rage!");
            s_fight_map     = MAP_ROUTE12;
            s_fight_pending = 1;
            return;
        }
    }
    /* Route 16 SNORLAX */
    if (wCurMap == MAP_ROUTE16 && !CheckEvent(EVENT_BEAT_ROUTE16_SNORLAX)) {
        if (coords_match(kR16Coords)) {
            Text_ShowASCII("SNORLAX woke up!\nIt attacked in a\ngrumpy rage!");
            s_fight_map     = MAP_ROUTE16;
            s_fight_pending = 1;
            return;
        }
    }
    /* No nearby SNORLAX — mirrors PlayedFluteNoEffectText */
    Text_ShowASCII("Played the\nPOKE FLUTE.\n\nNow, that's a\ncatchy tune!");
}

int PokeFlute_ConsumeSnorlaxBattle(void) {
    if (!s_fight_pending) return 0;
    s_fight_pending = 0;
    s_in_battle     = 1;
    return 1;
}

int PokeFlute_ConsumeSnorlaxPostBattle(void) {
    if (!s_in_battle) return 0;
    s_in_battle = 0;
    return 1;
}

void PokeFlute_OnSnorlaxVictory(void) {
    /* Mirrors Route12DefaultScript defeated path: show calmed-down text, set beat event. */
    if (s_fight_map == MAP_ROUTE12) {
        SetEvent(EVENT_BEAT_ROUTE12_SNORLAX);
        ClearEvent(EVENT_FIGHT_ROUTE12_SNORLAX);
        /* Mirrors Route12SnorlaxCalmedDownText */
        Text_ShowASCII("SNORLAX calmed\ndown! With a big\nyawn, it returned\nto the mountains!");
    } else if (s_fight_map == MAP_ROUTE16) {
        SetEvent(EVENT_BEAT_ROUTE16_SNORLAX);
        ClearEvent(EVENT_FIGHT_ROUTE16_SNORLAX);
        /* Mirrors Route16SnorlaxReturnedToMountainsText */
        Text_ShowASCII("With a big yawn,\nSNORLAX returned\nto the mountains!");
    }
    s_fight_map = 0;
}

void PokeFlute_OnSnorlaxCaught(void) {
    /* Caught: set beat event silently (mirrors Route12DefaultScript caught path). */
    if (s_fight_map == MAP_ROUTE12) {
        SetEvent(EVENT_BEAT_ROUTE12_SNORLAX);
        ClearEvent(EVENT_FIGHT_ROUTE12_SNORLAX);
    } else if (s_fight_map == MAP_ROUTE16) {
        SetEvent(EVENT_BEAT_ROUTE16_SNORLAX);
        ClearEvent(EVENT_FIGHT_ROUTE16_SNORLAX);
    }
    s_fight_map = 0;
}

void PokeFlute_LoadMap(void) {
    /* Hide SNORLAX NPC sprite if already beaten, mirroring TOGGLE_ROUTE_12_SNORLAX /
     * TOGGLE_ROUTE_16_SNORLAX.  Must be called after NPC_Load() each map transition. */
    if (wCurMap == MAP_ROUTE12 && CheckEvent(EVENT_BEAT_ROUTE12_SNORLAX))
        NPC_HideSprite(0);   /* SNORLAX is NPC slot 0 in kNpcs_Route12 */
    if (wCurMap == MAP_ROUTE16 && CheckEvent(EVENT_BEAT_ROUTE16_SNORLAX))
        NPC_HideSprite(6);   /* SNORLAX is NPC slot 6 in kNpcs_Route16 */
}
