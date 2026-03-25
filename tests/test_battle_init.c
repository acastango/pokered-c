/* test_battle_init.c — Unit tests for battle_init.c trainer functions.
 *
 * Covers:
 *   Pokemon_WriteMovesForLevel  (4 tests)
 *   Battle_ReadTrainer          (6 tests)
 *   Battle_StartTrainer         (6 tests)
 */
#include "test_runner.h"
#include "../src/game/battle/battle_init.h"
#include "../src/game/pokemon.h"
#include "../src/platform/hardware.h"
#include "../src/game/constants.h"

/* ================================================================
 * Reset helper
 * ================================================================ */
static void init_reset(void) {
    extern void WRAMClear(void);
    WRAMClear();
    wIsInBattle  = 1;
    wPartyCount  = 1;
    wPartyMons[0].base.species  = SPECIES_BULBASAUR;
    wPartyMons[0].base.hp       = 45;
    wPartyMons[0].level         = 5;
    wPartyMons[0].max_hp        = 45;
    wPartyMons[0].base.moves[0] = 1;  /* POUND */
    wPartyMons[0].base.pp[0]    = 35;
    wPlayerMonNumber = 0;
}

/* ================================================================
 * Pokemon_WriteMovesForLevel
 * ================================================================ */

/* Level 7 — Rattata's first level-up table entry.
 * WriteMovesForLevel alone (no start_moves pre-fill) should give 1 move at lv7. */
TEST(WriteMovesForLevel, level7_populates_one_move) {
    uint8_t moves[4] = {0};
    uint8_t pp[4]    = {0};
    Pokemon_WriteMovesForLevel(moves, pp, SPECIES_RATTATA, 7);
    /* Level-7 entry should be in slot 0 */
    EXPECT_TRUE(moves[0] != 0);
    EXPECT_TRUE(pp[0] > 0);
    /* Slots 1-3 should remain empty (no higher entries yet) */
    EXPECT_EQ(moves[1], 0);
    EXPECT_EQ(moves[2], 0);
    EXPECT_EQ(moves[3], 0);
}

/* High level — more moves accumulated */
TEST(WriteMovesForLevel, higher_level_more_moves) {
    uint8_t moves_lv1[4] = {0}; uint8_t pp_lv1[4] = {0};
    uint8_t moves_lv20[4] = {0}; uint8_t pp_lv20[4] = {0};
    Pokemon_WriteMovesForLevel(moves_lv1,  pp_lv1,  SPECIES_RATTATA, 1);
    Pokemon_WriteMovesForLevel(moves_lv20, pp_lv20, SPECIES_RATTATA, 20);
    /* lv20 should have equal or more non-zero moves than lv1 */
    int cnt1 = 0, cnt20 = 0;
    for (int i = 0; i < 4; i++) {
        if (moves_lv1[i])  cnt1++;
        if (moves_lv20[i]) cnt20++;
    }
    EXPECT_TRUE(cnt20 >= cnt1);
}

/* PP should be non-zero for any non-zero move */
TEST(WriteMovesForLevel, pp_populated) {
    uint8_t moves[4] = {0};
    uint8_t pp[4]    = {0};
    Pokemon_WriteMovesForLevel(moves, pp, SPECIES_RATTATA, 20);
    for (int i = 0; i < 4; i++) {
        if (moves[i]) {
            EXPECT_TRUE(pp[i] > 0);
        }
    }
}

/* Species 0 should be a no-op */
TEST(WriteMovesForLevel, species_zero_noop) {
    uint8_t moves[4] = {0};
    uint8_t pp[4]    = {0};
    Pokemon_WriteMovesForLevel(moves, pp, 0, 50);
    int any = 0;
    for (int i = 0; i < 4; i++) if (moves[i]) any = 1;
    EXPECT_TRUE(!any);
}

/* ================================================================
 * Battle_ReadTrainer
 * ================================================================ */

/* Brock (class 34, trainer_no 1): format B, Geodude lv12 + Onix lv14 */
TEST(ReadTrainer, brock_party_count) {
    init_reset();
    Battle_ReadTrainer(34, 1);
    EXPECT_EQ(wEnemyPartyCount, 2);
}

TEST(ReadTrainer, brock_geodude_level) {
    init_reset();
    Battle_ReadTrainer(34, 1);
    EXPECT_EQ(wEnemyMons[0].base.species, SPECIES_GEODUDE);
    EXPECT_EQ(wEnemyMons[0].level, 12);
}

TEST(ReadTrainer, brock_onix_level) {
    init_reset();
    Battle_ReadTrainer(34, 1);
    EXPECT_EQ(wEnemyMons[1].base.species, SPECIES_ONIX);
    EXPECT_EQ(wEnemyMons[1].level, 14);
}

/* Youngster (class 1, trainer_no 1): format A, lv11, Rattata + Ekans */
TEST(ReadTrainer, youngster1_format_a) {
    init_reset();
    Battle_ReadTrainer(1, 1);
    EXPECT_EQ(wEnemyPartyCount, 2);
    EXPECT_EQ(wEnemyMons[0].base.species, SPECIES_RATTATA);
    EXPECT_EQ(wEnemyMons[0].level, 11);
    EXPECT_EQ(wEnemyMons[1].base.species, SPECIES_EKANS);
    EXPECT_EQ(wEnemyMons[1].level, 11);
}

/* Youngster trainer_no 2: lv14, Spearow only */
TEST(ReadTrainer, youngster2_trainer_no) {
    init_reset();
    Battle_ReadTrainer(1, 2);
    EXPECT_EQ(wEnemyPartyCount, 1);
    EXPECT_EQ(wEnemyMons[0].base.species, SPECIES_SPEAROW);
    EXPECT_EQ(wEnemyMons[0].level, 14);
}

/* Trainer party mons should have HP == max_hp (full health) */
TEST(ReadTrainer, mons_start_at_full_hp) {
    init_reset();
    Battle_ReadTrainer(35, 1);   /* Misty: Staryu lv18, Starmie lv21 */
    EXPECT_TRUE(wEnemyMons[0].base.hp > 0);
    EXPECT_EQ(wEnemyMons[0].base.hp, wEnemyMons[0].max_hp);
    EXPECT_TRUE(wEnemyMons[1].base.hp > 0);
    EXPECT_EQ(wEnemyMons[1].base.hp, wEnemyMons[1].max_hp);
}

/* ================================================================
 * Battle_StartTrainer
 * ================================================================ */

/* wIsInBattle must be 2 for trainer battles */
TEST(StartTrainer, wIsInBattle_is_2) {
    init_reset();
    Battle_StartTrainer(34, 1);  /* Brock */
    EXPECT_EQ(wIsInBattle, 2);
}

/* wAICount must be 0xFF (trainer AI marker, core.asm:6686) */
TEST(StartTrainer, wAICount_is_ff) {
    init_reset();
    Battle_StartTrainer(34, 1);
    EXPECT_EQ(wAICount, 0xFF);
}

/* First alive enemy mon loaded into wEnemyMon after StartTrainer */
TEST(StartTrainer, first_enemy_mon_loaded) {
    init_reset();
    Battle_StartTrainer(34, 1);  /* Brock: slot 0 = Geodude lv12 */
    EXPECT_EQ(wEnemyMon.species, SPECIES_GEODUDE);
    EXPECT_EQ(wEnemyMon.level, 12);
    EXPECT_TRUE(wEnemyMon.hp > 0);
}

/* wBattleMon loaded from player's wPartyMons[0] */
TEST(StartTrainer, player_mon_loaded) {
    init_reset();
    Battle_StartTrainer(34, 1);
    EXPECT_EQ(wBattleMon.species, SPECIES_BULBASAUR);
    EXPECT_EQ(wBattleMon.hp, 45);
}

/* wEnemyMonPartyPos should reflect the slot EnemySendOut found (slot 0) */
TEST(StartTrainer, enemy_party_pos_set) {
    init_reset();
    Battle_StartTrainer(34, 1);
    EXPECT_EQ(wEnemyMonPartyPos, 0);
}

/* wEnemyPartyCount set correctly for Misty */
TEST(StartTrainer, misty_enemy_party_count) {
    init_reset();
    Battle_StartTrainer(35, 1);  /* Misty: 2 mons */
    EXPECT_EQ(wEnemyPartyCount, 2);
    EXPECT_EQ(wEnemyMon.species, SPECIES_STARYU);
}

/* ================================================================
 * LoneMoves (special_moves.asm)
 * ================================================================ */

/* Brock (class 34) + wLoneAttackNo=1 → Onix (slot 1) gets BIDE in moves[2] */
TEST(ReadTrainer, lonemoves_brock_onix_bide) {
    init_reset();
    wLoneAttackNo = 1;
    Battle_ReadTrainer(34, 1);
    EXPECT_EQ(wEnemyMons[1].base.moves[2], MOVE_BIDE);
}

/* Misty (class 35) + wLoneAttackNo=2 → Starmie (slot 1) gets BUBBLEBEAM in moves[2] */
TEST(ReadTrainer, lonemoves_misty_starmie_bubblebeam) {
    init_reset();
    wLoneAttackNo = 2;
    Battle_ReadTrainer(35, 1);
    EXPECT_EQ(wEnemyMons[1].base.moves[2], MOVE_BUBBLEBEAM);
}

/* Format A trainer (Youngster, class 1) must NOT get LoneMoves even if wLoneAttackNo is set */
TEST(ReadTrainer, lonemoves_format_a_not_applied) {
    init_reset();
    wLoneAttackNo = 1;
    Battle_ReadTrainer(1, 1);
    /* Youngster has 2 mons; moves[2] of slot 1 must NOT be BIDE */
    EXPECT_TRUE(wEnemyMons[1].base.moves[2] != MOVE_BIDE);
}

/* wLoneAttackNo=0 with format B trainer → no lone move written */
TEST(ReadTrainer, lonemoves_zero_not_applied) {
    init_reset();
    wLoneAttackNo = 0;
    Battle_ReadTrainer(34, 1);  /* Brock */
    /* With no wLoneAttackNo, moves[2] should NOT be BIDE */
    EXPECT_TRUE(wEnemyMons[1].base.moves[2] != MOVE_BIDE);
}

/* ================================================================
 * TeamMoves (special_moves.asm) — Elite 4
 * ================================================================ */

/* Lorelei (class 44) + wLoneAttackNo=0 → Lapras (slot 4) gets BLIZZARD in moves[2] */
TEST(ReadTrainer, teammoves_lorelei_blizzard) {
    init_reset();
    wLoneAttackNo = 0;
    Battle_ReadTrainer(44, 1);
    EXPECT_TRUE(wEnemyPartyCount >= 5);
    EXPECT_EQ(wEnemyMons[4].base.moves[2], MOVE_BLIZZARD);
}

/* Bruno (class 33) + wLoneAttackNo=0 → Machamp (slot 4) gets FISSURE in moves[2] */
TEST(ReadTrainer, teammoves_bruno_fissure) {
    init_reset();
    wLoneAttackNo = 0;
    Battle_ReadTrainer(33, 1);
    EXPECT_TRUE(wEnemyPartyCount >= 5);
    EXPECT_EQ(wEnemyMons[4].base.moves[2], MOVE_FISSURE);
}

/* ================================================================
 * RIVAL3 special case (special_moves.asm)
 * ================================================================ */

/* RIVAL3 (class 43) starter=Bulbasaur → Pidgeot gets SKY_ATTACK, slot 5 gets MEGA_DRAIN */
TEST(ReadTrainer, rival3_bulbasaur_starter) {
    init_reset();
    wLoneAttackNo  = 0;
    wRivalStarter  = STARTER3;   /* Bulbasaur */
    Battle_ReadTrainer(43, 1);
    EXPECT_TRUE(wEnemyPartyCount >= 6);
    EXPECT_EQ(wEnemyMons[0].base.moves[2], MOVE_SKY_ATTACK);
    EXPECT_EQ(wEnemyMons[5].base.moves[2], MOVE_MEGA_DRAIN);
}

/* RIVAL3 starter=Charmander → slot 5 gets FIRE_BLAST */
TEST(ReadTrainer, rival3_charmander_starter) {
    init_reset();
    wLoneAttackNo  = 0;
    wRivalStarter  = STARTER1;   /* Charmander */
    Battle_ReadTrainer(43, 1);
    EXPECT_TRUE(wEnemyPartyCount >= 6);
    EXPECT_EQ(wEnemyMons[0].base.moves[2], MOVE_SKY_ATTACK);
    EXPECT_EQ(wEnemyMons[5].base.moves[2], MOVE_FIRE_BLAST);
}
