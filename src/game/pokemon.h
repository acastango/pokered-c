#pragma once
/* pokemon.h — Pokémon data utilities.
 *
 * Pokemon_InitMon:  Initialize a party_mon_t from Pokédex number + level.
 * CalcExpForLevel: Experience required to reach a given level.
 * Pokemon_GetName: ASCII name for a Pokédex number.
 */
#include <stdint.h>
#include "types.h"

/* Initialize mon from Pokédex number (1-151) and level (1-100).
 * Fills species, types, catch_rate, starting moves, PP, DVs (all 0),
 * stat exp (all 0), exp for level, computed stats, current HP = max HP. */
void Pokemon_InitMon(party_mon_t *mon, uint8_t dex, uint8_t level);

/* Experience required to reach `level` under `growth_rate` (GROWTH_* constant).
 * Returns 0 for level < 2. */
uint32_t CalcExpForLevel(uint8_t growth_rate, uint8_t level);

/* ASCII name for Pokédex number 1-151.  Returns "" for 0 or out-of-range. */
const char *Pokemon_GetName(uint8_t dex);
