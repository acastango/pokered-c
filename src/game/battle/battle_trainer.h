#pragma once
/* battle_trainer.h — Enemy mon replacement and end-of-battle logic.
 *
 * Ports from engine/battle/core.asm (bank $0F):
 *   AnyEnemyPokemonAliveCheck  (core.asm:874)
 *   LoadEnemyMonFromParty      (core.asm:1670)
 *   EnemySendOut state portion (core.asm:1276)
 *   ReplaceFaintedEnemyMon     (core.asm:892)
 *   TrainerBattleVictory       (core.asm:915) — state portion only
 *   HandlePlayerBlackOut       (core.asm:1132) — state portion only
 *   TryRunningFromBattle       (core.asm:1496)
 *
 * UI calls (PrintText, PlayMusic, DrawHUDs, PlayCry) are omitted —
 * pure battle state only.
 *
 * ALWAYS refer to pokered-master ASM before modifying anything here.
 */
#include <stdint.h>

/* Battle_AnyEnemyPokemonAliveCheck — AnyEnemyPokemonAliveCheck (core.asm:874).
 *
 * Scans wEnemyMons[0..wEnemyPartyCount-1] HP fields.
 * Returns nonzero if at least one enemy party mon has HP > 0. */
int Battle_AnyEnemyPokemonAliveCheck(void);

/* Battle_LoadEnemyMonFromParty — LoadEnemyMonFromParty (core.asm:1670).
 *
 * Copies wEnemyMons[wEnemyMonPartyPos] into wEnemyMon:
 *   - Species, HP, status, types, catch rate, moves, DVs, PP, level, stats
 *   - Saves stats to wEnemyMonUnmodified* block
 *   - Resets wEnemyMonStatMods to 7 (neutral)
 *   - Applies burn / paralysis stat penalties (hWhoseTurn = 0 targets wEnemyMon) */
void Battle_LoadEnemyMonFromParty(void);

/* Battle_EnemySendOut_State — EnemySendOut state portion (core.asm:1276).
 *
 * Finds the next alive enemy mon (skipping current slot + fainted mons),
 * updates wEnemyMonPartyPos, loads it via Battle_LoadEnemyMonFromParty, and
 * resets per-switch-in enemy battle state:
 *   wEnemyBattleStatus1/2/3, wEnemyDisabledMove, wEnemyDisabledMoveNumber,
 *   wEnemyMonMinimized, wEnemyConfusedCounter, wEnemyToxicCounter,
 *   wEnemyBideAccumulatedDamage, wEnemyNumAttacksLeft.
 * Clears BSTAT1_USING_TRAPPING from wPlayerBattleStatus1 (player's wrap ends).
 * Saves wLastSwitchInEnemyMonHP.
 *
 * Returns 1 if a replacement was found, 0 if no alive enemy mons remain. */
int Battle_EnemySendOut_State(void);

/* Battle_ReplaceFaintedEnemyMon — ReplaceFaintedEnemyMon (core.asm:892).
 *
 * Calls Battle_EnemySendOut_State, then zeros
 * wEnemyMoveNum / wActionResultOrTookBattleTurn / wAILayer2Encouragement.
 *
 * Returns 1 if replacement succeeded, 0 if no alive enemy mons. */
int Battle_ReplaceFaintedEnemyMon(void);

/* Battle_TrainerBattleVictory — TrainerBattleVictory state portion (core.asm:915).
 *
 * Sets wBattleResult = BATTLE_OUTCOME_TRAINER_VICTORY, clears wIsInBattle.
 * Omits: music change, prize money award, PrintText. */
void Battle_TrainerBattleVictory(void);

/* Battle_HandlePlayerBlackOut — HandlePlayerBlackOut state portion (core.asm:1132).
 *
 * Sets wBattleResult = BATTLE_OUTCOME_BLACKOUT, sets wIsInBattle = 0xFF (lost).
 * Omits: PrintText (blacked out message), ClearBikeFlag. */
void Battle_HandlePlayerBlackOut(void);

/* Battle_TryRunningFromBattle — TryRunningFromBattle (core.asm:1496).
 *
 * Wild battle only (returns 0 immediately if wIsInBattle != 1).
 * Increments wNumRunAttempts, then:
 *   divisor = (wEnemyMon.spd >> 2) & 0xFF
 *   if divisor == 0 → always escape
 *   odds = (wBattleMon.spd * 32) / divisor + 30 * (wNumRunAttempts - 1)
 *   if odds >= 256 → always escape
 *   else escape if BattleRandom() < odds
 *
 * On success: sets wEscapedFromBattle = 1, returns 1.
 * On failure: returns 0. */
int Battle_TryRunningFromBattle(void);
