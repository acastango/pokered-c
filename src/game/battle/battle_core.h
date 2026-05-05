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

typedef enum {
    BATTLE_EVENT_NONE = 0,
    BATTLE_EVENT_PLAY_ANIM = 1,
    BATTLE_EVENT_STATUS_MSG = 2,
    BATTLE_EVENT_HIT_SFX = 3,
    BATTLE_EVENT_HP_TARGET = 4,
    BATTLE_EVENT_MOVE_RESULT = 5,
    BATTLE_EVENT_RESIDUAL_MSG = 6,
} battle_event_type_t;

typedef enum {
    BATTLE_MOVE_RESULT_NONE = 0,
    BATTLE_MOVE_RESULT_MISS = 1,
    BATTLE_MOVE_RESULT_NO_EFFECT = 2,
    BATTLE_MOVE_RESULT_UNAFFECTED = 3,
} battle_move_result_t;

typedef enum {
    BATTLE_RESIDUAL_MSG_NONE = 0,
    BATTLE_RESIDUAL_MSG_BURN = 1,
    BATTLE_RESIDUAL_MSG_POISON = 2,
    BATTLE_RESIDUAL_MSG_LEECH_SEED = 3,
} battle_residual_msg_t;

typedef struct {
    uint8_t type;
    uint8_t arg0; /* PLAY_ANIM: anim id | STATUS_MSG: battle_status_msg_t
                   * HIT_SFX: damage multiplier | HP_TARGET: 0 enemy / 1 player
                   * MOVE_RESULT: battle_move_result_t
                   * RESIDUAL_MSG: battle_residual_msg_t */
    uint8_t arg1; /* PLAY_ANIM: forced hWhoseTurn (0/1/0xFF) | STATUS_MSG: 0=block,1=pre
                   * RESIDUAL_MSG: side (0=player,1=enemy) */
    uint8_t arg2; /* STATUS_MSG: side (0=player,1=enemy), otherwise reserved */
} battle_event_t;

/* ASM-derived battle event queue for per-execute ordered side effects.
 * Queue is cleared at the start of each Battle_Execute*Move() call. */
void BattleEvent_ResetTurnQueue(void);
int BattleEvent_Pop(battle_event_t *out);
int BattleEvent_HasPending(void);

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
    BSTAT_MSG_MOVE_DISABLED,     /* "[mon]'s move is disabled!"        */
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

/* Last executed move hit-phase snapshots for UI sequencing.
 * For ATTACK_TWICE/TWINEEDLE-style multi-hit moves, *_HitCount can be >1 and
 * *_FirstTargetHP stores target HP after hit #1 (before hit #2 animation/HP bar). */
uint8_t  Battle_GetLastPlayerHitCount(void);
uint16_t Battle_GetLastPlayerFirstTargetHP(void);
uint8_t  Battle_GetLastEnemyHitCount(void);
uint16_t Battle_GetLastEnemyFirstTargetHP(void);

/* STATUS_AFFECTED_ANIM pending flags (core.asm:3475 / 5851 path).
 * Set only when confusion self-hit or full paralysis aborts a Fly/Charge turn.
 * Cleared at the start of each Battle_Execute*Move() call. */
uint8_t Battle_GetPlayerStatusAffectedAnimPending(void);
uint8_t Battle_GetEnemyStatusAffectedAnimPending(void);
uint8_t Battle_GetPlayerStatusAnimId(void);
uint8_t Battle_GetEnemyStatusAnimId(void);
uint8_t Battle_GetPlayerConfusionSelfHitAnimPending(void);
uint8_t Battle_GetEnemyConfusionSelfHitAnimPending(void);
