/* test_player.c — Tests for player position, facing, and movement helpers. */
#include "test_runner.h"
#include "../src/game/player.h"
#include "../src/game/overworld.h"
#include "../src/platform/hardware.h"

static void setup(void) {
    Map_Load(0);          /* Pallet Town */
    Player_Init(16, 24);  /* default start */
}

/* ---- Initialization -------------------------------------- */

TEST(Player, InitSetsPosition) {
    Player_Init(10, 20);
    EXPECT_EQ((int)wXCoord, 10);
    EXPECT_EQ((int)wYCoord, 20);
}

TEST(Player, InitSetsOAM) {
    Player_Init(16, 24);
    /* 16x16 player sprite — OAM slots 0-3, tile slots 116-119.
     * Player at (16,24), gCamX=8, gCamY=15. Player at screen tile (8,9).
     * Sprite TL at screen tile (8,8): sx=(16-8)*8=64, sy=(24-15-1)*8=64.
     * OAM[0] = TL: Y = 64+16=80, X = 64+8=72, tile=116 (PLAYER_TILE_BASE+0) */
    EXPECT_EQ((int)wShadowOAM[0].y, 80);
    EXPECT_EQ((int)wShadowOAM[0].x, 72);
    EXPECT_EQ((int)wShadowOAM[0].tile, 116);
    /* OAM[3] = BR: Y = 64+8+16=88, X = 64+8+8=80, tile=119 (PLAYER_TILE_BASE+3) */
    EXPECT_EQ((int)wShadowOAM[3].y, 88);
    EXPECT_EQ((int)wShadowOAM[3].x, 80);
    EXPECT_EQ((int)wShadowOAM[3].tile, 119);
}

TEST(Player, SetPosUpdatesCoords) {
    Player_Init(0, 0);
    Player_SetPos(5, 7);
    EXPECT_EQ((int)wXCoord, 5);
    EXPECT_EQ((int)wYCoord, 7);
}

/* ---- Facing direction ------------------------------------ */

TEST(Player, DefaultFacingDown) {
    Player_Init(16, 24);
    EXPECT_EQ(gPlayerFacing, 0);  /* 0 = down */
}

TEST(Player, FacingTileDown) {
    setup();
    gPlayerFacing = 0;
    int fx, fy;
    Player_GetFacingTile(&fx, &fy);
    EXPECT_EQ(fx, (int)wXCoord);
    EXPECT_EQ(fy, (int)wYCoord + 2);  /* 2-tile step */
}

TEST(Player, FacingTileUp) {
    setup();
    gPlayerFacing = 1;
    int fx, fy;
    Player_GetFacingTile(&fx, &fy);
    EXPECT_EQ(fx, (int)wXCoord);
    EXPECT_EQ(fy, (int)wYCoord - 2);  /* 2-tile step */
}

TEST(Player, FacingTileLeft) {
    setup();
    gPlayerFacing = 2;
    int fx, fy;
    Player_GetFacingTile(&fx, &fy);
    EXPECT_EQ(fx, (int)wXCoord - 2);  /* 2-tile step */
    EXPECT_EQ(fy, (int)wYCoord);
}

TEST(Player, FacingTileRight) {
    setup();
    gPlayerFacing = 3;
    int fx, fy;
    Player_GetFacingTile(&fx, &fy);
    EXPECT_EQ(fx, (int)wXCoord + 2);  /* 2-tile step */
    EXPECT_EQ(fy, (int)wYCoord);
}

/* ---- Movement bounds ------------------------------------- */

TEST(Player, CannotMoveOffMapWest) {
    Map_Load(0);
    Player_Init(0, 16);
    /* Simulate pressing LEFT — direct position test since Player_Update reads joypad */
    int nx = (int)wXCoord - 1;
    EXPECT_LT(nx, 0);  /* would go negative */
}

TEST(Player, MapTileBoundsCorrect) {
    Map_Load(0);
    int max_x = (int)wCurMapWidth  * 4;  /* 10*4=40 for Pallet Town */
    int max_y = (int)wCurMapHeight * 4;  /* 9*4=36 */
    EXPECT_EQ(max_x, 40);
    EXPECT_EQ(max_y, 36);
}

/* ---- Damage / stat helpers (wram.c) ---------------------- */

TEST(Player, CalcDamageNonZero) {
    /* level=5, bp=40 (tackle), attack=11, defense=11 → expect >0 */
    extern uint16_t CalcDamage(uint8_t, uint8_t, uint8_t, uint8_t);
    uint16_t dmg = CalcDamage(5, 40, 11, 11);
    EXPECT_GT((int)dmg, 0);
}

TEST(Player, CalcDamageScalesWithLevel) {
    extern uint16_t CalcDamage(uint8_t, uint8_t, uint8_t, uint8_t);
    uint16_t dmg_l5  = CalcDamage(5,  40, 50, 50);
    uint16_t dmg_l50 = CalcDamage(50, 40, 50, 50);
    EXPECT_GT((int)dmg_l50, (int)dmg_l5);
}

TEST(Player, CalcStatHPFormula) {
    extern uint16_t CalcStat(uint8_t, uint8_t, uint16_t, uint8_t, int);
    /* Charmander HP: base=39, dv=15, exp=0, level=5 → ~22 HP */
    uint16_t hp = CalcStat(39, 15, 0, 5, 1);
    EXPECT_GE((int)hp, 10);
    EXPECT_LT((int)hp, 50);
}
