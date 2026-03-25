#pragma once
/* trainer_sight.h — Gen 1 trainer line-of-sight detection and engagement.
 *
 * Ports CheckFightingMapTrainers / TrainerEngage / TrainerWalkUpToPlayer from
 * home/trainers.asm and engine/overworld/trainer_sight.asm.
 *
 * ALWAYS refer to pokered-master ASM before modifying anything here.
 *
 * Call order each map load:
 *   Trainer_LoadMap()  — set initial NPC facing directions from trainer data
 *
 * Call order each overworld tick (from game.c):
 *   if (Trainer_IsEngaging()) {
 *       if (Trainer_SightTick()) { ... start trainer battle ... }
 *       return;
 *   }
 *   ... after step completes: Trainer_CheckSight() ...
 */
#include <stdint.h>

/* Initialize trainer state for the current map.
 * Applies the initial facing direction for each trainer NPC.
 * Call after NPC_Load() whenever the map changes. */
void Trainer_LoadMap(void);

/* Per-step check: scan all trainers on the current map for line-of-sight.
 * If a trainer sees the player, starts the engagement state machine.
 * Call from game.c when gStepJustCompleted fires, before wild encounters. */
void Trainer_CheckSight(void);

/* Per-frame update: advances the engagement state machine.
 * Returns 1 on the frame when the battle is ready to start.
 * Caller should then launch Music_Play(MUSIC_TRAINER_BATTLE) and
 * BattleTransition_Start(), followed by Battle_StartTrainer(class, no).
 * Returns 0 while the sequence is still in progress. */
int Trainer_SightTick(void);

/* Returns 1 if trainer engagement is in progress (sight → walk → text). */
int Trainer_IsEngaging(void);

/* Mark the currently-engaged trainer as defeated in wEventFlags.
 * Call from game.c after a trainer battle ends (player won). */
void Trainer_MarkCurrentDefeated(void);

/* Trainer class and party number of the engaged trainer.
 * Valid while Trainer_IsEngaging() or after Trainer_SightTick() returns 1. */
extern uint8_t gEngagedTrainerClass;
extern uint8_t gEngagedTrainerNo;
