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

/* Encode an ASCII name into pokered charset (0x50-terminated, padded). */
void Pokemon_EncodeNameString(const char *src, uint8_t *dst);

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

/* Add a new Pokémon to the player's party at the given level with random DVs.
 * Mirrors _AddPartyMon (engine/pokemon/add_mon.asm).
 * Does nothing if the party is already full (PARTY_LENGTH). */
void Pokemon_AddToParty(uint8_t species, uint8_t level);

/* Store a battle mon into the current PC box.
 * Returns 1 on success, 0 if the current box is full or invalid. */
int Pokemon_SendBattleMonToBox(const battle_mon_t *mon);

/* Move a party mon into the current PC box.
 * Returns 1 on success, 0 on invalid slot or full box. */
int Pokemon_DepositPartyMonToBox(int party_slot);

/* Move a mon from the current PC box into the party.
 * Returns 1 on success, 0 on invalid slot or full party. */
int Pokemon_WithdrawBoxMonToParty(int box_slot);

/* Release a mon from the current PC box.
 * Returns 1 on success, 0 on invalid slot. */
int Pokemon_ReleaseBoxMon(int box_slot);

/* Restore HP, PP, and status for all party Pokémon.
 * Mirrors HealParty (engine/events/heal_party.asm). */
void Pokemon_HealParty(void);
