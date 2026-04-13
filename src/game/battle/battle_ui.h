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
/* Restore the battle UI to the idle "draw HUD + choose action" state.
 * Called after loading a save state that was captured mid-battle. */
void BattleUI_Restore(void);

/* Advance the battle UI by one frame (60 Hz tick).
 * Must only be called when Text_IsOpen() is false. */
void BattleUI_Tick(void);

/* Returns 1 while a battle is in progress, 0 when it has ended. */
int BattleUI_IsActive(void);

/* Debug: returns current bui_state_t value as int. */
int BattleUI_GetState(void);

/* Queue badge-info text to be shown after the key-item jingle in gym-leader
 * defeat sequences.  Set before the battle starts (e.g. in GS_BROCK_PRE_WAIT).
 * Mirrors the text_far _PewterGymBrockBoulderBadgeInfoText chained after
 * _PewterGymBrockReceivedBoulderBadgeText with sound_level_up between them. */
/* "[PLAYER] received the BADGE!" — shown simultaneously with key-item jingle. */
void BattleUI_SetBadgeRecvText(const char *text);
/* Badge info pages shown after the received text is dismissed. */
void BattleUI_SetBadgeInfoText(const char *text);
