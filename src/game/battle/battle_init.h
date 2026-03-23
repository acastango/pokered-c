#pragma once
/* battle_init.h — Battle setup: copies party/wild state into wBattleMon/wEnemyMon.
 *
 * Port of StartBattle (engine/battle/core.asm) — state setup only.
 * UI (transition animation, trainer text) omitted for skeleton driver.
 */

/* Battle_Start — initialize a wild battle.
 *
 * Reads wCurPartySpecies + wCurEnemyLevel (set by wild encounter detection).
 * Reads wPartyMons[0] as the player's active mon.
 * Writes wBattleMon, wEnemyMon, and all battle status vars.
 */
void Battle_Start(void);
