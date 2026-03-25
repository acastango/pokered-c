#pragma once
/* battle_driver.h — Full battle loop orchestrator.
 *
 * Battle_RunLoop: blocking turn loop that drives the full Gen 1 battle flow.
 *   - Dispatches ENEMY_FAINTED → wild/trainer victory, blackout, or enemy
 *     replacement (including simultaneous player faint handling).
 *   - Dispatches PLAYER_FAINTED → AnyPartyAlive check, exp award for
 *     simultaneous faint, ChooseNextMon for the next alive party slot.
 *   - Dispatches ESCAPED cleanly.
 * Auto-selects the player's first available move each turn.
 * Prints turn-by-turn state to stdout.
 *
 * Caller must set wIsInBattle (1=wild, 2=trainer), load wBattleMon and
 * wEnemyMon, and for trainer battles populate wEnemyMons/wEnemyPartyCount
 * before calling.
 */
void Battle_RunLoop(void);
