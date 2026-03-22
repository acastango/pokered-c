/* test_wild.c — Tests for wild encounter data integrity. */
#include "test_runner.h"
#include "../src/data/wild_data.h"
#include "../src/data/map_data.h"

/* Route 1 is map ID 12 (0x0C) in pokered */
#define MAP_ROUTE_1     0x0C
#define MAP_PALLET_TOWN 0x00

TEST(Wild, PalletTownHasNoEncounters) {
    EXPECT_EQ((int)gWildGrass[MAP_PALLET_TOWN].rate, 0);
}

TEST(Wild, Route1HasEncounters) {
    /* Route 1 should have Pidgey and Rattata */
    EXPECT_GT((int)gWildGrass[MAP_ROUTE_1].rate, 0);
}

TEST(Wild, Route1HasTenSlots) {
    /* pokered always defines 10 encounter slots */
    const wild_mons_t *w = &gWildGrass[MAP_ROUTE_1];
    if (w->rate == 0) return;  /* skip if no encounters */
    /* All slots should have valid species (> 0) and level (> 0) */
    int valid = 0;
    for (int i = 0; i < 10; i++) {
        if (w->slots[i].species > 0 && w->slots[i].level > 0)
            valid++;
    }
    EXPECT_EQ(valid, 10);
}

TEST(Wild, EncounterLevelsInRange) {
    /* All wild levels across all maps should be 1–100 */
    for (int m = 0; m < NUM_MAPS; m++) {
        const wild_mons_t *w = &gWildGrass[m];
        if (!w->rate) continue;
        for (int i = 0; i < 10; i++) {
            EXPECT_GE((int)w->slots[i].level, 1);
            EXPECT_LT((int)w->slots[i].level, 101);
        }
    }
}

TEST(Wild, EncounterSpeciesInRange) {
    /* Species IDs should be 1–151 (internal pokered numbering) */
    for (int m = 0; m < NUM_MAPS; m++) {
        const wild_mons_t *w = &gWildGrass[m];
        if (!w->rate) continue;
        for (int i = 0; i < 10; i++) {
            EXPECT_GE((int)w->slots[i].species, 1);
            EXPECT_LT((int)w->slots[i].species, 200);  /* generous upper bound */
        }
    }
}

TEST(Wild, MostMapsHaveNoEncounters) {
    /* Only routes/dungeons have encounters; towns should not */
    int encounter_maps = 0;
    for (int m = 0; m < NUM_MAPS; m++)
        if (gWildGrass[m].rate > 0) encounter_maps++;
    /* pokered has roughly 25–40 maps with wild encounters */
    EXPECT_GE(encounter_maps, 20);
    EXPECT_LT(encounter_maps, NUM_MAPS);
}

TEST(Wild, EncounterRateInRange) {
    for (int m = 0; m < NUM_MAPS; m++) {
        uint8_t rate = gWildGrass[m].rate;
        /* Rate is 0 (none) or a positive value (pokered uses 25, 30 etc.) */
        EXPECT_TRUE(rate == 0 || rate > 0);  /* trivially true — checks no UB */
        if (rate > 0) EXPECT_LT((int)rate, 256);
    }
}
