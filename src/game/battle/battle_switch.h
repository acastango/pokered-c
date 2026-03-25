#pragma once
/* battle_switch.h — Player mon switching in battle.
 *
 * Ports from engine/battle/core.asm (bank $0F):
 *   AnyPartyAlive              (core.asm:1455)
 *   HasMonFainted              (core.asm:1473)
 *   LoadBattleMonFromParty     (core.asm:1626)
 *   SendOutMon state portion   (core.asm:1723)
 *   ReadPlayerMonCurHPAndStatus (core.asm:1800)
 *   SwitchPlayerMon            (core.asm:2419)
 *   ChooseNextMon              (core.asm:1083)
 *   ApplyBadgeStatBoosts       (core.asm:6454)
 *
 * UI calls (PrintText, animations, DrawHUDs, PlayCry) are omitted —
 * pure battle state only.
 *
 * ALWAYS refer to pokered-master ASM before modifying anything here.
 */
#include <stdint.h>

/* Battle_AnyPartyAlive — AnyPartyAlive (core.asm:1455).
 *
 * Returns nonzero if at least one party mon has HP > 0. */
int Battle_AnyPartyAlive(void);

/* Battle_HasMonFainted — HasMonFainted (core.asm:1473).
 *
 * Returns 1 (zero flag set) if wPartyMons[slot].hp == 0, else 0. */
int Battle_HasMonFainted(uint8_t slot);

/* Battle_LoadBattleMonFromParty — LoadBattleMonFromParty (core.asm:1626).
 *
 * Copies party mon at wPlayerMonNumber into wBattleMon:
 *   - Species, HP, status, types, catch rate, moves, DVs, PP, level, stats
 *   - Saves stats to wPlayerMonUnmodified* block
 *   - Resets wPlayerMonStatMods to 7 (neutral)
 *   - Applies burn / paralysis stat penalties (ApplyBurnAndParalysisPenaltiesToPlayer)
 *   - Applies badge stat boosts (ApplyBadgeStatBoosts) */
void Battle_LoadBattleMonFromParty(void);

/* Battle_SendOutMon_State — SendOutMon state portion (core.asm:1723).
 *
 * Resets per-switch-in battle state when a new player mon enters:
 *   wBoostExpByExpAll, wDamageMultipliers, wPlayerMoveNum,
 *   wPlayerUsedMove, wEnemyUsedMove,
 *   wPlayerBattleStatus1/2/3,
 *   wPlayerDisabledMove, wPlayerDisabledMoveNumber,
 *   wPlayerMonMinimized, wPlayerConfusedCounter, wPlayerToxicCounter,
 *   wPlayerBideAccumulatedDamage.
 * Clears USING_TRAPPING_MOVE from wEnemyBattleStatus1.
 * Sets hWhoseTurn = 1 (player side entering). */
void Battle_SendOutMon_State(void);

/* Battle_ReadPlayerMonCurHPAndStatus — ReadPlayerMonCurHPAndStatus (core.asm:1800).
 *
 * Copies wBattleMon HP + status back to wPartyMons[wPlayerMonNumber].
 * Called before retreating to keep party data current. */
void Battle_ReadPlayerMonCurHPAndStatus(void);

/* Battle_SwitchPlayerMon — SwitchPlayerMon (core.asm:2419).
 *
 * Voluntary player switch (PKMN menu during battle):
 *   1. Syncs current mon HP/status back to party (ReadPlayerMonCurHPAndStatus)
 *   2. Sets wPlayerMonNumber = new_slot
 *   3. Sets wPartyGainExpFlags and wPartyFoughtCurrentEnemyFlags bits
 *   4. Calls Battle_LoadBattleMonFromParty
 *   5. Calls Battle_SendOutMon_State
 *   6. Sets wActionResultOrTookBattleTurn = 1 (turn consumed)
 *
 * Caller is responsible for validating new_slot is not the current mon
 * and has not fainted (HasMonFainted). */
void Battle_SwitchPlayerMon(uint8_t new_slot);

/* Battle_ChooseNextMon — ChooseNextMon (core.asm:1083).
 *
 * Forced switch after the active player mon faints:
 *   1. Sets wPlayerMonNumber = new_slot
 *   2. Sets wPartyGainExpFlags and wPartyFoughtCurrentEnemyFlags bits
 *   3. Calls Battle_LoadBattleMonFromParty
 *   4. Calls Battle_SendOutMon_State
 *
 * Does NOT sync HP/status back (retreating mon already fainted).
 * Does NOT set wActionResultOrTookBattleTurn (ChooseNextMon sets it
 * only for link battles; skipped here). */
void Battle_ChooseNextMon(uint8_t new_slot);
