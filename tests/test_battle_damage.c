/* test_battle_damage.c — Unit tests for Gen 1 battle damage functions.
 *
 * Covers all five Phase 1 functions:
 *   Battle_CalcDamage, Battle_CriticalHitTest, Battle_RandomizeDamage,
 *   Battle_CalcHitChance, Battle_MoveHitTest.
 *
 * All Gen 1 bugs are faithfully tested:
 *   - Focus Energy halves crit rate instead of doubling it (GETTING_PUMPED bug)
 *   - Substitute never blocks HP-draining moves (CheckTargetSubstitute overwrites A)
 *
 * RNG control: hRandomAdd / hRandomSub set directly before each RNG-dependent
 * test.  The PRNG is deterministic:
 *   hRandomAdd += 0x05; hRandomSub -= 0x03; return hRandomAdd ^ hRandomSub;
 *
 * Useful precomputed seeds (set BEFORE the call that consumes BattleRandom):
 *   seed_crit_hit()  — hRandomAdd=0xFB, hRandomSub=0x03 → BattleRandom=0x00
 *                       rlc3(0x00)=0x00 → guaranteed crit (r < any b > 0)
 *   seed_crit_miss() — hRandomAdd=0x7B, hRandomSub=0x82 → BattleRandom=0xFF
 *                       rlc3(0xFF)=0xFF → guaranteed no-crit (r >= b for b≤255)
 *   seed_r30()       — hRandomAdd=0xBE, hRandomSub=0x03 → BattleRandom=0xC3
 *                       rlc3(0xC3)=0x1E=30 → used for Focus Energy bug test
 *   seed_acc_hit()   — hRandomAdd=0xFB, hRandomSub=0x03 → BattleRandom=0x00
 *                       0 < any acc → guaranteed accuracy hit
 *   seed_acc_miss()  — hRandomAdd=0x00, hRandomSub=0xFF → BattleRandom=0xF9=249
 *                       249 >= 1 → accuracy miss for acc=1
 *   seed_rand_252()  — hRandomAdd=0x00, hRandomSub=0xFF → BattleRandom=0xF9=249
 *                       rrca(249)=252 → RandomizeDamage: damage × 252 / 255
 */
#include "test_runner.h"
#include "../src/game/battle/battle.h"
#include "../src/platform/hardware.h"
#include "../src/game/constants.h"

/* ---- Reset helper ----------------------------------------- */
static void battle_reset(void) {
    extern void WRAMClear(void);
    WRAMClear();  /* zeros damage/flags, sets stat mods to 7 */
    hRandomAdd = 0;
    hRandomSub = 0;
    hWhoseTurn = 0;                   /* player's turn */
    wPlayerMoveEffect   = EFFECT_NONE;
    wPlayerMovePower    = 80;
    wPlayerMoveType     = TYPE_NORMAL;
    wPlayerMoveAccuracy = 255;
    wPlayerMoveNum      = 1;          /* some non-high-crit move ID */
    wEnemyMoveEffect    = EFFECT_NONE;
    wEnemyMovePower     = 80;
    wEnemyMoveType      = TYPE_NORMAL;
    wEnemyMoveAccuracy  = 255;
    wEnemyMoveNum       = 1;
}

/* ---- RNG seeds (see file header for derivations) ----------- */
static void seed_crit_hit(void)  { hRandomAdd = 0xFB; hRandomSub = 0x03; }
static void seed_crit_miss(void) { hRandomAdd = 0x7B; hRandomSub = 0x82; }
static void seed_r30(void)       { hRandomAdd = 0xBE; hRandomSub = 0x03; }
static void seed_acc_hit(void)   { hRandomAdd = 0xFB; hRandomSub = 0x03; }
static void seed_acc_miss(void)  { hRandomAdd = 0x00; hRandomSub = 0xFF; }

/* ====================================================================
 * Battle_CalcDamage
 *
 * Formula: ((level×2/5 + 2) × power × attack / defense) / 50
 * Accumulated into wDamage; result capped at 997 then +2 → range [2, 999].
 * ==================================================================== */

/* level=5, power=40, attack=10, defense=10
 * d = (5*2/5 + 2)*40*10/10/50 = 4*40/50 = 3 → wDamage = 3+2 = 5 */
TEST(CalcDamage, BasicFormula_Lv5) {
    battle_reset();
    wDamage = 0;
    Battle_CalcDamage(10, 10, 40, 5);
    EXPECT_EQ((int)wDamage, 5);
}

/* level=100, power=90, attack=100, defense=60
 * d = (100*2/5+2)*90*100/60/50 = 42*9000/60/50 = 42*150/50 = 42*3 = 126
 * → wDamage = 126+2 = 128 */
TEST(CalcDamage, BasicFormula_Lv100) {
    battle_reset();
    wDamage = 0;
    Battle_CalcDamage(100, 60, 90, 100);
    EXPECT_EQ((int)wDamage, 128);
}

/* Status move (power=0) returns 0, leaves wDamage untouched */
TEST(CalcDamage, StatusMove_NoDamage) {
    battle_reset();
    wDamage = 0;
    int ret = Battle_CalcDamage(100, 100, 0, 50);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((int)wDamage, 0);
}

/* Extreme values must cap at 999 */
TEST(CalcDamage, Cap_At_999) {
    battle_reset();
    wDamage = 0;
    /* attack=255, defense=1, power=250, level=100:
     * d = 42*250*255/1/50 = 53550 → capped at 997 → wDamage=999 */
    Battle_CalcDamage(255, 1, 250, 100);
    EXPECT_EQ((int)wDamage, 999);
}

/* Any non-zero-power move does at least MIN_NEUTRAL_DAMAGE (2) */
TEST(CalcDamage, MinDamageIsTwo) {
    battle_reset();
    wDamage = 0;
    Battle_CalcDamage(1, 255, 1, 1);   /* very weak */
    EXPECT_GE((int)wDamage, MIN_NEUTRAL_DAMAGE);
}

/* EFFECT_EXPLODE: defense is halved before the formula (min 1).
 * level=50, power=250, attack=200, defense=200 (halved to 100):
 * d = (50*2/5+2)*250*200/100/50 = 22*250*200/100/50 = 22*500/50 = 22*10 = 220
 * → wDamage = 220+2 = 222 */
TEST(CalcDamage, Explode_HalvesDefense) {
    battle_reset();
    wPlayerMoveEffect = EFFECT_EXPLODE;
    wDamage = 0;
    Battle_CalcDamage(200, 200, 250, 50);
    EXPECT_EQ((int)wDamage, 222);
}

/* Accumulation across calls: second call adds to existing wDamage.
 * level=50, power=50, attack=50, defense=50:
 * d = (50*2/5+2)*50*50/50/50 = 22*50/50 = 22
 * Call 1: wDamage = 0+22+2 = 24
 * Call 2: wDamage = 24+22+2 = 48 */
TEST(CalcDamage, Accumulates_AcrossCalls) {
    battle_reset();
    wDamage = 0;
    Battle_CalcDamage(50, 50, 50, 50);
    EXPECT_EQ((int)wDamage, 24);
    Battle_CalcDamage(50, 50, 50, 50);
    EXPECT_EQ((int)wDamage, 48);
}

/* ====================================================================
 * Battle_CriticalHitTest
 *
 * Electrode (species 0x8D, dex 101) has base speed 140.
 * Normal crit: b = 140>>1=70, ×2=140, ÷2=70  →  crit if rlc3(r) < 70
 * ==================================================================== */

/* Seed → rlc3(r)=0 < 70 → CRIT */
TEST(CriticalHit, Electrode_Crits) {
    battle_reset();
    wBattleMon.species = SPECIES_ELECTRODE;  /* Electrode, speed 140 */
    wPlayerMovePower   = 90;
    seed_crit_hit();
    Battle_CriticalHitTest();
    EXPECT_EQ((int)wCriticalHitOrOHKO, 1);
}

/* Seed → rlc3(r)=0xFF=255 ≥ 70 → NO CRIT */
TEST(CriticalHit, Electrode_NoCrit) {
    battle_reset();
    wBattleMon.species = SPECIES_ELECTRODE;
    wPlayerMovePower   = 90;
    seed_crit_miss();
    Battle_CriticalHitTest();
    EXPECT_EQ((int)wCriticalHitOrOHKO, 0);
}

/* Status move (power=0): always skip crit roll */
TEST(CriticalHit, StatusMove_NeverCrits) {
    battle_reset();
    wBattleMon.species = SPECIES_ELECTRODE;
    wPlayerMovePower   = 0;
    seed_crit_hit();
    Battle_CriticalHitTest();
    EXPECT_EQ((int)wCriticalHitOrOHKO, 0);
}

/* GEN 1 BUG — Focus Energy halves crit rate instead of doubling it.
 *
 * Seed produces rlc3(r)=30.
 * Without Focus Energy: b=70 → 30 < 70 → CRIT.
 * With Focus Energy:    b=17 → 30 ≥ 17 → NO CRIT.  (bug: should be b=280→0xFF)
 *
 * hRandomAdd=0xBE, hRandomSub=0x03:
 *   → BattleRandom=0xC3, rlc3(0xC3)=(0xC3<<3)|(0xC3>>5)=0x18|0x06=0x1E=30 */
TEST(CriticalHit, FocusEnergyBug_WithoutFE_IsCrit) {
    battle_reset();
    wBattleMon.species   = SPECIES_ELECTRODE;
    wPlayerMovePower     = 90;
    wPlayerBattleStatus2 = 0;
    seed_r30();
    Battle_CriticalHitTest();
    EXPECT_EQ((int)wCriticalHitOrOHKO, 1);  /* 30 < 70 */
}

TEST(CriticalHit, FocusEnergyBug_WithFE_NoCrit) {
    battle_reset();
    wBattleMon.species   = SPECIES_ELECTRODE;
    wPlayerMovePower     = 90;
    wPlayerBattleStatus2 = (uint8_t)(1 << BSTAT2_GETTING_PUMPED);
    /* b = 70 >> 1 = 35 (FE halves instead of doubling — the bug)
     * then non-high-crit halves again: b = 35 >> 1 = 17
     * rlc3(r)=30 ≥ 17 → no crit */
    seed_r30();
    Battle_CriticalHitTest();
    EXPECT_EQ((int)wCriticalHitOrOHKO, 0);  /* bug confirmed: FE reduced crit chance */
}

/* High-crit move (Slash = MOVE_SLASH = 0xA3) saturates b to 0xFF.
 * Electrode: b=70 → ×2=140 → high-crit: ×2=0xFF → ×2=0xFF → threshold=0xFF.
 * rlc3(r)=30 < 0xFF → CRIT. */
TEST(CriticalHit, HighCritMove_Slash) {
    battle_reset();
    wBattleMon.species = SPECIES_ELECTRODE;
    wPlayerMovePower   = 70;
    wPlayerMoveNum     = MOVE_SLASH;
    seed_r30();
    Battle_CriticalHitTest();
    EXPECT_EQ((int)wCriticalHitOrOHKO, 1);
}

/* Karate Chop is also a high-crit move */
TEST(CriticalHit, HighCritMove_KarateChop) {
    battle_reset();
    wBattleMon.species = SPECIES_ELECTRODE;
    wPlayerMovePower   = 50;
    wPlayerMoveNum     = MOVE_KARATE_CHOP;
    seed_r30();
    Battle_CriticalHitTest();
    EXPECT_EQ((int)wCriticalHitOrOHKO, 1);
}

/* Enemy turn: reads wEnemyMon.species and wEnemyMovePower */
TEST(CriticalHit, EnemyTurn) {
    battle_reset();
    hWhoseTurn         = 1;
    wEnemyMon.species  = SPECIES_ELECTRODE;
    wEnemyMovePower    = 90;
    seed_crit_hit();
    Battle_CriticalHitTest();
    EXPECT_EQ((int)wCriticalHitOrOHKO, 1);
}

/* ====================================================================
 * Battle_RandomizeDamage
 *
 * Skips if wDamage ≤ 1.
 * Otherwise: loop BattleRandom+rrca until result ≥ 217, then
 *            wDamage = wDamage × result / 255.
 * Range: [85%, 100%] of original damage.
 * ==================================================================== */

TEST(RandomizeDamage, Zero_Unchanged) {
    battle_reset();
    wDamage = 0;
    Battle_RandomizeDamage();
    EXPECT_EQ((int)wDamage, 0);
}

TEST(RandomizeDamage, One_Unchanged) {
    battle_reset();
    wDamage = 1;
    Battle_RandomizeDamage();
    EXPECT_EQ((int)wDamage, 1);
}

/* hRandomAdd=0x00, hRandomSub=0xFF:
 *   BattleRandom → hRandomAdd=0x05, hRandomSub=0xFC → 0x05^0xFC=0xF9=249 (odd)
 *   rrca(249) = (249>>1)|((249&1)<<7) = 124|128 = 252 ≥ 217 → accept
 *   wDamage = 100 * 252 / 255 = 98 */
TEST(RandomizeDamage, KnownSeed_100Damage) {
    battle_reset();
    wDamage    = 100;
    hRandomAdd = 0x00;
    hRandomSub = 0xFF;
    Battle_RandomizeDamage();
    EXPECT_EQ((int)wDamage, 98);
}

/* Result is always in [85%, 100%] range regardless of seed.
 * Runs 20 trials with sequential RNG state. */
TEST(RandomizeDamage, AlwaysInRange) {
    battle_reset();
    hRandomAdd = 0xAA;
    hRandomSub = 0x55;
    int fails = 0;
    for (int i = 0; i < 20; i++) {
        wDamage = 200;
        Battle_RandomizeDamage();
        /* min: 200*217/255=170; max: 200*255/255=200 */
        if ((int)wDamage < 170 || (int)wDamage > 200) fails++;
    }
    EXPECT_EQ(fails, 0);
}

/* ====================================================================
 * Battle_CalcHitChance
 *
 * Scales wPlayerMoveAccuracy by:
 *   1. Attacker accuracy stage (StatModifierRatios[acc_mod-1])
 *   2. Reflected defender evasion stage (eff_eva = 14 - evasion_mod)
 * Floored at 1 per iteration; capped at 255 at end.
 * ==================================================================== */

/* Normal stages (7 = 1/1): accuracy passes through unchanged */
TEST(CalcHitChance, NormalStages_Unchanged) {
    battle_reset();
    /* WRAMClear already set all stat mods to 7 */
    wPlayerMoveAccuracy = 200;
    Battle_CalcHitChance();
    EXPECT_EQ((int)wPlayerMoveAccuracy, 200);
}

TEST(CalcHitChance, FullAccuracy_NormalStages) {
    battle_reset();
    wPlayerMoveAccuracy = 255;
    Battle_CalcHitChance();
    EXPECT_EQ((int)wPlayerMoveAccuracy, 255);
}

/* Defender evasion +1 (stage 8, ratio 66/100).
 * eff_eva = 14-8 = 6; stage 6 = {66,100}.
 * acc_mod=7 → stage 7 = {1,1}: val = 255*1/1 = 255.
 * Then stage 6: val = 255*66/100 = 168. */
TEST(CalcHitChance, DefenderEvasionUp1) {
    battle_reset();
    wPlayerMoveAccuracy          = 255;
    wPlayerMonStatMods[MOD_ACCURACY] = 7;
    wEnemyMonStatMods[MOD_EVASION]   = 8;
    Battle_CalcHitChance();
    EXPECT_EQ((int)wPlayerMoveAccuracy, 168);
}

/* Attacker accuracy -2 (stage 5, ratio 50/100).
 * val = 200*50/100 = 100; eff_eva normal (7→{1,1}): val stays 100. */
TEST(CalcHitChance, AttackerAccuracyDown2) {
    battle_reset();
    wPlayerMoveAccuracy              = 200;
    wPlayerMonStatMods[MOD_ACCURACY] = 5;
    wEnemyMonStatMods[MOD_EVASION]   = 7;
    Battle_CalcHitChance();
    EXPECT_EQ((int)wPlayerMoveAccuracy, 100);
}

/* Extreme case: acc_mod=1 (25/100), eva_mod=13 (eff_eva=1, 25/100).
 * val = 1*25/100 = 0 → floored to 1; 1*25/100 = 0 → floored to 1.
 * Result: 1 (never zero). */
TEST(CalcHitChance, FloorAtOne) {
    battle_reset();
    wPlayerMoveAccuracy              = 1;
    wPlayerMonStatMods[MOD_ACCURACY] = 1;
    wEnemyMonStatMods[MOD_EVASION]   = 13;
    Battle_CalcHitChance();
    EXPECT_GE((int)wPlayerMoveAccuracy, 1);
}

/* Enemy turn: reads/writes wEnemyMoveAccuracy */
TEST(CalcHitChance, EnemyTurn) {
    battle_reset();
    hWhoseTurn = 1;
    wEnemyMoveAccuracy               = 200;
    wEnemyMonStatMods[MOD_ACCURACY]  = 7;
    wPlayerMonStatMods[MOD_EVASION]  = 7;
    Battle_CalcHitChance();
    EXPECT_EQ((int)wEnemyMoveAccuracy, 200);
}

/* ====================================================================
 * Battle_MoveHitTest
 *
 * On hit:  returns normally, wMoveMissed unchanged (0).
 * On miss: wDamage=0, wMoveMissed=1, BSTAT1_USING_TRAPPING cleared.
 * ==================================================================== */

/* Dream Eater requires target to be asleep */
TEST(MoveHitTest, DreamEater_SleepingTarget_Hits) {
    battle_reset();
    wPlayerMoveEffect = EFFECT_DREAM_EATER;
    wPlayerMovePower  = 100;
    wEnemyMon.status  = 0x03;   /* sleep: 3 turns remaining */
    seed_acc_hit();
    wMoveMissed = 0;
    Battle_MoveHitTest();
    EXPECT_EQ((int)wMoveMissed, 0);
}

TEST(MoveHitTest, DreamEater_AwakeTarget_Misses) {
    battle_reset();
    wPlayerMoveEffect = EFFECT_DREAM_EATER;
    wPlayerMovePower  = 100;
    wEnemyMon.status  = 0;      /* awake */
    wMoveMissed = 0;
    Battle_MoveHitTest();
    EXPECT_EQ((int)wMoveMissed, 1);
    EXPECT_EQ((int)wDamage, 0);
}

/* Swift always hits, even past INVULNERABLE */
TEST(MoveHitTest, Swift_AlwaysHits_EvenInvulnerable) {
    battle_reset();
    wPlayerMoveEffect   = EFFECT_SWIFT;
    wEnemyBattleStatus1 = (uint8_t)(1 << BSTAT1_INVULNERABLE);
    wMoveMissed = 0;
    Battle_MoveHitTest();
    EXPECT_EQ((int)wMoveMissed, 0);
}

/* INVULNERABLE target (Fly/Dig) blocks all non-Swift moves */
TEST(MoveHitTest, Invulnerable_Target_Misses) {
    battle_reset();
    wPlayerMoveEffect   = EFFECT_NONE;
    wEnemyBattleStatus1 = (uint8_t)(1 << BSTAT1_INVULNERABLE);
    wMoveMissed = 0;
    Battle_MoveHitTest();
    EXPECT_EQ((int)wMoveMissed, 1);
}

/* Mist blocks stat-lowering effects in range [EFFECT_ATTACK_DOWN1, EFFECT_HAZE] */
TEST(MoveHitTest, Mist_BlocksStatLower) {
    battle_reset();
    wPlayerMoveEffect   = EFFECT_ATTACK_DOWN1;  /* 0x12, inside Mist range */
    wEnemyBattleStatus2 = (uint8_t)(1 << BSTAT2_PROTECTED_BY_MIST);
    wMoveMissed = 0;
    Battle_MoveHitTest();
    EXPECT_EQ((int)wMoveMissed, 1);
}

/* Mist does NOT block damage moves (EFFECT_NONE) */
TEST(MoveHitTest, Mist_DoesNotBlockDamage) {
    battle_reset();
    wPlayerMoveEffect   = EFFECT_NONE;
    wPlayerMoveAccuracy = 255;
    wEnemyBattleStatus2 = (uint8_t)(1 << BSTAT2_PROTECTED_BY_MIST);
    seed_acc_hit();
    wMoveMissed = 0;
    Battle_MoveHitTest();
    EXPECT_EQ((int)wMoveMissed, 0);
}

/* X-Accuracy bypasses the accuracy roll entirely */
TEST(MoveHitTest, XAccuracy_AlwaysHits) {
    battle_reset();
    wPlayerMoveEffect    = EFFECT_NONE;
    wPlayerMoveAccuracy  = 1;    /* near-zero accuracy — would normally miss */
    wPlayerBattleStatus2 = (uint8_t)(1 << BSTAT2_USING_X_ACCURACY);
    wMoveMissed = 0;
    Battle_MoveHitTest();
    EXPECT_EQ((int)wMoveMissed, 0);
}

/* GEN 1 BUG — Substitute doesn't block HP-draining moves.
 *
 * CheckTargetSubstitute stores 0/1 in A (overwriting the move effect),
 * so the subsequent `cp DRAIN_HP_EFFECT` (0x03) always fails because
 * A is 1, not 0x03.  Drain moves pass right through a substitute. */
TEST(MoveHitTest, SubstituteBug_DrainPassesThrough) {
    battle_reset();
    wPlayerMoveEffect   = EFFECT_DRAIN_HP;   /* Absorb / Mega Drain */
    wPlayerMoveAccuracy = 255;
    wEnemyBattleStatus2 = (uint8_t)(1 << BSTAT2_HAS_SUBSTITUTE);
    seed_acc_hit();
    wMoveMissed = 0;
    Battle_MoveHitTest();
    EXPECT_EQ((int)wMoveMissed, 0);   /* Bug: substitute did NOT block drain */
}

/* Normal 100% accuracy hits with favorable roll */
TEST(MoveHitTest, FullAccuracy_Hits) {
    battle_reset();
    wPlayerMoveEffect   = EFFECT_NONE;
    wPlayerMoveAccuracy = 255;
    seed_acc_hit();   /* BattleRandom → 0 < 255 → hit */
    wMoveMissed = 0;
    Battle_MoveHitTest();
    EXPECT_EQ((int)wMoveMissed, 0);
}

/* Low accuracy misses with unfavorable roll */
TEST(MoveHitTest, LowAccuracy_Misses) {
    battle_reset();
    wPlayerMoveEffect   = EFFECT_NONE;
    wPlayerMoveAccuracy = 1;
    seed_acc_miss();   /* BattleRandom → 249 ≥ 1 → miss */
    wMoveMissed = 0;
    Battle_MoveHitTest();
    EXPECT_EQ((int)wMoveMissed, 1);
}

/* On miss: BSTAT1_USING_TRAPPING cleared from attacker */
TEST(MoveHitTest, OnMiss_ClearsTrappingFlag) {
    battle_reset();
    wPlayerMoveEffect    = EFFECT_NONE;
    wPlayerMoveAccuracy  = 1;
    wPlayerBattleStatus1 = (uint8_t)(1 << BSTAT1_USING_TRAPPING);
    seed_acc_miss();
    Battle_MoveHitTest();
    EXPECT_EQ((int)(wPlayerBattleStatus1 & (1 << BSTAT1_USING_TRAPPING)), 0);
}

/* ====================================================================
 * Battle_AdjustDamageForMoveType
 * Exact port of AdjustDamageForMoveType (core.asm:5075).
 * ==================================================================== */

/* No STAB, no type match: wDamage unchanged */
TEST(AdjustDamageForMoveType, NoStab_NoTypeMatch_Unchanged) {
    battle_reset();
    /* Normal move (TYPE_NORMAL) vs Water/Water defender — no type entry */
    wBattleMon.type1     = TYPE_FIRE;
    wBattleMon.type2     = TYPE_FIRE;
    wEnemyMon.type1      = TYPE_NORMAL;
    wEnemyMon.type2      = TYPE_NORMAL;
    wPlayerMoveType      = TYPE_NORMAL;  /* no STAB (Fire != Normal) */
    wDamage              = 100;
    Battle_AdjustDamageForMoveType();
    EXPECT_EQ((int)wDamage, 100);
    EXPECT_EQ((int)(wDamageMultipliers & (1 << BIT_STAB_DAMAGE)), 0);
}

/* STAB only: Grass move from Grass mon vs Fire/Fire — no type chart entry (Grass vs Normal?) */
TEST(AdjustDamageForMoveType, StabOnly_NoTypeMatch) {
    battle_reset();
    wBattleMon.type1     = TYPE_GRASS;
    wBattleMon.type2     = TYPE_GRASS;
    wEnemyMon.type1      = TYPE_NORMAL;
    wEnemyMon.type2      = TYPE_NORMAL;
    wPlayerMoveType      = TYPE_GRASS;   /* STAB: Grass mon uses Grass move */
    wDamage              = 100;
    Battle_AdjustDamageForMoveType();
    /* STAB: 100 + floor(100/2) = 150 */
    EXPECT_EQ((int)wDamage, 150);
    EXPECT_NE((int)(wDamageMultipliers & (1 << BIT_STAB_DAMAGE)), 0);
}

/* Super effective: Water move vs Fire/Fire mono-type.
 * eff=20, wDamage*20/10 = 200. */
TEST(AdjustDamageForMoveType, SuperEffective_MonoType) {
    battle_reset();
    wBattleMon.type1     = TYPE_NORMAL;
    wBattleMon.type2     = TYPE_NORMAL;
    wEnemyMon.type1      = TYPE_FIRE;
    wEnemyMon.type2      = TYPE_FIRE;   /* mono-type */
    wPlayerMoveType      = TYPE_WATER;
    wDamage              = 100;
    Battle_AdjustDamageForMoveType();
    /* No STAB (Normal != Water).  SE vs Fire: 100*20/10 = 200.
     * Mono-type: applied exactly once. */
    EXPECT_EQ((int)wDamage, 200);
}

/* Not very effective: Fire move vs Water/Water mono-type.
 * eff=5, wDamage*5/10 = floor(50) = 50. */
TEST(AdjustDamageForMoveType, NotVeryEffective_MonoType) {
    battle_reset();
    wBattleMon.type1     = TYPE_NORMAL;
    wBattleMon.type2     = TYPE_NORMAL;
    wEnemyMon.type1      = TYPE_WATER;
    wEnemyMon.type2      = TYPE_WATER;
    wPlayerMoveType      = TYPE_FIRE;
    wDamage              = 100;
    Battle_AdjustDamageForMoveType();
    EXPECT_EQ((int)wDamage, 50);
}

/* Immunity: Normal move vs Ghost/Ghost mono-type.
 * eff=0 → wDamage=0 → wMoveMissed=1. */
TEST(AdjustDamageForMoveType, Immune_SetsMissFlag) {
    battle_reset();
    wBattleMon.type1     = TYPE_NORMAL;
    wBattleMon.type2     = TYPE_NORMAL;
    wEnemyMon.type1      = TYPE_GHOST;
    wEnemyMon.type2      = TYPE_GHOST;
    wPlayerMoveType      = TYPE_NORMAL;
    wDamage              = 100;
    wMoveMissed          = 0;
    Battle_AdjustDamageForMoveType();
    EXPECT_EQ((int)wDamage, 0);
    EXPECT_EQ((int)wMoveMissed, 1);
}

/* Dual-type 4× — Water vs Fire/Rock: SE vs Fire (2×), SE vs Rock (2×) = 4×.
 * 100 * 2 * 2 = 400. */
TEST(AdjustDamageForMoveType, DualType_4x) {
    battle_reset();
    wBattleMon.type1     = TYPE_NORMAL;
    wBattleMon.type2     = TYPE_NORMAL;
    wEnemyMon.type1      = TYPE_FIRE;
    wEnemyMon.type2      = TYPE_ROCK;
    wPlayerMoveType      = TYPE_WATER;
    wDamage              = 100;
    Battle_AdjustDamageForMoveType();
    /* Water SE vs Fire (×2): 200.  Water SE vs Rock (×2): 400. */
    EXPECT_EQ((int)wDamage, 400);
}

/* Dual-type cancel — Fire vs Grass/Rock: SE (×2) then NVE (÷2) = 100 (no change). */
TEST(AdjustDamageForMoveType, DualType_CancelOut) {
    battle_reset();
    wBattleMon.type1     = TYPE_NORMAL;
    wBattleMon.type2     = TYPE_NORMAL;
    wEnemyMon.type1      = TYPE_GRASS;
    wEnemyMon.type2      = TYPE_ROCK;
    wPlayerMoveType      = TYPE_FIRE;
    wDamage              = 100;
    Battle_AdjustDamageForMoveType();
    /* Fire SE vs Grass (×2)=200, Fire NVE vs Rock (÷2)=100 */
    EXPECT_EQ((int)wDamage, 100);
}

/* STAB + super effective: Water Blastoise vs Fire/Rock.
 * STAB: 100→150. SE vs Fire: 300. SE vs Rock: 600. */
TEST(AdjustDamageForMoveType, Stab_Plus_4x) {
    battle_reset();
    wBattleMon.type1     = TYPE_WATER;
    wBattleMon.type2     = TYPE_WATER;
    wEnemyMon.type1      = TYPE_FIRE;
    wEnemyMon.type2      = TYPE_ROCK;
    wPlayerMoveType      = TYPE_WATER;
    wDamage              = 100;
    Battle_AdjustDamageForMoveType();
    EXPECT_EQ((int)wDamage, 600);
    EXPECT_NE((int)(wDamageMultipliers & (1 << BIT_STAB_DAMAGE)), 0);
}

/* Enemy turn: reads wEnemyMoveType and wEnemyMon/wBattleMon correctly. */
TEST(AdjustDamageForMoveType, EnemyTurn) {
    battle_reset();
    hWhoseTurn           = 1;            /* enemy's turn */
    wEnemyMon.type1      = TYPE_WATER;
    wEnemyMon.type2      = TYPE_WATER;   /* attacker */
    wBattleMon.type1     = TYPE_FIRE;
    wBattleMon.type2     = TYPE_FIRE;    /* defender */
    wEnemyMoveType       = TYPE_WATER;   /* STAB + SE */
    wDamage              = 100;
    Battle_AdjustDamageForMoveType();
    /* STAB: 150. SE vs Fire: 300. Mono-type Fire: once. */
    EXPECT_EQ((int)wDamage, 300);
}

/* wDamageMultipliers tracks last effectiveness correctly.
 * Fire vs Grass/Rock: SE(20) then NVE(5).
 * After each, low 7 bits = eff, bit 7 = STAB. */
TEST(AdjustDamageForMoveType, DamageMultipliers_Updated) {
    battle_reset();
    wBattleMon.type1     = TYPE_NORMAL;
    wBattleMon.type2     = TYPE_NORMAL;
    wEnemyMon.type1      = TYPE_GRASS;
    wEnemyMon.type2      = TYPE_ROCK;
    wPlayerMoveType      = TYPE_FIRE;
    wDamage              = 100;
    Battle_AdjustDamageForMoveType();
    /* Last eff applied = 5 (NVE vs Rock), no STAB */
    EXPECT_EQ((int)(wDamageMultipliers & 0x7F), 5);
    EXPECT_EQ((int)(wDamageMultipliers & (1 << BIT_STAB_DAMAGE)), 0);
}

/* ====================================================================
 * Battle_CalculateModifiedStats
 *
 * Applies stage ratios from kBattleStatModRatios[stage-1] to unmodified
 * stats, caps at 999, floors at 1, writes into wBattleMon or wEnemyMon.
 * ==================================================================== */

/* Stage 7 (normal) = {1,1}: stat unchanged */
TEST(CalculateModifiedStats, NormalStage_NoChange) {
    battle_reset();
    wCalculateWhoseStats           = 0;
    wPlayerMonStatMods[MOD_ATTACK] = STAT_STAGE_NORMAL;
    wPlayerMonUnmodifiedAttack     = 100;
    Battle_CalculateModifiedStats();
    EXPECT_EQ((int)wBattleMon.atk, 100);
}

/* Stage 8 = {15,10} = 1.5×: 100 → 150 */
TEST(CalculateModifiedStats, StageBoost) {
    battle_reset();
    wCalculateWhoseStats           = 0;
    wPlayerMonStatMods[MOD_ATTACK] = 8;
    wPlayerMonUnmodifiedAttack     = 100;
    Battle_CalculateModifiedStats();
    EXPECT_EQ((int)wBattleMon.atk, 150);
}

/* Stage 6 = {66,100}: 100 → 66 */
TEST(CalculateModifiedStats, StageDrop) {
    battle_reset();
    wCalculateWhoseStats           = 0;
    wPlayerMonStatMods[MOD_ATTACK] = 6;
    wPlayerMonUnmodifiedAttack     = 100;
    Battle_CalculateModifiedStats();
    EXPECT_EQ((int)wBattleMon.atk, 66);
}

/* Stage 13 = {4,1} = 4×: 300 → 1200 capped at 999 */
TEST(CalculateModifiedStats, Cap_At_999) {
    battle_reset();
    wCalculateWhoseStats           = 0;
    wPlayerMonStatMods[MOD_ATTACK] = 13;
    wPlayerMonUnmodifiedAttack     = 300;
    Battle_CalculateModifiedStats();
    EXPECT_EQ((int)wBattleMon.atk, 999);
}

/* Stage 1 = {25,100}: 3*25/100=0 → floor at 1 */
TEST(CalculateModifiedStats, Floor_At_1) {
    battle_reset();
    wCalculateWhoseStats           = 0;
    wPlayerMonStatMods[MOD_ATTACK] = 1;
    wPlayerMonUnmodifiedAttack     = 3;
    Battle_CalculateModifiedStats();
    EXPECT_EQ((int)wBattleMon.atk, 1);
}

/* wCalculateWhoseStats != 0 → applies to wEnemyMon */
TEST(CalculateModifiedStats, EnemySide) {
    battle_reset();
    wCalculateWhoseStats           = 1;
    wEnemyMonStatMods[MOD_ATTACK]  = 8;   /* 1.5× */
    wEnemyMonUnmodifiedAttack      = 200;
    Battle_CalculateModifiedStats();
    EXPECT_EQ((int)wEnemyMon.atk, 300);
}

/* ====================================================================
 * Battle_ApplyBurnAndParalysisPenalties
 *
 * hWhoseTurn=1 → applies to wBattleMon (player side switching in).
 * hWhoseTurn=0 → applies to wEnemyMon (enemy side switching in).
 * PAR: spd >>= 2 (min 1).  BRN: atk >>= 1 (min 1).
 * ==================================================================== */

TEST(ApplyBurnAndParalysisPenalties, PlayerSide_Paralysis) {
    battle_reset();
    hWhoseTurn        = 1;
    wBattleMon.status = STATUS_PAR;
    wBattleMon.spd    = 100;
    Battle_ApplyBurnAndParalysisPenalties();
    EXPECT_EQ((int)wBattleMon.spd, 25);
}

TEST(ApplyBurnAndParalysisPenalties, PlayerSide_Burn) {
    battle_reset();
    hWhoseTurn        = 1;
    wBattleMon.status = STATUS_BRN;
    wBattleMon.atk    = 100;
    Battle_ApplyBurnAndParalysisPenalties();
    EXPECT_EQ((int)wBattleMon.atk, 50);
}

TEST(ApplyBurnAndParalysisPenalties, EnemySide_Paralysis) {
    battle_reset();
    hWhoseTurn       = 0;
    wEnemyMon.status = STATUS_PAR;
    wEnemyMon.spd    = 100;
    Battle_ApplyBurnAndParalysisPenalties();
    EXPECT_EQ((int)wEnemyMon.spd, 25);
}

TEST(ApplyBurnAndParalysisPenalties, EnemySide_Burn) {
    battle_reset();
    hWhoseTurn       = 0;
    wEnemyMon.status = STATUS_BRN;
    wEnemyMon.atk    = 100;
    Battle_ApplyBurnAndParalysisPenalties();
    EXPECT_EQ((int)wEnemyMon.atk, 50);
}

/* PAR with spd=1: 1>>2=0, floored at 1 */
TEST(ApplyBurnAndParalysisPenalties, MinOne_Paralysis) {
    battle_reset();
    hWhoseTurn        = 1;
    wBattleMon.status = STATUS_PAR;
    wBattleMon.spd    = 1;
    Battle_ApplyBurnAndParalysisPenalties();
    EXPECT_EQ((int)wBattleMon.spd, 1);
}

/* BRN with atk=1: 1>>1=0, floored at 1 */
TEST(ApplyBurnAndParalysisPenalties, MinOne_Burn) {
    battle_reset();
    hWhoseTurn        = 1;
    wBattleMon.status = STATUS_BRN;
    wBattleMon.atk    = 1;
    Battle_ApplyBurnAndParalysisPenalties();
    EXPECT_EQ((int)wBattleMon.atk, 1);
}

/* No status condition → no stat change */
TEST(ApplyBurnAndParalysisPenalties, NoStatus_NoChange) {
    battle_reset();
    hWhoseTurn        = 1;
    wBattleMon.status = 0;
    wBattleMon.atk    = 100;
    wBattleMon.spd    = 100;
    Battle_ApplyBurnAndParalysisPenalties();
    EXPECT_EQ((int)wBattleMon.atk, 100);
    EXPECT_EQ((int)wBattleMon.spd, 100);
}

/* ====================================================================
 * Battle_GetDamageVarsForPlayerAttack
 *
 * Selects physical (ATK/DEF) or special (SPC/SPC) based on move type.
 * Doubles defender stat for Reflect/Light Screen.
 * On crit: bypasses stages — party mon for attacker, CalcStat for enemy.
 * Doubles level on crit.  Calls Battle_CalcDamage.
 * ==================================================================== */

TEST(GetDamageVarsForPlayerAttack, PowerZero_Returns0) {
    battle_reset();
    wPlayerMovePower = 0;
    int ret = Battle_GetDamageVarsForPlayerAttack();
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((int)wDamage, 0);
}

/* Physical: ATK vs DEF.
 * atk=100, def=60, power=90, level=100:
 * d = (100*2/5+2)*90*100/60/50 = 42*150/50 = 126 → wDamage=128 */
TEST(GetDamageVarsForPlayerAttack, Physical_Normal) {
    battle_reset();
    wBattleMon.atk   = 100;
    wBattleMon.level = 100;
    wEnemyMon.def    = 60;
    wPlayerMovePower = 90;
    wPlayerMoveType  = TYPE_NORMAL;
    Battle_GetDamageVarsForPlayerAttack();
    EXPECT_EQ((int)wDamage, 128);
}

/* Special: SPC vs SPC (TYPE_FIRE >= TYPE_SPECIAL_THRESHOLD).
 * spc=100 vs spc=60, power=90, level=100 → wDamage=128 */
TEST(GetDamageVarsForPlayerAttack, Special_Normal) {
    battle_reset();
    wBattleMon.spc   = 100;
    wBattleMon.level = 100;
    wEnemyMon.spc    = 60;
    wPlayerMovePower = 90;
    wPlayerMoveType  = TYPE_FIRE;
    Battle_GetDamageVarsForPlayerAttack();
    EXPECT_EQ((int)wDamage, 128);
}

/* Reflect: enemy DEF 60 → 120.
 * d = 42*90*100/120/50 = 63 → wDamage=65 */
TEST(GetDamageVarsForPlayerAttack, Reflect_DoublesEnemyDef) {
    battle_reset();
    wBattleMon.atk      = 100;
    wBattleMon.level    = 100;
    wEnemyMon.def       = 60;
    wPlayerMovePower    = 90;
    wPlayerMoveType     = TYPE_NORMAL;
    wEnemyBattleStatus3 = (1 << BSTAT3_HAS_REFLECT);
    Battle_GetDamageVarsForPlayerAttack();
    EXPECT_EQ((int)wDamage, 65);
}

/* Light Screen: enemy SPC 60 → 120 → wDamage=65 */
TEST(GetDamageVarsForPlayerAttack, LightScreen_DoublesEnemySpc) {
    battle_reset();
    wBattleMon.spc      = 100;
    wBattleMon.level    = 100;
    wEnemyMon.spc       = 60;
    wPlayerMovePower    = 90;
    wPlayerMoveType     = TYPE_FIRE;
    wEnemyBattleStatus3 = (1 << BSTAT3_HAS_LIGHT_SCREEN);
    Battle_GetDamageVarsForPlayerAttack();
    EXPECT_EQ((int)wDamage, 65);
}

/* Crit: offense = party mon ATK, defense = enemy_raw_stat, level doubled.
 * wBattleMon.atk=50 is bypassed; party[0].atk=100 used.
 * wEnemyMon.def=200 bypassed; Blastoise (species=0x1C, dex=9) base DEF=100:
 *   raw_def = CalcStat(100, 0, 0, 50, 0) = 105
 * level=50 doubled → 100.
 * CalcDamage(100, 105, 80, 100): d=42*80*100/105/50=64 → wDamage=66 */
TEST(GetDamageVarsForPlayerAttack, Crit_BypassStats) {
    battle_reset();
    wCriticalHitOrOHKO    = 1;
    wPlayerMonNumber      = 0;
    wPartyMons[0].atk     = 100;
    wBattleMon.atk        = 50;   /* stage-modified stat, bypassed on crit */
    wBattleMon.level      = 50;
    wEnemyMon.species     = SPECIES_BLASTOISE;  /* Blastoise: base DEF=100 */
    wEnemyMon.dvs         = 0;
    wEnemyMon.level       = 50;
    wEnemyMon.def         = 200;  /* bypassed on crit */
    wPlayerMovePower      = 80;
    wPlayerMoveType       = TYPE_NORMAL;
    Battle_GetDamageVarsForPlayerAttack();
    EXPECT_EQ((int)wDamage, 66);
}

/* ====================================================================
 * Battle_GetDamageVarsForEnemyAttack
 *
 * Mirror of GetDamageVarsForPlayerAttack with enemy as attacker.
 * On crit: offense = enemy_raw_stat, defense = player party mon stat.
 * ==================================================================== */

TEST(GetDamageVarsForEnemyAttack, PowerZero_Returns0) {
    battle_reset();
    wEnemyMovePower = 0;
    int ret = Battle_GetDamageVarsForEnemyAttack();
    EXPECT_EQ(ret, 0);
    EXPECT_EQ((int)wDamage, 0);
}

/* Physical: enemy ATK vs player DEF.
 * atk=100, def=60, power=90, level=100 → wDamage=128 */
TEST(GetDamageVarsForEnemyAttack, Physical_Normal) {
    battle_reset();
    wEnemyMon.atk   = 100;
    wEnemyMon.level = 100;
    wBattleMon.def  = 60;
    wEnemyMovePower = 90;
    wEnemyMoveType  = TYPE_NORMAL;
    Battle_GetDamageVarsForEnemyAttack();
    EXPECT_EQ((int)wDamage, 128);
}

/* Special: enemy SPC vs player SPC → wDamage=128 */
TEST(GetDamageVarsForEnemyAttack, Special_Normal) {
    battle_reset();
    wEnemyMon.spc   = 100;
    wEnemyMon.level = 100;
    wBattleMon.spc  = 60;
    wEnemyMovePower = 90;
    wEnemyMoveType  = TYPE_FIRE;
    Battle_GetDamageVarsForEnemyAttack();
    EXPECT_EQ((int)wDamage, 128);
}

/* Player has Reflect: player DEF 60 → 120 → wDamage=65 */
TEST(GetDamageVarsForEnemyAttack, Reflect_DoublesPlayerDef) {
    battle_reset();
    wEnemyMon.atk        = 100;
    wEnemyMon.level      = 100;
    wBattleMon.def       = 60;
    wEnemyMovePower      = 90;
    wEnemyMoveType       = TYPE_NORMAL;
    wPlayerBattleStatus3 = (1 << BSTAT3_HAS_REFLECT);
    Battle_GetDamageVarsForEnemyAttack();
    EXPECT_EQ((int)wDamage, 65);
}

/* Crit: enemy offense = enemy_raw_stat(STAT_ATTACK), player defense from party.
 * wEnemyMon.atk=200 bypassed; Bulbasaur (dex=1) base ATK=49:
 *   raw_atk = CalcStat(49, 0, 0, 50, 0) = 54
 * wPartyMons[0].def=100 (player party), wBattleMon.def=10 bypassed.
 * level=50 doubled → 100.
 * CalcDamage(54, 100, 80, 100): d=42*80*54/100/50=36 → wDamage=38 */
TEST(GetDamageVarsForEnemyAttack, Crit_BypassStats) {
    battle_reset();
    wCriticalHitOrOHKO    = 1;
    wPlayerMonNumber      = 0;
    wEnemyMon.species     = SPECIES_BULBASAUR;  /* Bulbasaur: base ATK=49 */
    wEnemyMon.dvs         = 0;
    wEnemyMon.level       = 50;
    wEnemyMon.atk         = 200;  /* bypassed on crit */
    wPartyMons[0].def     = 100;
    wBattleMon.def        = 10;   /* bypassed on crit */
    wEnemyMovePower       = 80;
    wEnemyMoveType        = TYPE_NORMAL;
    Battle_GetDamageVarsForEnemyAttack();
    EXPECT_EQ((int)wDamage, 38);
}
