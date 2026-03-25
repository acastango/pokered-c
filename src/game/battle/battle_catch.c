/* battle_catch.c — Pokéball catch mechanic.
 *
 * Ports ItemUseBall catch calculation (engine/items/item_effects.asm:104).
 * Ghost / Marowak special cases and box-full checks are caller responsibility.
 * Animation is skipped.
 *
 * ALWAYS refer to pokered-master source before modifying.
 */

#include "battle_catch.h"
#include "../../platform/hardware.h"
#include "../constants.h"

/* ============================================================
 * Battle_CatchAttempt — ItemUseBall (item_effects.asm:177-416)
 *
 * Algorithm (from source comments + code):
 *
 * 1. Master Ball → always catch.
 *
 * 2. Draw Rand1.
 *    - Poké Ball:         accept any value [0, 255]
 *    - Great Ball:        re-draw if Rand1 > 200  → effective range [0, 200]
 *    - Ultra/Safari Ball: re-draw if Rand1 > 150  → effective range [0, 150]
 *
 * 3. Status bonus (subtract from Rand1):
 *    - Freeze/Sleep:           subtract 25
 *    - Burn/Paralysis/Poison:  subtract 12
 *    If subtraction underflows → catch.
 *
 * 4. W = floor( floor(MaxHP * 255 / BallFactor) / max(floor(HP/4), 1) )
 *    BallFactor: Great Ball = 8, all others = 12.
 *    X = min(W, 255).
 *
 * 5. If Rand1 - Status > CatchRate → fail (go to shake calc).
 *
 * 6. If W > 255 → catch.
 *
 * 7. Draw Rand2. If Rand2 > X → fail (go to shake calc).
 *    Otherwise → catch.
 *
 * Shake count (item_effects.asm:322-412) — only reached on fail:
 *
 * 8. Y = floor(CatchRate * 100 / BallFactor2)
 *    BallFactor2: Poké Ball=255, Great Ball=200, Ultra/Safari Ball=150.
 *
 * 9. Z_raw = floor(X * Y / 255).
 *
 * 10. Status2 bonus (added to Z_raw → Z):
 *     - Freeze/Sleep:          +10
 *     - Burn/Paralysis/Poison: +5
 *
 * 11. Shakes from Z:
 *     Z <  10 → 0 shakes
 *     Z <  30 → 1 shake
 *     Z <  70 → 2 shakes
 *     Z >= 70 → 3 shakes
 * ============================================================ */
catch_result_t Battle_CatchAttempt(uint8_t ball_id) {
    /* Master Ball always succeeds (item_effects.asm:194-195) */
    if (ball_id == ITEM_MASTER_BALL) return CATCH_RESULT_SUCCESS;

    /* ---- Rand1 draw with ball-specific ceiling (item_effects.asm:185-214) ---- */
    uint8_t rand1;
    for (;;) {
        rand1 = BattleRandom();
        if (ball_id == ITEM_POKE_BALL)  break;             /* any value OK */
        if (rand1 <= 200)               break;             /* Great Ball threshold */
        if (ball_id == ITEM_GREAT_BALL) continue;          /* re-draw if > 200 */
        if (rand1 <= 150)               break;             /* Ultra/Safari Ball */
        /* rand1 > 150 and not Great Ball → re-draw */
    }

    /* ---- Status ailment bonus (item_effects.asm:216-236) ---- */
    uint8_t status = wEnemyMon.status;
    if (status) {
        uint8_t sub;
        if (status & ((1 << 5) | 0x07)) /* FRZ (bit5) | SLP_MASK (bits 0-2) */
            sub = 25;
        else
            sub = 12;  /* BRN/PAR/PSN */
        if (sub > rand1) return CATCH_RESULT_SUCCESS;      /* underflow → catch */
        rand1 -= sub;
    }

    /* ---- W = floor( floor(MaxHP*255 / BallFactor) / max(floor(HP/4), 1) )
     *      (item_effects.asm:241-296) ---- */
    uint8_t ball_factor = (ball_id == ITEM_GREAT_BALL) ? 8 : 12;

    uint32_t num   = (uint32_t)wEnemyMon.max_hp * 255 / ball_factor;
    uint8_t  hp4   = (uint8_t)(wEnemyMon.hp >> 2);
    if (hp4 == 0) hp4 = 1;                                /* max(HP/4, 1) */
    uint32_t W     = num / hp4;

    uint8_t  X     = (W > 255) ? 255 : (uint8_t)W;      /* min(W, 255) */

    /* ---- CatchRate check (item_effects.asm:300-303) ---- */
    uint8_t catch_rate = wEnemyMon.catch_rate;
    if (rand1 > catch_rate) goto fail;                    /* Rand1-Status > CatchRate */

    /* ---- W overflow check (item_effects.asm:305-308) ---- */
    if (W > 255) return CATCH_RESULT_SUCCESS;

    /* ---- Rand2 check (item_effects.asm:310-316) ---- */
    {
        uint8_t rand2 = BattleRandom();
        if (rand2 > X) goto fail;
    }

    return CATCH_RESULT_SUCCESS;

fail:;
    /* ---- Shake calculation (item_effects.asm:322-412) ---- */

    /* Y = floor(CatchRate * 100 / BallFactor2) (item_effects.asm:325-355) */
    uint8_t ball_factor2;
    if      (ball_id == ITEM_POKE_BALL)  ball_factor2 = 255;
    else if (ball_id == ITEM_GREAT_BALL) ball_factor2 = 200;
    else                                  ball_factor2 = 150; /* Ultra/Safari */

    uint32_t Y = (uint32_t)catch_rate * 100 / ball_factor2;

    /* If Y > 255: 3 shakes (item_effects.asm:357-363; note: can't happen in practice) */
    if (Y > 255) return CATCH_RESULT_3_SHAKES;

    /* Z_raw = floor(X * Y / 255) (item_effects.asm:365-374) */
    uint32_t Z_raw = (uint32_t)X * (uint8_t)Y / 255;
    uint8_t  Z     = (Z_raw > 255) ? 255 : (uint8_t)Z_raw;

    /* Status2 bonus (item_effects.asm:376-392) */
    if (status) {
        uint8_t bonus;
        if (status & ((1 << 5) | 0x07)) /* FRZ | SLP */
            bonus = 10;
        else
            bonus = 5;  /* BRN/PAR/PSN */
        uint16_t z2 = (uint16_t)Z + bonus;
        Z = (z2 > 255) ? 255 : (uint8_t)z2;
    }

    /* Shake thresholds (item_effects.asm:395-412) */
    if (Z <  10) return CATCH_RESULT_0_SHAKES;
    if (Z <  30) return CATCH_RESULT_1_SHAKE;
    if (Z <  70) return CATCH_RESULT_2_SHAKES;
    return CATCH_RESULT_3_SHAKES;
}
