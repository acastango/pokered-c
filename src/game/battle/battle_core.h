#pragma once
/* battle_core.h — Gen 1 battle turn execution engine declarations.
 *
 * Port of engine/battle/core.asm (bank $0F):
 *   ExecutePlayerMove          (core.asm:3073)
 *   ExecuteEnemyMove           (core.asm:5457)
 *   HandlePoisonBurnLeechSeed  (core.asm:470)
 *   HandlePlayerMonFainted     (core.asm:969)
 *   HandleEnemyMonFainted      (core.asm:699)
 *
 * UI calls (PrintText, PlayAnimation, DrawHUDs, DelayFrames) are omitted —
 * pure battle state only.
 *
 * ALWAYS refer to pokered-master ASM before modifying anything here.
 */
#include <stdint.h>

/* Battle_ExecutePlayerMove — port of ExecutePlayerMove (core.asm:3073).
 *
 * Sets hWhoseTurn=0, runs CheckPlayerStatusConditions, CheckForDisobedience,
 * damage calculation, MoveHitTest, applies attack to enemy, then dispatches
 * move effects in the correct order (ResidualEffects1/2, SpecialEffectsCont,
 * SetDamageEffects, AlwaysHappenSideEffects, SpecialEffects).
 *
 * Returns immediately if wPlayerSelectedMove==CANNOT_MOVE or
 * wActionResultOrTookBattleTurn is set. */
void Battle_ExecutePlayerMove(void);

/* Battle_ExecuteEnemyMove — port of ExecuteEnemyMove (core.asm:5457).
 *
 * Mirror of Battle_ExecutePlayerMove for the enemy side.
 * Calls CheckEnemyStatusConditions, SwapPlayerAndEnemyLevels around damage
 * calc, applies attack to player, handles multi-hit and rage/bide. */
void Battle_ExecuteEnemyMove(void);

/* Battle_HandlePoisonBurnLeechSeed — port of HandlePoisonBurnLeechSeed (core.asm:470).
 *
 * End-of-turn residual damage for the mon whose turn just ended (hWhoseTurn).
 * Order: burn/poison 1/16-HP (multiplied by toxic counter if badly poisoned),
 *        leech seed 1/16-HP (transfers to opponent HP, capped at max).
 *
 * Returns 0 if the mon fainted (HP reached 0), nonzero if still alive. */
int Battle_HandlePoisonBurnLeechSeed(void);

/* Battle_HandlePlayerMonFainted — port of HandlePlayerMonFainted (core.asm:969).
 *
 * Clears state after the player's active mon faints:
 *   - Sets wInHandlePlayerMonFainted = 1
 *   - Calls RemoveFaintedPlayerMon state logic (clear enemy bide, player status)
 *   - If enemy also has 0 HP, triggers FaintEnemyPokemon state (simultaneous faint)
 * No UI calls; battle loop continuation skipped. */
void Battle_HandlePlayerMonFainted(void);

/* Battle_HandleEnemyMonFainted — port of HandleEnemyMonFainted (core.asm:699).
 *
 * Clears state after the enemy mon faints:
 *   - Sets wInHandlePlayerMonFainted = 0
 *   - Calls FaintEnemyPokemon state logic (clear player bide high byte bug,
 *     clear enemy battle statuses and disabled/minimized flags)
 * No UI calls; exp award / trainer-victory / loop continuation skipped. */
void Battle_HandleEnemyMonFainted(void);

/* ---- Status message tracking ---- */

/* Status messages emitted during CheckPlayerStatusConditions /
 * CheckEnemyStatusConditions.  Populated per Battle_Execute*Move() call.
 * Read by battle_ui.c to display the appropriate in-battle text.
 *
 * Block messages (PSTAT_DONE returned, move was NOT used this turn):
 *   FAST_ASLEEP, WOKE_UP, FROZEN, CANT_MOVE, FLINCHED, MUST_RECHARGE,
 *   HURT_ITSELF, FULLY_PARALYZED.
 * Pre-move messages (shown before move text; mon still moves this turn):
 *   IS_CONFUSED, DISABLED_NO_MORE, CONFUSED_NO_MORE. */
typedef enum {
    BSTAT_MSG_NONE = 0,
    BSTAT_MSG_FAST_ASLEEP,       /* "[mon] is fast asleep!"            */
    BSTAT_MSG_WOKE_UP,           /* "[mon] woke up!"                   */
    BSTAT_MSG_FROZEN,            /* "[mon] is frozen solid!"           */
    BSTAT_MSG_CANT_MOVE,         /* "[mon] can't move!" (trapping)     */
    BSTAT_MSG_FLINCHED,          /* "[mon] flinched!"                  */
    BSTAT_MSG_MUST_RECHARGE,     /* "[mon] must recharge!"             */
    BSTAT_MSG_HURT_ITSELF,       /* "It hurt itself in its confusion!" */
    BSTAT_MSG_FULLY_PARALYZED,   /* "[mon]'s fully paralyzed!"         */
    BSTAT_MSG_IS_CONFUSED,       /* "[mon] is confused!" (pre-move)    */
    BSTAT_MSG_DISABLED_NO_MORE,  /* "[mon]'s disabled no more!"        */
    BSTAT_MSG_CONFUSED_NO_MORE,  /* "[mon]'s confused no more!"        */
} battle_status_msg_t;

/* Battle_GetPlayerStatusMsg / Battle_GetPlayerPreStatusMsg —
 * Return the block / pre-move status message set during the last
 * Battle_ExecutePlayerMove() call.  BSTAT_MSG_NONE if none. */
battle_status_msg_t Battle_GetPlayerStatusMsg(void);
battle_status_msg_t Battle_GetPlayerPreStatusMsg(void);

/* Battle_GetEnemyStatusMsg / Battle_GetEnemyPreStatusMsg —
 * Return the block / pre-move status message set during the last
 * Battle_ExecuteEnemyMove() call.  BSTAT_MSG_NONE if none. */
battle_status_msg_t Battle_GetEnemyStatusMsg(void);
battle_status_msg_t Battle_GetEnemyPreStatusMsg(void);
