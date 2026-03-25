/* battle_transition.h
 *
 * Non-blocking state machine for the wild/trainer battle transition animation.
 * Ports engine/battle/battle_transitions.asm.
 *
 * Usage:
 *   BattleTransition_Start(is_trainer, enemy_level, player_level, is_dungeon);
 *   // each 60 Hz frame:
 *   if (BattleTransition_Tick()) { ... battle is ready to start ... }
 */
#pragma once
#include <stdint.h>

/* Start a battle transition.
 *   is_trainer   : 1 if trainer battle, 0 if wild
 *   enemy_level  : wCurEnemyLevel
 *   player_level : level of first non-fainted party mon
 *   is_dungeon   : 1 if current map is a dungeon map
 */
void BattleTransition_Start(int is_trainer, int enemy_level,
                             int player_level, int is_dungeon);

/* Tick one 60 Hz frame.  Returns 1 when the transition is complete
 * (screen is fully black, battle can begin). */
int BattleTransition_Tick(void);

/* True while a transition is running (IDLE and DONE both return 0). */
int BattleTransition_IsActive(void);
