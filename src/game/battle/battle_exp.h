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

/* Battle_EvolutionAfterBattle — EvolutionAfterBattle (engine/pokemon/evos_moves.asm:13).
 * For each party mon with wCanEvolveFlags set, check evo conditions and evolve.
 * EVOLVE_TRADE is skipped (not link battling in smoke test).
 * EVOLVE_ITEM replicates the wCurItem == wCurPartySpecies overlap bug faithfully.
 * Animation is skipped — always treated as accepted. */
void Battle_EvolutionAfterBattle(void);

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
