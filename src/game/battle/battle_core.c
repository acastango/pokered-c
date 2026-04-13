/* battle_core.c — Gen 1 battle turn execution engine.
 *
 * Ports of engine/battle/core.asm (bank $0F):
 *   ExecutePlayerMove          (core.asm:3073)
 *   CheckPlayerStatusConditions (core.asm:3328)
 *   CheckForDisobedience       (core.asm:3830)
 *   HandleSelfConfusionDamage  (core.asm:3672)
 *   HandleCounterMove          (core.asm:4547)
 *   ApplyAttackToEnemyPokemon  (core.asm:4612)
 *   ApplyAttackToPlayerPokemon (core.asm:4731)
 *   AttackSubstitute           (core.asm:4849)
 *   HandleBuildingRage         (core.asm:4912)
 *   ExecuteEnemyMove           (core.asm:5457)
 *   CheckEnemyStatusConditions (core.asm:5677)
 *   HandlePoisonBurnLeechSeed  (core.asm:470)
 *   HandlePoisonBurnLeechSeed_DecreaseOwnHP (core.asm:550)
 *   HandlePoisonBurnLeechSeed_IncreaseEnemyHP (core.asm:618)
 *   HandleEnemyMonFainted      (core.asm:699)
 *   FaintEnemyPokemon          (core.asm:732) — state portion only
 *   HandlePlayerMonFainted     (core.asm:969)
 *   RemoveFaintedPlayerMon     (core.asm:1002) — state portion only
 *   SwapPlayerAndEnemyLevels   (core.asm:6188)
 *
 * UI calls (PrintText, PlayAnimation, DrawHUDs, DelayFrames) are omitted.
 * ALWAYS refer to pokered-master ASM before modifying anything here.
 */

#include "battle_core.h"
#include "battle.h"
#include "battle_effects.h"
#include "battle_exp.h"
#include "battle_switch.h"
#include "battle_trainer.h"
#include "battle_loop.h"
#include "../../platform/hardware.h"

/* ============================================================
 * Effect dispatch arrays — exact copies of data/battle/*.asm
 *
 * Terminated by 0xFF (the ASM uses db -1 which is 0xFF unsigned).
 * ============================================================ */

/* ResidualEffects1 (data/battle/residual_effects_1.asm):
 * Non-damage effects that skip damage calc entirely; dispatched via JumpMoveEffect. */
static const uint8_t kResidualEffects1[] = {
    EFFECT_CONVERSION,    /* 0x18 */
    EFFECT_HAZE,          /* 0x19 */
    EFFECT_SWITCH_TELEPORT, /* 0x1C */
    EFFECT_MIST,          /* 0x2E */
    EFFECT_FOCUS_ENERGY,  /* 0x2F */
    EFFECT_CONFUSION,     /* 0x31 */
    EFFECT_HEAL,          /* 0x38 */
    EFFECT_TRANSFORM,     /* 0x39 */
    EFFECT_LIGHT_SCREEN,  /* 0x40 */
    EFFECT_REFLECT,       /* 0x41 */
    EFFECT_POISON,        /* 0x42 */
    EFFECT_PARALYZE,      /* 0x43 */
    EFFECT_SUBSTITUTE,    /* 0x4F */
    EFFECT_MIMIC,         /* 0x52 */
    EFFECT_LEECH_SEED,    /* 0x54 */
    EFFECT_SPLASH,        /* 0x55 */
    0xFF
};

/* SpecialEffectsCont (data/battle/special_effects.asm, fallthrough section):
 * Damaging moves whose effect runs BEFORE damage calc; JumpMoveEffect called
 * but execution continues (doesn't skip damage). */
static const uint8_t kSpecialEffectsCont[] = {
    EFFECT_THRASH,        /* 0x1B THRASH_PETAL_DANCE_EFFECT */
    EFFECT_TRAPPING,      /* 0x2A */
    0xFF
};

/* SetDamageEffects (data/battle/set_damage_effects.asm):
 * Moves that set damage outside normal formula; skip damage calc, go to MoveHitTest. */
static const uint8_t kSetDamageEffects[] = {
    EFFECT_SUPER_FANG,    /* 0x28 */
    EFFECT_SPECIAL_DAMAGE, /* 0x29 */
    0xFF
};

/* AlwaysHappenSideEffects (data/battle/always_happen_effects.asm):
 * Effects that run even if the target fainted. */
static const uint8_t kAlwaysHappenSideEffects[] = {
    EFFECT_DRAIN_HP,           /* 0x03 */
    EFFECT_EXPLODE,            /* 0x07 */
    EFFECT_DREAM_EATER,        /* 0x08 */
    EFFECT_PAY_DAY,            /* 0x10 */
    EFFECT_TWO_TO_FIVE_ATTACKS, /* 0x1D */
    EFFECT_1E,                 /* 0x1E */
    EFFECT_ATTACK_TWICE,       /* 0x2C */
    EFFECT_RECOIL,             /* 0x30 */
    EFFECT_TWINEEDLE,          /* 0x4D */
    EFFECT_RAGE,               /* 0x51 */
    0xFF
};

/* ResidualEffects2 (data/battle/residual_effects_2.asm):
 * Stat-affecting moves, sleep, bide — run after hit/miss check then skip rest. */
static const uint8_t kResidualEffects2[] = {
    EFFECT_01,            /* 0x01 unused */
    EFFECT_ATTACK_UP1,    /* 0x0A */
    EFFECT_DEFENSE_UP1,   /* 0x0B */
    EFFECT_SPEED_UP1,     /* 0x0C */
    EFFECT_SPECIAL_UP1,   /* 0x0D */
    EFFECT_ACCURACY_UP1,  /* 0x0E */
    EFFECT_EVASION_UP1,   /* 0x0F */
    EFFECT_ATTACK_DOWN1,  /* 0x12 */
    EFFECT_DEFENSE_DOWN1, /* 0x13 */
    EFFECT_SPEED_DOWN1,   /* 0x14 */
    EFFECT_SPECIAL_DOWN1, /* 0x15 */
    EFFECT_ACCURACY_DOWN1, /* 0x16 */
    EFFECT_EVASION_DOWN1, /* 0x17 */
    EFFECT_BIDE,          /* 0x1A */
    EFFECT_SLEEP,         /* 0x20 */
    EFFECT_ATTACK_UP2,    /* 0x32 */
    EFFECT_DEFENSE_UP2,   /* 0x33 */
    EFFECT_SPEED_UP2,     /* 0x34 */
    EFFECT_SPECIAL_UP2,   /* 0x35 */
    EFFECT_ACCURACY_UP2,  /* 0x36 */
    EFFECT_EVASION_UP2,   /* 0x37 */
    EFFECT_ATTACK_DOWN2,  /* 0x3A */
    EFFECT_DEFENSE_DOWN2, /* 0x3B */
    EFFECT_SPEED_DOWN2,   /* 0x3C */
    EFFECT_SPECIAL_DOWN2, /* 0x3D */
    EFFECT_ACCURACY_DOWN2, /* 0x3E */
    EFFECT_EVASION_DOWN2, /* 0x3F */
    0xFF
};

/* SpecialEffects (data/battle/special_effects.asm, full list):
 * Effects already handled earlier — skip the final JumpMoveEffect call.
 * Includes SpecialEffectsCont at the end. */
static const uint8_t kSpecialEffects[] = {
    EFFECT_DRAIN_HP,           /* 0x03 */
    EFFECT_EXPLODE,            /* 0x07 */
    EFFECT_DREAM_EATER,        /* 0x08 */
    EFFECT_PAY_DAY,            /* 0x10 */
    EFFECT_SWIFT,              /* 0x11 */
    EFFECT_TWO_TO_FIVE_ATTACKS, /* 0x1D */
    EFFECT_1E,                 /* 0x1E */
    EFFECT_CHARGE,             /* 0x27 */
    EFFECT_SUPER_FANG,         /* 0x28 */
    EFFECT_SPECIAL_DAMAGE,     /* 0x29 */
    EFFECT_FLY,                /* 0x2B */
    EFFECT_ATTACK_TWICE,       /* 0x2C */
    EFFECT_JUMP_KICK,          /* 0x2D */
    EFFECT_RECOIL,             /* 0x30 */
    /* SpecialEffectsCont (fallthrough): */
    EFFECT_THRASH,             /* 0x1B */
    EFFECT_TRAPPING,           /* 0x2A */
    0xFF
};

/* Combat log sink — set by debug_overlay.c when logging is enabled. */
void (*gCombatLogSink)(const char *line) = NULL;

/* ============================================================
 * Internal helper: array membership test.
 * ============================================================ */
static int is_in_array(uint8_t val, const uint8_t *arr) {
    for (; *arr != 0xFF; arr++) {
        if (*arr == val) return 1;
    }
    return 0;
}

/* ============================================================
 * SwapPlayerAndEnemyLevels — core.asm:6188
 *
 * Needed because Battle_CalcDamage uses wBattleMon.level (player level)
 * for the formula. Enemy attack: swap so enemy's level becomes the
 * "player level" for the calc, then swap back.
 * ============================================================ */
static void swap_player_enemy_levels(void) {
    uint8_t tmp = wBattleMon.level;
    wBattleMon.level = wEnemyMon.level;
    wEnemyMon.level = tmp;
}

/* ============================================================
 * AttackSubstitute — core.asm:4849
 *
 * Routes damage to the target's substitute.
 * Player's turn (hWhoseTurn==0): target = enemy → enemy's substitute.
 * Enemy's turn (hWhoseTurn!=0): target = player → player's substitute.
 *
 * Gen 1 bug: when hWhoseTurn is temporarily swapped before calling
 * ApplyDamageToPlayerPokemon (e.g. self-confusion damage), damage
 * accidentally hits the opposing side's substitute instead.
 * ============================================================ */
static void attack_substitute(void) {
    uint8_t  *sub_hp;
    uint8_t  *bstat2;
    uint8_t  *atk_effect;

    if (hWhoseTurn == 0) {   /* player's turn: target is enemy */
        sub_hp     = &wEnemySubstituteHP;
        bstat2     = &wEnemyBattleStatus2;
        atk_effect = &wPlayerMoveEffect;
    } else {                 /* enemy's turn: target is player */
        sub_hp     = &wPlayerSubstituteHP;
        bstat2     = &wPlayerBattleStatus2;
        atk_effect = &wEnemyMoveEffect;
    }

    /* Damage > 0xFF (high byte set) always breaks substitute (core.asm:4872) */
    if ((wDamage >> 8) != 0) goto substitute_broke;

    {
        uint8_t dmg = (uint8_t)wDamage;
        if (*sub_hp > dmg) {
            *sub_hp -= dmg;
            return;   /* substitute absorbed the hit */
        }
    }

substitute_broke:
    *bstat2 &= ~(1u << BSTAT2_HAS_SUBSTITUTE);
    /* Nullify attacker's move effect (core.asm:4900-4901) */
    *atk_effect = 0;
}

/* ============================================================
 * apply_damage_to_enemy_pokemon — ApplyDamageToEnemyPokemon (core.asm:4678)
 *
 * Subtracts wDamage from wEnemyMon.hp.
 * On overkill: clamps HP to 0 and sets wDamage = pre-overkill HP.
 * Checks substitute before subtracting (routes to AttackSubstitute).
 * ============================================================ */
static void apply_damage_to_enemy_pokemon(void) {
    if (wDamage == 0) return;

    if (wEnemyBattleStatus2 & (1u << BSTAT2_HAS_SUBSTITUTE)) {
        attack_substitute();
        return;
    }

    if (wEnemyMon.hp <= wDamage) {
        wDamage = wEnemyMon.hp;   /* wDamage = actual damage dealt */
        wEnemyMon.hp = 0;
    } else {
        wEnemyMon.hp -= wDamage;
    }
    BLOG("  %s took %d dmg -> %d/%d HP", BMON_E(), wDamage, wEnemyMon.hp, wEnemyMon.max_hp);
}

/* ============================================================
 * apply_damage_to_player_pokemon — ApplyDamageToPlayerPokemon (core.asm:4797)
 * ============================================================ */
static void apply_damage_to_player_pokemon(void) {
    if (wDamage == 0) return;

    if (wPlayerBattleStatus2 & (1u << BSTAT2_HAS_SUBSTITUTE)) {
        attack_substitute();
        return;
    }

    if (wBattleMon.hp <= wDamage) {
        wDamage = wBattleMon.hp;
        wBattleMon.hp = 0;
    } else {
        wBattleMon.hp -= wDamage;
    }
    BLOG("  %s took %d dmg -> %d/%d HP", BMON_P(), wDamage, wBattleMon.hp, wBattleMon.max_hp);
}

/* ============================================================
 * apply_attack_to_enemy_pokemon — ApplyAttackToEnemyPokemon (core.asm:4612)
 *
 * Handles OHKO, Super Fang, Special Damage, and normal-power checks
 * before calling apply_damage_to_enemy_pokemon.
 * ============================================================ */
static void apply_attack_to_enemy_pokemon(void) {
    uint8_t effect = wPlayerMoveEffect;

    if (effect == EFFECT_OHKO) {
        /* OHKO: wDamage already set by JumpMoveEffect to enemy max HP */
        goto do_apply;
    }
    if (effect == EFFECT_SUPER_FANG) {
        /* Half current HP, minimum 1 (core.asm:4624-4641) */
        uint16_t half = wEnemyMon.hp >> 1;
        wDamage = (half == 0) ? 1 : half;
        goto do_apply;
    }
    if (effect == EFFECT_SPECIAL_DAMAGE) {
        /* Fixed damage by move number and attacker level (core.asm:4642-4676) */
        uint8_t level = wBattleMon.level;
        uint8_t move  = wPlayerMoveNum;
        uint8_t dmg;
        if (move == MOVE_SEISMIC_TOSS || move == MOVE_NIGHT_SHADE) {
            dmg = level;
        } else if (move == MOVE_SONICBOOM) {
            dmg = SONICBOOM_DAMAGE;
        } else if (move == MOVE_DRAGON_RAGE) {
            dmg = DRAGON_RAGE_DAMAGE;
        } else {
            /* Psywave: loop until 1 <= r < level*1.5 (core.asm:4657-4669) */
            uint8_t max_dmg = (uint8_t)(level + (level >> 1));
            uint8_t r;
            do {
                r = BattleRandom();
            } while (r == 0 || r >= max_dmg);
            dmg = r;
        }
        wDamage = dmg;
        goto do_apply;
    }
    /* Normal: skip if base power is 0 (core.asm:4621-4622) */
    if (wPlayerMovePower == 0) return;

do_apply:
    apply_damage_to_enemy_pokemon();
}

/* ============================================================
 * apply_attack_to_player_pokemon — ApplyAttackToPlayerPokemon (core.asm:4731)
 * ============================================================ */
static void apply_attack_to_player_pokemon(void) {
    uint8_t effect = wEnemyMoveEffect;

    if (effect == EFFECT_OHKO) {
        goto do_apply;
    }
    if (effect == EFFECT_SUPER_FANG) {
        uint16_t half = wBattleMon.hp >> 1;
        wDamage = (half == 0) ? 1 : half;
        goto do_apply;
    }
    if (effect == EFFECT_SPECIAL_DAMAGE) {
        /* Enemy's level is in wEnemyMon.level, but at this point levels are
         * NOT swapped (SwapPlayerAndEnemyLevels was already called back).
         * The ASM reads wEnemyMonLevel directly (core.asm:4762). */
        uint8_t level = wEnemyMon.level;
        uint8_t move  = wEnemyMoveNum;
        uint8_t dmg;
        if (move == MOVE_SEISMIC_TOSS || move == MOVE_NIGHT_SHADE) {
            dmg = level;
        } else if (move == MOVE_SONICBOOM) {
            dmg = SONICBOOM_DAMAGE;
        } else if (move == MOVE_DRAGON_RAGE) {
            dmg = DRAGON_RAGE_DAMAGE;
        } else {
            /* Psywave (enemy): range [0, level*1.5) — 0 is possible! (core.asm:4782-4789) */
            uint8_t max_dmg = (uint8_t)(level + (level >> 1));
            uint8_t r;
            do {
                r = BattleRandom();
            } while (r >= max_dmg);
            dmg = r;
        }
        wDamage = dmg;
        goto do_apply;
    }
    if (wEnemyMovePower == 0) return;

do_apply:
    apply_damage_to_player_pokemon();
}

/* ============================================================
 * HandleBuildingRage — core.asm:4912
 *
 * If the pokemon that was just attacked is using Rage, raise its
 * Attack stage by 1.  Temporarily flips hWhoseTurn to call
 * StatModifierUpEffect on the correct side.
 * ============================================================ */
static void handle_building_rage(void) {
    uint8_t  *bstat2;
    uint8_t  *stat_mods;
    uint8_t  *move_num;
    uint8_t  *move_effect;

    /* Player's turn: the attacked mon is the ENEMY */
    if (hWhoseTurn == 0) {
        bstat2      = &wEnemyBattleStatus2;
        stat_mods   = wEnemyMonStatMods;
        move_num    = &wEnemyMoveNum;
        move_effect = &wEnemyMoveEffect;
    } else {
        bstat2      = &wPlayerBattleStatus2;
        stat_mods   = wPlayerMonStatMods;
        move_num    = &wPlayerMoveNum;
        move_effect = &wPlayerMoveEffect;
    }

    if (!(*bstat2 & (1u << BSTAT2_USING_RAGE))) return;
    if (stat_mods[MOD_ATTACK] >= STAT_STAGE_MAX) return;

    /* Temporarily zero the attacked mon's move number and set effect to
     * ATTACK_UP1 so JumpMoveEffect raises its Attack stage. */
    uint8_t saved_num    = *move_num;
    uint8_t saved_effect = *move_effect;
    *move_num    = 0;
    *move_effect = EFFECT_ATTACK_UP1;

    /* Flip turn so JumpMoveEffect targets the attacked (Rage-using) mon */
    hWhoseTurn ^= 1;
    Battle_JumpMoveEffect();
    hWhoseTurn ^= 1;

    /* Restore move number to RAGE; clear effect */
    *move_num    = MOVE_RAGE;
    *move_effect = saved_effect;
    (void)saved_num;
}

/* ============================================================
 * HandleSelfConfusionDamage — core.asm:3672
 *
 * Calculates and applies typeless 40-power damage to the player's
 * own mon (confused hurt-self).  No crit, no type effectiveness,
 * no randomization, always hits.
 *
 * Gen 1 implementation detail: wEnemyMonDefense is temporarily set to
 * wBattleMonDefense before GetDamageVarsForPlayerAttack so the formula
 * uses the player's own defense stat for the "defender" slot.
 * ============================================================ */
static void handle_self_confusion_damage(void) {
    /* Save and override enemy defense so attacker hurts self (core.asm:3675-3683) */
    uint16_t saved_enemy_def = wEnemyMon.def;
    wEnemyMon.def = wBattleMon.def;

    /* Save and override move parameters (core.asm:3684-3694) */
    uint8_t saved_effect = wPlayerMoveEffect;
    uint8_t saved_power  = wPlayerMovePower;
    uint8_t saved_type   = wPlayerMoveType;
    wPlayerMoveEffect = 0;
    wPlayerMovePower  = 40;
    wPlayerMoveType   = 0;   /* typeless */
    wCriticalHitOrOHKO = 0;  /* no crit (core.asm:3690) */

    /* Calculate damage (core.asm:3695-3696) */
    Battle_GetDamageVarsForPlayerAttack();  /* includes CalcDamage */

    /* Restore (core.asm:3698-3705) */
    wPlayerMoveEffect = saved_effect;
    wPlayerMovePower  = saved_power;
    wPlayerMoveType   = saved_type;
    wEnemyMon.def = saved_enemy_def;

    /* Apply to player (core.asm:3714: jp ApplyDamageToPlayerPokemon)
     * hWhoseTurn is still 0 (player's turn), so the substitute check
     * targets the ENEMY's substitute — that's the Gen 1 confusion bug. */
    apply_damage_to_player_pokemon();
}

/* ============================================================
 * handle_counter_move — HandleCounterMove (core.asm:4547)
 *
 * Returns 0 if the current move IS Counter (caller must jump to
 * handle_missed to check wMoveMissed), nonzero if move != Counter
 * (caller continues to GetDamageVarsFor*Attack).
 * ============================================================ */
static int handle_counter_move(void) {
    uint8_t cur_move;
    uint8_t opp_selected;
    uint8_t opp_power;
    uint8_t opp_type;

    if (hWhoseTurn == 0) {   /* player's turn */
        cur_move     = wPlayerMoveNum;
        opp_selected = wEnemySelectedMove;
        opp_power    = wEnemyMovePower;
        opp_type     = wEnemyMoveType;
    } else {                 /* enemy's turn */
        cur_move     = wEnemyMoveNum;
        opp_selected = wPlayerSelectedMove;
        opp_power    = wPlayerMovePower;
        opp_type     = wPlayerMoveType;
    }

    if (cur_move != MOVE_COUNTER) return 1;  /* not Counter: continue */

    /* Counter is being used — initialise missed = true (core.asm:4568-4569) */
    wMoveMissed = 1;

    if (opp_selected == MOVE_COUNTER) return 0;   /* opponent also Countering: miss */
    if (opp_power == 0)               return 0;   /* opponent used status move: miss */

    /* Only Normal or Fighting type moves are counterable (core.asm:4577-4584) */
    if (opp_type != TYPE_NORMAL && opp_type != TYPE_FIGHTING) return 0;

    /* wDamage must be nonzero (core.asm:4588-4590) */
    if (wDamage == 0) return 0;

    /* Counter hits: double wDamage, cap at 0xFFFF (core.asm:4593-4604) */
    uint32_t doubled = (uint32_t)wDamage * 2;
    wDamage = (doubled > 0xFFFF) ? 0xFFFF : (uint16_t)doubled;
    wMoveMissed = 0;

    /* Also run the normal hit test (core.asm:4608) */
    Battle_MoveHitTest();

    return 0;   /* was Counter: always jump to handle_missed */
}

/* ============================================================
 * Return codes from check_*_status_conditions.
 * These mirror the HL label targets set in the ASM before jp .returnToHL.
 * ============================================================ */
#define PSTAT_CAN_MOVE    1   /* Z=1: proceed normally */
#define PSTAT_DONE        0   /* ExecutePlayerMoveDone / ExecuteEnemyMoveDone */
#define PSTAT_MISSED      2   /* HandleIfPlayerMoveMissed — Bide unleash */
#define PSTAT_CALC_DMG    3   /* PlayerCalcMoveDamage — Thrash */
#define PSTAT_GET_ANIM    4   /* GetPlayerAnimationType — Trapping continuation */
#define PSTAT_RAGE        5   /* PlayerCanExecuteMove — Rage (effect already 0) */

/* Status message tracking — set by check_*_status_conditions,
 * read by battle_ui.c via Battle_Get*StatusMsg(). */
static battle_status_msg_t s_player_status_msg = BSTAT_MSG_NONE;
static battle_status_msg_t s_player_pre_msg    = BSTAT_MSG_NONE;
static battle_status_msg_t s_enemy_status_msg  = BSTAT_MSG_NONE;
static battle_status_msg_t s_enemy_pre_msg     = BSTAT_MSG_NONE;

/* ============================================================
 * check_player_status_conditions — CheckPlayerStatusConditions (core.asm:3328)
 * ============================================================ */
static int check_player_status_conditions(void) {
    s_player_status_msg = BSTAT_MSG_NONE;
    s_player_pre_msg    = BSTAT_MSG_NONE;

    /* --- Sleep (core.asm:3330-3353) --- */
    uint8_t slp = wBattleMon.status & STATUS_SLP_MASK;
    if (slp) {
        slp--;
        wBattleMon.status = (uint8_t)((wBattleMon.status & ~STATUS_SLP_MASK) | slp);
        s_player_status_msg = (slp == 0) ? BSTAT_MSG_WOKE_UP : BSTAT_MSG_FAST_ASLEEP;
        wPlayerUsedMove = 0;
        return PSTAT_DONE;
    }

    /* --- Frozen (core.asm:3355-3363) --- */
    if (wBattleMon.status & STATUS_FRZ) {
        s_player_status_msg = BSTAT_MSG_FROZEN;
        wPlayerUsedMove = 0;
        return PSTAT_DONE;
    }

    /* --- HeldInPlace by enemy trapping move (core.asm:3365-3372) --- */
    if (wEnemyBattleStatus1 & (1u << BSTAT1_USING_TRAPPING)) {
        s_player_status_msg = BSTAT_MSG_CANT_MOVE;
        return PSTAT_DONE;
    }

    /* --- Flinched (core.asm:3374-3382) --- */
    if (wPlayerBattleStatus1 & (1u << BSTAT1_FLINCHED)) {
        wPlayerBattleStatus1 &= ~(1u << BSTAT1_FLINCHED);
        s_player_status_msg = BSTAT_MSG_FLINCHED;
        return PSTAT_DONE;
    }

    /* --- Hyper Beam recharge (core.asm:3384-3392) --- */
    if (wPlayerBattleStatus2 & (1u << BSTAT2_NEEDS_TO_RECHARGE)) {
        wPlayerBattleStatus2 &= ~(1u << BSTAT2_NEEDS_TO_RECHARGE);
        s_player_status_msg = BSTAT_MSG_MUST_RECHARGE;
        return PSTAT_DONE;
    }

    /* --- Disable counter decrement (core.asm:3394-3406) --- */
    if (wPlayerDisabledMove) {
        wPlayerDisabledMove--;
        if ((wPlayerDisabledMove & 0x0F) == 0) {
            wPlayerDisabledMove = 0;
            wPlayerDisabledMoveNumber = 0;
            s_player_pre_msg = BSTAT_MSG_DISABLED_NO_MORE;
        }
    }

    /* --- Confusion (core.asm:3408-3435) --- */
    if (wPlayerBattleStatus1 & (1u << BSTAT1_CONFUSED)) {
        wPlayerConfusedCounter--;
        if (wPlayerConfusedCounter == 0) {
            wPlayerBattleStatus1 &= ~(1u << BSTAT1_CONFUSED);
            s_player_pre_msg = BSTAT_MSG_CONFUSED_NO_MORE;
        } else {
            /* 50% chance to hurt self (cp 50 percent + 1 = cp 128) */
            if (BattleRandom() >= 128) {
                /* Hurt self: keep only CONFUSED bit, apply self-damage */
                wPlayerBattleStatus1 &= (1u << BSTAT1_CONFUSED);
                handle_self_confusion_damage();
                s_player_status_msg = BSTAT_MSG_HURT_ITSELF;
                goto mon_hurt_itself;
            }
            s_player_pre_msg = BSTAT_MSG_IS_CONFUSED;
        }
    }

    /* --- Tried to use disabled move (core.asm:3437-3447) --- */
    if (wPlayerDisabledMoveNumber &&
        wPlayerDisabledMoveNumber == wPlayerSelectedMove) {
        return PSTAT_DONE;
    }

    /* --- Paralysis (core.asm:3449-3457) --- */
    if (wBattleMon.status & STATUS_PAR) {
        if (BattleRandom() < 64) {   /* 25 percent = 63: jr nc → skip if >= 63 */
            s_player_status_msg = BSTAT_MSG_FULLY_PARALYZED;
            goto mon_hurt_itself;
        }
    }

    /* --- Bide accumulation / release (core.asm:3481-3529) --- */
    if (wPlayerBattleStatus1 & (1u << BSTAT1_STORING_ENERGY)) {
        wPlayerMoveNum = 0;
        /* Accumulate wDamage into bide total */
        wPlayerBideAccumulatedDamage += wDamage;
        wPlayerNumAttacksLeft--;
        if (wPlayerNumAttacksLeft > 0) return PSTAT_DONE;
        /* Unleash: double accumulated damage */
        wPlayerBattleStatus1 &= ~(1u << BSTAT1_STORING_ENERGY);
        uint16_t bide_dmg = wPlayerBideAccumulatedDamage * 2;
        wPlayerBideAccumulatedDamage = 0;
        wDamage = bide_dmg;
        if (bide_dmg == 0) wMoveMissed = 1;
        wPlayerMoveNum = MOVE_BIDE;
        return PSTAT_MISSED;  /* HandleIfPlayerMoveMissed */
    }

    /* --- Thrash continuation (core.asm:3531-3552) --- */
    if (wPlayerBattleStatus1 & (1u << BSTAT1_THRASHING_ABOUT)) {
        wPlayerMoveNum = MOVE_THRASH;
        wPlayerNumAttacksLeft--;
        if (wPlayerNumAttacksLeft > 0) return PSTAT_CALC_DMG;
        /* Counter hit 0: become confused */
        wPlayerBattleStatus1 &= ~(1u << BSTAT1_THRASHING_ABOUT);
        wPlayerBattleStatus1 |= (1u << BSTAT1_CONFUSED);
        uint8_t ctr = (BattleRandom() & 3) + 2;   /* 2-5 turns */
        wPlayerConfusedCounter = ctr;
        return PSTAT_CALC_DMG;
    }

    /* --- Trapping move continuation (core.asm:3554-3566) --- */
    if (wPlayerBattleStatus1 & (1u << BSTAT1_USING_TRAPPING)) {
        wPlayerNumAttacksLeft--;
        return PSTAT_GET_ANIM;
    }

    /* --- Rage (core.asm:3568-3579) --- */
    if (wPlayerBattleStatus2 & (1u << BSTAT2_USING_RAGE)) {
        wPlayerMoveEffect = 0;
        return PSTAT_RAGE;
    }

    return PSTAT_CAN_MOVE;

mon_hurt_itself:
    /* Clear BIDE/THRASH/CHARGING_UP/TRAPPING bits (core.asm:3463) */
    wPlayerBattleStatus1 &= ~((1u << BSTAT1_STORING_ENERGY) |
                               (1u << BSTAT1_THRASHING_ABOUT) |
                               (1u << BSTAT1_CHARGING_UP) |
                               (1u << BSTAT1_USING_TRAPPING));
    /* Note: INVULNERABLE is intentionally NOT cleared here (Gen 1 bug:
     * fully-paralyzed/confused mon during Fly/Dig stays invulnerable). */
    return PSTAT_DONE;
}

/* ============================================================
 * check_enemy_status_conditions — CheckEnemyStatusConditions (core.asm:5677)
 * ============================================================ */
static int check_enemy_status_conditions(void) {
    s_enemy_status_msg = BSTAT_MSG_NONE;
    s_enemy_pre_msg    = BSTAT_MSG_NONE;

    /* --- Sleep (core.asm:5678-5685) --- */
    uint8_t slp = wEnemyMon.status & STATUS_SLP_MASK;
    if (slp) {
        slp--;
        wEnemyMon.status = (uint8_t)((wEnemyMon.status & ~STATUS_SLP_MASK) | slp);
        s_enemy_status_msg = (slp == 0) ? BSTAT_MSG_WOKE_UP : BSTAT_MSG_FAST_ASLEEP;
        wEnemyUsedMove = 0;
        return PSTAT_DONE;
    }

    /* --- Frozen (core.asm:5705-5713) --- */
    if (wEnemyMon.status & STATUS_FRZ) {
        s_enemy_status_msg = BSTAT_MSG_FROZEN;
        wEnemyUsedMove = 0;
        return PSTAT_DONE;
    }

    /* --- HeldInPlace by player trapping (core.asm:5715-5723) --- */
    if (wPlayerBattleStatus1 & (1u << BSTAT1_USING_TRAPPING)) {
        s_enemy_status_msg = BSTAT_MSG_CANT_MOVE;
        return PSTAT_DONE;
    }

    /* --- Flinched (core.asm:5725-5733) --- */
    if (wEnemyBattleStatus1 & (1u << BSTAT1_FLINCHED)) {
        wEnemyBattleStatus1 &= ~(1u << BSTAT1_FLINCHED);
        s_enemy_status_msg = BSTAT_MSG_FLINCHED;
        return PSTAT_DONE;
    }

    /* --- Hyper Beam recharge (core.asm:5735-5743) --- */
    if (wEnemyBattleStatus2 & (1u << BSTAT2_NEEDS_TO_RECHARGE)) {
        wEnemyBattleStatus2 &= ~(1u << BSTAT2_NEEDS_TO_RECHARGE);
        s_enemy_status_msg = BSTAT_MSG_MUST_RECHARGE;
        return PSTAT_DONE;
    }

    /* --- Disable counter decrement (core.asm:5745-5757) --- */
    if (wEnemyDisabledMove) {
        wEnemyDisabledMove--;
        if ((wEnemyDisabledMove & 0x0F) == 0) {
            wEnemyDisabledMove = 0;
            wEnemyDisabledMoveNumber = 0;
            s_enemy_pre_msg = BSTAT_MSG_DISABLED_NO_MORE;
        }
    }

    /* --- Confusion (core.asm:5759-5800) --- */
    if (wEnemyBattleStatus1 & (1u << BSTAT1_CONFUSED)) {
        wEnemyConfusedCounter--;
        if (wEnemyConfusedCounter == 0) {
            wEnemyBattleStatus1 &= ~(1u << BSTAT1_CONFUSED);
            s_enemy_pre_msg = BSTAT_MSG_CONFUSED_NO_MORE;
        } else {
            /* Enemy uses cp $80 = 128 threshold (core.asm:5784) */
            if (BattleRandom() >= 0x80) {
                /* Hurt self: keep only CONFUSED bit */
                wEnemyBattleStatus1 &= (1u << BSTAT1_CONFUSED);
                /* Enemy confusion self-damage uses GetDamageVarsForEnemyAttack.
                 * Temporarily swap levels, zero effect, set power=40/type=0. */
                uint16_t saved_player_def = wBattleMon.def;
                wBattleMon.def = wEnemyMon.def;
                uint8_t saved_effect = wEnemyMoveEffect;
                uint8_t saved_power  = wEnemyMovePower;
                uint8_t saved_type   = wEnemyMoveType;
                wEnemyMoveEffect = 0;
                wEnemyMovePower  = 40;
                wEnemyMoveType   = 0;
                wCriticalHitOrOHKO = 0;
                swap_player_enemy_levels();
                Battle_GetDamageVarsForEnemyAttack();
                swap_player_enemy_levels();
                wEnemyMoveEffect = saved_effect;
                wEnemyMovePower  = saved_power;
                wEnemyMoveType   = saved_type;
                wBattleMon.def = saved_player_def;
                apply_damage_to_enemy_pokemon();
                s_enemy_status_msg = BSTAT_MSG_HURT_ITSELF;
                goto enemy_hurt_itself;
            }
            s_enemy_pre_msg = BSTAT_MSG_IS_CONFUSED;
        }
    }

    /* --- Tried to use disabled move (core.asm:5802-5814) --- */
    if (wEnemyDisabledMoveNumber &&
        wEnemyDisabledMoveNumber == wEnemySelectedMove) {
        return PSTAT_DONE;
    }

    /* --- Paralysis (core.asm:5816-5826) --- */
    if (wEnemyMon.status & STATUS_PAR) {
        if (BattleRandom() < 64) {
            s_enemy_status_msg = BSTAT_MSG_FULLY_PARALYZED;
            goto enemy_hurt_itself;
        }
    }

    /* --- Bide (core.asm:5856-5905) --- */
    if (wEnemyBattleStatus1 & (1u << BSTAT1_STORING_ENERGY)) {
        wEnemyMoveNum = 0;
        wEnemyBideAccumulatedDamage += wDamage;
        wEnemyNumAttacksLeft--;
        if (wEnemyNumAttacksLeft > 0) return PSTAT_DONE;
        wEnemyBattleStatus1 &= ~(1u << BSTAT1_STORING_ENERGY);
        uint16_t bide_dmg = wEnemyBideAccumulatedDamage * 2;
        wEnemyBideAccumulatedDamage = 0;
        wDamage = bide_dmg;
        if (bide_dmg == 0) wMoveMissed = 1;
        wEnemyMoveNum = MOVE_BIDE;
        swap_player_enemy_levels();   /* core.asm:5903 */
        return PSTAT_MISSED;
    }

    /* --- Thrash (core.asm:5906-5927) --- */
    if (wEnemyBattleStatus1 & (1u << BSTAT1_THRASHING_ABOUT)) {
        wEnemyMoveNum = MOVE_THRASH;
        wEnemyNumAttacksLeft--;
        if (wEnemyNumAttacksLeft > 0) return PSTAT_CALC_DMG;
        wEnemyBattleStatus1 &= ~(1u << BSTAT1_THRASHING_ABOUT);
        wEnemyBattleStatus1 |= (1u << BSTAT1_CONFUSED);
        wEnemyConfusedCounter = (BattleRandom() & 3) + 2;
        return PSTAT_CALC_DMG;
    }

    /* --- Trapping continuation (core.asm:5928-5939) --- */
    if (wEnemyBattleStatus1 & (1u << BSTAT1_USING_TRAPPING)) {
        wEnemyNumAttacksLeft--;
        return PSTAT_GET_ANIM;
    }

    /* --- Rage (core.asm:5940-5951) --- */
    if (wEnemyBattleStatus2 & (1u << BSTAT2_USING_RAGE)) {
        wEnemyMoveEffect = 0;
        return PSTAT_RAGE;
    }

    return PSTAT_CAN_MOVE;

enemy_hurt_itself:
    wEnemyBattleStatus1 &= ~((1u << BSTAT1_STORING_ENERGY) |
                              (1u << BSTAT1_THRASHING_ABOUT) |
                              (1u << BSTAT1_CHARGING_UP) |
                              (1u << BSTAT1_USING_TRAPPING));
    return PSTAT_DONE;
}

/* ============================================================
 * check_for_disobedience — CheckForDisobedience (core.asm:3830)
 *
 * Returns 1 if mon can use its move, 0 if it disobeys.
 * Skips the useRandomMove path (requires menu state not available
 * in pure-state context — treated as monDoesNothing).
 * ============================================================ */
static int check_for_disobedience(void) {
    wMonIsDisobedient = 0;

    /* Link battle: always obey (core.asm:3833-3838) */
    if (wLinkState == LINK_STATE_BATTLING) return 1;

    /* Check if mon is owned (same OT ID as player) */
    if (wPartyMons[wPlayerMonNumber].base.ot_id == wPlayerID) return 1;

    /* Mon is traded: determine obedience threshold from badge count */
    uint8_t threshold;
    if (wObtainedBadges & (1u << BIT_EARTHBADGE))   { threshold = 101; }
    else if (wObtainedBadges & (1u << BIT_MARSHBADGE))   { threshold = 70;  }
    else if (wObtainedBadges & (1u << BIT_RAINBOWBADGE)) { threshold = 50;  }
    else if (wObtainedBadges & (1u << BIT_CASCADEBADGE)) { threshold = 30;  }
    else                                                   { threshold = 10;  }

    uint8_t level = wBattleMon.level;
    if (level < threshold) return 1;   /* low enough level: always obeys */

    /* Two-loop RNG to decide disobedience tier (core.asm:3882-3932).
     * b = threshold + level (capped at 0xFF) */
    uint8_t b = (uint8_t)((threshold + (uint16_t)level > 0xFF) ? 0xFF
                            : (threshold + level));

    /* loop1: get random < b (core.asm:3882-3886) */
    uint8_t r1;
    do { r1 = (uint8_t)((BattleRandom() >> 4) | (BattleRandom() << 4)); }
    while (r1 >= b);

    /* If r1 < threshold: mon can use its move (core.asm:3887-3888) */
    if (r1 < threshold) return 1;

    /* loop2: get another random < b (core.asm:3889-3894) */
    uint8_t r2;
    do { r2 = BattleRandom(); } while (r2 >= b);

    if (r2 < threshold) {
        /* useRandomMove — requires menu state; fallback: do nothing */
        wMonIsDisobedient = 1;
        return 0;
    }

    /* Decide disobedience type (core.asm:3895-3907) */
    uint8_t diff = level - threshold;
    uint8_t r3   = (uint8_t)((BattleRandom() >> 4) | (BattleRandom() << 4));
    if ((int8_t)(r3 - diff) < 0) {
        /* Nap: random sleep 1-7 turns (core.asm:3908-3915) */
        uint8_t sleep_ctr;
        do {
            sleep_ctr = (uint8_t)(((BattleRandom() << 1) >> 4) & STATUS_SLP_MASK);
        } while (sleep_ctr == 0);
        wBattleMon.status = sleep_ctr;
        return 0;
    }
    if (r3 >= (uint8_t)(diff + b)) {
        /* MonDoesNothing (loafing/ignored) */
        return 0;
    }
    /* Won't obey: self-confusion damage */
    handle_self_confusion_damage();
    return 0;
}

/* ============================================================
 * get_current_move — GetCurrentMove (core.asm:5960)
 *
 * Loads move data from the selected move slot into the active-move
 * variables (wPlayerMoveNum/Effect/Power/Type/Acc/MaxPP or enemy equiv).
 * In the original, this is done via a predef lookup; here we read
 * gMoves[] directly.
 * ============================================================ */
static void get_current_move(void) {
    extern const move_t gMoves[];   /* data/moves_data.c */
    if (hWhoseTurn == 0) {
        uint8_t move_id = wPlayerSelectedMove;
        wPlayerMoveNum      = move_id;
        wPlayerMoveEffect   = gMoves[move_id].effect;
        wPlayerMovePower    = gMoves[move_id].power;
        wPlayerMoveType     = gMoves[move_id].type;
        wPlayerMoveAccuracy = gMoves[move_id].accuracy;
        wPlayerMoveMaxPP    = gMoves[move_id].pp;
    } else {
        uint8_t move_id = wEnemySelectedMove;
        wEnemyMoveNum      = move_id;
        wEnemyMoveEffect   = gMoves[move_id].effect;
        wEnemyMovePower    = gMoves[move_id].power;
        wEnemyMoveType     = gMoves[move_id].type;
        wEnemyMoveAccuracy = gMoves[move_id].accuracy;
        wEnemyMoveMaxPP    = gMoves[move_id].pp;
    }
}

/* ============================================================
 * decrement_pp — DecrementPP (core.asm:3133 / core.asm:5540)
 *
 * Decrements current PP for the move in the active slot.
 * Only touches the low 6 bits (bits 6-7 = PP_UP count, preserved).
 * Called after get_current_move(), gated by the CHARGING_UP check so
 * two-turn moves (Fly, Solar Beam) only lose PP on the first turn.
 * Never call for STRUGGLE — caller must guard with a move-ID check.
 * ============================================================ */
static void decrement_pp(void) {
    uint8_t  slot;
    uint8_t *pp;
    if (hWhoseTurn == 0) {
        /* Player: wPlayerMoveListIndex is set by DisplayBattleMenu (UI).
         * Until UI is implemented it defaults to 0, so pp[0] is decremented. */
        slot = wPlayerMoveListIndex;
        pp   = wBattleMon.pp;
    } else {
        slot = wEnemyMoveListIndex;   /* set by Battle_SelectEnemyMove */
        pp   = wEnemyMon.pp;
    }
    if (slot < 4 && (pp[slot] & 0x3F) > 0)
        pp[slot]--;
}

/* ============================================================
 * Battle_ExecutePlayerMove — ExecutePlayerMove (core.asm:3073)
 *
 * ASM flow (UI calls omitted in C):
 *   PlayerCalcMoveDamage (3134) → MoveHitTest (3149)
 *   HandleIfPlayerMoveMissed (3151):
 *     miss (non-EXPLODE) → PlayerCheckIfFlyOrChargeEffect (3184)
 *     hit / EXPLODE-miss → GetPlayerAnimationType (UI) → MirrorMoveCheck (3198)
 *   PlayerCheckIfFlyOrChargeEffect (3184):
 *     animation (UI) → MirrorMoveCheck (3198)
 *   MirrorMoveCheck (3198) → .next (3212) → ResidualEffects2 (3213)
 *     → wMoveMissed check (3218) → ApplyAttack (3226) → .notDone (3232)
 *     → AlwaysHappenSideEffects (3233) → multi-hit → SpecialEffects → Done
 * ============================================================ */
void Battle_ExecutePlayerMove(void) {
    hWhoseTurn = 0;

    /* CANNOT_MOVE: wPlayerSelectedMove == 0xFF (core.asm:3077-3079) */
    if (wPlayerSelectedMove == CANNOT_MOVE) goto execute_done;

    wMoveMissed       = 0;
    wMonIsDisobedient = 0;
    wMoveDidntMiss    = 0;
    wDamageMultipliers = DAMAGE_MULT_EFFECTIVE;

    /* Turn already taken (core.asm:3086-3088) */
    if (wActionResultOrTookBattleTurn) goto execute_done;

    /* CheckPlayerStatusConditions (core.asm:3091-3093) */
    {
        int sr = check_player_status_conditions();
        if (sr == PSTAT_DONE)     goto execute_done;
        if (sr == PSTAT_MISSED)   goto handle_if_player_move_missed;
        if (sr == PSTAT_CALC_DMG) goto player_calc_damage;
        if (sr == PSTAT_GET_ANIM) {
            /* Trapping continuation (GetPlayerAnimationType):
             * attack already set up; skip calc+hit test, go to .notDone */
            apply_attack_to_enemy_pokemon();
            wMoveDidntMiss = 1;
            goto execute_after_apply;
        }
        if (sr == PSTAT_RAGE)     goto player_can_execute_move;
        /* PSTAT_CAN_MOVE: fall through */
    }

    /* GetCurrentMove — load move data (core.asm:3095) */
    get_current_move();

    /* CHARGING_UP: skip to PlayerCanExecuteChargingMove (core.asm:3097-3098) */
    if (wPlayerBattleStatus1 & (1u << BSTAT1_CHARGING_UP)) {
        wPlayerBattleStatus1 &= ~((1u << BSTAT1_CHARGING_UP) |
                                   (1u << BSTAT1_INVULNERABLE));
        goto player_can_execute_move;
    }

    /* DecrementPP (core.asm:3133) — not called for Struggle or charging 2nd turn */
    if (wPlayerSelectedMove != MOVE_STRUGGLE)
        decrement_pp();

    /* CheckForDisobedience (core.asm:3099-3100) */
    if (!check_for_disobedience()) goto execute_done;

    /* CheckIfPlayerNeedsToChargeUp (core.asm:3102-3108) */
check_charge:
    if (wPlayerMoveEffect == EFFECT_CHARGE || wPlayerMoveEffect == EFFECT_FLY) {
        Battle_JumpMoveEffect();
        goto execute_done;
    }

player_can_execute_move:
    /* ResidualEffects1 (core.asm:3123-3127) */
    if (is_in_array(wPlayerMoveEffect, kResidualEffects1)) {
        Battle_JumpMoveEffect();
        goto execute_done;
    }

    /* SpecialEffectsCont — call but don't skip (core.asm:3130-3133) */
    if (is_in_array(wPlayerMoveEffect, kSpecialEffectsCont)) {
        Battle_JumpMoveEffect();
    }

player_calc_damage:
    /* SetDamageEffects — skip damage calc, jump to MoveHitTest (core.asm:3135-3139) */
    if (is_in_array(wPlayerMoveEffect, kSetDamageEffects)) {
        goto player_move_hit_test;
    }

    Battle_CriticalHitTest();

    /* HandleCounterMove (core.asm:3141-3142): Z=set means IS Counter → HandleIfPlayerMoveMissed */
    if (!handle_counter_move()) goto handle_if_player_move_missed;

    /* GetDamageVarsForPlayerAttack + CalculateDamage (core.asm:3143-3145):
     * Z set (power=0) → PlayerCheckIfFlyOrChargeEffect → MirrorMoveCheck */
    if (!Battle_GetDamageVarsForPlayerAttack()) goto player_check_fly_charge;
    Battle_AdjustDamageForMoveType();
    Battle_RandomizeDamage();

player_move_hit_test:
    Battle_MoveHitTest();

handle_if_player_move_missed:
    /* core.asm:3151 HandleIfPlayerMoveMissed */
    if (wMoveMissed && wPlayerMoveEffect != EFFECT_EXPLODE) {
        /* Normal miss → PlayerCheckIfFlyOrChargeEffect → MirrorMoveCheck */
        goto player_check_fly_charge;
    }
    /* Hit, or EXPLODE-miss: GetPlayerAnimationType (UI omitted) → MirrorMoveCheck */
    /* fall through */

player_check_fly_charge:
    /* core.asm:3184 PlayerCheckIfFlyOrChargeEffect — animation omitted */
    /* fall through to MirrorMoveCheck */

player_mirror_move_check:
    /* core.asm:3198 MirrorMoveCheck */
    if (wPlayerMoveEffect == EFFECT_MIRROR_MOVE) {
        if (wEnemyUsedMove != 0) {
            wPlayerSelectedMove = wEnemyUsedMove;
            get_current_move();
            wMonIsDisobedient = 0;
            goto check_charge;
        }
        goto execute_done;
    }
    if (wPlayerMoveEffect == EFFECT_METRONOME) {
        /* MetronomePickMove (core.asm:5013): random move, not 0/Struggle/Metronome */
        uint8_t pick;
        do { pick = BattleRandom(); }
        while (pick == 0 || pick >= MOVE_STRUGGLE || pick == 118 /* MOVE_METRONOME */);
        wPlayerSelectedMove = pick;
        get_current_move();
        wMonIsDisobedient = 0;
        goto check_charge;
    }

    /* core.asm:3213 .next — ResidualEffects2 (stat mods, sleep, bide) */
    if (is_in_array(wPlayerMoveEffect, kResidualEffects2)) {
        Battle_JumpMoveEffect();
        goto execute_done;
    }

    /* core.asm:3218 wMoveMissed re-check */
    if (wMoveMissed) {
        BLOG("  %s used %s -- missed!", BMON_P(), BMOVE(wPlayerMoveNum));
        if (wPlayerMoveEffect != EFFECT_EXPLODE) goto execute_done;
    } else {
        BLOG("  %s used %s", BMON_P(), BMOVE(wPlayerMoveNum));
        wPlayerUsedMove = wPlayerMoveNum;  /* DisplayUsedMoveText (used_move_text.asm) */
        /* Move hit: apply attack (core.asm:3226-3231) */
        apply_attack_to_enemy_pokemon();
        wMoveDidntMiss = 1;
    }
    /* .notDone falls through to execute_after_apply */

execute_after_apply:
    /* AlwaysHappenSideEffects (core.asm:3233-3237) */
    if (is_in_array(wPlayerMoveEffect, kAlwaysHappenSideEffects)) {
        Battle_JumpMoveEffect();
    }

    /* Check if enemy fainted (core.asm:3238-3242) */
    if (wEnemyMon.hp == 0) goto execute_done;

    handle_building_rage();

    /* Multi-hit loop (core.asm:3245-3257):
     * ASM jumps back to GetPlayerAnimationType → MirrorMoveCheck on each hit.
     * C simplification: skip Mirror/Metronome/ResidualEffects2 (can't fire mid-multihit)
     * and go directly to execute_after_apply to re-apply damage. */
    if (wPlayerBattleStatus1 & (1u << BSTAT1_ATTACKING_MULTIPLE)) {
        wPlayerNumAttacksLeft--;
        if (wPlayerNumAttacksLeft > 0) {
            apply_attack_to_enemy_pokemon();
            if (wEnemyMon.hp == 0) {
                wPlayerBattleStatus1 &= ~(1u << BSTAT1_ATTACKING_MULTIPLE);
                goto execute_done;
            }
            handle_building_rage();
            goto execute_after_apply;
        }
        wPlayerBattleStatus1 &= ~(1u << BSTAT1_ATTACKING_MULTIPLE);
        wPlayerNumHits = 0;
    }

    if (wPlayerMoveEffect == 0) goto execute_done;

    /* SpecialEffects — final effect call (core.asm:3262-3266) */
    if (!is_in_array(wPlayerMoveEffect, kSpecialEffects)) {
        Battle_JumpMoveEffect();
    }

    goto execute_done;

execute_done:
    wActionResultOrTookBattleTurn = 0;
}

/* ============================================================
 * Battle_ExecuteEnemyMove — ExecuteEnemyMove (core.asm:5457)
 *
 * ASM flow mirrors the player version but with SwapPlayerAndEnemyLevels
 * calls wrapping damage calc so Battle_CalcDamage always uses wBattleMon.level:
 *   EnemyCalcMoveDamage (5524): swap → ... calc ... → swap (net: calc uses enemy level)
 *   Normal hit (5553): swap → unswapped → GetEnemyAnimationType (UI) → EnemyCheckIfMirrorMoveEffect
 *   Normal miss (5551): → EnemyCheckIfFlyOrChargeEffect (5585): swap → unswapped → mirror check
 *   EXPLODE miss (5563): swap → unswapped → animation → mirror check
 *   Power=0 (5538): → EnemyCheckIfFlyOrChargeEffect (5585): swap → unswapped → mirror check
 *   All paths arrive at EnemyCheckIfMirrorMoveEffect (5600) with levels unswapped.
 * ============================================================ */
void Battle_ExecuteEnemyMove(void) {
    if (wEnemySelectedMove == CANNOT_MOVE) goto enemy_execute_done;

    wMoveMissed    = 0;
    wMoveDidntMiss = 0;
    wDamageMultipliers = DAMAGE_MULT_EFFECTIVE;

    /* CheckEnemyStatusConditions (core.asm:5481-5483) */
    {
        int sr = check_enemy_status_conditions();
        if (sr == PSTAT_DONE)     goto enemy_execute_done;
        if (sr == PSTAT_MISSED)   goto enemy_handle_if_missed;
        if (sr == PSTAT_CALC_DMG) goto enemy_calc_damage;
        if (sr == PSTAT_GET_ANIM) {
            /* Trapping continuation (GetEnemyAnimationType → EnemyCheckIfMirrorMoveEffect):
             * no level swaps on this path — apply directly to player, skip to .notDone */
            apply_attack_to_player_pokemon();
            wMoveDidntMiss = 1;
            goto enemy_after_apply;
        }
        if (sr == PSTAT_RAGE)     goto enemy_can_execute_move;
        /* PSTAT_CAN_MOVE: fall through */
    }

    /* CHARGING_UP (core.asm:5486-5487) */
    if (wEnemyBattleStatus1 & (1u << BSTAT1_CHARGING_UP)) {
        wEnemyBattleStatus1 &= ~((1u << BSTAT1_CHARGING_UP) |
                                  (1u << BSTAT1_INVULNERABLE));
        goto enemy_can_execute_move;
    }

    get_current_move();   /* fills wEnemyMove* from wEnemySelectedMove */

    /* DecrementPP (core.asm:5540) — not called for Struggle or charging 2nd turn
     * (CHARGING_UP path jumps over get_current_move + this block entirely). */
    if (wEnemySelectedMove != MOVE_STRUGGLE)
        decrement_pp();

    /* CheckIfEnemyNeedsToChargeUp (core.asm:5490-5496) */
enemy_check_charge:
    if (wEnemyMoveEffect == EFFECT_CHARGE || wEnemyMoveEffect == EFFECT_FLY) {
        Battle_JumpMoveEffect();
        goto enemy_execute_done;
    }

enemy_can_execute_move:
    wMonIsDisobedient = 0;

    /* ResidualEffects1 (core.asm:5514-5518) */
    if (is_in_array(wEnemyMoveEffect, kResidualEffects1)) {
        Battle_JumpMoveEffect();
        goto enemy_execute_done;
    }

    /* SpecialEffectsCont (core.asm:5520-5523) */
    if (is_in_array(wEnemyMoveEffect, kSpecialEffectsCont)) {
        Battle_JumpMoveEffect();
    }

enemy_calc_damage:
    swap_player_enemy_levels();   /* core.asm:5525 — swap 1 */

    /* SetDamageEffects (core.asm:5527-5530) */
    if (is_in_array(wEnemyMoveEffect, kSetDamageEffects)) {
        goto enemy_move_hit_test;   /* still swapped (1 swap) */
    }

    Battle_CriticalHitTest();

    /* HandleCounterMove (core.asm:5532-5533): Z=set → HandleIfEnemyMoveMissed.
     * Levels are swapped (1 swap) — NO extra swap here unlike original C code. */
    if (!handle_counter_move()) goto enemy_handle_if_missed;

    swap_player_enemy_levels();           /* core.asm:5534 — swap 2 (unswapped) */
    Battle_GetDamageVarsForEnemyAttack(); /* core.asm:5535 */
    swap_player_enemy_levels();           /* core.asm:5536 — swap 3 (swapped) */

    /* CalculateDamage Z=set (power=0) → EnemyCheckIfFlyOrChargeEffect (5538):
     * levels are swapped (3 swaps); EnemyCheckIfFlyOrChargeEffect will unswap. */
    if (!wDamage) goto enemy_check_fly_charge;
    Battle_AdjustDamageForMoveType();
    Battle_RandomizeDamage();

enemy_move_hit_test:
    Battle_MoveHitTest();

enemy_handle_if_missed:
    /* core.asm:5544 HandleIfEnemyMoveMissed */
    if (wMoveMissed) {
        if (wEnemyMoveEffect == EFFECT_EXPLODE) {
            /* HandleExplosionMiss (5563): swap → unswapped → animation → mirror check */
            swap_player_enemy_levels();
            goto enemy_mirror_move_check;
        }
        /* Normal miss → EnemyCheckIfFlyOrChargeEffect (5551) */
        goto enemy_check_fly_charge;
    }
    /* Hit: swap → unswapped (5553), animation omitted → mirror check directly */
    swap_player_enemy_levels();
    goto enemy_mirror_move_check;

enemy_check_fly_charge:
    /* core.asm:5585 EnemyCheckIfFlyOrChargeEffect:
     * called when levels are swapped (odd swap count) — this swap restores them. */
    swap_player_enemy_levels();   /* core.asm:5586 — unswaps */
    /* animation omitted — fall through to EnemyCheckIfMirrorMoveEffect */

enemy_mirror_move_check:
    /* core.asm:5600 EnemyCheckIfMirrorMoveEffect — levels always unswapped here */
    if (wEnemyMoveEffect == EFFECT_MIRROR_MOVE) {
        if (wPlayerUsedMove != 0) {
            wEnemySelectedMove = wPlayerUsedMove;
            get_current_move();
            goto enemy_check_charge;
        }
        goto enemy_execute_done;
    }
    if (wEnemyMoveEffect == EFFECT_METRONOME) {
        uint8_t pick;
        do { pick = BattleRandom(); }
        while (pick == 0 || pick >= MOVE_STRUGGLE || pick == 118 /* MOVE_METRONOME */);
        wEnemySelectedMove = pick;
        get_current_move();
        goto enemy_check_charge;
    }

    /* core.asm:5613 .notMetronomeEffect — ResidualEffects2 */
    if (is_in_array(wEnemyMoveEffect, kResidualEffects2)) {
        Battle_JumpMoveEffect();
        goto enemy_execute_done;
    }

    /* core.asm:5618 wMoveMissed re-check */
    if (wMoveMissed) {
        BLOG("  %s used %s -- missed!", BMON_E(), BMOVE(wEnemyMoveNum));
        if (wEnemyMoveEffect != EFFECT_EXPLODE) goto enemy_execute_done;
    } else {
        BLOG("  %s used %s", BMON_E(), BMOVE(wEnemyMoveNum));
        wEnemyUsedMove = wEnemyMoveNum;  /* DisplayUsedMoveText (used_move_text.asm) */
        /* Move hit: apply attack to player (core.asm:5627-5631) */
        apply_attack_to_player_pokemon();
        wMoveDidntMiss = 1;
    }
    /* .handleExplosionMiss / .notDone falls through */

enemy_after_apply:
    /* AlwaysHappenSideEffects (core.asm:5633-5637) */
    if (is_in_array(wEnemyMoveEffect, kAlwaysHappenSideEffects)) {
        Battle_JumpMoveEffect();
    }

    if (wBattleMon.hp == 0) goto enemy_execute_done;

    handle_building_rage();

    /* Multi-hit (core.asm:5644-5656): ASM jumps back to GetEnemyAnimationType
     * → EnemyCheckIfMirrorMoveEffect per hit; C goes to enemy_after_apply directly. */
    if (wEnemyBattleStatus1 & (1u << BSTAT1_ATTACKING_MULTIPLE)) {
        wEnemyNumAttacksLeft--;
        if (wEnemyNumAttacksLeft > 0) {
            apply_attack_to_player_pokemon();
            if (wBattleMon.hp == 0) {
                wEnemyBattleStatus1 &= ~(1u << BSTAT1_ATTACKING_MULTIPLE);
                goto enemy_execute_done;
            }
            handle_building_rage();
            goto enemy_after_apply;
        }
        wEnemyBattleStatus1 &= ~(1u << BSTAT1_ATTACKING_MULTIPLE);
        wEnemyNumHits = 0;
    }

    if (wEnemyMoveEffect == 0) goto enemy_execute_done;

    /* SpecialEffects (core.asm:5661-5664) */
    if (!is_in_array(wEnemyMoveEffect, kSpecialEffects)) {
        Battle_JumpMoveEffect();
    }

enemy_execute_done:
    wActionResultOrTookBattleTurn = 0;
}

/* ============================================================
 * poison_decrease_own_hp — HandlePoisonBurnLeechSeed_DecreaseOwnHP (core.asm:550)
 *
 * Decrements the given mon's HP by max_hp/16 (minimum 1),
 * multiplied by toxic counter if BADLY_POISONED.
 * Returns the actual damage dealt (for leech seed transfer).
 * ============================================================ */
static uint16_t poison_decrease_own_hp(battle_mon_t *mon) {
    uint8_t  *bstat3;
    uint8_t  *toxic;

    if (hWhoseTurn == 0) {
        bstat3 = &wPlayerBattleStatus3;
        toxic  = &wPlayerToxicCounter;
    } else {
        bstat3 = &wEnemyBattleStatus3;
        toxic  = &wEnemyToxicCounter;
    }

    /* damage = max_hp / 16, minimum 1 (core.asm:561-571) */
    uint16_t damage = mon->max_hp / 16;
    if (damage == 0) damage = 1;

    /* Badly poisoned: increment counter, multiply damage (core.asm:580-591) */
    if (*bstat3 & (1u << BSTAT3_BADLY_POISONED)) {
        (*toxic)++;
        damage *= *toxic;
    }

    /* Subtract from HP, clamp to 0 (core.asm:593-611) */
    if (mon->hp <= damage) {
        damage = mon->hp;
        mon->hp = 0;
    } else {
        mon->hp -= (uint16_t)damage;
    }
    return damage;
}

/* ============================================================
 * poison_increase_enemy_hp — HandlePoisonBurnLeechSeed_IncreaseEnemyHP (core.asm:618)
 *
 * Adds damage to the opposing mon's HP, capped at max_hp.
 * Opposing mon is: hWhoseTurn==0 → wEnemyMon, hWhoseTurn!=0 → wBattleMon.
 * ============================================================ */
static void poison_increase_enemy_hp(uint16_t damage) {
    battle_mon_t *opp = (hWhoseTurn == 0) ? &wEnemyMon : &wBattleMon;
    uint16_t new_hp = opp->hp + damage;
    if (new_hp > opp->max_hp) new_hp = opp->max_hp;
    opp->hp = new_hp;
}

/* ============================================================
 * Battle_HandlePoisonBurnLeechSeed — core.asm:470
 * ============================================================ */
int Battle_HandlePoisonBurnLeechSeed(void) {
    battle_mon_t *mon = (hWhoseTurn == 0) ? &wBattleMon : &wEnemyMon;

    /* Burn / Poison (core.asm:479-495) */
    if (mon->status & (STATUS_BRN | STATUS_PSN)) {
        poison_decrease_own_hp(mon);
    }

    /* Leech Seed (core.asm:497-523): SEEDED bit is bit 7 of bstat2 */
    uint8_t bstat2 = (hWhoseTurn == 0) ? wPlayerBattleStatus2 : wEnemyBattleStatus2;
    if (bstat2 & (1u << BSTAT2_SEEDED)) {
        uint16_t dmg = poison_decrease_own_hp(mon);
        poison_increase_enemy_hp(dmg);
    }

    /* Return 0 if mon fainted (core.asm:525-527) */
    return (mon->hp != 0) ? 1 : 0;
}

/* ============================================================
 * faint_enemy_pokemon_state — FaintEnemyPokemon state portion (core.asm:732)
 *
 * Clears the battle flags that need resetting when the enemy mon faints.
 * Skips: UI (SlideDownFaintedMonPic, ClearScreenArea), exp award.
 * ============================================================ */
static void faint_enemy_pokemon_state(void) {
    /* Trainer battle: zero HP in enemy party data (core.asm:736-743) so
     * AnyEnemyPokemonAliveCheck sees the slot as fainted. */
    if (wIsInBattle == 2) {
        wEnemyMons[wEnemyMonPartyPos].base.hp = 0;
    }

    /* Bug: only zeroes the HIGH byte of player's bide damage (core.asm:756-757) */
    wPlayerBideAccumulatedDamage &= 0x00FF;

    /* Clear player ATTACKING_MULTIPLE_TIMES (core.asm:745-746) */
    wPlayerBattleStatus1 &= ~(1u << BSTAT1_ATTACKING_MULTIPLE);

    /* Clear all enemy battle statuses (core.asm:758-763) */
    wEnemyBattleStatus1  = 0;
    wEnemyBattleStatus2  = 0;
    wEnemyBattleStatus3  = 0;

    /* Clear enemy disabled move, minimized flag (core.asm:764-766) */
    wEnemyDisabledMove        = 0;
    wEnemyDisabledMoveNumber  = 0;
    wEnemyMonMinimized        = 0;

    /* Clear move tracking (core.asm:767-769) */
    wPlayerUsedMove = 0;
    wEnemyUsedMove  = 0;
}

/* ============================================================
 * remove_fainted_player_mon_state — RemoveFaintedPlayerMon state portion (core.asm:1002)
 * ============================================================ */
static void remove_fainted_player_mon_state(void) {
    /* Sync HP and status from battle copy back to party array.
     * Mirrors ReadPlayerMonCurHPAndStatus (core.asm:1800), called at the top
     * of RemoveFaintedPlayerMon.  Without this, the party menu reads the
     * pre-battle (stale) HP for the fainted mon and won't show "FNT". */
    wPartyMons[wPlayerMonNumber].base.hp     = wBattleMon.hp;     /* = 0 */
    wPartyMons[wPlayerMonNumber].base.status = wBattleMon.status;

    /* Clear enemy ATTACKING_MULTIPLE_TIMES (core.asm:1010) */
    wEnemyBattleStatus1 &= ~(1u << BSTAT1_ATTACKING_MULTIPLE);

    /* Zero BOTH bytes of enemy bide accumulated damage (core.asm:1019-1021) */
    wEnemyBideAccumulatedDamage = 0;

    /* Clear player battle status (core.asm:1022) */
    wBattleMon.status = 0;
    /* Status cleared above — keep party copy in sync */
    wPartyMons[wPlayerMonNumber].base.status = 0;
}

/* ============================================================
 * Battle_HandleEnemyMonFainted — core.asm:699 (full flow, core.asm:699-869)
 *
 * 1. Clear faint flags, award exp.
 * 2. Check AnyPartyAlive → blackout if all player mons gone.
 * 3. Wild battle → wild victory, no replacement.
 * 4. Trainer battle → check AnyEnemyPokemonAlive → trainer victory.
 * 5. Simultaneous faint → set wForcePlayerToChooseMon.
 * 6. ReplaceFaintedEnemyMon.
 * ============================================================ */
void Battle_HandleEnemyMonFainted(void) {
    BLOG("  %s fainted!", BMON_E());
    wInHandlePlayerMonFainted = 0;
    faint_enemy_pokemon_state();

    /* GainExperience (core.asm:840 callfar GainExperience) */
    wBoostExpByExpAll = 0;
    Battle_GainExperience();

    /* AnyPartyAlive check (core.asm:843): if all player mons fainted, blackout */
    if (!Battle_AnyPartyAlive()) {
        Battle_HandlePlayerBlackOut();
        return;
    }

    /* Wild battle: no replacement (core.asm:848 jp z,.notTrainerBattle) */
    if (wIsInBattle != 2) {
        /* Wild victory — battle is over */
        wBattleResult = BATTLE_OUTCOME_WILD_VICTORY;
        return;
    }

    /* Trainer battle: check if any enemy mons remain (core.asm:851-858) */
    if (!Battle_AnyEnemyPokemonAliveCheck()) {
        Battle_TrainerBattleVictory();
        return;
    }

    /* Simultaneous faint: if player mon also at 0 HP, signal driver to prompt
     * for a new mon (core.asm:863-868 call ChooseNextMon).
     * We set the flag; the driver/UI handles actual mon selection. */
    if (wBattleMon.hp == 0) {
        wForcePlayerToChooseMon = 1;
    }

    /* Replace fainted enemy mon (core.asm:869 call ReplaceFaintedEnemyMon) */
    Battle_ReplaceFaintedEnemyMon();
}

/* ============================================================
 * Status message accessors
 * ============================================================ */
battle_status_msg_t Battle_GetPlayerStatusMsg(void)    { return s_player_status_msg; }
battle_status_msg_t Battle_GetPlayerPreStatusMsg(void) { return s_player_pre_msg;    }
battle_status_msg_t Battle_GetEnemyStatusMsg(void)     { return s_enemy_status_msg;  }
battle_status_msg_t Battle_GetEnemyPreStatusMsg(void)  { return s_enemy_pre_msg;     }

/* ============================================================
 * Battle_HandlePlayerMonFainted — core.asm:969
 * ============================================================ */
void Battle_HandlePlayerMonFainted(void) {
    BLOG("  %s fainted!", BMON_P());
    wInHandlePlayerMonFainted = 1;
    remove_fainted_player_mon_state();

    /* If enemy also has 0 HP (simultaneous faint), faint enemy too (core.asm:982-987) */
    if (wEnemyMon.hp == 0) {
        faint_enemy_pokemon_state();
    }
}
