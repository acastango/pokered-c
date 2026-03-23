/* test_battle_effects.c — Unit tests for Gen 1 move effects engine (Phase 4).
 *
 * Tests pure battle-state changes produced by Battle_JumpMoveEffect and
 * the public helpers Battle_QuarterSpeedDueToParalysis / Battle_HalveAttackDueToBurn.
 *
 * UI calls are omitted from the implementation so every test is deterministic.
 *
 * PRNG: BattleRandom() = (hRandomAdd += 5) ^ (hRandomSub -= 3).
 * CRITICAL: if (hRandomAdd & 7) == (hRandomSub & 7) the lower 3 bits of every
 * output are always 0.  The sleep counter retry loop hangs in that state.
 * Safe seed rule: always ensure hRandomAdd & 7 != hRandomSub & 7.
 * seed_always_trigger() / seed_never_trigger() both satisfy this rule.
 */
#include "test_runner.h"
#include "../src/game/battle/battle.h"
#include "../src/game/battle/battle_effects.h"
#include "../src/platform/hardware.h"
#include "../src/game/constants.h"

/* ================================================================
 * Reset helpers
 * ================================================================ */
static void battle_reset(void) {
    extern void WRAMClear(void);
    WRAMClear();
    hRandomAdd = 0;
    hRandomSub = 1;   /* add&7=0 != sub&7=1 → safe seed baseline */
    hWhoseTurn = 0;   /* player attacks enemy by default */

    wPlayerMoveEffect   = EFFECT_NONE;
    wPlayerMovePower    = 80;
    wPlayerMoveType     = TYPE_NORMAL;
    wPlayerMoveAccuracy = 255;
    wPlayerMoveNum      = 1;

    wEnemyMoveEffect    = EFFECT_NONE;
    wEnemyMovePower     = 80;
    wEnemyMoveType      = TYPE_NORMAL;
    wEnemyMoveAccuracy  = 255;
    wEnemyMoveNum       = 1;

    /* Reasonable base stats */
    wBattleMon.atk = 100;  wBattleMon.def = 80;
    wBattleMon.spd = 90;   wBattleMon.spc = 70;
    wBattleMon.max_hp = 200; wBattleMon.hp = 200;
    wBattleMon.level = 50;

    wEnemyMon.atk = 100;  wEnemyMon.def = 80;
    wEnemyMon.spd = 90;   wEnemyMon.spc = 70;
    wEnemyMon.max_hp = 200; wEnemyMon.hp = 200;
    wEnemyMon.level = 50;
}

/* Forces BattleRandom() to return 0x00 (triggers any PCT threshold). */
static void seed_always_trigger(void) {
    /* Want (hRandomAdd+5) ^ (hRandomSub-3) == 0.
     * Let hRandomAdd = 0x00 → after: 0x05.
     * Need hRandomSub such that hRandomSub-3 == 0x05 → hRandomSub = 0x08.
     * Check: add&7=0, sub&7=0 → BAD (would cause lower-3-bits hang).
     * Use 0x01 offset instead: add=0x01 → after=0x06, sub=0x09 → after=0x06, XOR=0. */
    hRandomAdd = 0x01;
    hRandomSub = 0x09;  /* 0x01&7=1, 0x09&7=1 — BUT we only need one call to trigger */
    /* Actually that violates our rule but it's fine for a single threshold check,
     * since the sleep counter loop isn't involved.  For sleep tests use bypass. */
}

/* Forces BattleRandom() to return 0xFF (blocks all PCT thresholds). */
static void seed_never_trigger(void) {
    /* Want (hRandomAdd+5) ^ (hRandomSub-3) == 0xFF.
     * hRandomAdd = 0x00 → after = 0x05.
     * 0x05 ^ X = 0xFF → X = 0xFA. hRandomSub such that hRandomSub-3 = 0xFA → hRandomSub = 0xFD.
     * Check: 0x00 & 7 = 0, 0xFD & 7 = 5. Different → safe. */
    hRandomAdd = 0x00;
    hRandomSub = 0xFD;
}

/* Safe seed for sleep tests that bypasses the MoveHitTest accuracy roll.
 * Set NEEDS_TO_RECHARGE on target before calling — Effect_Sleep skips
 * MoveHitTest in that branch, so the first BattleRandom is the sleep counter.
 * With hRandomAdd=0x02, hRandomSub=0x00: first call = (7)^(0xFD) = 7^253 = ...
 * Let's just use add=2, sub=0: add&7=2, sub&7=0 → different, safe. */
static void seed_sleep_bypass(void) {
    hRandomAdd = 0x02;
    hRandomSub = 0x00;  /* add&7=2 != sub&7=0 → safe */
}

/* ================================================================
 * Battle_QuarterSpeedDueToParalysis
 * ================================================================ */

TEST(QuarterSpeed, PlayerTurn_EnemyParalyzed_QuartersEnemySpd) {
    battle_reset();
    hWhoseTurn = 0;
    wEnemyMon.status = STATUS_PAR;
    wEnemyMon.spd = 100;
    Battle_QuarterSpeedDueToParalysis();
    EXPECT_EQ((int)wEnemyMon.spd, 25);  /* 100 >> 2 = 25 */
}

TEST(QuarterSpeed, PlayerTurn_EnemyNotParalyzed_NoChange) {
    battle_reset();
    hWhoseTurn = 0;
    wEnemyMon.status = 0;
    wEnemyMon.spd = 100;
    Battle_QuarterSpeedDueToParalysis();
    EXPECT_EQ((int)wEnemyMon.spd, 100);
}

TEST(QuarterSpeed, EnemyTurn_PlayerParalyzed_QuartersPlayerSpd) {
    battle_reset();
    hWhoseTurn = 1;
    wBattleMon.status = STATUS_PAR;
    wBattleMon.spd = 80;
    Battle_QuarterSpeedDueToParalysis();
    EXPECT_EQ((int)wBattleMon.spd, 20);  /* 80 >> 2 = 20 */
}

TEST(QuarterSpeed, Min1_WhenSpdVeryLow) {
    battle_reset();
    hWhoseTurn = 0;
    wEnemyMon.status = STATUS_PAR;
    wEnemyMon.spd = 1;  /* 1 >> 2 = 0, clamped to 1 */
    Battle_QuarterSpeedDueToParalysis();
    EXPECT_EQ((int)wEnemyMon.spd, 1);
}

/* ================================================================
 * Battle_HalveAttackDueToBurn
 * ================================================================ */

TEST(HalveAtk, PlayerTurn_EnemyBurned_HalvesEnemyAtk) {
    battle_reset();
    hWhoseTurn = 0;
    wEnemyMon.status = STATUS_BRN;
    wEnemyMon.atk = 100;
    Battle_HalveAttackDueToBurn();
    EXPECT_EQ((int)wEnemyMon.atk, 50);
}

TEST(HalveAtk, PlayerTurn_EnemyNotBurned_NoChange) {
    battle_reset();
    hWhoseTurn = 0;
    wEnemyMon.status = 0;
    wEnemyMon.atk = 100;
    Battle_HalveAttackDueToBurn();
    EXPECT_EQ((int)wEnemyMon.atk, 100);
}

TEST(HalveAtk, EnemyTurn_PlayerBurned_HalvesPlayerAtk) {
    battle_reset();
    hWhoseTurn = 1;
    wBattleMon.status = STATUS_BRN;
    wBattleMon.atk = 60;
    Battle_HalveAttackDueToBurn();
    EXPECT_EQ((int)wBattleMon.atk, 30);
}

TEST(HalveAtk, Min1_WhenAtkVeryLow) {
    battle_reset();
    hWhoseTurn = 0;
    wEnemyMon.status = STATUS_BRN;
    wEnemyMon.atk = 1;
    Battle_HalveAttackDueToBurn();
    EXPECT_EQ((int)wEnemyMon.atk, 1);
}

/* ================================================================
 * Effect_Sleep (EFFECT_SLEEP = 0x20)
 *
 * To avoid the PRNG lower-3-bits-always-0 hang (when add&7 == sub&7),
 * we bypass Battle_MoveHitTest by pre-setting NEEDS_TO_RECHARGE on the target.
 * Effect_Sleep skips the accuracy check in that code path, so the first
 * BattleRandom call goes directly to the sleep counter retry loop.
 * ================================================================ */

TEST(SleepEffect, PlayerAttacks_SetsEnemySleepCounter) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_SLEEP;
    seed_sleep_bypass();
    wEnemyMon.status = 0;
    /* Bypass MoveHitTest via the recharge path */
    wEnemyBattleStatus2 = (1 << BSTAT2_NEEDS_TO_RECHARGE);
    Battle_JumpMoveEffect();
    EXPECT_TRUE(IS_ASLEEP(wEnemyMon.status));
    EXPECT_FALSE(wEnemyBattleStatus2 & (1 << BSTAT2_NEEDS_TO_RECHARGE)); /* cleared */
}

TEST(SleepEffect, EnemyAlreadyAsleep_NoChange) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_SLEEP;
    wPlayerMoveAccuracy = 255;
    wEnemyMon.status = 3;  /* already asleep (counter=3) */
    Battle_JumpMoveEffect();
    EXPECT_EQ((int)(wEnemyMon.status & STATUS_SLP_MASK), 3); /* still 3 */
}

TEST(SleepEffect, EnemyAlreadyStatused_NoChange) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_SLEEP;
    wPlayerMoveAccuracy = 255;
    wEnemyMon.status = STATUS_PAR;
    Battle_JumpMoveEffect();
    EXPECT_FALSE(IS_ASLEEP(wEnemyMon.status));
    EXPECT_TRUE(IS_PARALYZED(wEnemyMon.status));
}

TEST(SleepEffect, SleepCounter_NeverZero) {
    /* Run many iterations verifying counter is always 1-7 */
    for (int i = 0; i < 20; i++) {
        battle_reset();
        hWhoseTurn = 0;
        wPlayerMoveEffect = EFFECT_SLEEP;
        /* Safe seed: ensure add&7 != sub&7 */
        hRandomAdd = (uint8_t)(i * 3 + 2);
        hRandomSub = (uint8_t)(i * 7 + 0);
        /* Make add&7 != sub&7 by forcing sub&7 to differ */
        while ((hRandomSub & 7) == (hRandomAdd & 7)) hRandomSub++;
        wEnemyMon.status = 0;
        wEnemyBattleStatus2 = (1 << BSTAT2_NEEDS_TO_RECHARGE); /* bypass accuracy */
        Battle_JumpMoveEffect();
        if (IS_ASLEEP(wEnemyMon.status)) {
            uint8_t ctr = wEnemyMon.status & STATUS_SLP_MASK;
            EXPECT_GT((int)ctr, 0);
            EXPECT_LT((int)ctr, 8);
        }
    }
}

/* ================================================================
 * Effect_Poison (EFFECT_POISON = 0x42)
 * ================================================================ */

TEST(PoisonEffect, PlayerAttacks_SetsEnemyPoisoned) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_POISON;
    wPlayerMoveAccuracy = 255;
    wEnemyMon.status = 0;
    wEnemyMon.type1 = TYPE_NORMAL;
    wEnemyMon.type2 = TYPE_NORMAL;
    Battle_JumpMoveEffect();
    EXPECT_TRUE(IS_POISONED(wEnemyMon.status));
}

TEST(PoisonEffect, PoisonType_Blocked) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_POISON;
    wPlayerMoveAccuracy = 255;
    wEnemyMon.status = 0;
    wEnemyMon.type1 = TYPE_POISON;
    wEnemyMon.type2 = TYPE_POISON;
    Battle_JumpMoveEffect();
    EXPECT_FALSE(IS_POISONED(wEnemyMon.status));
}

TEST(PoisonEffect, AlreadyStatused_Blocked) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_POISON;
    wPlayerMoveAccuracy = 255;
    wEnemyMon.status = STATUS_PAR;
    wEnemyMon.type1 = TYPE_NORMAL;
    wEnemyMon.type2 = TYPE_NORMAL;
    Battle_JumpMoveEffect();
    EXPECT_FALSE(IS_POISONED(wEnemyMon.status));
}

TEST(PoisonEffect, SubstituteBlocks) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_POISON;
    wPlayerMoveAccuracy = 255;
    wEnemyMon.status = 0;
    wEnemyMon.type1 = TYPE_NORMAL;
    wEnemyMon.type2 = TYPE_NORMAL;
    wEnemyBattleStatus2 = (1 << BSTAT2_HAS_SUBSTITUTE);
    Battle_JumpMoveEffect();
    EXPECT_FALSE(IS_POISONED(wEnemyMon.status));
}

/* ================================================================
 * Poison side effects (EFFECT_POISON_SIDE1 = 0x02, SIDE2 = 0x21)
 * ================================================================ */

TEST(PoisonSide, Side1_Triggers_WhenRngLow) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_POISON_SIDE1;
    wEnemyMon.status = 0;
    wEnemyMon.type1 = TYPE_NORMAL;
    wEnemyMon.type2 = TYPE_NORMAL;
    seed_always_trigger();
    Battle_JumpMoveEffect();
    EXPECT_TRUE(IS_POISONED(wEnemyMon.status));
}

TEST(PoisonSide, Side1_NoTrigger_WhenRngHigh) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_POISON_SIDE1;
    wEnemyMon.status = 0;
    wEnemyMon.type1 = TYPE_NORMAL;
    wEnemyMon.type2 = TYPE_NORMAL;
    seed_never_trigger();
    Battle_JumpMoveEffect();
    EXPECT_FALSE(IS_POISONED(wEnemyMon.status));
}

/* ================================================================
 * Toxic (EFFECT_POISON + MOVE_TOXIC) — sets BADLY_POISONED flag
 * ================================================================ */

TEST(ToxicEffect, SetsBadlyPoisonedFlag) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect   = EFFECT_POISON;
    wPlayerMoveNum      = MOVE_TOXIC;
    wPlayerMoveAccuracy = 255;
    wEnemyMon.status    = 0;
    wEnemyMon.type1     = TYPE_NORMAL;
    wEnemyMon.type2     = TYPE_NORMAL;
    Battle_JumpMoveEffect();
    EXPECT_TRUE(IS_POISONED(wEnemyMon.status));
    EXPECT_TRUE(wEnemyBattleStatus3 & (1 << BSTAT3_BADLY_POISONED));
}

TEST(ToxicEffect, ToxicCounter_ResetToZero) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect   = EFFECT_POISON;
    wPlayerMoveNum      = MOVE_TOXIC;
    wPlayerMoveAccuracy = 255;
    wEnemyToxicCounter  = 5;
    wEnemyMon.status    = 0;
    wEnemyMon.type1     = TYPE_NORMAL;
    wEnemyMon.type2     = TYPE_NORMAL;
    Battle_JumpMoveEffect();
    EXPECT_EQ((int)wEnemyToxicCounter, 0);
}

/* ================================================================
 * Effect_Explode (EFFECT_EXPLODE = 0x07)
 * Sets attacker HP to 0 and clears attacker status/leech-seed.
 * ================================================================ */

TEST(ExplodeEffect, PlayerTurn_SetsAttackerHP_ToZero) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_EXPLODE;
    wBattleMon.hp = 200;
    Battle_JumpMoveEffect();
    EXPECT_EQ((int)wBattleMon.hp, 0);
}

TEST(ExplodeEffect, PlayerTurn_ClearsAttackerStatus) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_EXPLODE;
    wBattleMon.status = STATUS_BRN;
    Battle_JumpMoveEffect();
    EXPECT_EQ((int)wBattleMon.status, 0);
}

/* ================================================================
 * Effect_Haze (EFFECT_HAZE = 0x19)
 * Resets stat mods on both sides; cures TARGET's status only.
 * ================================================================ */

TEST(HazeEffect, ResetsAllStatMods_BothSides) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_HAZE;
    for (int i = 0; i < NUM_STAT_MODS; i++) {
        wPlayerMonStatMods[i] = 10;
        wEnemyMonStatMods[i]  = 4;
    }
    Battle_JumpMoveEffect();
    for (int i = 0; i < NUM_STAT_MODS; i++) {
        EXPECT_EQ((int)wPlayerMonStatMods[i], STAT_STAGE_NORMAL);
        EXPECT_EQ((int)wEnemyMonStatMods[i],  STAT_STAGE_NORMAL);
    }
}

TEST(HazeEffect, ClearsTargetStatus_NotAttackerStatus) {
    /* Player attacks (hWhoseTurn=0): target = enemy.
     * Enemy status should be cleared, player status should NOT be cleared. */
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_HAZE;
    wBattleMon.status = STATUS_PAR;   /* attacker — should remain */
    wEnemyMon.status  = STATUS_BRN;   /* target — should be cleared */
    Battle_JumpMoveEffect();
    EXPECT_EQ((int)wEnemyMon.status,  0);         /* target cleared */
    EXPECT_EQ((int)wBattleMon.status, STATUS_PAR); /* attacker unchanged */
}

/* ================================================================
 * Effect_Mist (EFFECT_MIST = 0x2E)
 * ================================================================ */

TEST(MistEffect, SetsProtectedByMist_PlayerSide) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_MIST;
    wPlayerBattleStatus2 = 0;
    Battle_JumpMoveEffect();
    EXPECT_TRUE(wPlayerBattleStatus2 & (1 << BSTAT2_PROTECTED_BY_MIST));
}

TEST(MistEffect, SetsProtectedByMist_EnemySide) {
    battle_reset();
    hWhoseTurn = 1;
    wEnemyMoveEffect = EFFECT_MIST;
    wEnemyBattleStatus2 = 0;
    Battle_JumpMoveEffect();
    EXPECT_TRUE(wEnemyBattleStatus2 & (1 << BSTAT2_PROTECTED_BY_MIST));
}

/* ================================================================
 * Effect_FocusEnergy (EFFECT_FOCUS_ENERGY = 0x2F)
 * ================================================================ */

TEST(FocusEnergy, SetsGettingPumped_PlayerSide) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_FOCUS_ENERGY;
    wPlayerBattleStatus2 = 0;
    Battle_JumpMoveEffect();
    EXPECT_TRUE(wPlayerBattleStatus2 & (1 << BSTAT2_GETTING_PUMPED));
}

TEST(FocusEnergy, SetsGettingPumped_EnemySide) {
    battle_reset();
    hWhoseTurn = 1;
    wEnemyMoveEffect = EFFECT_FOCUS_ENERGY;
    wEnemyBattleStatus2 = 0;
    Battle_JumpMoveEffect();
    EXPECT_TRUE(wEnemyBattleStatus2 & (1 << BSTAT2_GETTING_PUMPED));
}

/* ================================================================
 * Effect_Substitute (EFFECT_SUBSTITUTE = 0x4F)
 * Gen 1 quirk: only UNDERFLOW (carry) triggers failure — a mon that
 * reaches exactly 0 HP still creates the substitute.
 * ================================================================ */

TEST(SubstituteEffect, SetsHasSubstitute_WhenEnoughHP) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect    = EFFECT_SUBSTITUTE;
    wBattleMon.max_hp    = 200;
    wBattleMon.hp        = 150;  /* > 200/4=50, no underflow */
    wPlayerBattleStatus2 = 0;
    Battle_JumpMoveEffect();
    EXPECT_TRUE(wPlayerBattleStatus2 & (1 << BSTAT2_HAS_SUBSTITUTE));
}

TEST(SubstituteEffect, HPCost_DeductedFromAttacker) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_SUBSTITUTE;
    wBattleMon.max_hp = 200;
    wBattleMon.hp     = 150;
    Battle_JumpMoveEffect();
    EXPECT_EQ((int)wBattleMon.hp, 100);  /* 150 - 200/4=50 → 100 */
}

TEST(SubstituteEffect, ExactlyZeroHP_StillSucceeds) {
    /* hp == cost → hp becomes exactly 0, no underflow → substitute created */
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect    = EFFECT_SUBSTITUTE;
    wBattleMon.max_hp    = 200;
    wBattleMon.hp        = 50;  /* == cost (200/4=50), becomes 0, no carry */
    wPlayerBattleStatus2 = 0;
    Battle_JumpMoveEffect();
    EXPECT_TRUE(wPlayerBattleStatus2 & (1 << BSTAT2_HAS_SUBSTITUTE));
}

TEST(SubstituteEffect, Fails_WhenHPLessThanCost) {
    /* hp < cost → underflow → failure */
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect    = EFFECT_SUBSTITUTE;
    wBattleMon.max_hp    = 200;
    wBattleMon.hp        = 49;  /* < 50 = cost → underflow → fail */
    wPlayerBattleStatus2 = 0;
    Battle_JumpMoveEffect();
    EXPECT_FALSE(wPlayerBattleStatus2 & (1 << BSTAT2_HAS_SUBSTITUTE));
}

TEST(SubstituteEffect, SubstituteHP_SetToQuarterMaxHP) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect   = EFFECT_SUBSTITUTE;
    wBattleMon.max_hp   = 200;
    wBattleMon.hp       = 150;
    wPlayerSubstituteHP = 0;
    Battle_JumpMoveEffect();
    EXPECT_EQ((int)wPlayerSubstituteHP, 50);  /* max_hp/4 = 50 */
}

/* ================================================================
 * Effect_Bide (EFFECT_BIDE = 0x1A)
 * ================================================================ */

TEST(BideEffect, SetsStoringEnergyFlag) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect    = EFFECT_BIDE;
    wPlayerBattleStatus1 = 0;
    Battle_JumpMoveEffect();
    EXPECT_TRUE(wPlayerBattleStatus1 & (1 << BSTAT1_STORING_ENERGY));
}

TEST(BideEffect, ZeroesMovEffects_BothSides) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_BIDE;
    wEnemyMoveEffect  = EFFECT_POISON;
    Battle_JumpMoveEffect();
    EXPECT_EQ((int)wPlayerMoveEffect, 0);
    EXPECT_EQ((int)wEnemyMoveEffect,  0);
}

/* ================================================================
 * Effect_Recoil (EFFECT_RECOIL = 0x30)
 * ================================================================ */

TEST(RecoilEffect, AttackerTakesQuarterDamage) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_RECOIL;
    wDamage = 80;
    wBattleMon.hp = 100;
    Battle_JumpMoveEffect();
    EXPECT_EQ((int)wBattleMon.hp, 80);  /* recoil = 80/4 = 20; 100 - 20 = 80 */
}

TEST(RecoilEffect, RecoilMin1) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_RECOIL;
    wDamage = 3;     /* 3/4 = 0 → clamped to 1 */
    wBattleMon.hp = 50;
    Battle_JumpMoveEffect();
    EXPECT_EQ((int)wBattleMon.hp, 49);
}

/* ================================================================
 * Effect_LeechSeed (EFFECT_LEECH_SEED = 0x54)
 * ================================================================ */

TEST(LeechSeed, SetsSeededFlag_OnTarget) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect   = EFFECT_LEECH_SEED;
    wPlayerMoveAccuracy = 255;
    wEnemyBattleStatus2 = 0;
    wEnemyMon.type1     = TYPE_NORMAL;
    wEnemyMon.type2     = TYPE_NORMAL;
    Battle_JumpMoveEffect();
    EXPECT_TRUE(wEnemyBattleStatus2 & (1 << BSTAT2_SEEDED));
}

/* ================================================================
 * Effect_Charge (EFFECT_CHARGE = 0x27)
 * ================================================================ */

TEST(ChargeEffect, SetsChargingUpFlag_PlayerSide) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect    = EFFECT_CHARGE;
    wPlayerBattleStatus1 = 0;
    wChargeMoveNum       = 0;
    Battle_JumpMoveEffect();
    EXPECT_TRUE(wPlayerBattleStatus1 & (1 << BSTAT1_CHARGING_UP));
}

/* ================================================================
 * Effect_HyperBeam (EFFECT_HYPER_BEAM = 0x50)
 * Sets NEEDS_TO_RECHARGE on the attacker.
 * ================================================================ */

TEST(HyperBeam, SetsNeedsToRecharge_OnAttacker) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect    = EFFECT_HYPER_BEAM;
    wPlayerBattleStatus2 = 0;
    Battle_JumpMoveEffect();
    EXPECT_TRUE(wPlayerBattleStatus2 & (1 << BSTAT2_NEEDS_TO_RECHARGE));
}

/* ================================================================
 * Effect_Rage (EFFECT_RAGE = 0x51)
 * ================================================================ */

TEST(RageEffect, SetsUsingRageFlag) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect    = EFFECT_RAGE;
    wPlayerBattleStatus2 = 0;
    Battle_JumpMoveEffect();
    EXPECT_TRUE(wPlayerBattleStatus2 & (1 << BSTAT2_USING_RAGE));
}

/* ================================================================
 * Null-effect entries (EFFECT_NONE) — no-op
 * ================================================================ */

TEST(NullEffects, EFFECT_NONE_IsNoOp) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_NONE;
    wDamage = 42;
    Battle_JumpMoveEffect();
    EXPECT_EQ((int)wDamage, 42);
}

/* ================================================================
 * Effect_Paralyze (EFFECT_PARALYZE = 0x43)
 * ================================================================ */

TEST(ParalyzeEffect, SetsParalyze_OnTarget) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect   = EFFECT_PARALYZE;
    wPlayerMoveAccuracy = 255;
    wEnemyMon.status    = 0;
    wEnemyMon.type1     = TYPE_NORMAL;
    wEnemyMon.type2     = TYPE_NORMAL;
    Battle_JumpMoveEffect();
    EXPECT_TRUE(IS_PARALYZED(wEnemyMon.status));
}

TEST(ParalyzeEffect, Blocked_WhenAlreadyStatused) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect   = EFFECT_PARALYZE;
    wPlayerMoveAccuracy = 255;
    wEnemyMon.status    = STATUS_PSN;
    wEnemyMon.type1     = TYPE_NORMAL;
    wEnemyMon.type2     = TYPE_NORMAL;
    Battle_JumpMoveEffect();
    EXPECT_FALSE(IS_PARALYZED(wEnemyMon.status));
}

/* ================================================================
 * Effect_Heal (EFFECT_HEAL = 0x38)
 * Gen 1 bug: max_hp < 256 heals full max_hp instead of half.
 * Rest always heals to full and puts attacker to sleep (2 turns).
 * ================================================================ */

TEST(HealEffect, HealsHalfMaxHP_WhenMaxHPHigh) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_HEAL;
    wBattleMon.max_hp = 300;  /* >= 256, heals max_hp/2 = 150 */
    wBattleMon.hp     = 50;
    Battle_JumpMoveEffect();
    EXPECT_EQ((int)wBattleMon.hp, 200);  /* 50 + 150 = 200 */
}

TEST(HealEffect, Gen1Bug_MaxHPLessThan256_HealsFullHP) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_HEAL;
    wBattleMon.max_hp = 200;  /* < 256, heals full max_hp = 200 */
    wBattleMon.hp     = 50;
    Battle_JumpMoveEffect();
    EXPECT_EQ((int)wBattleMon.hp, 200);  /* capped at max_hp */
}

TEST(HealEffect, CapsAtMaxHP) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_HEAL;
    wBattleMon.max_hp = 300;
    wBattleMon.hp     = 280;
    Battle_JumpMoveEffect();
    EXPECT_EQ((int)wBattleMon.hp, 300);  /* 280 + 150 > 300, capped */
}

TEST(HealEffect, FailsIfAtFullHP) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_HEAL;
    wBattleMon.max_hp = 300;
    wBattleMon.hp     = 300;  /* already full */
    Battle_JumpMoveEffect();
    EXPECT_EQ((int)wBattleMon.hp, 300);  /* no change */
}

/* ================================================================
 * Effect_Rest (MOVE_REST uses EFFECT_HEAL)
 * Always heals to full HP regardless of max_hp size.
 * ================================================================ */

TEST(HealEffect_Rest, RestHealsToFull) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_HEAL;
    wPlayerMoveNum    = MOVE_REST;
    wBattleMon.max_hp = 300;
    wBattleMon.hp     = 1;
    Battle_JumpMoveEffect();
    EXPECT_EQ((int)wBattleMon.hp, 300);
}

TEST(HealEffect_Rest, RestSetsAttackerAsleep_TwoTurns) {
    battle_reset();
    hWhoseTurn = 0;
    wPlayerMoveEffect = EFFECT_HEAL;
    wPlayerMoveNum    = MOVE_REST;
    wBattleMon.max_hp = 200;
    wBattleMon.hp     = 100;
    wBattleMon.status = 0;
    Battle_JumpMoveEffect();
    EXPECT_EQ((int)(wBattleMon.status & STATUS_SLP_MASK), 2);
}
