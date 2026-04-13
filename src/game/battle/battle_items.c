/* battle_items.c — Item use in battle.
 *
 * Ports UseItem_ dispatch + per-item handlers from
 * engine/items/item_effects.asm.
 *
 * Covered handlers (battle-relevant only):
 *   ItemUseBall       — Pokéballs (via Battle_CatchAttempt)
 *   ItemUseMedicine   — potions, status cures, revives
 *   ItemUseXAccuracy  — item_effects.asm:1544
 *   ItemUsePokeDoll   — item_effects.asm:1606
 *   ItemUseGuardSpec  — item_effects.asm:1614
 *   ItemUseDireHit    — item_effects.asm:1630
 *   ItemUseXStat      — item_effects.asm:1638
 *   UnusableItem      — returns ITEM_USE_CANNOT_USE
 *
 * EvoStone / Vitamin are field-only (rejected with ITEM_USE_CANNOT_USE).
 * Item removal from bag is NOT performed here — caller's responsibility.
 *
 * ALWAYS refer to pokered-master source before modifying.
 */

#include "battle_items.h"
#include "battle_catch.h"
#include "../../platform/hardware.h"
#include "../constants.h"
#include "../pokedex.h"
#include <stdio.h>

/* ---- Forward declarations ---- */
static item_use_result_t use_ball(uint8_t item_id);
static item_use_result_t use_medicine(uint8_t item_id, uint8_t slot);
static item_use_result_t use_x_stat(uint8_t item_id);

/* ============================================================
 * Battle_UseItem — UseItem_ (item_effects.asm:1)
 *
 * Dispatches on item_id via the same logic as ItemUsePtrTable.
 * ============================================================ */
item_use_result_t Battle_UseItem(uint8_t item_id, uint8_t target_slot) {
    /* TMs/HMs and items with IDs >= HM01 can't be used this way */
    if (item_id >= ITEM_HM01) return ITEM_USE_CANNOT_USE;

    switch (item_id) {
    /* ---- Pokéballs ---- */
    case ITEM_MASTER_BALL:
    case ITEM_ULTRA_BALL:
    case ITEM_GREAT_BALL:
    case ITEM_POKE_BALL:
    case ITEM_SAFARI_BALL:
        return use_ball(item_id);

    /* ---- Status cures (ItemUseMedicine path: .cureStatusAilment) ---- */
    case ITEM_ANTIDOTE:
    case ITEM_BURN_HEAL:
    case ITEM_ICE_HEAL:
    case ITEM_AWAKENING:
    case ITEM_PARLYZ_HEAL:
    case ITEM_FULL_HEAL:
    /* ---- HP restoration (ItemUseMedicine path: .healHP) ---- */
    case ITEM_FULL_RESTORE:
    case ITEM_MAX_POTION:
    case ITEM_HYPER_POTION:
    case ITEM_SUPER_POTION:
    case ITEM_POTION:
    case ITEM_FRESH_WATER:
    case ITEM_SODA_POP:
    case ITEM_LEMONADE:
    /* ---- Revives ---- */
    case ITEM_REVIVE:
    case ITEM_MAX_REVIVE:
        return use_medicine(item_id, target_slot);

    /* ---- Battle-status items ---- */
    case ITEM_X_ACCURACY:
        /* ItemUseXAccuracy (item_effects.asm:1544): set USING_X_ACCURACY */
        wPlayerBattleStatus2 |= (1 << BSTAT2_USING_X_ACCURACY);
        printf("[item]   Used X Accuracy! Player attacks can't miss.\n");
        return ITEM_USE_OK;

    case ITEM_GUARD_SPEC:
        /* ItemUseGuardSpec (item_effects.asm:1614): set PROTECTED_BY_MIST */
        wPlayerBattleStatus2 |= (1 << BSTAT2_PROTECTED_BY_MIST);
        printf("[item]   Used Guard Spec.! Player protected from stat reductions.\n");
        return ITEM_USE_OK;

    case ITEM_DIRE_HIT:
        /* ItemUseDireHit (item_effects.asm:1630): set GETTING_PUMPED (Focus Energy) */
        wPlayerBattleStatus2 |= (1 << BSTAT2_GETTING_PUMPED);
        printf("[item]   Used Dire Hit! Player is getting pumped.\n");
        return ITEM_USE_OK;

    case ITEM_X_ATTACK:
    case ITEM_X_DEFEND:
    case ITEM_X_SPEED:
    case ITEM_X_SPECIAL:
        return use_x_stat(item_id);

    /* ---- Escape ---- */
    case ITEM_POKE_DOLL:
        /* ItemUsePokeDoll (item_effects.asm:1606): only works in wild battle */
        if (wIsInBattle != 1) return ITEM_USE_CANNOT_USE;
        wEscapedFromBattle = 1;
        printf("[item]   Used Poké Doll! Fled from battle.\n");
        return ITEM_USE_FLED;

    /* ---- POKe FLUTE — ItemUsePokeFlute in-battle (item_effects.asm:1671) ---- */
    case ITEM_POKE_FLUTE: {
        int any_asleep = 0;
        /* Wake all player party mons (WakeUpEntireParty) */
        for (int i = 0; i < wPartyCount; i++) {
            if (wPartyMons[i].base.status & STATUS_SLP_MASK) {
                any_asleep = 1;
                wPartyMons[i].base.status &= (uint8_t)~STATUS_SLP_MASK;
            }
        }
        /* Always clear active battle mon sleep (wBattleMonStatus) */
        if (wBattleMon.status & STATUS_SLP_MASK) {
            any_asleep = 1;
            wBattleMon.status &= (uint8_t)~STATUS_SLP_MASK;
            if (wPlayerMonNumber < wPartyCount)
                wPartyMons[wPlayerMonNumber].base.status = wBattleMon.status;
        }
        /* Clear enemy active mon sleep (wEnemyMonStatus) — always in Gen 1 */
        if (wEnemyMon.status & STATUS_SLP_MASK) {
            any_asleep = 1;
            wEnemyMon.status &= (uint8_t)~STATUS_SLP_MASK;
        }
        return any_asleep ? ITEM_USE_OK : ITEM_USE_FAILED;
    }

    default:
        return ITEM_USE_CANNOT_USE;
    }
}

/* ============================================================
 * use_ball — ItemUseBall (item_effects.asm:104)
 *
 * Only valid in wild battle (wIsInBattle == 1).
 * ============================================================ */
static item_use_result_t use_ball(uint8_t item_id) {
    if (wIsInBattle != 1) {
        /* Can't catch trainer's Pokémon (item_effects.asm:112-113) */
        printf("[item]   Can't use a ball against a trainer's Pokémon!\n");
        return ITEM_USE_CANNOT_USE;
    }

    catch_result_t result = Battle_CatchAttempt(item_id);

    switch (result) {
    case CATCH_RESULT_SUCCESS:
        Pokedex_SetOwned(wEnemyMon.species);
        printf("[item]   Gotcha! Enemy Pokémon was caught!\n");
        return ITEM_USE_CAUGHT;
    case CATCH_RESULT_0_SHAKES:
        printf("[item]   Oh no! The Pokémon broke free! (0 shakes)\n");
        break;
    case CATCH_RESULT_1_SHAKE:
        printf("[item]   Darn! Almost had it! (1 shake)\n");
        break;
    case CATCH_RESULT_2_SHAKES:
        printf("[item]   Argh! So close! (2 shakes)\n");
        break;
    case CATCH_RESULT_3_SHAKES:
        printf("[item]   Shoot! It was so close too! (3 shakes)\n");
        break;
    case CATCH_RESULT_CANNOT_CATCH:
        printf("[item]   The Pokémon can't be caught!\n");
        break;
    }

    return ITEM_USE_OK;  /* turn consumed even on miss */
}

/* ============================================================
 * use_medicine — ItemUseMedicine (item_effects.asm:805)
 *
 * Handles status cures, HP restoration, and revives.
 * ============================================================ */
static item_use_result_t use_medicine(uint8_t item_id, uint8_t slot) {
    if (slot >= wPartyCount) return ITEM_USE_FAILED;
    party_mon_t *p = &wPartyMons[slot];

    /* ---- Status-cure items (.cureStatusAilment path) ---- */
    uint8_t cure_mask = 0;
    switch (item_id) {
    case ITEM_ANTIDOTE:   cure_mask = STATUS_PSN; break;
    case ITEM_BURN_HEAL:  cure_mask = STATUS_BRN; break;
    case ITEM_ICE_HEAL:   cure_mask = STATUS_FRZ; break;
    case ITEM_AWAKENING:  cure_mask = STATUS_SLP_MASK; break;
    case ITEM_PARLYZ_HEAL:cure_mask = STATUS_PAR; break;
    case ITEM_FULL_HEAL:  cure_mask = 0xFF; break;  /* all status */
    default: break;
    }

    if (cure_mask) {
        /* Full Restore: fall through to HP heal after curing status */
        if (item_id == ITEM_FULL_RESTORE) {
            /* handled below */
        } else {
            if (!(p->base.status & cure_mask)) {
                printf("[item]   It won't have any effect.\n");
                return ITEM_USE_FAILED;
            }
            p->base.status &= (uint8_t)~cure_mask;
            /* Sync active mon status (item_effects.asm:902) */
            if (slot == wPlayerMonNumber) {
                wBattleMon.status = p->base.status;
                /* Clear badly-poisoned flag (item_effects.asm:904-906) */
                wPlayerBattleStatus3 &= (uint8_t)~(1 << BSTAT3_BADLY_POISONED);
            }
            printf("[item]   Status cured.\n");
            return ITEM_USE_OK;
        }
    }

    /* ---- HP-restore and Revive items (.healHP path) ---- */

    /* Revive / Max Revive: only works on fainted mons */
    if (item_id == ITEM_REVIVE || item_id == ITEM_MAX_REVIVE) {
        if (p->base.hp != 0) {
            printf("[item]   It won't have any effect.\n");
            return ITEM_USE_FAILED;
        }
        uint16_t restore = (item_id == ITEM_MAX_REVIVE) ? p->max_hp
                                                         : (uint16_t)(p->max_hp / 2);
        p->base.hp = restore;
        /* Revived mon becomes eligible for exp again if it fought
         * (item_effects.asm:946-950: set wPartyGainExpFlags bit) */
        if (wPartyFoughtCurrentEnemyFlags & (1u << slot))
            wPartyGainExpFlags |= (uint8_t)(1u << slot);
        if (slot == wPlayerMonNumber) wBattleMon.hp = p->base.hp;
        printf("[item]   Pokémon revived with %d HP.\n", (int)p->base.hp);
        return ITEM_USE_OK;
    }

    /* Sync active mon's HP from the battle copy.
     * Damage during battle is written to wBattleMon.hp; wPartyMons[].base.hp
     * is only flushed back at faint/end-of-battle, so it can be stale here.
     * Without this, the HP-full check below sees the pre-damage party HP and
     * incorrectly rejects a Potion on a damaged active mon. */
    if (wIsInBattle && slot == (uint8_t)wPlayerMonNumber)
        p->base.hp = wBattleMon.hp;

    /* Regular HP items — fail on already-fainted mons */
    if (p->base.hp == 0) {
        printf("[item]   It won't have any effect.\n");
        return ITEM_USE_FAILED;
    }

    /* Full Restore: heal to max + cure all status */
    if (item_id == ITEM_FULL_RESTORE) {
        if (p->base.hp == p->max_hp && p->base.status == 0) {
            printf("[item]   It won't have any effect.\n");
            return ITEM_USE_FAILED;
        }
        p->base.hp     = p->max_hp;
        p->base.status = 0;
        if (slot == wPlayerMonNumber) {
            wBattleMon.hp     = p->base.hp;
            wBattleMon.status = 0;
            wPlayerBattleStatus3 &= (uint8_t)~(1 << BSTAT3_BADLY_POISONED);
        }
        printf("[item]   Pokémon restored to full HP and status cleared.\n");
        return ITEM_USE_OK;
    }

    /* Max Potion: heal to max HP (item_effects.asm:1113-1127) */
    if (item_id == ITEM_MAX_POTION) {
        if (p->base.hp == p->max_hp) {
            printf("[item]   It won't have any effect.\n");
            return ITEM_USE_FAILED;
        }
        p->base.hp = p->max_hp;
        if (slot == wPlayerMonNumber) wBattleMon.hp = p->base.hp;
        printf("[item]   Pokémon's HP restored to max.\n");
        return ITEM_USE_OK;
    }

    /* Determine HP heal amount (.notUsingSoftboiled2 block, item_effects.asm:1076-1090)
     * Note: comparisons are item_id >= X, evaluating in order SODA_POP..POTION. */
    uint8_t heal;
    if      (item_id == ITEM_SODA_POP)     heal = 60;
    else if (item_id >  ITEM_SODA_POP)     heal = 80;   /* Lemonade */
    else if (item_id == ITEM_FRESH_WATER)  heal = 50;
    else if (item_id <  ITEM_SUPER_POTION) heal = 200;  /* Hyper Potion (0x12 < 0x13) */
    else if (item_id == ITEM_SUPER_POTION) heal = 50;
    else                                    heal = 20;   /* Potion */

    if (p->base.hp == p->max_hp) {
        printf("[item]   It won't have any effect.\n");
        return ITEM_USE_FAILED;
    }

    uint32_t new_hp = (uint32_t)p->base.hp + heal;
    p->base.hp = (uint16_t)(new_hp > p->max_hp ? p->max_hp : new_hp);

    if (slot == wPlayerMonNumber) wBattleMon.hp = p->base.hp;
    printf("[item]   HP restored by %d (now %d/%d).\n",
           (int)heal, (int)p->base.hp, (int)p->max_hp);
    return ITEM_USE_OK;
}

/* ============================================================
 * use_x_stat — ItemUseXStat (item_effects.asm:1638)
 *
 * Maps item_id to stat mod index via the same arithmetic as the ASM:
 *   effect = item_id - (X_ATTACK - ATTACK_UP1_EFFECT)
 *   X_ATTACK=0x41, ATTACK_UP1_EFFECT=0x0A → offset=0x37
 *
 * Stat mod indices: 0=Atk, 1=Def, 2=Spd, 3=Spc
 * ============================================================ */
static item_use_result_t use_x_stat(uint8_t item_id) {
    /* item_id - 0x37 maps to effect constant (0x0A..0x0D)
     * index into wPlayerMonStatMods = effect - ATTACK_UP1_EFFECT = effect - 0x0A */
    uint8_t stat_idx = (uint8_t)(item_id - ITEM_X_ATTACK); /* 0=Atk,1=Def,2=Spd,3=Spc */

    uint8_t cur = wPlayerMonStatMods[stat_idx];
    if (cur >= STAT_STAGE_MAX) {
        printf("[item]   It won't have any effect.\n");
        return ITEM_USE_FAILED;
    }
    wPlayerMonStatMods[stat_idx]++;
    printf("[item]   Player's stat (idx %d) rose! Stage now %d.\n",
           (int)stat_idx, (int)wPlayerMonStatMods[stat_idx]);
    return ITEM_USE_OK;
}
