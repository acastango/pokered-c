#pragma once
/* battle_loop.h — Gen 1 battle main loop declarations.
 *
 * Port of engine/battle/core.asm (bank $0F):
 *   SelectEnemyMove     (core.asm:2915)
 *   CheckNumAttacksLeft (core.asm:683)
 *   MainInBattleLoop    (core.asm:280) — pure state, no UI
 *
 * UI calls (DisplayBattleMenu, DrawHUDs, SaveScreenTiles) are omitted.
 * Trainer AI (AIEnemyTrainerChooseMoves) is omitted — enemy uses random
 * slot selection for both wild and trainer battles.
 *
 * ALWAYS refer to pokered-master ASM before modifying anything here.
 */
#include <stdint.h>

/* Result codes returned by Battle_RunTurn — mirrors the exit paths of
 * MainInBattleLoop (ret / jp HandleXxxMonFainted / jp EnemyRan). */
typedef enum {
    BATTLE_RESULT_CONTINUE       = 0,  /* both sides alive, loop again   */
    BATTLE_RESULT_PLAYER_FAINTED = 1,  /* player mon HP reached 0        */
    BATTLE_RESULT_ENEMY_FAINTED  = 2,  /* enemy mon HP reached 0         */
    BATTLE_RESULT_ESCAPED        = 3,  /* wEscapedFromBattle set          */
} battle_result_t;

/* Battle_SelectEnemyMove — port of SelectEnemyMove (core.asm:2915).
 *
 * Sets wEnemySelectedMove.  Returns immediately (keeping current value)
 * if the enemy is locked into a move (recharge/rage/charge/thrash/sleep/
 * frozen/bide/trapping).  If caught in player's trapping move, sets
 * wEnemySelectedMove = CANNOT_MOVE.
 *
 * Wild and trainer battles both use random 25%-per-slot selection;
 * disabled and empty (0) slots are retried.  Single available move
 * that is disabled → STRUGGLE. */
void Battle_SelectEnemyMove(void);

/* Battle_CheckNumAttacksLeft — port of CheckNumAttacksLeft (core.asm:683).
 *
 * Clears BSTAT1_USING_TRAPPING for each side whose NumAttacksLeft = 0. */
void Battle_CheckNumAttacksLeft(void);

/* Battle_RunTurn — port of MainInBattleLoop state logic (core.asm:280).
 *
 * Executes one full battle turn:
 *   1. HP=0 check at loop entry → faint handlers.
 *   2. Clear flinch bits (when player not locked).
 *   3. Auto-set wPlayerSelectedMove=CANNOT_MOVE if enemy is trapping.
 *   4. Call Battle_SelectEnemyMove.
 *   5. Determine move order: Quick Attack > Counter > speed > random tie.
 *   6. Execute both moves in order; apply residual (poison/burn/leech)
 *      after each attacker; check faint after each event.
 *   7. Call Battle_CheckNumAttacksLeft.
 *
 * Returns a battle_result_t indicating what ended the turn (or
 * BATTLE_RESULT_CONTINUE to keep looping). */
battle_result_t Battle_RunTurn(void);
