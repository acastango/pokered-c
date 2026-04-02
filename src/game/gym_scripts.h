#pragma once
#include <stdint.h>

/* gym_scripts.h — Gym battle state machine.
 *
 * Each gym leader has an NPC callback (e.g. GymScripts_BrockInteract) that
 * fires when the player presses A on them.  The callback activates the state
 * machine; subsequent ticks drive the pre/post-battle text sequences.
 *
 * Reference: scripts/PewterGym.asm (and other gym ASM scripts).
 */

/* Advance the state machine one frame.  Call from game.c every tick. */
void GymScripts_Tick(void);

/* Returns non-zero while the gym state machine is running (pre-battle text,
 * post-victory sequence, etc.).  game.c should return early and skip the
 * normal overworld loop while this is true. */
int  GymScripts_IsActive(void);

/* Returns 1 and writes class/no if a gym leader battle should start.
 * Clears the pending flag on read.  Call after GymScripts_Tick in game.c. */
int  GymScripts_GetPendingBattle(uint8_t *class_out, uint8_t *no_out);

/* Called from game.c after a gym leader battle ends in TRAINER_VICTORY.
 * Awards the badge, sets the event flag, and starts the post-battle text. */
void GymScripts_OnVictory(void);

/* Called by game.c after a gym trainer NPC battle ends in TRAINER_VICTORY. */
void GymScripts_OnGymTrainerVictory(void);

/* Read-and-clear the gym trainer battle flag.  Always call after any battle
 * that was initiated through gym scripts, regardless of outcome. */
int  GymScripts_ConsumeGymTrainer(void);

/* NPC script callbacks — set as the .script field in kNpcs_* arrays. */
/* Pewter Gym */
void GymScripts_BrockInteract(void);
void GymScripts_GymTrainerInteract(void);
void GymScripts_GuideInteract(void);
/* Cerulean Gym */
void GymScripts_MistyInteract(void);
void GymScripts_CeruleanTrainer0Interact(void);
void GymScripts_CeruleanTrainer1Interact(void);
void GymScripts_CeruleanGuideInteract(void);
