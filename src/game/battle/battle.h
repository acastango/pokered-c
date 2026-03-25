#pragma once
/* battle.h — Gen 1 battle engine declarations.
 *
 * Port of engine/battle/core.asm + engine/battle/effects.asm (bank $0F).
 * ALWAYS refer to the original pokered-master ASM before modifying anything here.
 *
 * Phase 1 (isolated calc functions, no UI):
 *   Battle_CalcDamage      — core.asm:4299 CalculateDamage
 *   Battle_CriticalHitTest — core.asm:4478 CriticalHitTest
 *   Battle_RandomizeDamage — core.asm:5420 RandomizeDamage
 *   Battle_MoveHitTest     — core.asm:5228 MoveHitTest
 *   Battle_CalcHitChance   — core.asm:5348 CalcHitChance
 */
#include <stdint.h>
#include <stdio.h>
#include "../../platform/hardware.h"
#include "../../data/moves_data.h"
#include "../../data/base_stats.h"
#include "../pokemon.h"

/* ---- Battle debug log ---- */
#define BLOG(fmt, ...) fprintf(stderr, "[BATTLE] " fmt "\n", ##__VA_ARGS__)
#define BMON_P()  Pokemon_GetName(gSpeciesToDex[wBattleMon.species])
#define BMON_E()  Pokemon_GetName(gSpeciesToDex[wEnemyMon.species])
#define BMOVE(id) ((unsigned)(id) < NUM_MOVE_DEFS && gMoveNames[(id)] ? gMoveNames[(id)] : "???")

/* StatModifierRatios — data/battle/stat_modifiers.asm
 * stages[stage-1] = {numerator, denominator}  (stage 1-13)
 * Stage 7 (normal) = 1/1.  Stage 1 = 25/100 = 0.25.  Stage 13 = 4/1 = 4.0. */
extern const uint8_t kBattleStatModRatios[13][2];

/* ---- Phase 1: isolated calculation functions ---- */

/* Battle_CalcDamage — exact port of CalculateDamage (core.asm:4299).
 *
 * Formula: ((level×2/5 + 2) × power × attack / defense) / 50
 * Accumulates into wDamage (adds to existing value for multi-hit),
 * caps at MAX_NEUTRAL_DAMAGE − MIN_NEUTRAL_DAMAGE (997),
 * then adds MIN_NEUTRAL_DAMAGE (2) → final range [2, 999].
 *
 * Reads hWhoseTurn and wPlayerMoveEffect/wEnemyMoveEffect to handle:
 *   - EFFECT_EXPLODE: halves defense (min 1)
 *   - EFFECT_TWO_TO_FIVE_ATTACKS / EFFECT_1E: skip power=0 early-exit
 *   - EFFECT_OHKO: not yet implemented (returns 0)
 *
 * Returns 1 if damage was computed, 0 if skipped (power=0 or OHKO stub). */
int Battle_CalcDamage(uint8_t attack, uint8_t defense, uint8_t power, uint8_t level);

/* Battle_CriticalHitTest — exact port of CriticalHitTest (core.asm:4478).
 *
 * Sets wCriticalHitOrOHKO = 1 if the current move is a critical hit, 0 otherwise.
 * Crit rate = (base_speed/2) / 256 for normal moves.
 * Focus Energy bug (Gen 1): halves crit rate instead of doubling.
 * High-crit moves (Slash, Razor Leaf, Karate Chop, Crabhammer): ×8 rate. */
void Battle_CriticalHitTest(void);

/* Battle_RandomizeDamage — exact port of RandomizeDamage (core.asm:5420).
 *
 * Skips if wDamage ≤ 1.
 * Otherwise: loop BattleRandom+rrca until result ≥ 217 (≈85%×255),
 * then multiply wDamage × [217..255] / 255. */
void Battle_RandomizeDamage(void);

/* Battle_MoveHitTest — exact port of MoveHitTest (core.asm:5228).
 *
 * Returns without setting wMoveMissed if the move hits.
 * Sets wDamage=0, wMoveMissed=1 and clears BSTAT1_USING_TRAPPING on miss.
 * Checks (in order): Dream Eater (needs sleep), Swift (always hits),
 * Substitute bug, INVULNERABLE (Fly/Dig), Mist, X-Accuracy, accuracy roll. */
void Battle_MoveHitTest(void);

/* ---- Phase 3: stat setup functions ---- */

/* Battle_CalculateModifiedStats — exact port of CalculateModifiedStats (core.asm:6365).
 *
 * Applies stage modifiers to all 4 battle stats (ATK, DEF, SPD, SPC) for the
 * side indicated by wCalculateWhoseStats (0=player wBattleMon, nonzero=enemy wEnemyMon).
 * Source values: wPlayerMonUnmodified* / wEnemyMonUnmodified*.
 * Stage ratio: StatModifierRatios[stage-1] = {num, den}.
 * Result capped at MAX_NEUTRAL_DAMAGE (999), floored at 1. */
void Battle_CalculateModifiedStats(void);

/* Battle_ApplyBurnAndParalysisPenalties — port of ApplyBurnAndParalysisPenalties (core.asm:6278).
 *
 * Called at mon switch-in (after storing unmodified stats).  hWhoseTurn controls
 * which side's stats are modified:
 *   hWhoseTurn = 1 (player side switch-in):
 *     PAR → wBattleMon.spd >>= 2 (min 1)
 *     BRN → wBattleMon.atk >>= 1 (min 1)
 *   hWhoseTurn = 0 (enemy side switch-in):
 *     PAR → wEnemyMon.spd >>= 2 (min 1)
 *     BRN → wEnemyMon.atk >>= 1 (min 1) */
void Battle_ApplyBurnAndParalysisPenalties(void);

/* Battle_GetDamageVarsForPlayerAttack — port of GetDamageVarsForPlayerAttack (core.asm:4030).
 *
 * Prepares and calls Battle_CalcDamage for the player's attack:
 *   1. Clears wDamage; returns 0 if wPlayerMovePower == 0.
 *   2. Selects physical (ATK vs DEF) or special (SPC vs SPC) based on move type
 *      vs TYPE_SPECIAL_THRESHOLD (0x14).
 *   3. Doubles defender stat if Reflect/Light Screen active on enemy.
 *   4. On crit: bypasses stages — uses wPartyMons[wPlayerMonNumber] for player
 *      attacker stat and recalculates enemy defender stat from base/DVs/level.
 *   5. Scales both stats >> 2 if either high byte is nonzero (prevents overflow).
 *   6. Doubles level on crit.
 * Returns 1 if damage was calculated, 0 if power == 0. */
int Battle_GetDamageVarsForPlayerAttack(void);

/* Battle_GetDamageVarsForEnemyAttack — port of GetDamageVarsForEnemyAttack (core.asm:4143).
 *
 * Same as Battle_GetDamageVarsForPlayerAttack but for the enemy's attack:
 *   Player = defender, enemy = attacker.
 *   Reflect/LightScreen checked on wPlayerBattleStatus3.
 *   On crit: uses wPartyMons[wPlayerMonNumber] for player defender stat,
 *   recalculates enemy attacker stat from base/DVs/level.
 * Returns 1 if damage was calculated, 0 if power == 0. */
int Battle_GetDamageVarsForEnemyAttack(void);

/* Battle_AdjustDamageForMoveType — exact port of AdjustDamageForMoveType (core.asm:5075).
 *
 * Applies STAB and type-chart multipliers to wDamage in order:
 *  1. STAB: if move type matches attacker type1 or type2,
 *           wDamage += (wDamage >> 1)  (= floor(1.5×), no cap).
 *           Sets bit BIT_STAB_DAMAGE (7) of wDamageMultipliers.
 *  2. Type chart: scans TypeEffects table for entries where atk == move_type.
 *           For each match: if defending type1 matches, applies eff once (skips type2
 *           check for that entry); else if defending type2 matches, applies eff once.
 *           eff ∈ {0=immune, 5=NVE, 20=SE}.  Normal (10) has no entry — not applied.
 *           Dual-type mons can be hit by two separate entries (e.g. 4× = 2×2×).
 *           Mono-type mons: matching entry applies exactly once (type2 check skipped).
 *           Each match also updates wDamageMultipliers = (stab_bit) + eff.
 *  3. If wDamage == 0 after any application: sets wMoveMissed = 1. */
void Battle_AdjustDamageForMoveType(void);

/* Battle_CalcHitChance — exact port of CalcHitChance (core.asm:5348).
 *
 * Scales wPlayerMoveAccuracy (or wEnemyMoveAccuracy) by:
 *   1. Attacker's accuracy stage (StatModifierRatios[acc_stage-1])
 *   2. Reflected evasion stage (14 - evasion_stage) of defender
 * Result floored at 1, capped at 255.  Stored back into the accuracy var. */
void Battle_CalcHitChance(void);
