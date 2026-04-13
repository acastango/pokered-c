#pragma once

/* pokeflute.h — POKe FLUTE overworld field item effect.
 *
 * Mirrors ItemUsePokeFlute (engine/items/item_effects.asm) and the
 * Route12DefaultScript / Route16DefaultScript post-battle handlers
 * (scripts/Route12.asm, Route16.asm).
 *
 * Call order (all from game.c unless noted):
 *   1. PokeFlute_Use()              — from bag_menu.c USE handler (overworld)
 *   2. PokeFlute_ConsumeSnorlaxBattle() — main overworld tick, after Text_IsOpen() guard
 *   3. PokeFlute_ConsumeSnorlaxPostBattle() — battle-end block
 *   4. PokeFlute_OnSnorlaxVictory() / PokeFlute_OnSnorlaxCaught() — battle-end outcome
 *   5. PokeFlute_LoadMap()          — in fire_map_onload_callbacks()
 */

/* Called from bag_menu.c when ITEM_POKE_FLUTE is used in the overworld.
 * Checks current map and player coords; shows woke-up text and arms the
 * battle trigger if adjacent to SNORLAX, otherwise shows no-effect text. */
void PokeFlute_Use(void);

/* Called in fire_map_onload_callbacks(): hides the SNORLAX NPC sprite on
 * Route 12 / Route 16 if EVENT_BEAT_ROUTExx_SNORLAX is already set. */
void PokeFlute_LoadMap(void);

/* Returns 1 and arms s_in_battle if a SNORLAX battle is ready to start.
 * Place this call after the `if (Text_IsOpen()) return;` gate in the main
 * overworld tick so it only fires once the woke-up text has been dismissed. */
int  PokeFlute_ConsumeSnorlaxBattle(void);

/* Returns 1 and clears s_in_battle if the just-ended battle was a SNORLAX.
 * Call once in the battle-end block before the outcome dispatch. */
int  PokeFlute_ConsumeSnorlaxPostBattle(void);

/* Outcome callbacks — call after ConsumeSnorlaxPostBattle() returns 1. */
void PokeFlute_OnSnorlaxVictory(void); /* wild mon fainted — shows calmed-down text */
void PokeFlute_OnSnorlaxCaught(void);  /* SNORLAX caught   — silent, sets beat event */
