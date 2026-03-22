/* anim.c — Animated background tile update.
 *
 * Mirrors UpdateMovingBgTiles from home/vcopy.asm.
 *
 * Two animations, both driven by hMovingBGTilesCounter1 (increments every
 * VBlank call to Anim_UpdateTiles):
 *
 *   Water tile ($14):
 *     Every 20 VBlanks, rotate each of the 16 GFX bytes one bit left or right
 *     (circular, mirrors rlca/rrca).  Direction = (counter2 & 4) ? left : right.
 *     After water, if TILEANIM_WATER: reset counter1 to 0.
 *     If TILEANIM_WATER_FLOWER: leave counter1 at 20 so next VBlank hits 21.
 *
 *   Flower tile ($03):
 *     Fires when counter1 reaches 21 (one VBlank after the water event).
 *     Replaces the tile GFX with one of 3 pre-built 16-byte frames.
 *     Frame = counter2 & 3: 0,1→frame1  2→frame2  3→frame3.
 *     Resets counter1 to 0.
 *
 * counter2 increments (mod 8) on every water event, cycling 0-7.
 * counter2 persists across map transitions (original behaviour).
 *
 * Called at 60 Hz (every VBlank) before the 30 Hz overworld gate in GameTick,
 * matching the original's VBlank ISR placement.
 */
#include "anim.h"
#include "../platform/display.h"
#include <stdint.h>

/* Tile indices in tile_gfx[] that the animation targets. */
#define ANIM_WATER_TILE   0x14
#define ANIM_FLOWER_TILE  0x03

/* Mirrors hTileAnimations / hMovingBGTilesCounter1 / wMovingBGTilesCounter2. */
static uint8_t hTileAnimations        = 0;
static uint8_t hMovingBGTilesCounter1 = 0;
static uint8_t wMovingBGTilesCounter2 = 0;

/* Flower animation frames — 3 × 16 bytes.
 * Decoded from gfx/tilesets/flower/flower{1,2,3}.png (8×8, 2-bit grayscale).
 * Mirrors FlowerTile1/FlowerTile2/FlowerTile3 INCBINs in home/vcopy.asm.
 *
 * rgbgfx maps PNG value 0 (black) → GB color 3 (darkest), so all bytes
 * are bitwise-inverted relative to a naive grayscale decode (~byte each). */
static const uint8_t kFlowerFrames[3][16] = {
    /* flower1 (~byte each — 2bpp grayscale: black=GB color 3, white=GB color 0) */
    { 0x81, 0x00, 0x00, 0x18, 0x00, 0x24, 0x85, 0x5A,
      0x1C, 0x42, 0x18, 0xA5, 0x00, 0x7E, 0x81, 0x18 },
    /* flower2 */
    { 0x81, 0x00, 0x00, 0x0C, 0x00, 0x12, 0x82, 0x2D,
      0x0E, 0xE1, 0x0C, 0x73, 0x00, 0x3E, 0x81, 0x18 },
    /* flower3 */
    { 0x81, 0x18, 0x00, 0x24, 0x04, 0x5A, 0x9D, 0x42,
      0x18, 0x24, 0x00, 0xDB, 0x00, 0x7E, 0x81, 0x18 },
};

void Anim_SetTileset(uint8_t anim_type) {
    hTileAnimations        = anim_type;
    hMovingBGTilesCounter1 = 0;
    /* wMovingBGTilesCounter2 intentionally NOT reset — persists across maps */
}

/* Circular rotate right one bit (mirrors rrca: bit0 → bit7). */
static inline uint8_t rrca8(uint8_t v) { return (v >> 1) | (uint8_t)(v << 7); }

/* Circular rotate left one bit (mirrors rlca: bit7 → bit0). */
static inline uint8_t rlca8(uint8_t v) { return (v << 1) | (v >> 7); }

void Anim_UpdateTiles(void) {
    if (!hTileAnimations) return;

    hMovingBGTilesCounter1++;
    if (hMovingBGTilesCounter1 < 20) return;

    /* --- Flower (counter1 == 21, TILEANIM_WATER_FLOWER only) --- */
    if (hMovingBGTilesCounter1 == 21) {
        hMovingBGTilesCounter1 = 0;
        uint8_t v = wMovingBGTilesCounter2 & 3;
        int f = (v < 2) ? 0 : (v == 2) ? 1 : 2;
        Display_LoadTile(ANIM_FLOWER_TILE, kFlowerFrames[f]);
        return;
    }

    /* --- Water (counter1 == 20) --- */
    wMovingBGTilesCounter2 = (wMovingBGTilesCounter2 + 1) & 7;

    uint8_t buf[16];
    Display_GetTile(ANIM_WATER_TILE, buf);
    if (wMovingBGTilesCounter2 & 4) {
        for (int i = 0; i < 16; i++) buf[i] = rlca8(buf[i]);
    } else {
        for (int i = 0; i < 16; i++) buf[i] = rrca8(buf[i]);
    }
    Display_LoadTile(ANIM_WATER_TILE, buf);

    /* TILEANIM_WATER:        reset counter — flower step never fires.
     * TILEANIM_WATER_FLOWER: leave counter at 20 — next VBlank hits 21. */
    if (hTileAnimations == TILEANIM_WATER) {
        hMovingBGTilesCounter1 = 0;
    }
}
