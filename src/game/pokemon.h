#pragma once
/* pokemon.h — Pokémon data utilities.
 *
 * Pokemon_InitMon:  Initialize a party_mon_t from Pokédex number + level.
 * CalcExpForLevel: Experience required to reach a given level.
 * Pokemon_GetName: ASCII name for a Pokédex number.
 */
#include <stdint.h>
#include "types.h"

/* Initialize mon from internal species ID and level (1-100).
 * species: internal species ID (SPECIES_* constant), NOT a Pokédex number.
 * Fills species, types, catch_rate, starting moves, PP, DVs (all 0),
 * stat exp (all 0), exp for level, computed stats, current HP = max HP. */
void Pokemon_InitMon(party_mon_t *mon, uint8_t species, uint8_t level);

/* Experience required to reach `level` under `growth_rate` (GROWTH_* constant).
 * Returns 0 for level < 2. */
uint32_t CalcExpForLevel(uint8_t growth_rate, uint8_t level);

/* ASCII name for Pokédex number 1-151.  Returns "" for 0 or out-of-range. */
const char *Pokemon_GetName(uint8_t dex);

/* Fill `moves` and `pp` arrays with the last 4 moves learned by `species_id`
 * up to `level`.  Ports WriteMonMoves (engine/pokemon/evos_moves.asm:383).
 *
 * species_id: internal species ID (SPECIES_* constant).
 * moves/pp:   arrays of NUM_MOVES (4) entries, must be zeroed by caller.
 *
 * Algorithm: skip the evo block (entries until 0x00 terminator), then iterate
 * (learn_level, move_id) pairs.  If the move fits in an empty slot, fill it.
 * If all slots full, shift left (discard moves[0]) and place the new move at
 * slot 3 (the "last 4 learned" Gen 1 behaviour). */
void Pokemon_WriteMovesForLevel(uint8_t *moves, uint8_t *pp,
                                uint8_t species_id, uint8_t level);
