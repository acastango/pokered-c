#pragma once
/* battle_effects.h — Gen 1 move effects engine declarations.
 *
 * Port of engine/battle/effects.asm + engine/battle/move_effects/*.asm (bank $0F).
 * ALWAYS refer to the original pokered-master ASM before modifying anything here.
 *
 * Phase 4: JumpMoveEffect dispatches to ~87 effect handlers via switch.
 * UI calls (PrintText, PlayAnimation, DelayFrames) are omitted — pure state only.
 *
 * Key ASM references:
 *   engine/battle/effects.asm        — JumpMoveEffect, inline handlers
 *   data/moves/effects_pointers.asm  — full dispatch table
 *   engine/battle/move_effects/*.asm — farCall handlers (drain, haze, heal, etc.)
 */
#include <stdint.h>

/* Battle_JumpMoveEffect — exact port of JumpMoveEffect (effects.asm:1).
 *
 * Reads hWhoseTurn to select wPlayerMoveEffect or wEnemyMoveEffect,
 * then dispatches to the appropriate handler.  NULL entries in the original
 * pointer table (MIRROR_MOVE, SWIFT, SUPER_FANG, SPECIAL_DAMAGE, JUMP_KICK,
 * METRONOME) return immediately — those are handled in core.asm before this
 * is called, or require no additional state change. */
void Battle_JumpMoveEffect(void);

/* Battle_QuarterSpeedDueToParalysis — port of QuarterSpeedDueToParalysis
 * (core.asm:6283).
 *
 * Applies PAR speed penalty to the MON WHOSE TURN IS NOT ACTIVE.
 * Called after every stat change (Gen 1 quirk — comment in ASM: "these
 * shouldn't be here").  On player's turn: quarters enemy speed if paralyzed.
 * On enemy's turn: quarters player speed if paralyzed.  Min 1. */
void Battle_QuarterSpeedDueToParalysis(void);

/* Battle_HalveAttackDueToBurn — port of HalveAttackDueToBurn (core.asm:6326).
 *
 * Same as above but for BRN → halves attack of non-active side.  Min 1. */
void Battle_HalveAttackDueToBurn(void);
