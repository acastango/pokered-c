/* test_battle_core.c — Unit tests for Phase 5 battle turn execution engine.
 *
 * Tests Battle_ExecutePlayerMove, Battle_ExecuteEnemyMove,
 * Battle_HandlePoisonBurnLeechSeed, Battle_HandleEnemyMonFainted,
 * Battle_HandlePlayerMonFainted.
 *
 * UI calls are omitted from the implementation, so tests are deterministic.
 *
 * PRNG notes:
 *   BattleRandom() = (hRandomAdd += 5) ^ (hRandomSub -= 3).
 *   seed_never_trigger(): first result = 0xFF (no paralysis trigger, no crit).
 *   seed_always_trigger(): first result = 0x00 (triggers any % threshold).
 *   X_ACCURACY (BSTAT2_USING_X_ACCURACY) bypasses random accuracy roll.
 *   wBattleMon.spd = 0 ensures crit rate = 0 (no crits).
 */
#include "test_runner.h"
#include "../src/game/battle/battle.h"
#include "../src/game/battle/battle_core.h"
#include "../src/game/battle/battle_effects.h"
#include "../src/platform/hardware.h"
#include "../src/game/constants.h"

/* ================================================================
 * Reset helpers
 * ================================================================ */
static void battle_reset(void) {
    extern void WRAMClear(void);
    WRAMClear();
    hRandomAdd = 0x00;
    hRandomSub = 0xFD;   /* seed_never_trigger: first BattleRandom = 0xFF */
    hWhoseTurn = 0;

    /* Player mon: spd=0 → crit rate=0 */
    wBattleMon.atk = 100;  wBattleMon.def = 80;
    wBattleMon.spd = 0;    wBattleMon.spc = 70;
    wBattleMon.max_hp = 200; wBattleMon.hp = 200;
    wBattleMon.level = 50;

    wEnemyMon.atk = 100;  wEnemyMon.def = 80;
    wEnemyMon.spd = 0;    wEnemyMon.spc = 70;
    wEnemyMon.max_hp = 200; wEnemyMon.hp = 200;
    wEnemyMon.level = 50;

    /* Default move: normal 80-power, always hits, no effect */
    wPlayerSelectedMove = 1;
    wPlayerMoveNum      = 1;
    wPlayerMoveEffect   = EFFECT_NONE;
    wPlayerMovePower    = 80;
    wPlayerMoveType     = TYPE_NORMAL;
    wPlayerMoveAccuracy = 255;
    wPlayerMoveMaxPP    = 35;

    wEnemySelectedMove  = 1;
    wEnemyMoveNum       = 1;
    wEnemyMoveEffect    = EFFECT_NONE;
    wEnemyMovePower     = 80;
    wEnemyMoveType      = TYPE_NORMAL;
    wEnemyMoveAccuracy  = 255;
    wEnemyMoveMaxPP     = 35;

    wDamageMultipliers  = DAMAGE_MULT_EFFECTIVE;
}

/* Enable X-Accuracy: bypasses the random accuracy roll. */
static void enable_x_accuracy(void) {
    wPlayerBattleStatus2 |= (1u << BSTAT2_USING_X_ACCURACY);
}

/* seed_never_trigger: BattleRandom() first result = 0xFF */
static void seed_never_trigger(void) {
    hRandomAdd = 0x00;
    hRandomSub = 0xFD;
}

/* seed_always_trigger: BattleRandom() first result = 0x00 */
static void seed_always_trigger(void) {
    hRandomAdd = 0xFB;
    hRandomSub = 0x08;  /* (0xFB+5)^(0x08-3) = 0x00^0x05 = 0x05... */
    /* Actually: want (add+5)^(sub-3) = 0.
     * add=0x01 → 0x06; sub=0x06 → sub-3=0x03; 0x06^0x03=0x05. Not 0.
     * Let add+5 = X, sub-3 = X, so add=X-5, sub=X+3.
     * X=0x80: add=0x7B, sub=0x83. 0x7B&7=3, 0x83&7=3 — same! Avoid.
     * X=0x40: add=0x3B, sub=0x43. 0x3B&7=3, 0x43&7=3 — same! Avoid.
     * X=0x20: add=0x1B, sub=0x23. 0x1B&7=3, 0x23&7=3 — same!
     * X=0x10: add=0x0B, sub=0x13. 0x0B&7=3, 0x13&7=3 — same!
     * X=0xC0: add=0xBB, sub=0xC3. 0xBB&7=3, 0xC3&7=3 — same!
     * For X XOR X = 0, we always get same low bits. That's the issue.
     * Instead use: want result = 0x01 (near 0, triggers any threshold > 0).
     * (add+5) ^ (sub-3) = 0x01.
     * add=0x00 → 0x05; sub-3=0x04 → sub=0x07.
     * 0x00&7=0, 0x07&7=7. Different → safe. Result=0x05^0x04=0x01. */
    hRandomAdd = 0x00;
    hRandomSub = 0x07;  /* first BattleRandom = 0x01 (triggers thresholds > 1) */
}

/* ================================================================
 * Battle_HandlePoisonBurnLeechSeed
 * ================================================================ */

TEST(HandlePoisonBurnLeechSeed, no_status_no_damage) {
    battle_reset();
    hWhoseTurn = 0;
    int alive = Battle_HandlePoisonBurnLeechSeed();
    EXPECT_EQ(alive, 1);
    EXPECT_EQ((int)wBattleMon.hp, 200);
}

TEST(HandlePoisonBurnLeechSeed, poison_reduces_hp_by_sixteenth) {
    battle_reset();
    hWhoseTurn = 0;
    wBattleMon.status = STATUS_PSN;
    wBattleMon.max_hp = 160;
    wBattleMon.hp = 160;
    int alive = Battle_HandlePoisonBurnLeechSeed();
    /* 160/16 = 10 damage */
    EXPECT_EQ((int)wBattleMon.hp, 150);
    EXPECT_EQ(alive, 1);
}

TEST(HandlePoisonBurnLeechSeed, burn_reduces_hp_by_sixteenth) {
    battle_reset();
    hWhoseTurn = 0;
    wBattleMon.status = STATUS_BRN;
    wBattleMon.max_hp = 160;
    wBattleMon.hp = 160;
    Battle_HandlePoisonBurnLeechSeed();
    EXPECT_EQ((int)wBattleMon.hp, 150);
}

TEST(HandlePoisonBurnLeechSeed, min_damage_one) {
    battle_reset();
    hWhoseTurn = 0;
    wBattleMon.status = STATUS_PSN;
    wBattleMon.max_hp = 15;   /* 15/16 = 0, should clamp to 1 */
    wBattleMon.hp = 15;
    Battle_HandlePoisonBurnLeechSeed();
    EXPECT_EQ((int)wBattleMon.hp, 14);
}

TEST(HandlePoisonBurnLeechSeed, toxic_counter_increments_and_scales_damage) {
    battle_reset();
    hWhoseTurn = 0;
    wBattleMon.status = STATUS_PSN;
    wPlayerBattleStatus3 |= (1u << BSTAT3_BADLY_POISONED);
    wPlayerToxicCounter = 1;   /* will become 2 before multiplying */
    wBattleMon.max_hp = 160;
    wBattleMon.hp = 160;
    Battle_HandlePoisonBurnLeechSeed();
    /* base = 160/16 = 10; counter increments to 2; damage = 10*2 = 20 */
    EXPECT_EQ((int)wPlayerToxicCounter, 2);
    EXPECT_EQ((int)wBattleMon.hp, 140);
}

TEST(HandlePoisonBurnLeechSeed, faint_returns_zero) {
    battle_reset();
    hWhoseTurn = 0;
    wBattleMon.status = STATUS_PSN;
    wBattleMon.max_hp = 16;
    wBattleMon.hp = 1;   /* 16/16 = 1 damage → HP = 0 */
    int alive = Battle_HandlePoisonBurnLeechSeed();
    EXPECT_EQ((int)wBattleMon.hp, 0);
    EXPECT_EQ(alive, 0);
}

TEST(HandlePoisonBurnLeechSeed, leech_seed_damages_and_heals_enemy) {
    battle_reset();
    hWhoseTurn = 0;
    /* Seeded player, no burn/poison */
    wPlayerBattleStatus2 |= (1u << BSTAT2_SEEDED);
    wBattleMon.max_hp = 160;
    wBattleMon.hp = 160;
    wEnemyMon.max_hp = 200;
    wEnemyMon.hp = 100;   /* partially damaged enemy */
    Battle_HandlePoisonBurnLeechSeed();
    /* 160/16 = 10 damage to player, 10 heal to enemy */
    EXPECT_EQ((int)wBattleMon.hp, 150);
    EXPECT_EQ((int)wEnemyMon.hp, 110);
}

TEST(HandlePoisonBurnLeechSeed, leech_seed_heal_capped_at_max) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerBattleStatus2 |= (1u << BSTAT2_SEEDED);
    wBattleMon.max_hp = 160;
    wBattleMon.hp = 160;
    wEnemyMon.max_hp = 200;
    wEnemyMon.hp = 195;   /* only 5 room left */
    Battle_HandlePoisonBurnLeechSeed();
    /* heal is capped at max_hp=200 */
    EXPECT_EQ((int)wEnemyMon.hp, 200);
}

TEST(HandlePoisonBurnLeechSeed, enemy_side_hwhosturn_1) {
    battle_reset();
    hWhoseTurn = 1;
    wEnemyMon.status = STATUS_PSN;
    wEnemyMon.max_hp = 160;
    wEnemyMon.hp = 160;
    Battle_HandlePoisonBurnLeechSeed();
    EXPECT_EQ((int)wEnemyMon.hp, 150);
    EXPECT_EQ((int)wBattleMon.hp, 200);   /* player untouched */
}

/* ================================================================
 * Battle_HandleEnemyMonFainted
 * ================================================================ */

TEST(HandleEnemyMonFainted, clears_enemy_battle_statuses) {
    battle_reset();
    wEnemyBattleStatus1 = 0xFF;
    wEnemyBattleStatus2 = 0xFF;
    wEnemyBattleStatus3 = 0xFF;
    Battle_HandleEnemyMonFainted();
    EXPECT_EQ((int)wEnemyBattleStatus1, 0);
    EXPECT_EQ((int)wEnemyBattleStatus2, 0);
    EXPECT_EQ((int)wEnemyBattleStatus3, 0);
}

TEST(HandleEnemyMonFainted, clears_disabled_and_minimized) {
    battle_reset();
    wEnemyDisabledMove       = 0x35;
    wEnemyDisabledMoveNumber = 0x02;
    wEnemyMonMinimized       = 1;
    Battle_HandleEnemyMonFainted();
    EXPECT_EQ((int)wEnemyDisabledMove, 0);
    EXPECT_EQ((int)wEnemyDisabledMoveNumber, 0);
    EXPECT_EQ((int)wEnemyMonMinimized, 0);
}

TEST(HandleEnemyMonFainted, sets_wInHandlePlayerMonFainted_to_zero) {
    battle_reset();
    wInHandlePlayerMonFainted = 1;
    Battle_HandleEnemyMonFainted();
    EXPECT_EQ((int)wInHandlePlayerMonFainted, 0);
}

TEST(HandleEnemyMonFainted, clears_player_multi_hit_flag) {
    battle_reset();
    wPlayerBattleStatus1 |= (1u << BSTAT1_ATTACKING_MULTIPLE);
    Battle_HandleEnemyMonFainted();
    EXPECT_EQ((int)(wPlayerBattleStatus1 & (1u << BSTAT1_ATTACKING_MULTIPLE)), 0);
}

TEST(HandleEnemyMonFainted, bide_bug_only_zeros_high_byte) {
    battle_reset();
    /* Gen 1 bug: only the high byte of wPlayerBideAccumulatedDamage is zeroed */
    wPlayerBideAccumulatedDamage = 0x01FF;   /* high byte=0x01, low byte=0xFF */
    Battle_HandleEnemyMonFainted();
    /* Only high byte cleared → 0x00FF */
    EXPECT_EQ((int)wPlayerBideAccumulatedDamage, 0x00FF);
}

TEST(HandleEnemyMonFainted, clears_used_move_tracking) {
    battle_reset();
    wPlayerUsedMove = 0x12;
    wEnemyUsedMove  = 0x34;
    Battle_HandleEnemyMonFainted();
    EXPECT_EQ((int)wPlayerUsedMove, 0);
    EXPECT_EQ((int)wEnemyUsedMove,  0);
}

/* ================================================================
 * Battle_HandlePlayerMonFainted
 * ================================================================ */

TEST(HandlePlayerMonFainted, sets_wInHandlePlayerMonFainted_to_one) {
    battle_reset();
    wInHandlePlayerMonFainted = 0;
    Battle_HandlePlayerMonFainted();
    EXPECT_EQ((int)wInHandlePlayerMonFainted, 1);
}

TEST(HandlePlayerMonFainted, clears_player_status) {
    battle_reset();
    wBattleMon.status = STATUS_PSN;
    Battle_HandlePlayerMonFainted();
    EXPECT_EQ((int)wBattleMon.status, 0);
}

TEST(HandlePlayerMonFainted, clears_enemy_bide_both_bytes) {
    battle_reset();
    wEnemyBideAccumulatedDamage = 0x01FF;
    Battle_HandlePlayerMonFainted();
    /* RemoveFaintedPlayerMon zeroes BOTH bytes (no bug here) */
    EXPECT_EQ((int)wEnemyBideAccumulatedDamage, 0);
}

TEST(HandlePlayerMonFainted, clears_enemy_attacking_multiple) {
    battle_reset();
    wEnemyBattleStatus1 |= (1u << BSTAT1_ATTACKING_MULTIPLE);
    Battle_HandlePlayerMonFainted();
    EXPECT_EQ((int)(wEnemyBattleStatus1 & (1u << BSTAT1_ATTACKING_MULTIPLE)), 0);
}

TEST(HandlePlayerMonFainted, simultaneous_faint_triggers_enemy_faint_state) {
    battle_reset();
    wEnemyMon.hp = 0;           /* enemy also fainted */
    wEnemyBattleStatus1 = 0xFF;
    wEnemyBattleStatus2 = 0xFF;
    Battle_HandlePlayerMonFainted();
    /* wInHandlePlayerMonFainted = 1 (player fainted) */
    EXPECT_EQ((int)wInHandlePlayerMonFainted, 1);
    /* FaintEnemyPokemon state was also triggered */
    EXPECT_EQ((int)wEnemyBattleStatus1, 0);
    EXPECT_EQ((int)wEnemyBattleStatus2, 0);
}

TEST(HandlePlayerMonFainted, no_enemy_faint_if_enemy_alive) {
    battle_reset();
    wEnemyMon.hp = 50;           /* enemy alive */
    wEnemyBattleStatus1 = 0xFF;
    Battle_HandlePlayerMonFainted();
    /* FaintEnemyPokemon NOT triggered */
    EXPECT_NE((int)wEnemyBattleStatus1, 0);
}

/* ================================================================
 * Battle_ExecutePlayerMove — guard conditions
 * ================================================================ */

TEST(ExecutePlayerMove, cannot_move_skips_execution) {
    battle_reset();
    wPlayerSelectedMove = CANNOT_MOVE;
    Battle_ExecutePlayerMove();
    EXPECT_EQ((int)wEnemyMon.hp, 200);   /* no damage dealt */
}

TEST(ExecutePlayerMove, action_taken_flag_skips_execution) {
    battle_reset();
    enable_x_accuracy();
    wActionResultOrTookBattleTurn = 1;
    Battle_ExecutePlayerMove();
    EXPECT_EQ((int)wEnemyMon.hp, 200);   /* no damage dealt */
    /* Flag is cleared at execute_done */
    EXPECT_EQ((int)wActionResultOrTookBattleTurn, 0);
}

/* ================================================================
 * Battle_ExecutePlayerMove — status conditions
 * ================================================================ */

TEST(ExecutePlayerMove, sleep_prevents_move) {
    battle_reset();
    wBattleMon.status = 3;   /* 3 turns of sleep remaining */
    wPlayerUsedMove = 0x42;  /* will be cleared */
    Battle_ExecutePlayerMove();
    EXPECT_EQ((int)wEnemyMon.hp, 200);   /* no damage */
    EXPECT_EQ((int)wPlayerUsedMove, 0);
    /* Counter decremented: 3→2 */
    EXPECT_EQ((int)(wBattleMon.status & STATUS_SLP_MASK), 2);
}

TEST(ExecutePlayerMove, frozen_prevents_move) {
    battle_reset();
    wBattleMon.status = STATUS_FRZ;
    wPlayerUsedMove = 0x01;
    Battle_ExecutePlayerMove();
    EXPECT_EQ((int)wEnemyMon.hp, 200);
    EXPECT_EQ((int)wPlayerUsedMove, 0);
}

TEST(ExecutePlayerMove, flinch_prevents_move_and_clears_flag) {
    battle_reset();
    wPlayerBattleStatus1 |= (1u << BSTAT1_FLINCHED);
    Battle_ExecutePlayerMove();
    EXPECT_EQ((int)wEnemyMon.hp, 200);
    EXPECT_EQ((int)(wPlayerBattleStatus1 & (1u << BSTAT1_FLINCHED)), 0);
}

TEST(ExecutePlayerMove, hyper_beam_recharge_prevents_move_and_clears_flag) {
    battle_reset();
    wPlayerBattleStatus2 |= (1u << BSTAT2_NEEDS_TO_RECHARGE);
    Battle_ExecutePlayerMove();
    EXPECT_EQ((int)wEnemyMon.hp, 200);
    EXPECT_EQ((int)(wPlayerBattleStatus2 & (1u << BSTAT2_NEEDS_TO_RECHARGE)), 0);
}

/* ================================================================
 * Battle_ExecutePlayerMove — ResidualEffects1 (no damage)
 * ================================================================ */

TEST(ExecutePlayerMove, splash_does_no_damage) {
    battle_reset();
    /* SPLASH = move 0x60 (96); get_current_move() loads EFFECT_SPLASH (kResidualEffects1)
     * which dispatches via JumpMoveEffect then exits — no damage applied. */
    wPlayerSelectedMove = 0x60;  /* MOVE_SPLASH */
    Battle_ExecutePlayerMove();
    EXPECT_EQ((int)wEnemyMon.hp, 200);
}

/* ================================================================
 * Battle_ExecutePlayerMove — normal damaging move
 * ================================================================ */

TEST(ExecutePlayerMove, normal_move_deals_damage) {
    battle_reset();
    enable_x_accuracy();
    /* EFFECT_NONE: falls through all arrays, applies damage then final JumpMoveEffect */
    wPlayerMoveEffect = EFFECT_NONE;
    wPlayerMovePower  = 80;
    wPlayerMoveType   = TYPE_NORMAL;
    Battle_ExecutePlayerMove();
    EXPECT_LT((int)wEnemyMon.hp, 200);
}

TEST(ExecutePlayerMove, zero_power_move_no_damage) {
    battle_reset();
    /* Use THRASH bypass to skip get_current_move() so our wPlayerMoveEffect/Power
     * values are not overridden.  Power=0 causes apply_attack_to_enemy to return
     * immediately — no damage. */
    wPlayerBattleStatus1 |= (1u << BSTAT1_THRASHING_ABOUT);
    wPlayerNumAttacksLeft = 2;
    wPlayerMoveEffect = EFFECT_NONE;
    wPlayerMovePower  = 0;
    Battle_ExecutePlayerMove();
    EXPECT_EQ((int)wEnemyMon.hp, 200);
}

TEST(ExecutePlayerMove, resets_action_result_at_end) {
    battle_reset();
    enable_x_accuracy();
    wActionResultOrTookBattleTurn = 0;
    Battle_ExecutePlayerMove();
    /* Always cleared at execute_done */
    EXPECT_EQ((int)wActionResultOrTookBattleTurn, 0);
}

/* ================================================================
 * Battle_ExecutePlayerMove — paralysis (25% chance to fail)
 * ================================================================ */

TEST(ExecutePlayerMove, paralysis_triggers_with_low_rng) {
    battle_reset();
    /* seed_always_trigger gives first BattleRandom = 0x01 < 64 → paralyzed */
    seed_always_trigger();
    wBattleMon.status = STATUS_PAR;
    wPlayerMoveEffect = EFFECT_NONE;
    wPlayerMovePower  = 80;
    Battle_ExecutePlayerMove();
    /* Paralyzed this turn → no damage */
    EXPECT_EQ((int)wEnemyMon.hp, 200);
}

TEST(ExecutePlayerMove, paralysis_does_not_trigger_with_high_rng) {
    battle_reset();
    /* seed_never_trigger gives first BattleRandom = 0xFF >= 64 → not paralyzed */
    seed_never_trigger();
    enable_x_accuracy();
    wBattleMon.status = STATUS_PAR;
    wPlayerMoveEffect = EFFECT_NONE;
    wPlayerMovePower  = 80;
    Battle_ExecutePlayerMove();
    /* Not paralyzed → damage applied */
    EXPECT_LT((int)wEnemyMon.hp, 200);
}

/* ================================================================
 * Battle_ExecutePlayerMove — Bide accumulation
 * ================================================================ */

TEST(ExecutePlayerMove, bide_accumulates_and_does_not_deal_damage) {
    battle_reset();
    wPlayerBattleStatus1 |= (1u << BSTAT1_STORING_ENERGY);
    wPlayerNumAttacksLeft = 2;   /* still 2 turns left */
    wDamage = 30;                /* damage accumulated this turn */
    Battle_ExecutePlayerMove();
    /* Bide accumulates: counter still >0 so returns DONE, no damage to enemy */
    EXPECT_EQ((int)wEnemyMon.hp, 200);
    EXPECT_GT((int)wPlayerBideAccumulatedDamage, 0);
    EXPECT_EQ((int)wPlayerNumAttacksLeft, 1);
}

/* ================================================================
 * Battle_ExecutePlayerMove — ResidualEffects2 dispatches and stops
 * ================================================================ */

TEST(ExecutePlayerMove, stat_up_effect_dispatches_via_residual2) {
    battle_reset();
    /* Use THRASH bypass to skip get_current_move() so our wPlayerMoveEffect
     * is not overridden.  EFFECT_ATTACK_UP1 is in kResidualEffects2: dispatched
     * at player_mirror_move_check before any damage is applied. */
    wPlayerBattleStatus1 |= (1u << BSTAT1_THRASHING_ABOUT);
    wPlayerNumAttacksLeft = 2;
    wPlayerMoveEffect = EFFECT_ATTACK_UP1;
    wPlayerMovePower  = 0;
    uint8_t initial_stage = wPlayerMonStatMods[MOD_ATTACK];
    Battle_ExecutePlayerMove();
    EXPECT_GT((int)wPlayerMonStatMods[MOD_ATTACK], (int)initial_stage);
    EXPECT_EQ((int)wEnemyMon.hp, 200);
}

/* ================================================================
 * Battle_ExecuteEnemyMove — basic smoke tests
 * ================================================================ */

TEST(ExecuteEnemyMove, cannot_move_skips) {
    battle_reset();
    hWhoseTurn = 1;
    wEnemySelectedMove = CANNOT_MOVE;
    Battle_ExecuteEnemyMove();
    EXPECT_EQ((int)wBattleMon.hp, 200);
}

TEST(ExecuteEnemyMove, sleep_prevents_move) {
    battle_reset();
    hWhoseTurn = 1;
    wEnemyMon.status = 2;
    Battle_ExecuteEnemyMove();
    EXPECT_EQ((int)wBattleMon.hp, 200);
    EXPECT_EQ((int)wEnemyUsedMove, 0);
    EXPECT_EQ((int)(wEnemyMon.status & STATUS_SLP_MASK), 1);
}

TEST(ExecuteEnemyMove, normal_move_deals_damage_to_player) {
    battle_reset();
    hWhoseTurn = 1;
    /* Enable X-Accuracy on enemy: use wEnemyBattleStatus2 */
    wEnemyBattleStatus2 |= (1u << BSTAT2_USING_X_ACCURACY);
    wEnemyMoveEffect = EFFECT_NONE;
    wEnemyMovePower  = 80;
    wEnemyMoveType   = TYPE_NORMAL;
    wEnemyMon.spd    = 0;   /* no crits */
    Battle_ExecuteEnemyMove();
    EXPECT_LT((int)wBattleMon.hp, 200);
}

TEST(ExecuteEnemyMove, frozen_prevents_move) {
    battle_reset();
    hWhoseTurn = 1;
    wEnemyMon.status = STATUS_FRZ;
    Battle_ExecuteEnemyMove();
    EXPECT_EQ((int)wBattleMon.hp, 200);
    EXPECT_EQ((int)wEnemyUsedMove, 0);
}

/* ================================================================
 * HandlePoisonBurnLeechSeed — interaction: both PSN + leech seed
 * ================================================================ */

TEST(HandlePoisonBurnLeechSeed, poison_and_leech_both_trigger) {
    battle_reset();
    hWhoseTurn = 0;
    wBattleMon.status = STATUS_PSN;
    wPlayerBattleStatus2 |= (1u << BSTAT2_SEEDED);
    wBattleMon.max_hp = 160;
    wBattleMon.hp = 160;
    wEnemyMon.hp = 50;
    wEnemyMon.max_hp = 200;
    Battle_HandlePoisonBurnLeechSeed();
    /* Poison: 10 damage; then leech seed: another 10 damage + 10 heal to enemy */
    EXPECT_EQ((int)wBattleMon.hp, 140);
    EXPECT_EQ((int)wEnemyMon.hp, 60);
}

/* ================================================================
 * PP decrement (DecrementPP, core.asm:3133 / 5540)
 * ================================================================ */

/* Enemy PP decrements by 1 after executing a move */
TEST(DecrementPP, enemy_pp_decrements) {
    battle_reset();
    hWhoseTurn = 1;
    wEnemyMon.pp[0]    = 10;
    wEnemyMoveListIndex = 0;
    wEnemyBattleStatus2 |= (1u << BSTAT2_USING_X_ACCURACY);
    Battle_ExecuteEnemyMove();
    EXPECT_EQ((int)wEnemyMon.pp[0], 9);
}

/* Player PP decrements by 1 after executing a move.
 * wPlayerMoveListIndex defaults to 0 (set by UI at battle time). */
TEST(DecrementPP, player_pp_decrements) {
    battle_reset();
    hWhoseTurn = 0;
    wBattleMon.pp[0]     = 10;
    wPlayerMoveListIndex  = 0;
    wPlayerBattleStatus2 |= (1u << BSTAT2_USING_X_ACCURACY);
    Battle_ExecutePlayerMove();
    EXPECT_EQ((int)wBattleMon.pp[0], 9);
}

/* PP_UP bits (bits 6-7) must not be touched by decrement */
TEST(DecrementPP, pp_up_bits_preserved) {
    battle_reset();
    hWhoseTurn = 1;
    /* PP_UP = 1 (bit6 set), current PP = 5 → byte = 0x45 */
    wEnemyMon.pp[0]    = (1u << 6) | 5;
    wEnemyMoveListIndex = 0;
    wEnemyBattleStatus2 |= (1u << BSTAT2_USING_X_ACCURACY);
    Battle_ExecuteEnemyMove();
    /* Expect PP_UP=1 still set, current PP=4 → byte = 0x44 */
    EXPECT_EQ((int)(wEnemyMon.pp[0] >> 6), 1);   /* PP_UP preserved */
    EXPECT_EQ((int)(wEnemyMon.pp[0] & 0x3F), 4); /* PP decremented */
}

/* Struggle must NOT decrement PP */
TEST(DecrementPP, struggle_no_decrement) {
    battle_reset();
    hWhoseTurn = 1;
    wEnemyMon.pp[0]     = 5;
    wEnemyMoveListIndex  = 0;
    wEnemySelectedMove   = MOVE_STRUGGLE;
    wEnemyBattleStatus2 |= (1u << BSTAT2_USING_X_ACCURACY);
    Battle_ExecuteEnemyMove();
    EXPECT_EQ((int)wEnemyMon.pp[0], 5);
}
