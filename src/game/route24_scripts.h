#pragma once
#include <stdint.h>

/* route24_scripts.h — Route 24 (Nugget Bridge) Rocket scripted encounter.
 *
 * Ports scripts/Route24.asm:
 *   Route24DefaultScript          — step trigger at (10,15)
 *   Route24AfterRocketBattleScript — post-battle text + nugget give
 *
 * Trigger: player steps on (10,15) in Route 24 (map 0x23)
 * while EVENT_GOT_NUGGET is not set.
 */

/* Called on every map load. */
void Route24Scripts_OnMapLoad(void);

/* Called after each player step completes in the overworld. */
void Route24Scripts_StepCheck(void);

/* Per-frame tick — drives the state machine. */
void Route24Scripts_Tick(void);

/* Returns 1 while the script is actively running. */
int  Route24Scripts_IsActive(void);

/* Returns 1 (and fills class/no) when the Rocket battle is ready.
 * Clears the pending flag on return.  Called by game.c each overworld tick. */
int  Route24Scripts_GetPendingBattle(uint8_t *class_out, uint8_t *no_out);

/* Returns 1 if the last battle started was the Route 24 Rocket.
 * Clears the flag on return.  Called by game.c after battle ends. */
int  Route24Scripts_ConsumeRocketBattle(void);

/* Called by game.c after the Rocket battle ends — player won. */
void Route24Scripts_OnVictory(void);

/* Called by game.c after the Rocket battle ends — player lost. */
void Route24Scripts_OnDefeat(void);
