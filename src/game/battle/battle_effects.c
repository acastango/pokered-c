/* battle_effects.c — Gen 1 move effects engine.
 *
 * Port of engine/battle/effects.asm + engine/battle/move_effects/*.asm.
 * ALWAYS refer to the original pokered-master ASM before modifying anything here.
 *
 * Phase 4 scope: pure battle-state logic only.
 * UI calls (PrintText, DrawHPBar, PlayAnimation, DelayFrames) are omitted.
 * All behaviour, thresholds, and quirks are faithful to the GB source.
 */
#include "battle_effects.h"
#include "battle.h"
#include "../../platform/hardware.h"
#include "../constants.h"
#include "../types.h"

/* ---- BattleRandom is already in wram.c / hardware.h ---- */
extern uint8_t BattleRandom(void);

/* ---- Percent thresholds  (N * 255 / 100 + 1, matching ASM `N percent + 1`) ---- */
/* BattleRandom() < threshold  →  effect triggers (approx N%).                      */
#define PCT10   26   /* 10 percent + 1 */
#define PCT20   52   /* 20 percent + 1 */
#define PCT25   64   /* 25 percent + 1 */
#define PCT30   77   /* 30 percent + 1 */
#define PCT33   85   /* 33 percent + 1 */
#define PCT40  103   /* 40 percent + 1 */

/* ---- BIT helpers (mirror ASM set/res [hl]) ---- */
#define SET_BIT(v, b)  ((v) |=  (1u << (b)))
#define RES_BIT(v, b)  ((v) &= ~(1u << (b)))
#define TST_BIT(v, b)  (((v) >> (b)) & 1u)

/* ============================================================
 * Internal helpers
 * ============================================================ */

/* CheckTargetSubstitute — port of CheckTargetSubstitute (effects.asm:1436).
 * Returns nonzero if the TARGET (opponent of attacker) has a substitute up.
 * hWhoseTurn=0 (player attacking) → target is enemy. */
static int CheckTargetSubstitute(void) {
    if (hWhoseTurn == 0)
        return TST_BIT(wEnemyBattleStatus2, BSTAT2_HAS_SUBSTITUTE);
    else
        return TST_BIT(wPlayerBattleStatus2, BSTAT2_HAS_SUBSTITUTE);
}

/* ClearHyperBeam — port of ClearHyperBeam (effects.asm:1181).
 * Clears the NEEDS_TO_RECHARGE flag on the TARGET's battle status 2.
 * (Target = opponent of the current attacker.) */
static void ClearHyperBeam(void) {
    if (hWhoseTurn == 0)
        RES_BIT(wEnemyBattleStatus2, BSTAT2_NEEDS_TO_RECHARGE);
    else
        RES_BIT(wPlayerBattleStatus2, BSTAT2_NEEDS_TO_RECHARGE);
}

/* QuarterSpeed16 — right-shift a uint16_t by 2, min 1.
 * Mirrors the two-pass srl/rr pattern used for paralysis speed penalty. */
static uint16_t QuarterSpeed16(uint16_t v) {
    v >>= 2;
    return v ? v : 1;
}

/* HalveVal16 — right-shift a uint16_t by 1, min 1. */
static uint16_t HalveVal16(uint16_t v) {
    v >>= 1;
    return v ? v : 1;
}

/* ---- BCD helpers for PayDay ---- */
static void AddBCD3(uint8_t *dst, const uint8_t *src) {
    /* 3-byte BCD add.  dst[2] = LSB, dst[0] = MSB (big-endian significance).
     * Uses DAA-equivalent carry propagation. */
    int carry = 0;
    for (int i = 2; i >= 0; i--) {
        int lo = (dst[i] & 0xF) + (src[i] & 0xF) + carry;
        carry = lo >= 10 ? 1 : 0;
        if (lo >= 10) lo -= 10;
        int hi = (dst[i] >> 4) + (src[i] >> 4) + carry;
        carry = hi >= 10 ? 1 : 0;
        if (hi >= 10) hi -= 10;
        dst[i] = (uint8_t)((hi << 4) | lo);
    }
    if (carry) {
        /* Overflow: cap all bytes at 0x99 (matching ASM fill-with-$99 path) */
        dst[0] = dst[1] = dst[2] = 0x99;
    }
}

/* ============================================================
 * Public helpers (declared in header)
 * ============================================================ */

/* Battle_QuarterSpeedDueToParalysis — core.asm:6283 */
void Battle_QuarterSpeedDueToParalysis(void) {
    if (hWhoseTurn == 0) {
        /* Player's turn → apply PAR penalty to enemy speed */
        if (!(wEnemyMon.status & STATUS_PAR)) return;
        wEnemyMon.spd = QuarterSpeed16(wEnemyMon.spd);
    } else {
        /* Enemy's turn → apply PAR penalty to player speed */
        if (!(wBattleMon.status & STATUS_PAR)) return;
        wBattleMon.spd = QuarterSpeed16(wBattleMon.spd);
    }
}

/* Battle_HalveAttackDueToBurn — core.asm:6326 */
void Battle_HalveAttackDueToBurn(void) {
    if (hWhoseTurn == 0) {
        /* Player's turn → apply BRN penalty to enemy attack */
        if (!(wEnemyMon.status & STATUS_BRN)) return;
        wEnemyMon.atk = HalveVal16(wEnemyMon.atk);
    } else {
        /* Enemy's turn → apply BRN penalty to player attack */
        if (!(wBattleMon.status & STATUS_BRN)) return;
        wBattleMon.atk = HalveVal16(wBattleMon.atk);
    }
}

/* ============================================================
 * Effect handlers (static, dispatched from Battle_JumpMoveEffect)
 * ============================================================ */

/* ---- SleepEffect (effects.asm:26) ----------------------------------------
 * Handles EFFECT_SLEEP (0x20) and the unused EFFECT_01 (0x01).
 * Sets wMoveMissed implicitly via MoveHitTest if accuracy check fails.
 * Clears NEEDS_TO_RECHARGE on the target (bypasses recharge if target was
 * recharging — known Gen 1 behaviour). */
static void Effect_Sleep(void) {
    uint8_t *target_status;
    uint8_t *target_bstat2;
    if (hWhoseTurn == 0) {
        target_status = &wEnemyMon.status;
        target_bstat2 = &wEnemyBattleStatus2;
    } else {
        target_status = &wBattleMon.status;
        target_bstat2 = &wPlayerBattleStatus2;
    }

    /* Clear recharge flag (target no longer needs to recharge). */
    int was_recharging = TST_BIT(*target_bstat2, BSTAT2_NEEDS_TO_RECHARGE);
    RES_BIT(*target_bstat2, BSTAT2_NEEDS_TO_RECHARGE);

    if (!was_recharging) {
        /* Normal path: check if already asleep or statused. */
        if (IS_ASLEEP(*target_status)) return;   /* already asleep */
        if (*target_status != 0)       return;   /* already statused */
        /* Accuracy test */
        Battle_MoveHitTest();
        if (wMoveMissed) return;
    }
    /* Set sleep counter to random 1-7 (matching ASM `and SLP_MASK; jr z, retry`) */
    uint8_t counter;
    do { counter = BattleRandom() & STATUS_SLP_MASK; } while (counter == 0);
    *target_status = counter;
}

/* ---- CheckDefrost (effects.asm:312) --------------------------------------
 * If target is frozen and attacker's move is Fire-type, thaw the target.
 * Returns nonzero if defrost occurred (caller should skip further status). */
static int CheckDefrost(uint8_t target_status) {
    if (!(target_status & STATUS_FRZ)) return 0;
    /* Check if the attacker's move is Fire-type */
    uint8_t move_type = (hWhoseTurn == 0) ? wPlayerMoveType : wEnemyMoveType;
    if (move_type != TYPE_FIRE) return 0;
    /* Defrost */
    if (hWhoseTurn == 0) {
        wEnemyMon.status = 0;
    } else {
        wBattleMon.status = 0;
    }
    return 1;
}

/* ---- PoisonEffect (effects.asm:78) ---------------------------------------
 * Handles EFFECT_POISON (0x42), EFFECT_POISON_SIDE1 (0x02),
 * EFFECT_POISON_SIDE2 (0x21).  Toxic (wPlayerMoveNum==MOVE_TOXIC) sets
 * BSTAT3_BADLY_POISONED in addition. */
static void Effect_Poison(void) {
    uint8_t effect = (hWhoseTurn == 0) ? wPlayerMoveEffect : wEnemyMoveEffect;
    uint8_t *target_status;
    uint8_t *target_type1;
    uint8_t *target_type2;
    uint8_t *target_bstat3;
    uint8_t *target_toxic_ctr;
    if (hWhoseTurn == 0) {
        target_status    = &wEnemyMon.status;
        target_type1     = &wEnemyMon.type1;
        target_type2     = &wEnemyMon.type2;
        target_bstat3    = &wEnemyBattleStatus3;
        target_toxic_ctr = &wEnemyToxicCounter;
    } else {
        target_status    = &wBattleMon.status;
        target_type1     = &wBattleMon.type1;
        target_type2     = &wBattleMon.type2;
        target_bstat3    = &wPlayerBattleStatus3;
        target_toxic_ctr = &wPlayerToxicCounter;
    }

    /* Substitute blocks Poison/Poison_side effects */
    if (CheckTargetSubstitute()) {
        if (effect == EFFECT_POISON) goto noEffect;
        return;
    }

    /* Already statused? */
    if (*target_status != 0) {
        if (effect == EFFECT_POISON) goto noEffect;
        return;
    }
    /* Can't poison a Poison-type */
    if (*target_type1 == TYPE_POISON || *target_type2 == TYPE_POISON) {
        if (effect == EFFECT_POISON) goto noEffect;
        return;
    }

    /* Side effect: probability check */
    if (effect == EFFECT_POISON_SIDE1) {
        if (BattleRandom() >= PCT20) return;
    } else if (effect == EFFECT_POISON_SIDE2) {
        if (BattleRandom() >= PCT40) return;
    } else {
        /* EFFECT_POISON (0x42): always hits; MoveHitTest was already run */
    }

    /* Apply poison */
    *target_status |= STATUS_PSN;

    /* Check for Toxic (move ID == MOVE_TOXIC) */
    uint8_t move_num = (hWhoseTurn == 0) ? wPlayerMoveNum : wEnemyMoveNum;
    if (move_num == MOVE_TOXIC) {
        SET_BIT(*target_bstat3, BSTAT3_BADLY_POISONED);
        *target_toxic_ctr = 0;
    }
    return;

noEffect:
    /* EFFECT_POISON with no effect */
    return;
}

/* ---- FreezeBurnParalyzeEffect (effects.asm:194) --------------------------
 * Handles BURN_SIDE1/2, FREEZE_SIDE1/2, PARALYZE_SIDE1/2.
 * Side1 = 10% chance, Side2 = 30% chance.
 * Blocks if: target already statused, move-type == target-type (e.g. can't
 * paralyze Normal with Body Slam), or substitute is up.
 * CheckDefrost: fire moves thaw frozen targets. */
static void Effect_FreezeBurnParalyze(void) {
    uint8_t effect  = (hWhoseTurn == 0) ? wPlayerMoveEffect : wEnemyMoveEffect;
    uint8_t mv_type = (hWhoseTurn == 0) ? wPlayerMoveType   : wEnemyMoveType;
    uint8_t *target_status;
    uint8_t *target_type1;
    uint8_t *target_type2;
    if (hWhoseTurn == 0) {
        target_status = &wEnemyMon.status;
        target_type1  = &wEnemyMon.type1;
        target_type2  = &wEnemyMon.type2;
    } else {
        target_status = &wBattleMon.status;
        target_type1  = &wBattleMon.type1;
        target_type2  = &wBattleMon.type2;
    }

    /* Substitute blocks */
    if (CheckTargetSubstitute()) return;

    /* Target already statused? */
    if (*target_status != 0) {
        CheckDefrost(*target_status);
        return;
    }

    /* Move-type vs target-type immunity */
    if (*target_type1 == mv_type || *target_type2 == mv_type) return;

    /* Map Side2 effects to Side1 base for comparison, determine chance */
    uint8_t base_effect = effect;
    uint8_t threshold   = PCT10;
    if (effect >= EFFECT_BURN_SIDE2 && effect <= EFFECT_PARALYZE_SIDE2) {
        /* Side2 effects: 30% chance; map to Side1 base */
        threshold   = PCT30;
        base_effect = effect - (EFFECT_BURN_SIDE2 - EFFECT_BURN_SIDE);
    }
    /* ASSERT: PARALYZE_SIDE2 - PARALYZE_SIDE1 == BURN_SIDE2 - BURN_SIDE1 (== 0x1E) */

    if (BattleRandom() >= threshold) return;

    /* Apply the status */
    if (base_effect == EFFECT_BURN_SIDE) {
        *target_status = STATUS_BRN;
        /* Halve attacker's (defender's for Gen 1) burn penalty applied later */
    } else if (base_effect == EFFECT_FREEZE_SIDE) {
        /* Freeze: clear Hyper Beam recharge on target first */
        ClearHyperBeam();
        *target_status = STATUS_FRZ;
    } else {
        /* Paralyze */
        *target_status = STATUS_PAR;
        /* Apply quarter-speed immediately (matching QuarterSpeedDueToParalysis) */
        if (hWhoseTurn == 0) {
            wEnemyMon.spd = QuarterSpeed16(wEnemyMon.spd);
        } else {
            wBattleMon.spd = QuarterSpeed16(wBattleMon.spd);
        }
    }
}

/* ---- ExplodeEffect (effects.asm:175) -------------------------------------
 * Sets the attacker's HP to 0 and clears Leech Seed status.
 * Halves defender's defense before damage calculation (in CalcDamage — that
 * part is already handled by the EFFECT_EXPLODE check in battle_damage.c). */
static void Effect_Explode(void) {
    if (hWhoseTurn == 0) {
        wBattleMon.hp = 0;
        wBattleMon.status = 0;
        RES_BIT(wPlayerBattleStatus2, BSTAT2_SEEDED);
    } else {
        wEnemyMon.hp = 0;
        wEnemyMon.status = 0;
        RES_BIT(wEnemyBattleStatus2, BSTAT2_SEEDED);
    }
}

/* ---- DrainHPEffect (move_effects/drain_hp.asm:1) -------------------------
 * Handles EFFECT_DRAIN_HP (0x03) and EFFECT_DREAM_EATER (0x08).
 * Divides wDamage by 2 (min 1), adds result to attacker HP (capped at max). */
static void Effect_DrainHP(void) {
    /* Halve damage, minimum 1 */
    uint16_t drain = wDamage >> 1;
    if (drain == 0) drain = 1;

    /* Add to attacker HP */
    uint16_t *attacker_hp;
    uint16_t  attacker_max;
    if (hWhoseTurn == 0) {
        attacker_hp  = &wBattleMon.hp;
        attacker_max =  wBattleMon.max_hp;
    } else {
        attacker_hp  = &wEnemyMon.hp;
        attacker_max =  wEnemyMon.max_hp;
    }
    uint32_t new_hp = (uint32_t)(*attacker_hp) + drain;
    *attacker_hp = (new_hp > attacker_max) ? attacker_max : (uint16_t)new_hp;
}

/* ---- StatModifierUpEffect (effects.asm:351) ------------------------------
 * Handles all stat-raise effects: ATTACK_UP1 through EVASION_UP1 (0x0A-0x0F)
 * and ATTACK_UP2 through EVASION_UP2 (0x32-0x37).
 * Affects the ATTACKER's stats.
 * Calls Battle_CalculateModifiedStats and then re-applies burn/paralysis. */
static void Effect_StatModifierUp(void) {
    uint8_t effect   = (hWhoseTurn == 0) ? wPlayerMoveEffect : wEnemyMoveEffect;
    uint8_t move_num = (hWhoseTurn == 0) ? wPlayerMoveNum    : wEnemyMoveNum;
    uint8_t *stat_mods = (hWhoseTurn == 0) ? wPlayerMonStatMods : wEnemyMonStatMods;

    /* Compute stat index (0=ATK, 1=DEF, 2=SPD, 3=SPC, 4=ACC, 5=EVA).
     * Mirrors ASM: sub ATTACK_UP1; if ≥ 8 → sub (ATTACK_UP2 - ATTACK_UP1). */
    uint8_t a = effect - EFFECT_ATTACK_UP1;
    if (a >= 8u) a -= (uint8_t)(EFFECT_ATTACK_UP2 - EFFECT_ATTACK_UP1);
    uint8_t idx = a;  /* 0-5 */

    /* Increment stage (saturate at STAT_STAGE_MAX = 13) */
    uint8_t new_mod = stat_mods[idx] + 1;
    if (new_mod > STAT_STAGE_MAX) {
        /* Restore and bail — "Nothing happened" (no UI in Phase 4) */
        return;
    }

    /* +2 effects increment again (ATTACK_UP2 etc.) */
    int is_plus2 = (effect >= EFFECT_ATTACK_UP2);  /* effect ≥ 0x32 */
    if (is_plus2) {
        new_mod++;
        if (new_mod > STAT_STAGE_MAX) new_mod = STAT_STAGE_MAX;
    }
    stat_mods[idx] = new_mod;

    /* Recalculate ATK/DEF/SPD/SPC (not ACC/EVA) */
    if (idx < 4) {
        wCalculateWhoseStats = hWhoseTurn ? 1 : 0;
        Battle_CalculateModifiedStats();
    }

    /* Minimize: set minimized flag */
    if (move_num == MOVE_MINIMIZE) {
        if (hWhoseTurn == 0) wPlayerMonMinimized = 1;
        else                 wEnemyMonMinimized  = 1;
    }

    /* Gen 1 quirk: re-apply burn/paralysis penalties after every stat change */
    Battle_QuarterSpeedDueToParalysis();
    Battle_HalveAttackDueToBurn();
}

/* ---- StatModifierDownEffect (effects.asm:539) ----------------------------
 * Handles all stat-lowering effects: ATTACK_DOWN1..EVASION_DOWN1 (0x12-0x17),
 * ATTACK_DOWN2..EVASION_DOWN2 (0x3A-0x3F), side-effects (0x44-0x47).
 * Affects the DEFENDER's stats.
 * In non-link single-player battle (player's turn only): 25% miss chance. */
static void Effect_StatModifierDown(void) {
    uint8_t effect  = (hWhoseTurn == 0) ? wPlayerMoveEffect : wEnemyMoveEffect;
    uint8_t *target_mods;
    uint8_t  target_bstat1_val;
    if (hWhoseTurn == 0) {
        /* Player attacks → lower enemy stats */
        target_mods       = wEnemyMonStatMods;
        target_bstat1_val = wEnemyBattleStatus1;
        /* Non-link single-player battle: 25% move miss chance */
        if (wLinkState != LINK_STATE_BATTLING) {
            if (BattleRandom() < PCT25) { wMoveMissed = 1; return; }
        }
    } else {
        /* Enemy attacks → lower player stats */
        target_mods       = wPlayerMonStatMods;
        target_bstat1_val = wPlayerBattleStatus1;
    }

    /* Side effects (0x44-0x47): 33% proc chance */
    if (effect >= EFFECT_ATTACK_DOWN_SIDE) {
        if (BattleRandom() >= PCT33) return;  /* no effect — move still hit */
        uint8_t idx = effect - EFFECT_ATTACK_DOWN_SIDE;  /* 0-3 */
        /* Decrement, floor at STAT_STAGE_MIN (1) */
        if (target_mods[idx] > STAT_STAGE_MIN) {
            target_mods[idx]--;
            wCalculateWhoseStats = (hWhoseTurn == 0) ? 1u : 0u;
            Battle_CalculateModifiedStats();
            Battle_QuarterSpeedDueToParalysis();
            Battle_HalveAttackDueToBurn();
        }
        return;
    }

    /* Substitute blocks non-side stat-down effects */
    if (CheckTargetSubstitute()) { wMoveMissed = 1; return; }

    /* Accuracy test for -1/-2 effects */
    Battle_MoveHitTest();
    if (wMoveMissed) return;

    /* Invulnerable (Fly/Dig) blocks */
    if (TST_BIT(target_bstat1_val, BSTAT1_INVULNERABLE)) { wMoveMissed = 1; return; }

    /* Compute stat index.  Mirrors ASM:
     *   sub ATTACK_DOWN1; if ≥ 8 → sub (ATTACK_DOWN2 - ATTACK_DOWN1). */
    uint8_t a = effect - EFFECT_ATTACK_DOWN1;
    if (a >= 8u) a -= (uint8_t)(EFFECT_ATTACK_DOWN2 - EFFECT_ATTACK_DOWN1);
    uint8_t idx = a;  /* 0-5 */

    /* Determine if -2 stage effect.
     * ASM: if 0x24 ≤ effect < 0x44 → double decrement. */
    int is_minus2 = (effect >= (uint8_t)(EFFECT_ATTACK_DOWN2 - 0x16u) &&
                     effect <  EFFECT_ATTACK_DOWN_SIDE);

    /* Decrement (first step) */
    if (target_mods[idx] <= STAT_STAGE_MIN) return;  /* can't lower, "nothing happened" */
    target_mods[idx]--;

    /* Second decrement for -2 effects */
    if (is_minus2 && target_mods[idx] > STAT_STAGE_MIN) {
        target_mods[idx]--;
    }

    /* Recalculate ATK/DEF/SPD/SPC (not ACC/EVA) */
    if (idx < 4) {
        wCalculateWhoseStats = (hWhoseTurn == 0) ? 1u : 0u;
        Battle_CalculateModifiedStats();
    }

    /* Gen 1 quirk: re-apply penalties */
    Battle_QuarterSpeedDueToParalysis();
    Battle_HalveAttackDueToBurn();
}

/* ---- BideEffect (effects.asm:764) ----------------------------------------
 * Sets STORING_ENERGY flag, clears accumulated damage, zeroes both move
 * effects (prevents other effects triggering during Bide turns), and sets
 * the Bide counter to 2 or 3 at random. */
static void Effect_Bide(void) {
    uint8_t  *bstat1;
    uint16_t *bide_dmg;
    uint8_t  *ctr;
    if (hWhoseTurn == 0) {
        bstat1   = &wPlayerBattleStatus1;
        bide_dmg = &wPlayerBideAccumulatedDamage;
        ctr      = &wPlayerNumAttacksLeft;
    } else {
        bstat1   = &wEnemyBattleStatus1;
        bide_dmg = &wEnemyBideAccumulatedDamage;
        ctr      = &wEnemyNumAttacksLeft;
    }
    SET_BIT(*bstat1, BSTAT1_STORING_ENERGY);
    *bide_dmg = 0;
    wPlayerMoveEffect = 0;
    wEnemyMoveEffect  = 0;
    /* Counter: 2 or 3 (BattleRandom & 1) + 2 */
    *ctr = (BattleRandom() & 1u) + 2u;
}

/* ---- ThrashPetalDanceEffect (effects.asm:791) ----------------------------
 * Sets THRASHING_ABOUT and randomises the thrash/petal-dance counter (2 or 3). */
static void Effect_ThrashPetalDance(void) {
    uint8_t *bstat1;
    uint8_t *ctr;
    if (hWhoseTurn == 0) {
        bstat1 = &wPlayerBattleStatus1;
        ctr    = &wPlayerNumAttacksLeft;
    } else {
        bstat1 = &wEnemyBattleStatus1;
        ctr    = &wEnemyNumAttacksLeft;
    }
    SET_BIT(*bstat1, BSTAT1_THRASHING_ABOUT);
    *ctr = (BattleRandom() & 1u) + 2u;
}

/* ---- SwitchAndTeleportEffect (effects.asm:810) ---------------------------
 * Wild battle: attempt to flee/teleport based on level comparison.
 *   playerLevel > enemyLevel → always succeed.
 *   Otherwise: rand[0, pLvl+eLvl] >= enemyLevel/4 → succeed.
 * Trainer battle: always fails (sets nothing, no escape).
 * On success: sets wEscapedFromBattle = 1. */
static void Effect_SwitchAndTeleport(void) {
    if (wIsInBattle != 1) {
        /* Trainer or no battle — fails silently */
        return;
    }

    uint8_t player_level = wBattleMon.level;
    uint8_t enemy_level  = wEnemyMon.level;

    if (hWhoseTurn == 0) {
        /* Player uses Teleport / Roar / Whirlwind */
        if (player_level >= enemy_level) {
            wEscapedFromBattle = 1;
            return;
        }
        uint8_t range = player_level + enemy_level + 1;
        uint8_t r;
        do { r = BattleRandom(); } while (r >= range);
        if (r >= (enemy_level >> 2)) {
            wEscapedFromBattle = 1;
        }
    } else {
        /* Enemy uses the move */
        if (enemy_level >= player_level) {
            wEscapedFromBattle = 1;
            return;
        }
        uint8_t range = enemy_level + player_level + 1;
        uint8_t r;
        do { r = BattleRandom(); } while (r >= range);
        if (r >= (player_level >> 2)) {
            wEscapedFromBattle = 1;
        }
    }
}

/* ---- TwoToFiveAttacksEffect (effects.asm:925) ----------------------------
 * Sets ATTACKING_MULTIPLE_TIMES flag and picks the hit count.
 * Handles ATTACK_TWICE_EFFECT (always 2 hits) and TWINEEDLE_EFFECT (2 hits,
 * remaps effect to POISON_SIDE_EFFECT1 for 2nd hit).
 * TWO_TO_FIVE_ATTACKS: 3/8 chance 2, 3/8 chance 3, 1/8 chance 4, 1/8 chance 5. */
static void Effect_TwoToFiveAttacks(void) {
    uint8_t *bstat1;
    uint8_t *ctr;
    uint8_t *num_hits;
    uint8_t *eff_ptr;
    if (hWhoseTurn == 0) {
        bstat1   = &wPlayerBattleStatus1;
        ctr      = &wPlayerNumAttacksLeft;
        num_hits = &wPlayerNumHits;
        eff_ptr  = &wPlayerMoveEffect;
    } else {
        bstat1   = &wEnemyBattleStatus1;
        ctr      = &wEnemyNumAttacksLeft;
        num_hits = &wEnemyNumHits;
        eff_ptr  = &wEnemyMoveEffect;
    }

    if (TST_BIT(*bstat1, BSTAT1_ATTACKING_MULTIPLE)) return;  /* already multi-hitting */
    SET_BIT(*bstat1, BSTAT1_ATTACKING_MULTIPLE);

    uint8_t effect = *eff_ptr;
    uint8_t n;

    if (effect == EFFECT_TWINEEDLE) {
        *eff_ptr = EFFECT_POISON_SIDE1;  /* Twineedle: 2 hits, 2nd has poison chance */
        n = 2;
    } else if (effect == EFFECT_ATTACK_TWICE) {
        n = 2;
    } else {
        /* TWO_TO_FIVE_ATTACKS: 3/8 each for 2 and 3, 1/8 each for 4 and 5 */
        uint8_t r = BattleRandom() & 3u;
        if (r >= 2) r = BattleRandom() & 3u;  /* re-roll for lower chance of 4/5 */
        n = r + 2u;  /* 2, 3, 4, or 5 */
    }
    *ctr      = n;
    *num_hits = n;
}

/* ---- FlinchSideEffect (effects.asm:971) ----------------------------------
 * 10% chance (FLINCH_SIDE_EFFECT1) or 30% chance (FLINCH_SIDE_EFFECT2).
 * Sets FLINCHED on the target, clears Hyper Beam recharge. */
static void Effect_FlinchSide(void) {
    if (CheckTargetSubstitute()) return;

    uint8_t effect = (hWhoseTurn == 0) ? wPlayerMoveEffect : wEnemyMoveEffect;
    uint8_t threshold = (effect == EFFECT_FLINCH_SIDE1) ? PCT10 : PCT30;
    if (BattleRandom() >= threshold) return;

    uint8_t *target_bstat1 = (hWhoseTurn == 0) ? &wEnemyBattleStatus1
                                                 : &wPlayerBattleStatus1;
    SET_BIT(*target_bstat1, BSTAT1_FLINCHED);
    ClearHyperBeam();
}

/* ---- OneHitKOEffect (move_effects/one_hit_ko.asm) -----------------------
 * Sets wCriticalHitOrOHKO = 0xFF (OHKO attempt).
 * If attacker speed > defender speed: wDamage = 0xFFFF, wCriticalHitOrOHKO = 2.
 * Otherwise: wMoveMissed = 1. */
static void Effect_OHKO(void) {
    wDamage           = 0;
    wCriticalHitOrOHKO = 0xFF;
    uint16_t atk_spd, def_spd;
    if (hWhoseTurn == 0) {
        atk_spd = wBattleMon.spd;
        def_spd = wEnemyMon.spd;
    } else {
        atk_spd = wEnemyMon.spd;
        def_spd = wBattleMon.spd;
    }
    if (atk_spd > def_spd) {
        wDamage            = 0xFFFF;
        wCriticalHitOrOHKO = 2;
    } else {
        wMoveMissed = 1;
    }
}

/* ---- ChargeEffect (effects.asm:998) --------------------------------------
 * Sets CHARGING_UP and (for Fly/Dig) INVULNERABLE.
 * Stores the move number in wChargeMoveNum for text selection. */
static void Effect_Charge(void) {
    uint8_t effect   = (hWhoseTurn == 0) ? wPlayerMoveEffect : wEnemyMoveEffect;
    uint8_t move_num = (hWhoseTurn == 0) ? wPlayerMoveNum    : wEnemyMoveNum;
    uint8_t *bstat1  = (hWhoseTurn == 0) ? &wPlayerBattleStatus1
                                          : &wEnemyBattleStatus1;
    SET_BIT(*bstat1, BSTAT1_CHARGING_UP);

    if (effect == EFFECT_FLY || move_num == MOVE_DIG) {
        SET_BIT(*bstat1, BSTAT1_INVULNERABLE);
    }
    wChargeMoveNum = move_num;
}

/* ---- TrappingEffect (effects.asm:1080) -----------------------------------
 * Sets USING_TRAPPING_MOVE, clears Hyper Beam on target, rolls 1-4 for
 * the trapping turn counter using the same 3/8–3/8–1/8–1/8 distribution. */
static void Effect_Trapping(void) {
    uint8_t *bstat1;
    uint8_t *ctr;
    if (hWhoseTurn == 0) {
        bstat1 = &wPlayerBattleStatus1;
        ctr    = &wPlayerNumAttacksLeft;
    } else {
        bstat1 = &wEnemyBattleStatus1;
        ctr    = &wEnemyNumAttacksLeft;
    }
    if (TST_BIT(*bstat1, BSTAT1_USING_TRAPPING)) return;

    /* Clear Hyper Beam on target (matching ASM comment) */
    ClearHyperBeam();
    SET_BIT(*bstat1, BSTAT1_USING_TRAPPING);

    uint8_t r = BattleRandom() & 3u;
    if (r >= 2) r = BattleRandom() & 3u;
    *ctr = r + 1u;  /* 1-4 */
}

/* ---- ConfusionSideEffect (effects.asm:1114) and ConfusionEffect ----------
 * ConfusionSideEffect: 10% proc chance (BattleRandom < PCT10).
 * ConfusionEffect: runs MoveHitTest first; blocks substitute.
 * Both: set CONFUSED flag and roll counter 2-5. */
static void EffectCore_ApplyConfusion(void) {
    /* Core confusion application — both paths arrive here */
    uint8_t *target_bstat1;
    uint8_t *target_ctr;
    if (hWhoseTurn == 0) {
        target_bstat1 = &wEnemyBattleStatus1;
        target_ctr    = &wEnemyConfusedCounter;
    } else {
        target_bstat1 = &wPlayerBattleStatus1;
        target_ctr    = &wPlayerConfusedCounter;
    }
    if (TST_BIT(*target_bstat1, BSTAT1_CONFUSED)) return;  /* already confused */
    SET_BIT(*target_bstat1, BSTAT1_CONFUSED);
    *target_ctr = (BattleRandom() & 3u) + 2u;  /* 2-5 turns */
}

static void Effect_ConfusionSide(void) {
    if (BattleRandom() >= PCT10) return;
    EffectCore_ApplyConfusion();
}

static void Effect_Confusion(void) {
    if (CheckTargetSubstitute()) return;
    Battle_MoveHitTest();
    if (wMoveMissed) return;
    EffectCore_ApplyConfusion();
}

/* ---- HyperBeamEffect (effects.asm:1171) ----------------------------------
 * Sets NEEDS_TO_RECHARGE on the ATTACKER's battle status 2. */
static void Effect_HyperBeam(void) {
    uint8_t *bstat2 = (hWhoseTurn == 0) ? &wPlayerBattleStatus2
                                         : &wEnemyBattleStatus2;
    SET_BIT(*bstat2, BSTAT2_NEEDS_TO_RECHARGE);
}

/* ---- RageEffect (effects.asm:1193) ----------------------------------------
 * Sets USING_RAGE on the attacker's battle status 2. */
static void Effect_Rage(void) {
    uint8_t *bstat2 = (hWhoseTurn == 0) ? &wPlayerBattleStatus2
                                         : &wEnemyBattleStatus2;
    SET_BIT(*bstat2, BSTAT2_USING_RAGE);
}

/* ---- MimicEffect (effects.asm:1203) ----------------------------------------
 * Simplified (no UI): picks a random non-null move from the opponent's moveset
 * and overwrites the Mimic move slot (wPlayerMoveListIndex / wEnemyMoveListIndex).
 * Skips if opponent is invulnerable.  Link battle path omitted (no link). */
static void Effect_Mimic(void) {
    Battle_MoveHitTest();
    if (wMoveMissed) return;

    /* Source moves: opponent's moveset.
     * Destination slot: the Mimic move's position in attacker's moveset. */
    uint8_t *src_moves;
    uint8_t *dst_moves;
    uint8_t  dst_slot;
    uint8_t  attacker_bstat1;
    uint8_t  target_bstat1;
    if (hWhoseTurn == 0) {
        attacker_bstat1 = wPlayerBattleStatus1;
        target_bstat1   = wEnemyBattleStatus1;
        src_moves = wEnemyMon.moves;
        dst_moves = wBattleMon.moves;
        dst_slot  = wPlayerMoveListIndex;
    } else {
        attacker_bstat1 = wEnemyBattleStatus1;
        target_bstat1   = wPlayerBattleStatus1;
        src_moves = wBattleMon.moves;
        dst_moves = wEnemyMon.moves;
        dst_slot  = wEnemyMoveListIndex;
    }

    /* Fail if target is invulnerable (Fly/Dig) */
    if (TST_BIT(target_bstat1, BSTAT1_INVULNERABLE)) { wMoveMissed = 1; return; }
    (void)attacker_bstat1;

    /* Pick a random non-null move from opponent */
    uint8_t picked;
    do {
        uint8_t r = BattleRandom() & 3u;
        picked = src_moves[r];
    } while (picked == 0);

    /* Overwrite the Mimic slot */
    if (dst_slot < 4) dst_moves[dst_slot] = picked;
}

/* ---- SplashEffect (effects.asm:1282) -------------------------------------
 * No state change.  "But nothing happened!" is printed by the caller. */
static void Effect_Splash(void) {
    /* Nothing to do */
}

/* ---- DisableEffect (effects.asm:1286) ------------------------------------
 * Picks a random non-null move with PP remaining (or any for non-link AI)
 * and disables it.  Packs (move_slot+1) into high nibble of wPlayerDisabledMove
 * and turn count (1-8) into low nibble. */
static void Effect_Disable(void) {
    Battle_MoveHitTest();
    if (wMoveMissed) return;

    uint8_t *target_disabled;
    uint8_t *target_moves;
    uint8_t *target_pp;
    uint8_t *target_disnum;
    if (hWhoseTurn == 0) {
        target_disabled = &wEnemyDisabledMove;
        target_moves    = wEnemyMon.moves;
        target_pp       = wEnemyMon.pp;
        target_disnum   = &wEnemyDisabledMoveNumber;
    } else {
        target_disabled = &wPlayerDisabledMove;
        target_moves    = wBattleMon.moves;
        target_pp       = wBattleMon.pp;
        target_disnum   = &wPlayerDisabledMoveNumber;
    }

    /* Already disabled? Fail. */
    if (*target_disabled != 0) { wMoveMissed = 1; return; }

    /* Pick a random non-null move slot with PP > 0. */
    uint8_t slot;
    for (;;) {
        slot = BattleRandom() & 3u;
        if (target_moves[slot] == 0) continue;
        /* Check PP (non-link AI has unlimited PP so skip check).
         * In link battle we'd check actual PP; for now always accept. */
        uint8_t pp = target_pp[slot] & PP_MASK;
        if (pp == 0) {
            /* No PP: only fail if all moves have 0 PP */
            uint8_t any_pp = (target_pp[0] | target_pp[1] | target_pp[2] | target_pp[3]) & PP_MASK;
            if (!any_pp) { wMoveMissed = 1; return; }
            continue;
        }
        break;
    }

    /* Pack into wPlayerDisabledMove / wEnemyDisabledMove:
     * high nibble = slot+1 (1-4), low nibble = turns (1-8). */
    uint8_t turns = (BattleRandom() & 7u) + 1u;
    *target_disabled = (uint8_t)(((slot + 1u) << 4) | turns);
    *target_disnum   = target_moves[slot];
}

/* ---- PayDayEffect (move_effects/pay_day.asm) -----------------------------
 * Computes level×2 as BCD and adds it to wTotalPayDayMoney. */
static void Effect_PayDay(void) {
    uint8_t level = (hWhoseTurn == 0) ? wBattleMon.level : wEnemyMon.level;
    uint8_t val   = level * 2u;  /* max 200 for level 100 */

    uint8_t pay[3];
    pay[0] = val / 100u;                              /* hundreds */
    pay[1] = (uint8_t)(((val % 100u) / 10u) << 4) |
             (uint8_t)(val % 10u);                    /* packed tens/ones in [1] */
    pay[2] = 0;                                       /* (unused high byte) */

    /* AddBCD adds [2] (LSB)→[0] (MSB).  Rearrange: pay[0]=MSB, pay[2]=LSB. */
    uint8_t bcd_src[3];
    bcd_src[0] = 0;
    bcd_src[1] = pay[0];   /* hundreds */
    bcd_src[2] = pay[1];   /* tens/ones packed */
    AddBCD3(wTotalPayDayMoney, bcd_src);
}

/* ---- MistEffect (move_effects/mist.asm) ----------------------------------
 * Sets PROTECTED_BY_MIST if not already set. */
static void Effect_Mist(void) {
    uint8_t *bstat2 = (hWhoseTurn == 0) ? &wPlayerBattleStatus2
                                         : &wEnemyBattleStatus2;
    if (TST_BIT(*bstat2, BSTAT2_PROTECTED_BY_MIST)) return;
    SET_BIT(*bstat2, BSTAT2_PROTECTED_BY_MIST);
}

/* ---- FocusEnergyEffect (move_effects/focus_energy.asm) -------------------
 * Sets GETTING_PUMPED (note: Gen 1 bug — halves crit rate instead of doubling). */
static void Effect_FocusEnergy(void) {
    uint8_t *bstat2 = (hWhoseTurn == 0) ? &wPlayerBattleStatus2
                                         : &wEnemyBattleStatus2;
    if (TST_BIT(*bstat2, BSTAT2_GETTING_PUMPED)) return;
    SET_BIT(*bstat2, BSTAT2_GETTING_PUMPED);
}

/* ---- RecoilEffect (move_effects/recoil.asm) ------------------------------
 * Subtracts wDamage/4 from attacker HP (wDamage/2 for Struggle).  Min 1.
 * HP floors at 0. */
static void Effect_Recoil(void) {
    uint8_t move_num = (hWhoseTurn == 0) ? wPlayerMoveNum : wEnemyMoveNum;
    uint16_t recoil  = (move_num == MOVE_STRUGGLE) ? (wDamage >> 1)
                                                    : (wDamage >> 2);
    if (recoil == 0) recoil = 1;

    uint16_t *hp = (hWhoseTurn == 0) ? &wBattleMon.hp : &wEnemyMon.hp;
    *hp = (*hp > recoil) ? (uint16_t)(*hp - recoil) : 0u;
}

/* ---- ConversionEffect (move_effects/conversion.asm) ----------------------
 * Copies the target's type1/type2 to the user.
 * Fails if target is invulnerable (Fly/Dig). */
static void Effect_Conversion(void) {
    uint8_t target_bstat1;
    uint8_t *user_type1, *user_type2;
    uint8_t src_type1,    src_type2;
    if (hWhoseTurn == 0) {
        target_bstat1 = wEnemyBattleStatus1;
        user_type1    = &wBattleMon.type1;
        user_type2    = &wBattleMon.type2;
        src_type1     = wEnemyMon.type1;
        src_type2     = wEnemyMon.type2;
    } else {
        target_bstat1 = wPlayerBattleStatus1;
        user_type1    = &wEnemyMon.type1;
        user_type2    = &wEnemyMon.type2;
        src_type1     = wBattleMon.type1;
        src_type2     = wBattleMon.type2;
    }
    if (TST_BIT(target_bstat1, BSTAT1_INVULNERABLE)) return;  /* fails */
    *user_type1 = src_type1;
    *user_type2 = src_type2;
}

/* ---- HazeEffect (move_effects/haze.asm) ----------------------------------
 * Resets all stat mods to 7 (both sides).
 * Copies wPlayerMonUnmodified* / wEnemyMonUnmodified* back to battle stats.
 * Cures non-volatile status on TARGET only.
 * Clears all volatile statuses (both sides). */
static void Effect_Haze(void) {
    /* Reset all stat mods to STAT_STAGE_NORMAL (7) */
    for (int i = 0; i < NUM_STAT_MODS; i++) {
        wPlayerMonStatMods[i] = STAT_STAGE_NORMAL;
        wEnemyMonStatMods[i]  = STAT_STAGE_NORMAL;
    }

    /* Restore unmodified stats to battle mons */
    wBattleMon.atk = wPlayerMonUnmodifiedAttack;
    wBattleMon.def = wPlayerMonUnmodifiedDefense;
    wBattleMon.spd = wPlayerMonUnmodifiedSpeed;
    wBattleMon.spc = wPlayerMonUnmodifiedSpecial;
    wEnemyMon.atk  = wEnemyMonUnmodifiedAttack;
    wEnemyMon.def  = wEnemyMonUnmodifiedDefense;
    wEnemyMon.spd  = wEnemyMonUnmodifiedSpeed;
    wEnemyMon.spc  = wEnemyMonUnmodifiedSpecial;

    /* Cure non-volatile status on target only.
     * If target was asleep or frozen: set selected move to 0xFF to block action. */
    uint8_t *target_status;
    uint8_t *target_selected;
    if (hWhoseTurn == 0) {
        /* Player attacked → target is enemy */
        target_status   = &wEnemyMon.status;
        target_selected = &wEnemySelectedMove;
    } else {
        target_status   = &wBattleMon.status;
        target_selected = &wPlayerSelectedMove;
    }
    uint8_t old_status = *target_status;
    *target_status = 0;
    if (old_status & (STATUS_FRZ | STATUS_SLP_MASK)) {
        *target_selected = 0xFF;  /* prevent action this turn */
    }

    /* Clear all volatile statuses on both sides */
    /* Player side */
    RES_BIT(wPlayerBattleStatus1, BSTAT1_CONFUSED);
    wPlayerBattleStatus2 &= ~((1u << BSTAT2_USING_X_ACCURACY) |
                               (1u << BSTAT2_PROTECTED_BY_MIST) |
                               (1u << BSTAT2_GETTING_PUMPED) |
                               (1u << BSTAT2_SEEDED));
    wPlayerBattleStatus3 &= ~((1u << BSTAT3_BADLY_POISONED) |
                               (1u << BSTAT3_HAS_LIGHT_SCREEN) |
                               (1u << BSTAT3_HAS_REFLECT));
    wPlayerDisabledMove        = 0;
    wPlayerDisabledMoveNumber  = 0;

    /* Enemy side */
    RES_BIT(wEnemyBattleStatus1, BSTAT1_CONFUSED);
    wEnemyBattleStatus2 &= ~((1u << BSTAT2_USING_X_ACCURACY) |
                              (1u << BSTAT2_PROTECTED_BY_MIST) |
                              (1u << BSTAT2_GETTING_PUMPED) |
                              (1u << BSTAT2_SEEDED));
    wEnemyBattleStatus3 &= ~((1u << BSTAT3_BADLY_POISONED) |
                              (1u << BSTAT3_HAS_LIGHT_SCREEN) |
                              (1u << BSTAT3_HAS_REFLECT));
    wEnemyDisabledMove        = 0;
    wEnemyDisabledMoveNumber  = 0;
}

/* ---- HealEffect (move_effects/heal.asm) ----------------------------------
 * Recover/Softboiled: heals max_hp/2 (Gen 1 bug: heals full max_hp if max_hp < 256).
 * Rest: sets sleep counter to 2, clears all status, THEN heals to full.
 * Fails if HP is already at max (approximate check — see comments). */
static void Effect_Heal(void) {
    uint8_t move_num = (hWhoseTurn == 0) ? wPlayerMoveNum : wEnemyMoveNum;
    uint16_t *hp;
    uint16_t  max_hp;
    if (hWhoseTurn == 0) {
        hp     = &wBattleMon.hp;
        max_hp =  wBattleMon.max_hp;
    } else {
        hp     = &wEnemyMon.hp;
        max_hp =  wEnemyMon.max_hp;
    }

    /* Fail check (Gen 1 approximate: compare low bytes with borrow from high).
     * hp_high cp max_hp_high sets carry; then sbc on low bytes.
     * If result == 0: fail.  Simplified: fail only if hp == max_hp. */
    {
        uint8_t hp_hi   = (uint8_t)(*hp >> 8);
        uint8_t hp_lo   = (uint8_t)(*hp & 0xFF);
        uint8_t max_hi  = (uint8_t)(max_hp >> 8);
        uint8_t max_lo  = (uint8_t)(max_hp & 0xFF);
        int borrow = (hp_hi < max_hi) ? 1 : 0;
        int diff   = (int)hp_lo - (int)max_lo - borrow;
        if (diff == 0) return;  /* fail */
    }

    if (move_num == MOVE_REST) {
        /* Rest: clear status, sleep for 2, and restore HP to max.
         * heal.asm: Rest path sets sleep counter to 2 then heals to max. */
        uint8_t *status = (hWhoseTurn == 0) ? &wBattleMon.status : &wEnemyMon.status;
        *status = 2;  /* STATUS bits 0-2 = sleep counter; 2 = 2 turns */
        *hp = max_hp;
        return;
    }

    /* Heal amount: Gen 1 bug — if max_hp HIGH byte == 0 (max_hp < 256), use full
     * max_hp as heal amount (should be max_hp/2 for Recover/Softboiled). */
    uint16_t heal_amt;
    if ((max_hp >> 8) == 0) {
        heal_amt = max_hp;  /* Gen 1 bug: heals to full for low HP mons */
    } else {
        heal_amt = max_hp >> 1;
    }

    uint32_t new_hp = (uint32_t)(*hp) + heal_amt;
    *hp = (new_hp > max_hp) ? max_hp : (uint16_t)new_hp;
}

/* ---- TransformEffect (move_effects/transform.asm) -----------------------
 * Copies target species, types, catch_rate, moves, DVs, ATK/DEF/SPD/SPC,
 * and stat mods to attacker.  Saves enemy DVs if enemy is transforming.
 * Sets BSTAT3_TRANSFORMED.  Known bugs in original are preserved:
 *   - invulnerability check reads wrong BattleStatus1. */
static void Effect_Transform(void) {
    /* Determine who is the attacker's source (we copy FROM target TO attacker) */
    battle_mon_t *attacker;
    battle_mon_t *target;
    uint8_t      *attacker_bstat3;
    uint16_t     *attacker_unmod_atk;   /* &wPlayerMonUnmodifiedAttack etc. */
    uint16_t     *target_unmod_atk;
    uint8_t      *attacker_stat_mods;
    uint8_t      *target_stat_mods;

    /* Bug: invulnerability check reads wrong side's BattleStatus1 in the ASM.
     * We replicate: player's turn checks wPlayerBattleStatus1 (bug);
     * enemy's turn checks wEnemyBattleStatus1. */
    uint8_t inv_check;
    if (hWhoseTurn == 0) {
        inv_check          = wEnemyBattleStatus1;  /* (bug: should be enemy) */
        attacker           = &wBattleMon;
        target             = &wEnemyMon;
        attacker_bstat3    = &wPlayerBattleStatus3;
        attacker_unmod_atk = &wPlayerMonUnmodifiedAttack;
        target_unmod_atk   = &wEnemyMonUnmodifiedAttack;
        attacker_stat_mods = wPlayerMonStatMods;
        target_stat_mods   = wEnemyMonStatMods;
    } else {
        inv_check          = wPlayerBattleStatus1;  /* (bug: should be player) */
        attacker           = &wEnemyMon;
        target             = &wBattleMon;
        attacker_bstat3    = &wEnemyBattleStatus3;
        attacker_unmod_atk = &wEnemyMonUnmodifiedAttack;
        target_unmod_atk   = &wPlayerMonUnmodifiedAttack;
        attacker_stat_mods = wEnemyMonStatMods;
        target_stat_mods   = wPlayerMonStatMods;
    }

    if (TST_BIT(inv_check, BSTAT1_INVULNERABLE)) return;  /* fails */

    /* Copy species */
    attacker->species   = target->species;
    /* Copy type1, type2, catch_rate, moves (4 bytes offset from species in struct) */
    attacker->type1     = target->type1;
    attacker->type2     = target->type2;
    attacker->catch_rate = target->catch_rate;
    for (int i = 0; i < 4; i++) attacker->moves[i] = target->moves[i];
    /* Copy DVs */
    if (hWhoseTurn != 0) {
        /* Enemy transforming: save its original DVs */
        wTransformedEnemyMonOriginalDVs = wEnemyMon.dvs;
    }
    attacker->dvs = target->dvs;
    /* Copy ATK/DEF/SPD/SPC (skip max_hp and level) */
    attacker->atk = target->atk;
    attacker->def = target->def;
    attacker->spd = target->spd;
    attacker->spc = target->spc;
    /* Set PP to 5 for each copied move (0 for empty slots) */
    for (int i = 0; i < 4; i++)
        attacker->pp[i] = attacker->moves[i] ? 5 : 0;

    /* Copy unmodified stats and stat mods */
    attacker_unmod_atk[0] = target_unmod_atk[0];  /* unmod ATK */
    attacker_unmod_atk[1] = target_unmod_atk[1];  /* unmod DEF */
    attacker_unmod_atk[2] = target_unmod_atk[2];  /* unmod SPD */
    attacker_unmod_atk[3] = target_unmod_atk[3];  /* unmod SPC */
    for (int i = 0; i < NUM_STAT_MODS; i++)
        attacker_stat_mods[i] = target_stat_mods[i];

    SET_BIT(*attacker_bstat3, BSTAT3_TRANSFORMED);
}

/* ---- ReflectLightScreenEffect (move_effects/reflect_light_screen.asm) ----
 * Sets BSTAT3_HAS_REFLECT or BSTAT3_HAS_LIGHT_SCREEN on the attacker's side. */
static void Effect_ReflectLightScreen(void) {
    uint8_t effect  = (hWhoseTurn == 0) ? wPlayerMoveEffect : wEnemyMoveEffect;
    uint8_t *bstat3 = (hWhoseTurn == 0) ? &wPlayerBattleStatus3
                                         : &wEnemyBattleStatus3;
    uint8_t bit = (effect == EFFECT_REFLECT) ? BSTAT3_HAS_REFLECT
                                             : BSTAT3_HAS_LIGHT_SCREEN;
    if (TST_BIT(*bstat3, bit)) return;  /* already active */
    SET_BIT(*bstat3, bit);
}

/* ---- ParalyzeEffect (move_effects/paralyze.asm) --------------------------
 * Full paralysis move (Thunder Wave, Stun Spore, etc.).
 * Blocks Ground-type targets against Electric moves.
 * Runs MoveHitTest.  Applies quarter-speed immediately. */
static void Effect_Paralyze(void) {
    uint8_t mv_type = (hWhoseTurn == 0) ? wPlayerMoveType : wEnemyMoveType;
    uint8_t *target_status, *target_type1, *target_type2;
    if (hWhoseTurn == 0) {
        target_status = &wEnemyMon.status;
        target_type1  = &wEnemyMon.type1;
        target_type2  = &wEnemyMon.type2;
    } else {
        target_status = &wBattleMon.status;
        target_type1  = &wBattleMon.type1;
        target_type2  = &wBattleMon.type2;
    }

    /* Already statused? */
    if (*target_status != 0) return;

    /* Electric move can't paralyze Ground-type */
    if (mv_type == TYPE_ELECTRIC &&
        (*target_type1 == TYPE_GROUND || *target_type2 == TYPE_GROUND)) return;

    Battle_MoveHitTest();
    if (wMoveMissed) return;

    *target_status = STATUS_PAR;
    /* Apply quarter-speed immediately */
    if (hWhoseTurn == 0) wEnemyMon.spd = QuarterSpeed16(wEnemyMon.spd);
    else                 wBattleMon.spd = QuarterSpeed16(wBattleMon.spd);
}

/* ---- SubstituteEffect (move_effects/substitute.asm) ----------------------
 * Creates a substitute at cost of max_hp/4 (low byte only, assumes ≤ 1023 HP).
 * Bug: if resulting HP == 0 exactly, substitute still created (only underflow
 * / carry triggers failure, not zero). */
static void Effect_Substitute(void) {
    uint16_t *hp;
    uint16_t  max_hp;
    uint8_t  *bstat2;
    uint8_t  *sub_hp_var;
    if (hWhoseTurn == 0) {
        hp         = &wBattleMon.hp;
        max_hp     =  wBattleMon.max_hp;
        bstat2     = &wPlayerBattleStatus2;
        sub_hp_var = &wPlayerSubstituteHP;
    } else {
        hp         = &wEnemyMon.hp;
        max_hp     =  wEnemyMon.max_hp;
        bstat2     = &wEnemyBattleStatus2;
        sub_hp_var = &wEnemySubstituteHP;
    }

    /* Already has substitute? Fail. */
    if (TST_BIT(*bstat2, BSTAT2_HAS_SUBSTITUTE)) return;

    /* sub_cost = low byte of (max_hp / 4) */
    uint8_t sub_cost = (uint8_t)(max_hp >> 2);

    /* Check: would current HP go negative (underflow)?
     * Bug: only carry (underflow) is checked, not zero. */
    uint8_t hp_lo = (uint8_t)(*hp & 0xFF);
    uint8_t hp_hi = (uint8_t)(*hp >> 8);
    if (sub_cost > hp_lo && hp_hi == 0) return;  /* not enough HP */
    if (sub_cost > hp_lo && hp_hi > 0) {
        hp_hi--;
        hp_lo = (uint8_t)(256u - (sub_cost - hp_lo));
    } else {
        hp_lo -= sub_cost;
    }
    *hp = ((uint16_t)hp_hi << 8) | hp_lo;

    *sub_hp_var = sub_cost;
    SET_BIT(*bstat2, BSTAT2_HAS_SUBSTITUTE);
}

/* ---- LeechSeedEffect (move_effects/leech_seed.asm) -----------------------
 * Runs MoveHitTest.  Fails against Grass-types and already-seeded targets.
 * Sets BSTAT2_SEEDED. */
static void Effect_LeechSeed(void) {
    Battle_MoveHitTest();
    if (wMoveMissed) return;

    uint8_t *target_type1, *target_type2, *target_bstat2;
    if (hWhoseTurn == 0) {
        target_type1  = &wEnemyMon.type1;
        target_type2  = &wEnemyMon.type2;
        target_bstat2 = &wEnemyBattleStatus2;
    } else {
        target_type1  = &wBattleMon.type1;
        target_type2  = &wBattleMon.type2;
        target_bstat2 = &wPlayerBattleStatus2;
    }

    /* Fail against Grass-type */
    if (*target_type1 == TYPE_GRASS || *target_type2 == TYPE_GRASS) {
        wMoveMissed = 1; return;
    }
    /* Already seeded? */
    if (TST_BIT(*target_bstat2, BSTAT2_SEEDED)) { wMoveMissed = 1; return; }

    SET_BIT(*target_bstat2, BSTAT2_SEEDED);
}

/* ============================================================
 * Battle_JumpMoveEffect — main dispatch (effects.asm:1 / effects_pointers.asm)
 * ============================================================ */
void Battle_JumpMoveEffect(void) {
    uint8_t effect = (hWhoseTurn == 0) ? wPlayerMoveEffect : wEnemyMoveEffect;

    switch (effect) {
    /* effect 0x01 — unused, maps to SleepEffect in pointer table */
    case 0x01:
    case EFFECT_SLEEP:          Effect_Sleep();            break;

    case EFFECT_POISON_SIDE1:
    case EFFECT_POISON_SIDE2:
    case EFFECT_POISON:         Effect_Poison();           break;

    case EFFECT_DRAIN_HP:
    case EFFECT_DREAM_EATER:    Effect_DrainHP();          break;

    case EFFECT_BURN_SIDE:
    case EFFECT_FREEZE_SIDE:
    case EFFECT_PARALYZE_SIDE:
    case EFFECT_BURN_SIDE2:
    case EFFECT_FREEZE_SIDE2:
    case EFFECT_PARALYZE_SIDE2: Effect_FreezeBurnParalyze(); break;

    case EFFECT_EXPLODE:        Effect_Explode();          break;

    case EFFECT_MIRROR_MOVE:    /* NULL — handled pre-table in core.asm */ break;

    case EFFECT_ATTACK_UP1:
    case EFFECT_DEFENSE_UP1:
    case EFFECT_SPEED_UP1:
    case EFFECT_SPECIAL_UP1:
    case EFFECT_ACCURACY_UP1:
    case EFFECT_EVASION_UP1:
    case EFFECT_ATTACK_UP2:
    case EFFECT_DEFENSE_UP2:
    case EFFECT_SPEED_UP2:
    case EFFECT_SPECIAL_UP2:
    case EFFECT_ACCURACY_UP2:
    case EFFECT_EVASION_UP2:    Effect_StatModifierUp();   break;

    case EFFECT_PAY_DAY:        Effect_PayDay();           break;

    case EFFECT_SWIFT:          /* NULL — always-hit handled by MoveHitTest */ break;

    case EFFECT_ATTACK_DOWN1:
    case EFFECT_DEFENSE_DOWN1:
    case EFFECT_SPEED_DOWN1:
    case EFFECT_SPECIAL_DOWN1:
    case EFFECT_ACCURACY_DOWN1:
    case EFFECT_EVASION_DOWN1:
    case EFFECT_ATTACK_DOWN2:
    case EFFECT_DEFENSE_DOWN2:
    case EFFECT_SPEED_DOWN2:
    case EFFECT_SPECIAL_DOWN2:
    case EFFECT_ACCURACY_DOWN2:
    case EFFECT_EVASION_DOWN2:
    case EFFECT_ATTACK_DOWN_SIDE:
    case EFFECT_DEFENSE_DOWN_SIDE:
    case EFFECT_SPEED_DOWN_SIDE:
    case EFFECT_SPECIAL_DOWN_SIDE: Effect_StatModifierDown(); break;

    case EFFECT_CONVERSION:     Effect_Conversion();       break;
    case EFFECT_HAZE:           Effect_Haze();             break;
    case EFFECT_BIDE:           Effect_Bide();             break;
    case EFFECT_THRASH:         Effect_ThrashPetalDance(); break;
    case EFFECT_SWITCH_TELEPORT: Effect_SwitchAndTeleport(); break;

    case EFFECT_TWO_TO_FIVE_ATTACKS:
    case EFFECT_1E:
    case EFFECT_ATTACK_TWICE:
    case EFFECT_TWINEEDLE:      Effect_TwoToFiveAttacks(); break;

    case EFFECT_FLINCH_SIDE1:
    case EFFECT_FLINCH_SIDE2:   Effect_FlinchSide();       break;

    case EFFECT_OHKO:           Effect_OHKO();             break;
    case EFFECT_CHARGE:
    case EFFECT_FLY:            Effect_Charge();           break;

    case EFFECT_SUPER_FANG:     /* NULL — handled in core.asm */ break;
    case EFFECT_SPECIAL_DAMAGE: /* NULL — handled in core.asm */ break;

    case EFFECT_TRAPPING:       Effect_Trapping();         break;
    case EFFECT_JUMP_KICK:      /* NULL — handled in core.asm */ break;

    case EFFECT_MIST:           Effect_Mist();             break;
    case EFFECT_FOCUS_ENERGY:   Effect_FocusEnergy();      break;
    case EFFECT_RECOIL:         Effect_Recoil();           break;
    case EFFECT_CONFUSION:      Effect_Confusion();        break;

    case EFFECT_HEAL:           Effect_Heal();             break;
    case EFFECT_TRANSFORM:      Effect_Transform();        break;

    case EFFECT_LIGHT_SCREEN:
    case EFFECT_REFLECT:        Effect_ReflectLightScreen(); break;

    case EFFECT_PARALYZE:       Effect_Paralyze();         break;
    case EFFECT_CONFUSION_SIDE: Effect_ConfusionSide();    break;
    case EFFECT_SUBSTITUTE:     Effect_Substitute();       break;
    case EFFECT_HYPER_BEAM:     Effect_HyperBeam();        break;
    case EFFECT_RAGE:           Effect_Rage();             break;
    case EFFECT_MIMIC:          Effect_Mimic();            break;
    case EFFECT_METRONOME:      /* NULL — handled pre-table in core.asm */ break;
    case EFFECT_LEECH_SEED:     Effect_LeechSeed();        break;
    case EFFECT_SPLASH:         Effect_Splash();           break;
    case EFFECT_DISABLE:        Effect_Disable();          break;

    default: break;  /* unknown effect — no-op */
    }
}
