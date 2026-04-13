#pragma once
/* route22_scripts.h — Route 22 rival encounter (rival 1 early-game battle).
 *
 * Trigger: player steps on (29,4) or (29,5) while
 *   EVENT_ROUTE22_RIVAL_WANTS_BATTLE + EVENT_1ST_ROUTE22_RIVAL_BATTLE set
 *   and EVENT_BEAT_ROUTE22_RIVAL_1ST_BATTLE not set.
 *
 * Those events are set at the end of the Oak's Lab Pokédex sequence
 * (OLS_RIVAL_LEAVE_DONE in oakslab_scripts.c).
 */
#include <stdint.h>

/* Call on every map load (after NPC_Load). */
void Route22Scripts_OnMapLoad(void);

/* Call after every step completes (before wild encounter check). */
void Route22Scripts_StepCheck(void);

/* Per-frame update — drives the emote/walk/text/exit state machine. */
void Route22Scripts_Tick(void);

/* Returns non-zero while the script is in progress (suppresses overworld). */
int Route22Scripts_IsActive(void);

/* If a rival battle is ready to start, fills class/no and returns 1 (once). */
int Route22Scripts_GetPendingBattle(uint8_t *class_out, uint8_t *no_out);

/* game.c calls this after a battle ends to check if it was this rival. */
int Route22Scripts_ConsumeRivalBattle(void);

/* Post-battle callbacks from game.c. */
void Route22Scripts_OnVictory(void);
void Route22Scripts_OnDefeat(void);
