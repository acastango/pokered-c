#pragma once
/* battle_items.h — Item use in battle.
 *
 * Ports UseItem_ (engine/items/item_effects.asm:1) dispatch logic for
 * all battle-relevant items.
 *
 * ALWAYS refer to pokered-master source before modifying.
 */
#include <stdint.h>

/* Result of Battle_UseItem — mirrors wActionResultOrTookBattleTurn semantics. */
typedef enum {
    ITEM_USE_FAILED      = 0,  /* item had no effect (wrong condition, full HP, etc.) */
    ITEM_USE_OK          = 1,  /* item used successfully, turn consumed */
    ITEM_USE_CANNOT_USE  = 2,  /* item can't be used in battle (field-only) */
    ITEM_USE_CAUGHT      = 3,  /* Pokéball — enemy caught */
    ITEM_USE_FLED        = 4,  /* Poké Doll — fled wild battle */
} item_use_result_t;

/* Battle_UseItem — UseItem_ dispatch (item_effects.asm:1).
 *
 * item_id:    item constant (ITEM_* from constants.h)
 * target_slot: party slot the item is aimed at (ignored for battle-status
 *              items and Pokéballs, used for medicine/revive)
 *
 * Effects on WRAM:
 *   Medicine   — modifies wPartyMons[target_slot].base.{hp,status}; syncs
 *                wBattleMon if target_slot == wPlayerMonNumber.
 *   X-items    — raises wPlayerMonStatMods stage +1 (clamped to STAT_STAGE_MAX).
 *   X_ACCURACY — sets BSTAT2_USING_X_ACCURACY in wPlayerBattleStatus2.
 *   Guard Spec — sets BSTAT2_PROTECTED_BY_MIST in wPlayerBattleStatus2.
 *   Dire Hit   — sets BSTAT2_GETTING_PUMPED in wPlayerBattleStatus2.
 *   Poké Doll  — sets wEscapedFromBattle = 1.
 *   Pokéball   — calls Battle_CatchAttempt; returns ITEM_USE_CAUGHT on success.
 *
 * Does NOT remove the item from the bag — caller handles inventory. */
item_use_result_t Battle_UseItem(uint8_t item_id, uint8_t target_slot);
