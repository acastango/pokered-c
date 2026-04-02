#pragma once
#include <stdint.h>

/* ss_anne_scripts.h — SS Anne story scripts.
 *
 * Covers:
 *   - Rival2 battle on SS_ANNE_2F (step trigger)
 *   - Captain cabin HM01 Cut sequence (NPC callback)
 *
 * Trainer room battles (1F_ROOMS, 2F_ROOMS, B1F_ROOMS, BOW) are handled
 * by the standard map_trainer_t / Trainer_CheckSight system via event_data.c.
 */

/* Called on every map load and after battle return. */
void SSAnneScripts_OnMapLoad(void);

/* Called after each player step completes in the overworld. */
void SSAnneScripts_StepCheck(void);

/* Per-frame tick — drives the rival and captain state machines. */
void SSAnneScripts_Tick(void);

/* Returns 1 while a script is actively running. */
int  SSAnneScripts_IsActive(void);

/* Returns 1 (and fills class/no) when the rival battle is ready to start.
 * Clears the pending flag on return. */
int  SSAnneScripts_GetPendingBattle(uint8_t *class_out, uint8_t *no_out);

/* Returns 1 if the last battle started was the SS Anne rival.
 * Clears the flag on return. */
int  SSAnneScripts_ConsumeRivalBattle(void);

/* Called by game.c after the rival battle ends. */
void SSAnneScripts_OnVictory(void);
void SSAnneScripts_OnDefeat(void);

/* npc_script_fn callback — wired into the captain NPC in event_data.c.
 * Called when player presses A on the captain. */
void SSAnne_CaptainScript(void);
