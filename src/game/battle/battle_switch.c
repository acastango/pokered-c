/* battle_switch.c — Player mon switching in battle.
 *
 * Ports from engine/battle/core.asm (bank $0F):
 *   AnyPartyAlive              (core.asm:1455)
 *   HasMonFainted              (core.asm:1473)
 *   LoadBattleMonFromParty     (core.asm:1626)
 *   SendOutMon state portion   (core.asm:1723)
 *   ReadPlayerMonCurHPAndStatus (core.asm:1800)
 *   SwitchPlayerMon            (core.asm:2419)
 *   ChooseNextMon              (core.asm:1083)
 *   ApplyBadgeStatBoosts       (core.asm:6454)
 *
 * UI calls (PrintText, animations, DrawHUDs, PlayCry) are omitted.
 * ALWAYS refer to pokered-master ASM before modifying anything here.
 */

#include "battle_switch.h"
#include "battle.h"
#include "../../platform/hardware.h"
#include "../constants.h"
#include <string.h>

/* ============================================================
 * apply_boost — ApplyBadgeStatBoosts helper (core.asm:6482)
 *
 * Multiplies stat by 1.125 (adds stat >> 3), capped at MAX_STAT_VALUE.
 * ASM: srl/rr three times to get stat/8, add to original, cap at 999.
 * ============================================================ */
static uint16_t apply_boost(uint16_t stat) {
    uint16_t boost = (uint16_t)(stat >> 3);
    uint32_t result = (uint32_t)stat + boost;
    return (uint16_t)(result > MAX_STAT_VALUE ? MAX_STAT_VALUE : result);
}

/* ============================================================
 * apply_badge_stat_boosts — ApplyBadgeStatBoosts (core.asm:6454)
 *
 * Boosts wBattleMon stats based on obtained badges (skipped in link battle).
 * Even badge bits boost stats in ATK/DEF/SPD/SPC order:
 *   bit 0 (Boulder) → ATK   bit 2 (Thunder) → DEF
 *   bit 4 (Soul)    → SPD   bit 6 (Volcano) → SPC
 * ============================================================ */
static void apply_badge_stat_boosts(void) {
    if (wLinkState == LINK_STATE_BATTLING) return;

    uint8_t b = wObtainedBadges;
    if (b & (1u << BIT_BOULDERBADGE)) wBattleMon.atk = apply_boost(wBattleMon.atk);
    if (b & (1u << BIT_THUNDERBADGE)) wBattleMon.def = apply_boost(wBattleMon.def);
    if (b & (1u << BIT_SOULBADGE))   wBattleMon.spd = apply_boost(wBattleMon.spd);
    if (b & (1u << BIT_VOLCANOBADGE)) wBattleMon.spc = apply_boost(wBattleMon.spc);
}

/* ============================================================
 * Battle_AnyPartyAlive — AnyPartyAlive (core.asm:1455)
 *
 * Scans wPartyMons[0..wPartyCount-1] HP fields.
 * Returns nonzero (d != 0) if any mon has HP > 0.
 * ============================================================ */
int Battle_AnyPartyAlive(void) {
    uint16_t hp_or = 0;
    for (uint8_t i = 0; i < wPartyCount; i++) {
        hp_or |= wPartyMons[i].base.hp;
    }
    return (hp_or != 0) ? 1 : 0;
}

/* ============================================================
 * Battle_HasMonFainted — HasMonFainted (core.asm:1473)
 *
 * Returns 1 if wPartyMons[slot].hp == 0 (Z flag set in ASM).
 * ============================================================ */
int Battle_HasMonFainted(uint8_t slot) {
    return (wPartyMons[slot].base.hp == 0) ? 1 : 0;
}

/* ============================================================
 * Battle_LoadBattleMonFromParty — LoadBattleMonFromParty (core.asm:1626)
 *
 * Copies the party mon at wPlayerMonNumber into wBattleMon.
 * Field mapping mirrors the battle_struct / party_struct byte layout.
 * ============================================================ */
void Battle_LoadBattleMonFromParty(void) {
    party_mon_t *p = &wPartyMons[wPlayerMonNumber];

    /* Copy fields that map directly (core.asm:1629-1642):
     * party.box_level → battle.party_pos (BoxLevel/PartyPos overlap in battle_struct)
     * OTID, exp, stat_exp are skipped. */
    wBattleMon.species    = p->base.species;
    wBattleMon.hp         = p->base.hp;
    wBattleMon.party_pos  = p->base.box_level;  /* BoxLevel→PartyPos (battle_struct alias) */
    wBattleMon.status     = p->base.status;
    wBattleMon.type1      = p->base.type1;
    wBattleMon.type2      = p->base.type2;
    wBattleMon.catch_rate = p->base.catch_rate;
    memcpy(wBattleMon.moves, p->base.moves, sizeof(wBattleMon.moves));

    /* DVs (core.asm:1643-1648): skip OTID/exp/stat_exp, copy dvs */
    wBattleMon.dvs = p->base.dvs;

    /* PP (core.asm:1649-1652): copy from party pp → battle pp */
    memcpy(wBattleMon.pp, p->base.pp, sizeof(wBattleMon.pp));

    /* Level + stats (core.asm:1653-1658): copied in one block */
    wBattleMon.level  = p->level;
    wBattleMon.max_hp = p->max_hp;
    wBattleMon.atk    = p->atk;
    wBattleMon.def    = p->def;
    wBattleMon.spd    = p->spd;
    wBattleMon.spc    = p->spc;

    /* wCurSpecies (core.asm:1659): used by GetMonHeader; set from battle copy */
    wCurSpecies = wBattleMon.species;

    /* Save unmodified stats for crit bypass (core.asm:1665-1669) */
    wPlayerMonUnmodifiedAttack  = p->atk;
    wPlayerMonUnmodifiedDefense = p->def;
    wPlayerMonUnmodifiedSpeed   = p->spd;
    wPlayerMonUnmodifiedSpecial = p->spc;

    /* ApplyBurnAndParalysisPenaltiesToPlayer (core.asm:1658):
     * set hWhoseTurn=1 so the function targets wBattleMon. */
    hWhoseTurn = 1;
    Battle_ApplyBurnAndParalysisPenalties();

    /* ApplyBadgeStatBoosts (core.asm:1659) */
    apply_badge_stat_boosts();

    /* Reset stat modifier stages to 7 (neutral) (core.asm:1660-1668) */
    for (int i = 0; i < NUM_STAT_MODS; i++) {
        wPlayerMonStatMods[i] = 7;
    }
}

/* ============================================================
 * Battle_SendOutMon_State — SendOutMon state portion (core.asm:1723)
 *
 * Resets all per-mon battle state flags when a new player mon enters.
 * ============================================================ */
void Battle_SendOutMon_State(void) {
    /* core.asm:1742-1768: zero out battle state vars */
    wBoostExpByExpAll               = 0;
    wDamageMultipliers              = 0;
    wPlayerMoveNum                  = 0;
    wPlayerUsedMove                 = 0;
    wEnemyUsedMove                  = 0;

    /* Zero player battle status flags (wPlayerStatsToDouble block,
     * core.asm:1751-1755: 5 bytes from wPlayerStatsToDouble which
     * immediately precedes wPlayerBattleStatus1 in wram.asm) */
    wPlayerBattleStatus1 = 0;
    wPlayerBattleStatus2 = 0;
    wPlayerBattleStatus3 = 0;

    /* Clear per-mon counters and flags (core.asm:1756-1762) */
    wPlayerDisabledMove        = 0;
    wPlayerDisabledMoveNumber  = 0;
    wPlayerMonMinimized        = 0;
    wPlayerConfusedCounter     = 0;
    wPlayerToxicCounter        = 0;
    wPlayerBideAccumulatedDamage = 0;
    wPlayerNumAttacksLeft      = 0;

    /* Clear enemy trapping lock (core.asm:1769-1770):
     * the enemy's trapping move ends when the target switches out */
    wEnemyBattleStatus1 &= (uint8_t)~(1u << BSTAT1_USING_TRAPPING);

    /* hWhoseTurn = 1 (core.asm:1771): player side entering */
    hWhoseTurn = 1;
}

/* ============================================================
 * Battle_ReadPlayerMonCurHPAndStatus — ReadPlayerMonCurHPAndStatus (core.asm:1800)
 *
 * Writes wBattleMon HP and status back to the party slot so the party
 * data stays accurate after each turn and before retreating.
 * ============================================================ */
void Battle_ReadPlayerMonCurHPAndStatus(void) {
    party_mon_t *p = &wPartyMons[wPlayerMonNumber];
    p->base.hp     = wBattleMon.hp;
    p->base.status = wBattleMon.status;
}

/* ============================================================
 * Battle_SwitchPlayerMon — SwitchPlayerMon (core.asm:2419)
 *
 * Voluntary mid-battle switch selected from the PKMN party menu.
 * Omits: RetreatMon text/animation, AnimateRetreatingPlayerMon.
 * ============================================================ */
void Battle_SwitchPlayerMon(uint8_t new_slot) {
    /* Sync current mon data back to party before retreating (core.asm:2420
     * callfar RetreatMon — data sync portion of RetreatMon) */
    Battle_ReadPlayerMonCurHPAndStatus();

    /* Set new active slot (core.asm:2425) */
    wPlayerMonNumber = new_slot;

    /* Set gain-exp flag for incoming mon (core.asm:2426-2432) */
    wPartyGainExpFlags |= (uint8_t)(1u << new_slot);

    /* Set fought-current-enemy flag (core.asm:2433) */
    wPartyFoughtCurrentEnemyFlags |= (uint8_t)(1u << new_slot);

    /* Load new mon data into battle slot (core.asm:2434) */
    Battle_LoadBattleMonFromParty();

    /* Reset per-switch-in state (core.asm:2435 call SendOutMon) */
    Battle_SendOutMon_State();

    /* Mark turn as consumed (core.asm:2419 — .notAlreadyOut path:
     * ld a, $1; ld [wActionResultOrTookBattleTurn], a) */
    wActionResultOrTookBattleTurn = 1;
}

/* ============================================================
 * Battle_ChooseNextMon — ChooseNextMon (core.asm:1083)
 *
 * Forced switch after the active player mon faints.
 * Omits: DisplayPartyMenu UI, GoBackToPartyMenu, link battle exchange.
 * ============================================================ */
void Battle_ChooseNextMon(uint8_t new_slot) {
    /* Set new active slot (core.asm:1109) */
    wPlayerMonNumber = new_slot;

    /* Set gain-exp flag (core.asm:1110-1116: FLAG_SET on wPartyGainExpFlags) */
    wPartyGainExpFlags |= (uint8_t)(1u << new_slot);

    /* Set fought-current-enemy flag (core.asm:1117) */
    wPartyFoughtCurrentEnemyFlags |= (uint8_t)(1u << new_slot);

    /* Load new mon data (core.asm:1118) */
    Battle_LoadBattleMonFromParty();

    /* Reset per-switch-in state (core.asm:1124 call SendOutMon) */
    Battle_SendOutMon_State();
}
