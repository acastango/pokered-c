/* battle_trainer.c — Enemy mon replacement and end-of-battle logic.
 *
 * Ports from engine/battle/core.asm (bank $0F):
 *   AnyEnemyPokemonAliveCheck  (core.asm:874)
 *   LoadEnemyMonFromParty      (core.asm:1670)
 *   EnemySendOut state portion (core.asm:1276)
 *   ReplaceFaintedEnemyMon     (core.asm:892)
 *   TrainerBattleVictory       (core.asm:915) — state portion only
 *   HandlePlayerBlackOut       (core.asm:1132) — state portion only
 *   TryRunningFromBattle       (core.asm:1496)
 *
 * UI calls (PrintText, PlayMusic, PlayCry, DrawHUDs) are omitted.
 * ALWAYS refer to pokered-master ASM before modifying anything here.
 */

#include "battle_trainer.h"
#include "battle_loop.h"
#include "battle.h"
#include "../../platform/hardware.h"
#include "../constants.h"
#include <string.h>
#include <stdio.h>

/* ============================================================
 * Battle_AnyEnemyPokemonAliveCheck — AnyEnemyPokemonAliveCheck (core.asm:874)
 *
 * Scans wEnemyMon1HP through wEnemyMon6HP for any nonzero value.
 * Z flag is cleared (carry) if any mon is alive.
 * In C: returns nonzero if any alive, 0 if all fainted.
 * ============================================================ */
int Battle_AnyEnemyPokemonAliveCheck(void) {
    for (uint8_t i = 0; i < wEnemyPartyCount; i++) {
        if (wEnemyMons[i].base.hp != 0) return 1;
    }
    return 0;
}

/* ============================================================
 * Battle_LoadEnemyMonFromParty — LoadEnemyMonFromParty (core.asm:1670)
 *
 * Enemy-side mirror of LoadBattleMonFromParty.
 * Copies wEnemyMons[wEnemyMonPartyPos] → wEnemyMon.
 * ============================================================ */
void Battle_LoadEnemyMonFromParty(void) {
    party_mon_t *p = &wEnemyMons[wEnemyMonPartyPos];

    /* Copy fields that map directly (mirrors core.asm:1629-1642):
     * party.box_level → battle.party_pos (BoxLevel/PartyPos overlap in battle_struct) */
    wEnemyMon.species    = p->base.species;
    wEnemyMon.hp         = p->base.hp;
    wEnemyMon.party_pos  = p->base.box_level;   /* BoxLevel→PartyPos */
    wEnemyMon.status     = p->base.status;
    wEnemyMon.type1      = p->base.type1;
    wEnemyMon.type2      = p->base.type2;
    wEnemyMon.catch_rate = p->base.catch_rate;
    memcpy(wEnemyMon.moves, p->base.moves, sizeof(wEnemyMon.moves));

    /* DVs — skip OTID/exp/stat_exp, copy dvs */
    wEnemyMon.dvs = p->base.dvs;

    /* PP */
    memcpy(wEnemyMon.pp, p->base.pp, sizeof(wEnemyMon.pp));

    /* Level + stats */
    wEnemyMon.level  = p->level;
    wEnemyMon.max_hp = p->max_hp;
    wEnemyMon.atk    = p->atk;
    wEnemyMon.def    = p->def;
    wEnemyMon.spd    = p->spd;
    wEnemyMon.spc    = p->spc;

    /* Save unmodified stats for crit bypass */
    wEnemyMonUnmodifiedAttack  = p->atk;
    wEnemyMonUnmodifiedDefense = p->def;
    wEnemyMonUnmodifiedSpeed   = p->spd;
    wEnemyMonUnmodifiedSpecial = p->spc;

    /* ApplyBurnAndParalysisPenaltiesToEnemy:
     * hWhoseTurn = 0 → function targets wEnemyMon. */
    hWhoseTurn = 0;
    Battle_ApplyBurnAndParalysisPenalties();

    /* Reset enemy stat modifier stages to 7 (neutral) */
    for (int i = 0; i < NUM_STAT_MODS; i++) {
        wEnemyMonStatMods[i] = 7;
    }
}

/* ============================================================
 * Battle_EnemySendOut_State — EnemySendOut state portion (core.asm:1276)
 *
 * Finds the next alive enemy mon, loads it, resets per-switch-in state.
 * Returns 1 if replacement found, 0 if no alive enemy mons remain.
 * ============================================================ */
int Battle_EnemySendOut_State(void) {
    /* Find next alive enemy mon (core.asm:1276-1315):
     * Loop b=0..PARTY_LENGTH-1, skip current slot and fainted mons. */
    uint8_t new_slot = 0xFF;
    for (uint8_t b = 0; b < wEnemyPartyCount; b++) {
        if (b == wEnemyMonPartyPos) continue;   /* skip current slot */
        if (wEnemyMons[b].base.hp == 0) continue; /* skip fainted */
        new_slot = b;
        break;
    }

    if (new_slot == 0xFF) return 0;  /* no alive enemy mons */

    wEnemyMonPartyPos = new_slot;
    Battle_LoadEnemyMonFromParty();

    /* Reset enemy per-switch-in battle state (mirrors SendOutMon_State for enemy):
     * clear all status flags, counters, and move locks. */
    wEnemyBattleStatus1  = 0;
    wEnemyBattleStatus2  = 0;
    wEnemyBattleStatus3  = 0;

    wEnemyDisabledMove        = 0;
    wEnemyDisabledMoveNumber  = 0;
    wEnemyMonMinimized        = 0;
    wEnemyConfusedCounter     = 0;
    wEnemyToxicCounter        = 0;
    wEnemyBideAccumulatedDamage = 0;
    wEnemyNumAttacksLeft      = 0;

    /* Clear player USING_TRAPPING: player's wrap ends when enemy switches out */
    wPlayerBattleStatus1 &= (uint8_t)~(1u << BSTAT1_USING_TRAPPING);

    /* Save enemy HP at switch-in for trainer AI tracking */
    wLastSwitchInEnemyMonHP = wEnemyMon.hp;

    printf("[trainer] Enemy sent out new Pokémon (party slot %d).\n",
           (int)wEnemyMonPartyPos);
    return 1;
}

/* ============================================================
 * Battle_ReplaceFaintedEnemyMon — ReplaceFaintedEnemyMon (core.asm:892)
 *
 * Calls EnemySendOut then clears post-faint battle vars.
 * Returns 1 on success, 0 if no alive enemy mons.
 * ============================================================ */
int Battle_ReplaceFaintedEnemyMon(void) {
    if (!Battle_EnemySendOut_State()) return 0;

    /* Zero post-faint battle vars (core.asm:896-910) */
    wEnemyMoveNum                  = 0;
    wActionResultOrTookBattleTurn  = 0;
    wAILayer2Encouragement         = 0;

    return 1;
}

/* ============================================================
 * Battle_TrainerBattleVictory — TrainerBattleVictory state portion (core.asm:915)
 *
 * Signals player victory over trainer.
 * Omits: music change, prize money award, trainer text.
 * ============================================================ */
void Battle_TrainerBattleVictory(void) {
    wBattleResult = BATTLE_OUTCOME_TRAINER_VICTORY;
    wIsInBattle   = 0;
    printf("[trainer] All enemy Pokémon fainted. Player wins!\n");
}

/* ============================================================
 * Battle_HandlePlayerBlackOut — HandlePlayerBlackOut state portion (core.asm:1132)
 *
 * Signals player defeat (blacked out).
 * Omits: PrintText (blacked out message), ClearBikeFlag.
 * ============================================================ */
void Battle_HandlePlayerBlackOut(void) {
    wBattleResult = BATTLE_OUTCOME_BLACKOUT;
    wIsInBattle   = (uint8_t)0xFF;  /* 0xFF = lost (wIsInBattle = -1) */
    printf("[trainer] Player blacked out!\n");
}

/* ============================================================
 * Battle_TryRunningFromBattle — TryRunningFromBattle (core.asm:1496)
 *
 * Flee formula (core.asm:1517-1545):
 *   divisor    = (enemy_spd >> 2) & 0xFF
 *   odds       = (player_spd * 32) / divisor
 *   odds      += 30 * (attempts - 1)
 *   escape if divisor == 0 || odds >= 256 || BattleRandom() < odds
 * ============================================================ */
int Battle_TryRunningFromBattle(void) {
    /* Wild battles only (core.asm:1496-1498) */
    if (wIsInBattle != 1) return 0;

    wNumRunAttempts++;

    uint8_t player_spd   = (uint8_t)wBattleMon.spd;
    uint8_t enemy_spd    = (uint8_t)wEnemyMon.spd;
    uint8_t odds_divisor = (uint8_t)((enemy_spd >> 2) & 0xFF);

    /* If divisor == 0, always escape (core.asm:1527-1530) */
    if (odds_divisor == 0) {
        wEscapedFromBattle = 1;
        return 1;
    }

    /* Compute odds (core.asm:1531-1542):
     * player_spd * 32 is 16-bit; / divisor truncates to byte width.
     * +30 per prior attempt (attempts-1, since we already incremented). */
    uint16_t odds = (uint16_t)((uint16_t)player_spd * 32u / odds_divisor);
    odds += (uint16_t)(30u * (wNumRunAttempts - 1u));

    /* If odds overflowed a byte (>= 256), always escape (core.asm:1539-1541) */
    if (odds >= 256u) {
        wEscapedFromBattle = 1;
        return 1;
    }

    /* Compare against BattleRandom (core.asm:1543-1547) */
    if (BattleRandom() < (uint8_t)odds) {
        wEscapedFromBattle = 1;
        return 1;
    }

    printf("[battle] Couldn't get away!\n");
    return 0;
}
