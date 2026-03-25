#pragma once
/* battle_ui.h — Non-blocking battle scene controller.
 *
 * Call BattleUI_Enter() immediately after Battle_Start() to hand
 * the battle over to the UI state machine.  Then route each
 * GameTick to BattleUI_Tick() while BattleUI_IsActive() is true.
 *
 * When BattleUI_IsActive() returns 0 the battle is over; the caller
 * is responsible for restoring the overworld scene.
 */

/* Start a battle UI session.
 * Precondition: Battle_Start() (or Battle_StartTrainer()) already called. */
void BattleUI_Enter(void);

/* Advance the battle UI by one frame (60 Hz tick).
 * Must only be called when Text_IsOpen() is false. */
void BattleUI_Tick(void);

/* Returns 1 while a battle is in progress, 0 when it has ended. */
int BattleUI_IsActive(void);
