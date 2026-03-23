/* test_battle_loop.c — Unit tests for Phase 6 battle main loop.
 *
 * Tests Battle_SelectEnemyMove, Battle_CheckNumAttacksLeft, Battle_RunTurn.
 *
 * RunTurn ordering tests use the "first attacker kills" approach:
 *   target HP = 1 so any hit is lethal.  If the expected first attacker goes
 *   first, it kills the target before the target can attack, and the attacker's
 *   own HP stays at 200.  The result code confirms which side fainted.
 */
#include "test_runner.h"
#include "../src/game/battle/battle.h"
#include "../src/game/battle/battle_core.h"
#include "../src/game/battle/battle_loop.h"
#include "../src/platform/hardware.h"
#include "../src/game/constants.h"

/* ================================================================
 * Reset helpers
 * ================================================================ */
static void battle_reset(void) {
    extern void WRAMClear(void);
    WRAMClear();
    hRandomAdd = 0x00;
    hRandomSub = 0xFD;   /* first BattleRandom = 0xFF */
    hWhoseTurn = 0;

    wBattleMon.atk = 100;  wBattleMon.def = 80;
    wBattleMon.spd = 50;   wBattleMon.spc = 70;
    wBattleMon.max_hp = 200; wBattleMon.hp = 200;
    wBattleMon.level = 50;

    wEnemyMon.atk = 100;  wEnemyMon.def = 80;
    wEnemyMon.spd = 50;   wEnemyMon.spc = 70;
    wEnemyMon.max_hp = 200; wEnemyMon.hp = 200;
    wEnemyMon.level = 50;

    /* Default player move: Tackle (move 1, EFFECT_NONE, power=40) */
    wPlayerSelectedMove = 1;
    wPlayerMoveEffect   = EFFECT_NONE;
    wPlayerMovePower    = 40;
    wPlayerMoveType     = TYPE_NORMAL;
    wPlayerMoveAccuracy = 255;

    /* Default enemy move pool: all Tackle */
    wEnemyMon.moves[0] = 1;
    wEnemyMon.moves[1] = 1;
    wEnemyMon.moves[2] = 1;
    wEnemyMon.moves[3] = 1;
    wEnemySelectedMove  = 1;
    wEnemyMoveEffect    = EFFECT_NONE;
    wEnemyMovePower     = 40;
    wEnemyMoveType      = TYPE_NORMAL;
    wEnemyMoveAccuracy  = 255;

    wDamageMultipliers  = DAMAGE_MULT_EFFECTIVE;

    /* X-Accuracy on both sides: guarantee hits */
    wPlayerBattleStatus2 |= (1u << BSTAT2_USING_X_ACCURACY);
    wEnemyBattleStatus2  |= (1u << BSTAT2_USING_X_ACCURACY);
}

/* ================================================================
 * Battle_SelectEnemyMove
 * ================================================================ */

TEST(SelectEnemyMove, LockedRecharging_NoChange) {
    battle_reset();
    wEnemyBattleStatus2 |= (1u << BSTAT2_NEEDS_TO_RECHARGE);
    wEnemySelectedMove = 0x42;   /* sentinel */
    Battle_SelectEnemyMove();
    EXPECT_EQ((int)wEnemySelectedMove, 0x42);   /* unchanged */
}

TEST(SelectEnemyMove, LockedSleep_NoChange) {
    battle_reset();
    wEnemyMon.status = 2;   /* 2 turns of sleep */
    wEnemySelectedMove = 0x42;
    Battle_SelectEnemyMove();
    EXPECT_EQ((int)wEnemySelectedMove, 0x42);
}

TEST(SelectEnemyMove, OnlyMoveDisabled_Struggle) {
    battle_reset();
    /* Only slot 0 has a move; slot 0 is disabled */
    wEnemyMon.moves[0] = 5;
    wEnemyMon.moves[1] = 0;
    wEnemyMon.moves[2] = 0;
    wEnemyMon.moves[3] = 0;
    /* wEnemyDisabledMove high nibble = 1 (slot 1, 1-based) = slot 0 */
    wEnemyDisabledMove = 0x11;   /* high=1 (slot1 1-based), low=1 turn */
    Battle_SelectEnemyMove();
    EXPECT_EQ((int)wEnemySelectedMove, (int)MOVE_STRUGGLE);
}

TEST(SelectEnemyMove, EmptySlotSkipped_AlwaysPicksOnlyValidSlot) {
    battle_reset();
    /* Only slot 0 has a move; slots 1/2/3 are empty (0) */
    wEnemyMon.moves[0] = 7;   /* arbitrary move ID */
    wEnemyMon.moves[1] = 0;
    wEnemyMon.moves[2] = 0;
    wEnemyMon.moves[3] = 0;
    wEnemyDisabledMove = 0;
    Battle_SelectEnemyMove();
    /* Must always land on slot 0 since others are 0 (retry on empty) */
    EXPECT_EQ((int)wEnemySelectedMove, 7);
    EXPECT_EQ((int)wEnemyMoveListIndex, 0);
}

TEST(SelectEnemyMove, TrappedPlayer_SetsCannotMove) {
    battle_reset();
    wPlayerBattleStatus1 |= (1u << BSTAT1_USING_TRAPPING);
    wEnemySelectedMove = 0x42;
    Battle_SelectEnemyMove();
    EXPECT_EQ((int)wEnemySelectedMove, (int)CANNOT_MOVE);
}

/* ================================================================
 * Battle_CheckNumAttacksLeft
 * ================================================================ */

TEST(CheckNumAttacksLeft, PlayerZero_ClearsTrapping) {
    battle_reset();
    wPlayerBattleStatus1 |= (1u << BSTAT1_USING_TRAPPING);
    wPlayerNumAttacksLeft = 0;
    Battle_CheckNumAttacksLeft();
    EXPECT_EQ((int)(wPlayerBattleStatus1 & (1u << BSTAT1_USING_TRAPPING)), 0);
}

TEST(CheckNumAttacksLeft, PlayerNonZero_KeepsTrapping) {
    battle_reset();
    wPlayerBattleStatus1 |= (1u << BSTAT1_USING_TRAPPING);
    wPlayerNumAttacksLeft = 2;
    Battle_CheckNumAttacksLeft();
    EXPECT_NE((int)(wPlayerBattleStatus1 & (1u << BSTAT1_USING_TRAPPING)), 0);
}

TEST(CheckNumAttacksLeft, EnemyZero_ClearsTrapping) {
    battle_reset();
    wEnemyBattleStatus1 |= (1u << BSTAT1_USING_TRAPPING);
    wEnemyNumAttacksLeft = 0;
    Battle_CheckNumAttacksLeft();
    EXPECT_EQ((int)(wEnemyBattleStatus1 & (1u << BSTAT1_USING_TRAPPING)), 0);
}

/* ================================================================
 * Battle_RunTurn — move ordering
 * ================================================================ */

TEST(RunTurn, FasterPlayer_EnemyFainted) {
    battle_reset();
    /* Player faster: player attacks first → kills enemy (hp=1) */
    wBattleMon.spd = 100;
    wEnemyMon.spd  = 10;
    wEnemyMon.hp   = 1;

    battle_result_t r = Battle_RunTurn();

    EXPECT_EQ((int)r, (int)BATTLE_RESULT_ENEMY_FAINTED);
    /* Player was never attacked (enemy died before getting a turn) */
    EXPECT_EQ((int)wBattleMon.hp, 200);
}

TEST(RunTurn, FasterEnemy_PlayerFainted) {
    battle_reset();
    /* Enemy faster: enemy attacks first → kills player (hp=1) */
    wBattleMon.spd = 10;
    wEnemyMon.spd  = 100;
    wBattleMon.hp  = 1;

    battle_result_t r = Battle_RunTurn();

    EXPECT_EQ((int)r, (int)BATTLE_RESULT_PLAYER_FAINTED);
    /* Enemy was never attacked (player died before getting a turn) */
    EXPECT_EQ((int)wEnemyMon.hp, 200);
}

TEST(RunTurn, QuickAttack_PlayerFirst_OverridesSpeed) {
    battle_reset();
    /* Player uses Quick Attack, enemy uses Tackle; player is slower but QA wins */
    wPlayerSelectedMove = MOVE_QUICK_ATTACK;
    wBattleMon.spd = 10;
    wEnemyMon.spd  = 100;
    wEnemyMon.hp   = 1;    /* dies on any hit */

    battle_result_t r = Battle_RunTurn();

    EXPECT_EQ((int)r, (int)BATTLE_RESULT_ENEMY_FAINTED);
    EXPECT_EQ((int)wBattleMon.hp, 200);   /* player untouched */
}

TEST(RunTurn, Counter_EnemyGoesFirst_PlayerFainted) {
    battle_reset();
    /* Player uses Counter → Counter goes last → enemy moves first */
    wPlayerSelectedMove = MOVE_COUNTER;
    wBattleMon.spd = 100;   /* player is faster but Counter is always last */
    wEnemyMon.spd  = 10;
    wBattleMon.hp  = 1;     /* dies when enemy attacks first */

    battle_result_t r = Battle_RunTurn();

    /* Enemy went first; player fainted before Counter fired */
    EXPECT_EQ((int)r, (int)BATTLE_RESULT_PLAYER_FAINTED);
    EXPECT_EQ((int)wEnemyMon.hp, 200);
}

/* ================================================================
 * Battle_RunTurn — both sides survive
 * ================================================================ */

TEST(RunTurn, BothSurvive_ReturnsContinue) {
    battle_reset();
    /* Both mons have 200 HP; Tackle deals ~20 damage — both survive */
    wBattleMon.spd = 100;
    wEnemyMon.spd  = 10;

    battle_result_t r = Battle_RunTurn();

    EXPECT_EQ((int)r, (int)BATTLE_RESULT_CONTINUE);
    EXPECT_LT((int)wEnemyMon.hp,  200);   /* enemy took damage */
    EXPECT_LT((int)wBattleMon.hp, 200);   /* player took damage */
}

/* ================================================================
 * Battle_RunTurn — residual damage (poison after move)
 * ================================================================ */

TEST(RunTurn, EnemyPoisoned_DiesAfterMoving) {
    battle_reset();
    /* Enemy faster → enemy moves first → enemy's own poison kills it */
    wBattleMon.spd = 10;
    wEnemyMon.spd  = 100;
    wEnemyMon.status   = STATUS_PSN;
    wEnemyMon.max_hp   = 16;   /* poison damage = 16/16 = 1 */
    wEnemyMon.hp       = 1;    /* exactly enough to die from 1 poison tick */

    battle_result_t r = Battle_RunTurn();

    EXPECT_EQ((int)r, (int)BATTLE_RESULT_ENEMY_FAINTED);
    EXPECT_EQ((int)wEnemyMon.hp, 0);
}

TEST(RunTurn, PlayerPoisoned_DiesAfterMoving) {
    battle_reset();
    /* Player faster → player moves first → player's own poison kills it */
    wBattleMon.spd = 100;
    wEnemyMon.spd  = 10;
    wBattleMon.status  = STATUS_PSN;
    wBattleMon.max_hp  = 16;
    wBattleMon.hp      = 1;
    /* Player deals some damage but not lethal to enemy */
    wEnemyMon.max_hp   = 200;
    wEnemyMon.hp       = 200;

    battle_result_t r = Battle_RunTurn();

    EXPECT_EQ((int)r, (int)BATTLE_RESULT_PLAYER_FAINTED);
    EXPECT_EQ((int)wBattleMon.hp, 0);
}

/* ================================================================
 * Battle_RunTurn — faint check at loop entry
 * ================================================================ */

TEST(RunTurn, PlayerAlreadyAtZeroHP_ImmediateFaint) {
    battle_reset();
    wBattleMon.hp = 0;

    battle_result_t r = Battle_RunTurn();

    EXPECT_EQ((int)r, (int)BATTLE_RESULT_PLAYER_FAINTED);
    EXPECT_EQ((int)wInHandlePlayerMonFainted, 1);
}

TEST(RunTurn, EnemyAlreadyAtZeroHP_ImmediateFaint) {
    battle_reset();
    wEnemyMon.hp = 0;

    battle_result_t r = Battle_RunTurn();

    EXPECT_EQ((int)r, (int)BATTLE_RESULT_ENEMY_FAINTED);
    EXPECT_EQ((int)wInHandlePlayerMonFainted, 0);
}
