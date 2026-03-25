#pragma once
/* battle_init.h — Battle setup: copies party/wild state into wBattleMon/wEnemyMon.
 *
 * Port of StartBattle / InitBattle (engine/battle/core.asm) — state only.
 * UI (transition animation, trainer text) omitted.
 */
#include <stdint.h>

/* Battle_Start — initialize a wild battle.
 *
 * Reads wCurPartySpecies + wCurEnemyLevel (set by wild encounter detection).
 * Reads wPartyMons[0] as the player's active mon.
 * Writes wBattleMon, wEnemyMon, and all battle status vars.
 */
void Battle_Start(void);

/* Battle_ReadTrainer — build wEnemyMons[] from trainer party data.
 *
 * Ports ReadTrainer (engine/battle/read_trainer_party.asm).
 * trainer_class: 1..NUM_TRAINERS  (OPP_YOUNGSTER=1 .. OPP_LANCE=47)
 * trainer_no:    1-based index within that class's party list
 * Sets wEnemyPartyCount. */
void Battle_ReadTrainer(uint8_t trainer_class, uint8_t trainer_no);

/* Battle_StartTrainer — initialize a full trainer battle.
 *
 * Ports InitBattle/InitOpponent/InitBattleCommon (engine/battle/core.asm:6642).
 * Sets wIsInBattle=2, wAICount=0xFF, builds enemy party via Battle_ReadTrainer,
 * then calls Battle_EnemySendOut_State to load the first alive enemy mon.
 */
void Battle_StartTrainer(uint8_t trainer_class, uint8_t trainer_no);
