#pragma once
/* battle_catch.h — Pokéball catch mechanic.
 *
 * Ports ItemUseBall (engine/items/item_effects.asm:104).
 * ALWAYS refer to pokered-master source before modifying.
 */
#include <stdint.h>

/* Result of a catch attempt — mirrors wPokeBallAnimData values from the ASM.
 * $43 = success, $10 = cannot catch, $20/$61/$62/$63 = 0/1/2/3 shakes. */
typedef enum {
    CATCH_RESULT_SUCCESS     = 0x43, /* captured */
    CATCH_RESULT_CANNOT_CATCH= 0x10, /* ghost / uncatchable */
    CATCH_RESULT_0_SHAKES    = 0x20,
    CATCH_RESULT_1_SHAKE     = 0x61,
    CATCH_RESULT_2_SHAKES    = 0x62,
    CATCH_RESULT_3_SHAKES    = 0x63,
} catch_result_t;

/* Battle_CatchAttempt — ItemUseBall catch calculation (item_effects.asm:177-416).
 *
 * ball_id: one of ITEM_MASTER_BALL / ITEM_POKE_BALL / ITEM_GREAT_BALL /
 *          ITEM_ULTRA_BALL / ITEM_SAFARI_BALL.
 *
 * Uses wEnemyMon (hp, max_hp, catch_rate, status) and BattleRandom().
 * Returns catch_result_t.  Does NOT modify party or box data — caller handles
 * the "add to party / box" bookkeeping. */
catch_result_t Battle_CatchAttempt(uint8_t ball_id);
