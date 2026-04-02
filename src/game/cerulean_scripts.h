#pragma once
#include <stdint.h>

/* cerulean_scripts.h — Cerulean City rival battle script.
 *
 * Ports CeruleanCityDefaultScript / RivalBattleScript / RivalDefeatedScript
 * / RivalCleanupScript from scripts/CeruleanCity.asm.
 *
 * Sequence:
 *   Player steps on (20,6) or (21,6) → rival walks 3 steps DOWN →
 *   pre-battle text → battle → post-battle text → rival walks away →
 *   rival hidden.
 */

/* Called on every map load and after battle return. */
void CeruleanScripts_OnMapLoad(void);

/* Called after each player step completes in the overworld. */
void CeruleanScripts_StepCheck(void);

/* Per-frame tick — drives the state machine. */
void CeruleanScripts_Tick(void);

/* Returns 1 while the script is actively running. */
int  CeruleanScripts_IsActive(void);

/* Returns 1 (and fills class/no) when a rival battle is ready to start.
 * Clears the pending flag on return.  Called by game.c each overworld tick. */
int  CeruleanScripts_GetPendingBattle(uint8_t *class_out, uint8_t *no_out);

/* Returns 1 if the last battle started was the Cerulean rival.
 * Clears the flag on return.  Called by game.c after battle ends. */
int  CeruleanScripts_ConsumeRivalBattle(void);

/* Called by game.c after the rival battle ends — player won. */
void CeruleanScripts_OnVictory(void);

/* Called by game.c after the rival battle ends — player lost. */
void CeruleanScripts_OnDefeat(void);
