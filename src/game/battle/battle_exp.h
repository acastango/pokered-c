#pragma once
/* battle_exp.h — Experience gain and level-up system.
 *
 * Ports engine/battle/experience.asm (GainExperience, DivideExpDataByNumMonsGainingExp)
 * and engine/pokemon/experience.asm (CalcLevelFromExperience).
 *
 * ALWAYS refer to pokered-master source before modifying.
 */
#include <stdint.h>

/* Battle_GainExperience — GainExperience (experience.asm:1).
 * Awards exp and stat exp to all party mons with wPartyGainExpFlags set.
 * Handles level-up stat recalc, HP gain, and LearnMoveFromLevelUp.
 * No-op if wLinkState == LINK_STATE_BATTLING. */
void Battle_GainExperience(void);

/* Battle_CalcLevelFromExp — CalcLevelFromExperience (pokemon/experience.asm:2).
 * Returns the level a mon with the given growth_rate should be at for exp.
 * Iterates levels 2..MAX_LEVEL, returns the highest level whose exp threshold
 * does not exceed exp. */
uint8_t Battle_CalcLevelFromExp(uint8_t growth_rate, uint32_t exp);

/* Battle_LearnMoveFromLevelUp — LearnMoveFromLevelUp (engine/pokemon/evos_moves.asm:323).
 * Checks the learnset for party slot `slot` at `new_level`.
 * If a move is learned and a free slot exists, adds it; otherwise replaces last slot.
 * Syncs to wBattleMon if slot == wPlayerMonNumber. */
void Battle_LearnMoveFromLevelUp(uint8_t slot, uint8_t new_level);

/* Debug exp rate multiplier: 100 = 1×, 150 = 1.5×, 200 = 2×, 300 = 3×.
 * Set via the "exprate" CLI command.  Applied inside Battle_GainExperience. */
extern int gDebugExpRate;

/* Battle_AddExpDirect — add raw exp to party slot directly (debug/testing).
 * Handles level-up stat recalc, HP delta, move learning, wBattleMon sync.
 * Does NOT queue text or trigger evolutions. */
void Battle_AddExpDirect(uint8_t slot, uint32_t amount);

/* Battle_EvolutionAfterBattle — silent fallback (no animation).
 * Loops Battle_CheckNextEvolution + Battle_ApplyEvolution until done.
 * Used by debug/tests; battle_ui.c uses the animated BUI_EVOLUTION path instead. */
void Battle_EvolutionAfterBattle(void);

/* Battle_CheckNextEvolution — find the first party mon in wCanEvolveFlags whose
 * evo conditions are met.  Returns 1 and fills *slot_out / *new_species_out;
 * returns 0 if nothing pending.  Does NOT modify any state. */
int Battle_CheckNextEvolution(uint8_t *slot_out, uint8_t *new_species_out);

/* Battle_ApplyEvolution — commit evolution for (slot, new_species): species change,
 * CalcStats, HP delta, types, wBattleMon sync, LearnMoveFromLevelUp, Pokédex flags.
 * Clears the wCanEvolveFlags bit for slot. */
void Battle_ApplyEvolution(uint8_t slot, uint8_t new_species);

/* Battle_CancelEvolution — clear the wCanEvolveFlags bit for slot without evolving. */
void Battle_CancelEvolution(uint8_t slot);

/* Stats snapshot attached to a level-up queue entry.
 * valid==1: this text is a "grew to level N!" message; show stats box after. */
typedef struct {
    int      valid;
    uint16_t atk, def, spd, spc;
} levelup_stats_t;

/* BattleExp_TakeNextText — returns the next pending exp / level-up text string,
 * or NULL when the queue is empty.  Each call advances the queue by one.
 * The queue is populated during Battle_GainExperience() and drained by battle_ui.c
 * via the BUI_EXP_DRAIN state.  Strings are valid until the next
 * Battle_GainExperience() call.
 * If stats_out is non-NULL, it is filled with level-up stat data (valid==1)
 * or zeroed (valid==0) for non-level-up entries. */
const char *BattleExp_TakeNextText(levelup_stats_t *stats_out);
