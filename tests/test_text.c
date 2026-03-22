/* test_text.c — Tests for font encoding, tile mapping, and text box state. */
#include "test_runner.h"
#include "../src/data/font_data.h"
#include "../src/game/text.h"
#include "../src/platform/hardware.h"
#include "../src/game/constants.h"

/* ---- Font_CharToTile mapping ----------------------------- */

TEST(Text, UppercaseAMapsToFontBase) {
    /* 'A' = pokered char $80 → tile 128 (FONT_TILE_BASE + 0) */
    EXPECT_EQ(Font_CharToTile(0x80), 128);
}

TEST(Text, UppercaseZMappedCorrectly) {
    /* 'Z' = $99 → tile 128 + 0x19 = 128 + 25 = 153 */
    EXPECT_EQ(Font_CharToTile(0x99), 153);
}

TEST(Text, LowercaseAMappedCorrectly) {
    /* 'a' = $A0 → tile 128 + 0x20 = 160 */
    EXPECT_EQ(Font_CharToTile(0xA0), 160);
}

TEST(Text, LastFontChar) {
    /* char $FF → tile 128 + 127 = 255 */
    EXPECT_EQ(Font_CharToTile(0xFF), 255);
}

TEST(Text, SpaceMapsToBlank) {
    /* $7F (space) → BLANK_TILE_SLOT = 126 */
    EXPECT_EQ(Font_CharToTile(0x7F), BLANK_TILE_SLOT);
}

TEST(Text, BoxTopLeftCorner) {
    /* $79 (┌) → BOX_TILE_BASE + 0 = 120 */
    EXPECT_EQ(Font_CharToTile(0x79), 120);
}

TEST(Text, BoxHorizontalLine) {
    /* $7A (─) → BOX_TILE_BASE + 1 = 121 */
    EXPECT_EQ(Font_CharToTile(0x7A), 121);
}

TEST(Text, BoxTopRightCorner) {
    /* $7B (┐) → BOX_TILE_BASE + 2 = 122 */
    EXPECT_EQ(Font_CharToTile(0x7B), 122);
}

TEST(Text, BoxVerticalLine) {
    /* $7C (│) → BOX_TILE_BASE + 3 = 123 */
    EXPECT_EQ(Font_CharToTile(0x7C), 123);
}

TEST(Text, BoxBottomLeftCorner) {
    /* $7D (└) → BOX_TILE_BASE + 4 = 124 */
    EXPECT_EQ(Font_CharToTile(0x7D), 124);
}

TEST(Text, BoxBottomRightCorner) {
    /* $7E (┘) → BOX_TILE_BASE + 5 = 125 */
    EXPECT_EQ(Font_CharToTile(0x7E), 125);
}

TEST(Text, ControlCharMapsToBlank) {
    /* Control chars ($00-$78 except $79-$7F) → BLANK_TILE_SLOT */
    EXPECT_EQ(Font_CharToTile(0x50), BLANK_TILE_SLOT);  /* terminator '@' */
    EXPECT_EQ(Font_CharToTile(0x4E), BLANK_TILE_SLOT);  /* <LINE> */
    EXPECT_EQ(Font_CharToTile(0x00), BLANK_TILE_SLOT);  /* NULL */
}

/* ---- Font tile data integrity ---------------------------- */

TEST(Text, FontTilesNonZero) {
    /* The 'A' glyph ($80, tile index 0) should have non-zero pixel data */
    int nonzero = 0;
    for (int i = 0; i < 16; i++)
        if (gFontTiles[0][i]) nonzero = 1;
    EXPECT_TRUE(nonzero);
}

TEST(Text, BoxTilesNonZero) {
    /* Box-drawing ┌ (tile 0) and ─ (tile 1) should have real pixel data */
    int nonzero_corner = 0, nonzero_hline = 0;
    for (int i = 0; i < 16; i++) {
        if (gBoxTiles[0][i]) nonzero_corner = 1;
        if (gBoxTiles[1][i]) nonzero_hline  = 1;
    }
    EXPECT_TRUE(nonzero_corner);
    EXPECT_TRUE(nonzero_hline);
}

TEST(Text, DistinctFontGlyphs) {
    /* 'A' and 'B' should have different pixel data */
    int same = 1;
    for (int i = 0; i < 16; i++)
        if (gFontTiles[0][i] != gFontTiles[1][i]) { same = 0; break; }
    EXPECT_FALSE(same);
}

/* ---- Text box state machine ------------------------------ */

TEST(Text, InitiallyClosed) {
    EXPECT_FALSE(Text_IsOpen());
}

TEST(Text, ShowBoxOpensDialog) {
    static const uint8_t hello[] = {0x87,0x84,0x8B,0x8B,0x8E,0x50};
    Text_ShowBox(hello);
    EXPECT_TRUE(Text_IsOpen());
    Text_Close();
}

TEST(Text, CloseEndsDialog) {
    static const uint8_t hi[] = {0x87,0x88,0x50};
    Text_ShowBox(hi);
    Text_Close();
    EXPECT_FALSE(Text_IsOpen());
}

TEST(Text, ShowBoxWritesToTileMap) {
    static const uint8_t msg[] = {0x80,0x50};  /* "A" then terminate */
    Text_ShowBox(msg);
    /* Row 12 col 0 should be ┌ = tile 120 (top border moved to row 12) */
    EXPECT_EQ((int)wTileMap[12*20 + 0], 120);
    /* Row 14 col 1 should be 'A' = tile 128 (first text row is now 14) */
    EXPECT_EQ((int)wTileMap[14*20 + 1], 128);
    Text_Close();
}

TEST(Text, TileSlotBoundsOK) {
    /* All possible pokered char codes should map to valid tile slots 0-255 */
    for (int c = 0; c <= 0xFF; c++) {
        int slot = Font_CharToTile((unsigned char)c);
        EXPECT_GE(slot, 0);
        EXPECT_LT(slot, 256);
    }
}
