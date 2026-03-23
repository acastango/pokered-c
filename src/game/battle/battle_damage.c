/* battle_damage.c — Gen 1 battle damage calculation functions.
 *
 * Exact ports of engine/battle/core.asm (bank $0F):
 *   Battle_CalcDamage      ← CalculateDamage      (core.asm:4299)
 *   Battle_CriticalHitTest ← CriticalHitTest      (core.asm:4478)
 *   Battle_RandomizeDamage ← RandomizeDamage       (core.asm:5420)
 *   Battle_MoveHitTest     ← MoveHitTest           (core.asm:5228)
 *   Battle_CalcHitChance   ← CalcHitChance         (core.asm:5348)
 *
 * ALWAYS refer to the pokered-master ASM before modifying anything here.
 * Every quirk and bug is intentional.
 */
#include "battle.h"
#include "../../data/base_stats.h"
#include "../../data/type_chart.h"

/* StatModifierRatios — data/battle/stat_modifiers.asm
 * {numerator, denominator} for stages 1–13 (index = stage - 1).
 * Stage 7 (normal) = 1/1.  Stage 1 = 0.25.  Stage 13 = 4.00. */
const uint8_t kBattleStatModRatios[13][2] = {
    {25, 100}, {28, 100}, {33, 100}, {40, 100}, {50, 100}, {66, 100},
    { 1,   1}, {15,  10}, { 2,   1}, {25,  10}, { 3,   1}, {35,  10}, { 4, 1}
};

/* HighCriticalMoves — data/battle/critical_hit_moves.asm
 * Moves that get 8× crit rate (applied as ×4 on top of the normal ×2).
 * Terminated by 0xFF. */
static const uint8_t kHighCritMoves[] = {
    MOVE_KARATE_CHOP,   /* 0x02 */
    MOVE_RAZOR_LEAF,    /* 0x4B */
    MOVE_CRABHAMMER,    /* 0x98 */
    MOVE_SLASH,         /* 0xA3 */
    0xFF
};

/* ============================================================
 * Battle_CalcDamage — CalculateDamage (core.asm:4299)
 *
 * Inputs match the GB registers set by GetDamageVarsForPlayerAttack/Enemy:
 *   attack   (b) — attacker's offensive stat (possibly scaled ÷4)
 *   defense  (c) — defender's defensive stat (possibly scaled ÷4)
 *   power    (d) — base power of the move
 *   level    (e) — attacker's level (doubled by caller for critical hits)
 *
 * Reads hWhoseTurn + wPlayerMoveEffect/wEnemyMoveEffect.
 * Accumulates into wDamage (callers zero it first via GetDamageVarsFor*).
 * ============================================================ */
int Battle_CalcDamage(uint8_t attack, uint8_t defense, uint8_t power, uint8_t level) {
    uint8_t effect = hWhoseTurn ? wEnemyMoveEffect : wPlayerMoveEffect;

    /* EXPLODE_EFFECT: srl c (halve defense), with minimum of 1 (core.asm:4316) */
    if (effect == EFFECT_EXPLODE) {
        defense >>= 1;
        if (defense == 0) defense = 1;
    }

    /* TWO_TO_FIVE_ATTACKS_EFFECT / EFFECT_1E: skip power=0 check (core.asm:4322) */
    int skip_power_check = (effect == EFFECT_TWO_TO_FIVE_ATTACKS ||
                            effect == EFFECT_1E);

    if (!skip_power_check) {
        /* OHKO: handled by JumpMoveEffect — not yet ported (core.asm:4328) */
        if (effect == EFFECT_OHKO) {
            return 0; /* TODO: JumpMoveEffect for OHKO */
        }
        if (power == 0) return 0;   /* no damage for status moves */
    }

    /* Formula: ((level×2 / 5) + 2) × power × attack / defense / 50
     *
     * The GB Multiply/Divide routines operate on a shared 4-byte HRAM buffer
     * (hProduct overlaps hDividend overlaps hMultiplicand), so each Multiply
     * result feeds directly into the next Divide without an explicit copy.
     * Faithfully replicated here with uint32_t. (core.asm:4337-4385) */
    uint32_t d = (uint32_t)level * 2 / 5 + 2;
    d = d * (uint32_t)power;
    d = d * (uint32_t)attack;
    d = d / (uint32_t)(defense ? defense : 1); /* guard against /0 like the ASM does */
    d = d / 50;

    /* Accumulate into wDamage and cap at MAX_NEUTRAL_DAMAGE - MIN_NEUTRAL_DAMAGE.
     * (core.asm:4387–4451)
     * wDamage is zeroed by GetDamageVarsFor* before the first call;
     * subsequent calls accumulate for multi-hit moves. */
    uint32_t total = (uint32_t)wDamage + d;
    if (total > (uint32_t)(MAX_NEUTRAL_DAMAGE - MIN_NEUTRAL_DAMAGE)) {
        total = MAX_NEUTRAL_DAMAGE - MIN_NEUTRAL_DAMAGE; /* cap at 997 */
    }

    /* Add MIN_NEUTRAL_DAMAGE (core.asm:4453-4460): final range [2, 999] */
    total += MIN_NEUTRAL_DAMAGE;
    wDamage = (uint16_t)total;
    return 1;
}

/* ============================================================
 * Battle_CriticalHitTest — CriticalHitTest (core.asm:4478)
 *
 * Sets wCriticalHitOrOHKO = 1 on a critical hit, 0 otherwise.
 *
 * Gen 1 crit formula (all integer, 8-bit):
 *   b = base_speed >> 1
 *   if Focus Energy (GETTING_PUMPED): b >>= 1  ← BUG: should be b <<= 1
 *   else: b <<= 1, cap 255
 *   if high-crit move: b <<= 2, cap 255  (×4 on top of the ×2 above)
 *   else: b >>= 1
 *   random = rlc(BattleRandom(), 3)   (left-rotate by 3 bits)
 *   crit if random < b
 * ============================================================ */
void Battle_CriticalHitTest(void) {
    wCriticalHitOrOHKO = 0;

    /* Determine which mon's base speed to use (core.asm:4481-4487) */
    uint8_t species = hWhoseTurn ? wEnemyMon.species : wBattleMon.species;
    /* GetMonHeader equivalent: convert internal ID → dex for gBaseStats lookup
     * (core.asm:4488-4490 calls GetMonHeader which does IndexToPokedex internally) */
    uint8_t dex = gSpeciesToDex[species];
    if (dex == 0 || dex > 151) return; /* guard: invalid species */
    uint8_t base_speed = gBaseStats[dex].spd;

    uint8_t b = base_speed >> 1; /* srl b → b = base_speed / 2 (core.asm:4491) */

    /* Read move power and move number depending on whose turn (core.asm:4492-4504) */
    uint8_t move_power  = hWhoseTurn ? wEnemyMovePower  : wPlayerMovePower;
    uint8_t move_num    = hWhoseTurn ? wEnemyMoveNum    : wPlayerMoveNum;
    uint8_t bstat2      = hWhoseTurn ? wEnemyBattleStatus2 : wPlayerBattleStatus2;

    if (move_power == 0) return; /* status moves never crit (core.asm:4501-4502) */

    /* Focus Energy check (core.asm:4505-4514).
     * GEN 1 BUG: Focus Energy uses `srl b` (÷2) instead of `sla b` (×2),
     * so it actually DECREASES crit rate by 75% instead of doubling it. */
    if (bstat2 & (1 << BSTAT2_GETTING_PUMPED)) {
        b >>= 1; /* srl b — the bug */
    } else {
        /* sla b: double b, cap at 0xFF (core.asm:4509-4511) */
        b = (b & 0x80) ? 0xFF : (uint8_t)(b << 1);
    }

    /* Scan HighCriticalMoves table (core.asm:4516-4533) */
    int is_high_crit = 0;
    for (int i = 0; kHighCritMoves[i] != 0xFF; i++) {
        if (kHighCritMoves[i] == move_num) { is_high_crit = 1; break; }
    }

    if (is_high_crit) {
        /* sla b (×2), cap; sla b again (×4 total), cap (core.asm:4526-4532) */
        b = (b & 0x80) ? 0xFF : (uint8_t)(b << 1);
        b = (b & 0x80) ? 0xFF : (uint8_t)(b << 1);
    } else {
        /* srl b: halve for normal moves (core.asm:4523) → ends up at base_speed/2 */
        b >>= 1;
    }

    /* BattleRandom, then rlc a three times (left-rotate 3 bits) (core.asm:4534-4537) */
    uint8_t r = BattleRandom();
    r = (uint8_t)((r << 3) | (r >> 5)); /* rlc × 3 */

    /* Crit if random < b (core.asm:4538-4542) */
    if (r < b) {
        wCriticalHitOrOHKO = 1;
    }
}

/* ============================================================
 * Battle_RandomizeDamage — RandomizeDamage (core.asm:5420)
 *
 * Skips randomization if wDamage ≤ 1.
 * Otherwise multiplies wDamage by a random value in [217, 255] then divides
 * by 255, giving ~85%–100% of the original damage.
 *
 * The loop uses `rrca` (right-rotate by 1) on the BattleRandom result to
 * spread the distribution, then rejects values below 217 (= 85%×255).
 * ============================================================ */
void Battle_RandomizeDamage(void) {
    uint16_t dmg = wDamage;

    /* Return if damage is 0 or 1 (core.asm:5421-5427):
     * ASM checks high byte != 0 OR low byte >= 2. */
    if (dmg <= 1) return;

    /* Loop: BattleRandom + rrca until result >= 217 (core.asm:5436-5441).
     * 85 percent = 85 * $FF / 100 = 216 (integer), so threshold = 217. */
    uint8_t r;
    do {
        r = BattleRandom();
        r = (uint8_t)((r >> 1) | (r << 7)); /* rrca */
    } while (r < 217);

    /* damage × r / 255 (core.asm:5442-5453) */
    uint32_t product = (uint32_t)dmg * r;
    wDamage = (uint16_t)(product / 255);
}

/* ============================================================
 * Battle_CalcHitChance — CalcHitChance (core.asm:5348)
 *
 * Scales the current move's accuracy by two stat modifier ratios:
 *   1. Attacker's accuracy stage
 *   2. Defender's evasion stage (reflected: effective stage = 14 - evasion_mod)
 *
 * Each scaling: new_val = old_val × num / den  (StatModifierRatios entry)
 * Result: floored at 1, capped at 255.
 * Stored into wPlayerMoveAccuracy or wEnemyMoveAccuracy.
 *
 * The two Multiply/Divide calls chain because in pokered HRAM,
 * hProduct and hMultiplicand share the same address — the product of the
 * first multiply feeds directly as the multiplicand for the second.
 * We reproduce this with a running uint32_t accumulator.
 * ============================================================ */
void Battle_CalcHitChance(void) {
    uint8_t *acc_ptr;
    uint8_t  acc_mod; /* attacker's accuracy stage (1-13) */
    uint8_t  eva_mod; /* defender's evasion stage (1-13) */

    if (hWhoseTurn == 0) {
        /* Player's turn: player accuracy, enemy evasion (core.asm:5349-5355) */
        acc_ptr = &wPlayerMoveAccuracy;
        acc_mod = wPlayerMonStatMods[MOD_ACCURACY];
        eva_mod = wEnemyMonStatMods[MOD_EVASION];
    } else {
        /* Enemy's turn (core.asm:5357-5362) */
        acc_ptr = &wEnemyMoveAccuracy;
        acc_mod = wEnemyMonStatMods[MOD_ACCURACY];
        eva_mod = wPlayerMonStatMods[MOD_EVASION];
    }

    /* Reflect evasion: effective stage = 14 - evasion_mod (core.asm:5364-5366).
     * This converts "higher evasion = harder to hit" into a ratio < 1 when
     * the target has raised evasion, matching how AccuracyMod works. */
    uint8_t eff_eva = (uint8_t)(14 - eva_mod);

    /* Two-iteration loop (core.asm:5375-5407):
     * Iteration 1: scale by accuracy ratio (acc_mod)
     * Iteration 2: scale by reflected-evasion ratio (eff_eva)
     * Both use StatModifierRatios[stage-1] = {num, den}. */
    uint32_t val = *acc_ptr;

    uint8_t stages[2] = {acc_mod, eff_eva};
    for (int i = 0; i < 2; i++) {
        uint8_t stage = stages[i];
        if (stage < 1)  stage = 1;   /* clamp to valid range */
        if (stage > 13) stage = 13;

        uint8_t num = kBattleStatModRatios[stage - 1][0];
        uint8_t den = kBattleStatModRatios[stage - 1][1];

        val = val * num / den;

        /* Floor at 1 (core.asm:5399-5403): if result == 0, set to 1 */
        if (val == 0) val = 1;
    }

    /* Cap at 255 and store (core.asm:5408-5416) */
    if (val > 255) val = 255;
    *acc_ptr = (uint8_t)val;
}

/* ============================================================
 * Battle_MoveHitTest — MoveHitTest (core.asm:5228)
 *
 * Determines whether the current move hits.  On miss, sets wDamage=0,
 * wMoveMissed=1, and clears the USING_TRAPPING_MOVE flag.
 *
 * Checks (in ASM order):
 *  1. Dream Eater: target must be asleep (core.asm:5240-5246)
 *  2. Swift: always hits, return immediately (core.asm:5247-5250)
 *  3. CheckTargetSubstitute bug (core.asm:5251-5258): substitute never blocks
 *     drain/dream-eater because CheckTargetSubstitute overwrites a with 0/1
 *  4. INVULNERABLE (Fly/Dig): always miss (core.asm:5260-5261)
 *  5. Mist: blocks stat-lowering effects in ranges [0x12,0x19] and [0x3A,0x41]
 *     (core.asm:5265-5311)
 *  6. X-Accuracy: bypass accuracy roll (core.asm:5288-5311)
 *  7. CalcHitChance + BattleRandom accuracy roll (core.asm:5312-5326)
 * ============================================================ */
void Battle_MoveHitTest(void) {
    int player_turn     = (hWhoseTurn == 0);
    uint8_t move_effect = player_turn ? wPlayerMoveEffect : wEnemyMoveEffect;
    uint8_t tgt_status  = player_turn ? wEnemyMon.status  : wBattleMon.status;
    uint8_t tgt_bstat1  = player_turn ? wEnemyBattleStatus1 : wPlayerBattleStatus1;
    uint8_t tgt_bstat2  = player_turn ? wEnemyBattleStatus2 : wPlayerBattleStatus2;
    uint8_t att_bstat2  = player_turn ? wPlayerBattleStatus2 : wEnemyBattleStatus2;

    /* 1. Dream Eater: target must be asleep (core.asm:5240-5246) */
    if (move_effect == EFFECT_DREAM_EATER) {
        if (!(tgt_status & STATUS_SLP_MASK)) goto move_missed;
    }

    /* 2. Swift always hits (core.asm:5247-5250) */
    if (move_effect == EFFECT_SWIFT) return;

    /* 3. CheckTargetSubstitute (core.asm:5251-5258).
     *
     * GEN 1 BUG: CheckTargetSubstitute stores 0 (no sub) or 1 (has sub) in A,
     * overwriting the move effect.  The subsequent `cp DRAIN_HP_EFFECT` and
     * `cp DREAM_EATER_EFFECT` always fail (a is 0 or 1, not 0x03 or 0x08),
     * so substitute never prevents HP-draining moves.  Faithfully reproduced. */
    {
        int has_sub = (tgt_bstat2 >> BSTAT2_HAS_SUBSTITUTE) & 1;
        if (has_sub) {
            uint8_t bug_a = (uint8_t)has_sub; /* 0 or 1, NOT the move effect */
            if (bug_a == EFFECT_DRAIN_HP)    goto move_missed; /* never true */
            if (bug_a == EFFECT_DREAM_EATER) goto move_missed; /* never true */
            /* Always falls through: substitute doesn't block drain/dream-eater */
        }
    }

    /* 4. INVULNERABLE bit set (target using Fly/Dig) — always miss (core.asm:5260) */
    if (tgt_bstat1 & (1 << BSTAT1_INVULNERABLE)) goto move_missed;

    /* 5. Mist check (core.asm:5265-5311).
     * Stat-lowering effects blocked by Mist: [0x12,0x19] and [0x3A,0x41]. */
    {
        int mist_blocked = (move_effect >= EFFECT_ATTACK_DOWN1 &&
                            move_effect <= EFFECT_HAZE) ||
                           (move_effect >= EFFECT_ATTACK_DOWN2 &&
                            move_effect <= EFFECT_REFLECT);
        if (mist_blocked && (tgt_bstat2 & (1 << BSTAT2_PROTECTED_BY_MIST))) {
            goto move_missed;
        }
    }

    /* 6. X-Accuracy: bypass accuracy roll entirely (core.asm:5288-5311) */
    if (att_bstat2 & (1 << BSTAT2_USING_X_ACCURACY)) return;

    /* 7. Scale accuracy then roll (core.asm:5312-5326).
     * CalcHitChance writes back to wPlayerMoveAccuracy/wEnemyMoveAccuracy. */
    Battle_CalcHitChance();
    {
        uint8_t acc  = player_turn ? wPlayerMoveAccuracy : wEnemyMoveAccuracy;
        uint8_t roll = BattleRandom();
        if (roll >= acc) goto move_missed;   /* miss if roll >= scaled accuracy */
    }
    return;

move_missed:
    wDamage     = 0;
    wMoveMissed = 1;
    /* Clear USING_TRAPPING_MOVE on the attacker (core.asm:5339-5345) */
    if (player_turn) {
        wPlayerBattleStatus1 &= (uint8_t)~(1 << BSTAT1_USING_TRAPPING);
    } else {
        wEnemyBattleStatus1  &= (uint8_t)~(1 << BSTAT1_USING_TRAPPING);
    }
}

/* ============================================================
 * Phase 3 helpers
 * ============================================================ */

/* enemy_raw_stat — mirrors GetEnemyMonStat for non-link battle (core.asm:4279).
 *
 * Recalculates one of the enemy's stats from species base stats, DVs, and level
 * with zero stat exp (b=0 in the ASM).  Used for critical hit stat bypass.
 *
 * stat_idx: STAT_ATTACK=1, STAT_DEFENSE=2, STAT_SPEED=3, STAT_SPECIAL_STAT=4.
 *
 * DV byte layout in battle_mon_t.dvs (little-endian uint16):
 *   low byte  (byte 0, addr MON_DVS+0) = (ATK_DV << 4) | DEF_DV
 *   high byte (byte 1, addr MON_DVS+1) = (SPD_DV << 4) | SPC_DV */
static uint16_t enemy_raw_stat(uint8_t stat_idx) {
    uint8_t dex = gSpeciesToDex[wEnemyMon.species]; /* internal ID → dex for gBaseStats */
    if (dex == 0 || dex >= 152) return 1;
    const base_stats_t *bs = &gBaseStats[dex];
    uint16_t dvs = wEnemyMon.dvs;
    uint8_t base, dv;
    switch (stat_idx) {
        default: return 1;
        case STAT_ATTACK:
            base = bs->atk;  dv = (uint8_t)((dvs >> 4) & 0xF);  break;
        case STAT_DEFENSE:
            base = bs->def;  dv = (uint8_t)(dvs & 0xF);          break;
        case STAT_SPEED:
            base = bs->spd;  dv = (uint8_t)((dvs >> 12) & 0xF); break;
        case STAT_SPECIAL_STAT:
            base = bs->spc;  dv = (uint8_t)((dvs >> 8) & 0xF);  break;
    }
    return CalcStat(base, dv, 0, wEnemyMon.level, 0);
}

/* scale_for_damage — mirrors the .scaleStats block in GetDamageVarsFor*Attack.
 *
 * If either high byte of the 16-bit offense/defense is non-zero (stat >= 256),
 * both are right-shifted by 2 (>> 2 = divide by 4).  This keeps the values
 * within 8 bits so they fit in the uint8_t inputs of Battle_CalcDamage.
 * If offense becomes 0 after scaling, it is bumped to 1 (prevents division by 0
 * in the original although here offense is the attacker stat, not the divisor;
 * the ASM bumps it to 1 when offensive stat is 0 to match the original behavior). */
static void scale_for_damage(uint16_t *offense, uint16_t *defense) {
    if ((*offense >> 8) | (*defense >> 8)) {
        *defense >>= 2;
        *offense >>= 2;
        if (*offense == 0) *offense = 1;
    }
}

/* ============================================================
 * Battle_CalculateModifiedStats — CalculateModifiedStats (core.asm:6365)
 *
 * Loops stats c=0..NUM_STATS-1 (ATK/DEF/SPD/SPC).
 * For each: new = unmodified * ratio_num / ratio_den, capped 1..999.
 * Writes back into wBattleMon (wCalculateWhoseStats==0) or wEnemyMon (nonzero).
 * ============================================================ */
void Battle_CalculateModifiedStats(void) {
    battle_mon_t *mon;
    uint8_t *mods;
    uint16_t unmod[NUM_STATS];

    if (wCalculateWhoseStats == 0) {
        mon = &wBattleMon;
        mods = wPlayerMonStatMods;
        unmod[0] = wPlayerMonUnmodifiedAttack;
        unmod[1] = wPlayerMonUnmodifiedDefense;
        unmod[2] = wPlayerMonUnmodifiedSpeed;
        unmod[3] = wPlayerMonUnmodifiedSpecial;
    } else {
        mon = &wEnemyMon;
        mods = wEnemyMonStatMods;
        unmod[0] = wEnemyMonUnmodifiedAttack;
        unmod[1] = wEnemyMonUnmodifiedDefense;
        unmod[2] = wEnemyMonUnmodifiedSpeed;
        unmod[3] = wEnemyMonUnmodifiedSpecial;
    }

    uint16_t *stat_ptrs[NUM_STATS] = { &mon->atk, &mon->def, &mon->spd, &mon->spc };

    for (int c = 0; c < NUM_STATS; c++) {
        uint8_t stage = mods[c];
        if (stage < 1)  stage = 1;
        if (stage > 13) stage = 13;

        uint8_t  num = kBattleStatModRatios[stage - 1][0];
        uint8_t  den = kBattleStatModRatios[stage - 1][1];
        uint32_t val = (uint32_t)unmod[c] * num / den;

        if (val > MAX_NEUTRAL_DAMAGE) val = MAX_NEUTRAL_DAMAGE;   /* cap at 999 */
        if (val == 0) val = 1;                                     /* floor at 1 */

        *stat_ptrs[c] = (uint16_t)val;
    }
}

/* ============================================================
 * Battle_ApplyBurnAndParalysisPenalties — (core.asm:6278)
 *
 * hWhoseTurn = 1: modifies player wBattleMon stats (player side switching in).
 * hWhoseTurn = 0: modifies enemy  wEnemyMon  stats (enemy  side switching in).
 * ============================================================ */
void Battle_ApplyBurnAndParalysisPenalties(void) {
    battle_mon_t *mon = (hWhoseTurn != 0) ? &wBattleMon : &wEnemyMon;

    /* QuarterSpeedDueToParalysis (core.asm:6283) */
    if (mon->status & STATUS_PAR) {
        uint16_t spd = (uint16_t)(mon->spd >> 2);
        if (spd == 0) spd = 1;
        mon->spd = spd;
    }

    /* HalveAttackDueToBurn (core.asm:6326) */
    if (mon->status & STATUS_BRN) {
        uint16_t atk = (uint16_t)(mon->atk >> 1);
        if (atk == 0) atk = 1;
        mon->atk = atk;
    }
}

/* ============================================================
 * Battle_GetDamageVarsForPlayerAttack — GetDamageVarsForPlayerAttack (core.asm:4030)
 *
 * Sets up and calls Battle_CalcDamage for the player's attack.
 * Returns 1 if damage was calculated, 0 if move power is 0.
 * ============================================================ */
int Battle_GetDamageVarsForPlayerAttack(void) {
    wDamage = 0;

    uint8_t power = wPlayerMovePower;
    if (power == 0) return 0;                        /* status move — early exit */

    uint8_t move_type = wPlayerMoveType;
    uint16_t offense, defense;

    if (move_type < TYPE_SPECIAL_THRESHOLD) {
        /* Physical attack: ATK vs DEF (core.asm:4044-4072) */
        defense = wEnemyMon.def;
        if (wEnemyBattleStatus3 & (1 << BSTAT3_HAS_REFLECT))
            defense <<= 1;                           /* Reflect: double enemy DEF */

        if (wCriticalHitOrOHKO) {
            /* Crit bypass stages: party mon ATK, recalculate enemy base DEF */
            offense = wPartyMons[wPlayerMonNumber].atk;
            defense = enemy_raw_stat(STAT_DEFENSE);
        } else {
            offense = wBattleMon.atk;
        }
    } else {
        /* Special attack: SPC vs SPC (core.asm:4073-4103) */
        defense = wEnemyMon.spc;
        if (wEnemyBattleStatus3 & (1 << BSTAT3_HAS_LIGHT_SCREEN))
            defense <<= 1;                           /* Light Screen: double enemy SPC */

        if (wCriticalHitOrOHKO) {
            offense = wPartyMons[wPlayerMonNumber].spc;
            defense = enemy_raw_stat(STAT_SPECIAL_STAT);
        } else {
            offense = wBattleMon.spc;
        }
    }

    scale_for_damage(&offense, &defense);            /* >> 2 if either >= 256 (core.asm:4107) */

    uint8_t level = wBattleMon.level;
    if (wCriticalHitOrOHKO) level = (uint8_t)(level << 1); /* double level on crit (core.asm:4136) */

    return Battle_CalcDamage((uint8_t)offense, (uint8_t)defense, power, level);
}

/* ============================================================
 * Battle_GetDamageVarsForEnemyAttack — GetDamageVarsForEnemyAttack (core.asm:4143)
 *
 * Mirror of GetDamageVarsForPlayerAttack with attacker = enemy, defender = player.
 * Returns 1 if damage was calculated, 0 if move power is 0.
 * ============================================================ */
int Battle_GetDamageVarsForEnemyAttack(void) {
    wDamage = 0;

    uint8_t power = wEnemyMovePower;
    if (power == 0) return 0;

    uint8_t move_type = wEnemyMoveType;
    uint16_t offense, defense;

    if (move_type < TYPE_SPECIAL_THRESHOLD) {
        /* Physical attack: enemy ATK vs player DEF (core.asm:4157-4185) */
        defense = wBattleMon.def;
        if (wPlayerBattleStatus3 & (1 << BSTAT3_HAS_REFLECT))
            defense <<= 1;

        if (wCriticalHitOrOHKO) {
            defense = wPartyMons[wPlayerMonNumber].def;
            offense = enemy_raw_stat(STAT_ATTACK);
        } else {
            offense = wEnemyMon.atk;
        }
    } else {
        /* Special attack: enemy SPC vs player SPC (core.asm:4186-4216) */
        defense = wBattleMon.spc;
        if (wPlayerBattleStatus3 & (1 << BSTAT3_HAS_LIGHT_SCREEN))
            defense <<= 1;

        if (wCriticalHitOrOHKO) {
            defense = wPartyMons[wPlayerMonNumber].spc;
            offense = enemy_raw_stat(STAT_SPECIAL_STAT);
        } else {
            offense = wEnemyMon.spc;
        }
    }

    scale_for_damage(&offense, &defense);

    uint8_t level = wEnemyMon.level;
    if (wCriticalHitOrOHKO) level = (uint8_t)(level << 1);

    return Battle_CalcDamage((uint8_t)offense, (uint8_t)defense, power, level);
}

/* ============================================================
 * Battle_AdjustDamageForMoveType — AdjustDamageForMoveType (core.asm:5075)
 *
 * Two-pass operation on wDamage:
 *  Pass 1 — STAB: move type vs. attacker's types.
 *  Pass 2 — Type chart: scan for matching (atk=move_type, def=defender_type) entries.
 *
 * Exact ASM replication notes:
 *  - STAB: hl=wDamage, bc=floor(hl/2) via (srl b; rr c), hl+=bc.
 *    Stored back into wDamage.  BIT_STAB_DAMAGE set in wDamageMultipliers.
 *  - Type scan: for each entry where atk==move_type, checks def==def_type1 first;
 *    if matched, applies and moves to next entry (def_type2 not checked for that
 *    entry).  Else if def==def_type2, applies.  Mono-type: one application max.
 *  - wDamageMultipliers per match: (existing STAB bit) + eff.
 *    Overwrites low 7 bits with eff each match.
 *  - Zero damage after any application → wMoveMissed = 1.
 * ============================================================ */
void Battle_AdjustDamageForMoveType(void) {
    uint8_t atk_type1, atk_type2, def_type1, def_type2, move_type;

    /* Load attacker/defender types and move type based on hWhoseTurn.
     * Player turn (core.asm:5077-5086): attacker=wBattleMon, defender=wEnemyMon.
     * Enemy turn  (core.asm:5091-5100): attacker=wEnemyMon,  defender=wBattleMon. */
    if (hWhoseTurn == 0) {
        atk_type1 = wBattleMon.type1;
        atk_type2 = wBattleMon.type2;
        def_type1 = wEnemyMon.type1;
        def_type2 = wEnemyMon.type2;
        move_type = wPlayerMoveType;
    } else {
        atk_type1 = wEnemyMon.type1;
        atk_type2 = wEnemyMon.type2;
        def_type1 = wBattleMon.type1;
        def_type2 = wBattleMon.type2;
        move_type = wEnemyMoveType;
    }
    wMoveType = move_type;  /* wMoveType = wPlayerMoveType/wEnemyMoveType (core.asm:5086/5100) */

    /* Pass 1 — STAB (core.asm:5102-5126).
     * Check move type against both attacker types (cp b / cp c).
     * If either matches: hl=wDamage, bc=floor(hl>>1), hl+=bc → floor(1.5×damage). */
    if (move_type == atk_type1 || move_type == atk_type2) {
        wDamage += (wDamage >> 1);                      /* floor(1.5 * wDamage) */
        wDamageMultipliers |= (uint8_t)(1 << BIT_STAB_DAMAGE);
    }

    /* Pass 2 — Type chart scan (core.asm:5127-5185).
     * Replicates the table loop: for each entry {atk,def,eff} where atk==move_type,
     * apply eff if def matches defender type1 (and skip type2 for that entry)
     * or def matches defender type2.
     * TypeEffectiveness() returns 10 for normal (no table entry) — not applied. */
    static const int kApplyThreshold = 10; /* entries with eff==10 are absent from table */

    /* Defender type 1 */
    uint8_t eff1 = TypeEffectiveness(move_type, def_type1);
    if (eff1 != kApplyThreshold) {
        uint8_t stab_bit = wDamageMultipliers & (uint8_t)(1 << BIT_STAB_DAMAGE);
        wDamageMultipliers = stab_bit + eff1;                /* (core.asm:5147-5153) */
        wDamage = (uint16_t)((uint32_t)wDamage * eff1 / 10);/* multiply then /10   */
        if (wDamage == 0) wMoveMissed = 1;                   /* (core.asm:5171-5176) */
    }

    /* Defender type 2 — only when different from type 1.
     * Mirrors ASM: for a mono-type mon, the single matching entry is consumed by
     * the type1 check (cp d → jr z, .matchingPairFound skips cp e). */
    if (def_type2 != def_type1) {
        uint8_t eff2 = TypeEffectiveness(move_type, def_type2);
        if (eff2 != kApplyThreshold) {
            uint8_t stab_bit = wDamageMultipliers & (uint8_t)(1 << BIT_STAB_DAMAGE);
            wDamageMultipliers = stab_bit + eff2;
            wDamage = (uint16_t)((uint32_t)wDamage * eff2 / 10);
            if (wDamage == 0) wMoveMissed = 1;
        }
    }
}
