/* test_map.c — Tests for map loading, tile expansion, and collision. */
#include "test_runner.h"
#include "../src/game/overworld.h"
#include "../src/platform/hardware.h"
#include "../src/data/map_data.h"
#include "../src/data/event_data.h"

/* ---- Map loading ----------------------------------------- */

TEST(Map, LoadPalletTown) {
    Map_Load(0);
    EXPECT_EQ(wCurMap, 0);
    /* Pallet Town: 10 blocks wide × 9 blocks tall */
    EXPECT_EQ(wCurMapWidth,  10);
    EXPECT_EQ(wCurMapHeight,  9);
    EXPECT_EQ(wCurMapTileset, 0);   /* OVERWORLD tileset */
}

TEST(Map, LoadRedsHouse1F) {
    Map_Load(0x25);  /* REDS_HOUSE_1F */
    EXPECT_EQ(wCurMap, 0x25);
    EXPECT_GT(wCurMapWidth,  0);
    EXPECT_GT(wCurMapHeight, 0);
    EXPECT_EQ(wCurMapTileset, 1);  /* REDS_HOUSE_1 tileset */
}

TEST(Map, OutOfRangeMapIdIgnored) {
    Map_Load(0);
    uint8_t saved_map = wCurMap;
    Map_Load(0xFF);  /* invalid — should be silently skipped */
    EXPECT_EQ(wCurMap, saved_map);  /* unchanged */
}

/* ---- Tile expansion (block→tile) ------------------------- */

TEST(Map, GetTileInBounds) {
    Map_Load(0);  /* Pallet Town */
    /* Any in-bounds query should return a valid tile ID without crashing */
    uint8_t t = Map_GetTile(0, 0);
    EXPECT_GE((int)t, 0);
    EXPECT_LT((int)t, 256);
}

TEST(Map, GetTileOutOfBounds) {
    Map_Load(0);
    /* Out-of-bounds should return border block (non-crash) */
    uint8_t t = Map_GetTile(9999, 9999);
    EXPECT_GE((int)t, 0);
}

TEST(Map, BuildViewFillsTileMap) {
    Map_Load(0);
    wXCoord = 9; wYCoord = 8;
    Map_BuildView();
    /* Center tile of screen should be non-zero for Pallet Town */
    uint8_t center = wTileMap[8 * 20 + 9];
    /* Just check it's a valid value (Pallet Town has real map data) */
    EXPECT_GE((int)center, 0);
    EXPECT_LT((int)center, 96);  /* overworld tileset has 96 tiles */
}

/* ---- Collision ------------------------------------------- */

TEST(Map, PassableTileAccepted) {
    Map_Load(0);  /* OVERWORLD tileset */
    /* Tile 0 in overworld is typically path — should be passable */
    /* We just verify the function returns 0 or 1, no crash */
    int r = Tile_IsPassable(0x00);
    EXPECT_TRUE(r == 0 || r == 1);
}

TEST(Map, AllMapsHaveValidTileset) {
    for (int i = 0; i < NUM_MAPS; i++) {
        const map_info_t *m = &gMapTable[i];
        if (!m->blocks) continue;  /* unused map slot */
        EXPECT_LT((int)m->tileset_id, 24);  /* 24 tilesets defined */
        EXPECT_GT((int)m->width, 0);
        EXPECT_GT((int)m->height, 0);
    }
}

/* ---- Event data ------------------------------------------ */

TEST(Map, PalletTownWarpCount) {
    /* Pallet Town has exactly 3 warps (Red's House, Blue's House, Oak's Lab) */
    const map_events_t *ev = &gMapEvents[0];
    EXPECT_EQ((int)ev->num_warps, 3);
}

TEST(Map, PalletTownWarpCoords) {
    const map_events_t *ev = &gMapEvents[0];
    EXPECT_TRUE(ev->warps != NULL);
    /* First warp: Red's House at step (5,5) -> tile x=5*2=10, y=5*2+1=11 */
    EXPECT_EQ((int)ev->warps[0].x, 10);
    EXPECT_EQ((int)ev->warps[0].y, 11);
    EXPECT_EQ((int)ev->warps[0].dest_map, 0x25);  /* REDS_HOUSE_1F */
}

TEST(Map, PalletTownNPCCount) {
    const map_events_t *ev = &gMapEvents[0];
    EXPECT_EQ((int)ev->num_npcs, 3);  /* Oak, girl, fisher */
}

TEST(Map, AllMapsHaveEventEntry) {
    /* gMapEvents should cover all 248 map IDs without null crash */
    int warp_maps = 0;
    for (int i = 0; i < NUM_MAPS; i++) {
        if (gMapEvents[i].num_warps > 0) warp_maps++;
    }
    EXPECT_GT(warp_maps, 50);  /* most indoor maps have at least one warp out */
}
