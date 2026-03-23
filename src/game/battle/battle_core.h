#pragma once
/* battle_core.h — Gen 1 battle turn execution engine declarations.
 *
 * Port of engine/battle/core.asm (bank $0F):
 *   ExecutePlayerMove          (core.asm:3073)
 *   ExecuteEnemyMove           (core.asm:5457)
 *   HandlePoisonBurnLeechSeed  (core.asm:470)
 *   HandlePlayerMonFainted     (core.asm:969)
 *   HandleEnemyMonFainted      (core.asm:699)
 *
 * UI calls (PrintText, PlayAnimation, DrawHUDs, DelayFrames) are omitted —
 * pure battle state only.
 *
 * ALWAYS refer to pokered-master ASM before modifying anything here.
 */
#include <stdint.h>

/* Battle_ExecutePlayerMove — port of ExecutePlayerMove (core.asm:3073).
 *
 * Sets hWhoseTurn=0, runs CheckPlayerStatusConditions, CheckForDisobedience,
 * damage calculation, MoveHitTest, applies attack to enemy, then dispatches
 * move effects in the correct order (ResidualEffects1/2, SpecialEffectsCont,
 * SetDamageEffects, AlwaysHappenSideEffects, SpecialEffects).
 *
 * Returns immediately if wPlayerSelectedMove==CANNOT_MOVE or
 * wActionResultOrTookBattleTurn is set. */
void Battle_ExecutePlayerMove(void);

/* Battle_ExecuteEnemyMove — port of ExecuteEnemyMove (core.asm:5457).
 *
 * Mirror of Battle_ExecutePlayerMove for the enemy side.
 * Calls CheckEnemyStatusConditions, SwapPlayerAndEnemyLevels around damage
 * calc, applies attack to player, handles multi-hit and rage/bide. */
void Battle_ExecuteEnemyMove(void);

/* Battle_HandlePoisonBurnLeechSeed — port of HandlePoisonBurnLeechSeed (core.asm:470).
 *
 * End-of-turn residual damage for the mon whose turn just ended (hWhoseTurn).
 * Order: burn/poison 1/16-HP (multiplied by toxic counter if badly poisoned),
 *        leech seed 1/16-HP (transfers to opponent HP, capped at max).
 *
 * Returns 0 if the mon fainted (HP reached 0), nonzero if still alive. */
int Battle_HandlePoisonBurnLeechSeed(void);

/* Battle_HandlePlayerMonFainted — port of HandlePlayerMonFainted (core.asm:969).
 *
 * Clears state after the player's active mon faints:
 *   - Sets wInHandlePlayerMonFainted = 1
 *   - Calls RemoveFaintedPlayerMon state logic (clear enemy bide, player status)
 *   - If enemy also has 0 HP, triggers FaintEnemyPokemon state (simultaneous faint)
 * No UI calls; battle loop continuation skipped. */
void Battle_HandlePlayerMonFainted(void);

/* Battle_HandleEnemyMonFainted — port of HandleEnemyMonFainted (core.asm:699).
 *
 * Clears state after the enemy mon faints:
 *   - Sets wInHandlePlayerMonFainted = 0
 *   - Calls FaintEnemyPokemon state logic (clear player bide high byte bug,
 *     clear enemy battle statuses and disabled/minimized flags)
 * No UI calls; exp award / trainer-victory / loop continuation skipped. */
void Battle_HandleEnemyMonFainted(void);
