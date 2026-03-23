/* battle_init.c — Battle setup: port of StartBattle (engine/battle/core.asm).
 *
 * Covers the state-initialization path only (wild battle, no trainer AI,
 * no transition animation).  Always uses wPartyMons[0] as the player mon.
 *
 * ALWAYS refer to pokered-master engine/battle/core.asm before modifying.
 */

#include "battle_init.h"
#include "../../platform/hardware.h"
#include "../../data/base_stats.h"
#include "../../data/moves_data.h"
#include "../constants.h"
#include <string.h>

void Battle_Start(void) {
    /* ---- Clear all per-battle status (core.asm:StartBattle init block) ---- */
    wPlayerBattleStatus1 = wPlayerBattleStatus2 = wPlayerBattleStatus3 = 0;
    wEnemyBattleStatus1  = wEnemyBattleStatus2  = wEnemyBattleStatus3  = 0;
    memset(wPlayerMonStatMods, STAT_STAGE_NORMAL, sizeof(wPlayerMonStatMods));
    memset(wEnemyMonStatMods,  STAT_STAGE_NORMAL, sizeof(wEnemyMonStatMods));
    wPlayerConfusedCounter = wEnemyConfusedCounter = 0;
    wPlayerToxicCounter    = wEnemyToxicCounter    = 0;
    wPlayerDisabledMove    = wEnemyDisabledMove    = 0;
    wPlayerNumAttacksLeft  = wEnemyNumAttacksLeft  = 0;
    wPlayerNumHits         = wEnemyNumHits         = 0;
    wPlayerBideAccumulatedDamage = wEnemyBideAccumulatedDamage = 0;
    wPlayerSubstituteHP    = wEnemySubstituteHP    = 0;
    wPlayerMonMinimized    = wEnemyMonMinimized    = 0;
    wPlayerDisabledMoveNumber = wEnemyDisabledMoveNumber = 0;
    wPlayerSelectedMove    = wEnemySelectedMove    = 0;
    wEscapedFromBattle     = 0;
    wMoveDidntMiss         = 0;
    wDamage                = 0;
    wDamageMultipliers     = DAMAGE_MULT_EFFECTIVE;
    wFirstMonsNotOutYet    = 1;
    wIsInBattle            = 1;   /* 1 = wild battle */
    wBattleType            = 0;   /* 0 = normal */
    hWhoseTurn             = 0;

    /* ---- Copy player mon (slot 0) → wBattleMon (core.asm:LoadPlayerMon) ---- */
    wPlayerMonNumber = 0;
    const party_mon_t *p = &wPartyMons[0];

    wBattleMon.species    = p->base.species;
    wBattleMon.hp         = p->base.hp;
    wBattleMon.party_pos  = 0;
    wBattleMon.status     = p->base.status;
    wBattleMon.type1      = p->base.type1;
    wBattleMon.type2      = p->base.type2;
    wBattleMon.catch_rate = p->base.catch_rate;
    memcpy(wBattleMon.moves, p->base.moves, 4);
    wBattleMon.dvs        = p->base.dvs;
    wBattleMon.level      = p->level;
    wBattleMon.max_hp     = p->max_hp;
    wBattleMon.atk        = p->atk;
    wBattleMon.def        = p->def;
    wBattleMon.spd        = p->spd;
    wBattleMon.spc        = p->spc;
    /* pp[] stores current PP in bits 0-5, PP_UP count in bits 6-7 — copy as-is */
    memcpy(wBattleMon.pp, p->base.pp, 4);

    /* ---- Generate wild enemy mon (core.asm:LoadEnemyMon) ---- */
    uint8_t dex = gSpeciesToDex[wCurPartySpecies];
    const base_stats_t *b = &gBaseStats[dex];
    uint8_t lv = wCurEnemyLevel;

    /* Random DVs: 4 bits each, rolled independently (core.asm wild DV generation) */
    uint8_t dv_atk = BattleRandom() & 0x0F;
    uint8_t dv_def = BattleRandom() & 0x0F;
    uint8_t dv_spd = BattleRandom() & 0x0F;
    uint8_t dv_spc = BattleRandom() & 0x0F;
    /* HP DV = bit 0 of each of atk/def/spd/spc (core.asm:DVToHPDV) */
    uint8_t dv_hp = (uint8_t)(((dv_atk & 1) << 3) | ((dv_def & 1) << 2) |
                               ((dv_spd & 1) << 1) |  (dv_spc & 1));

    wEnemyMon.species    = wCurPartySpecies;  /* internal species ID (core.asm:LoadEnemyMonData) */
    wEnemyMon.hp         = CalcStat(b->hp,  dv_hp,  0, lv, 1);
    wEnemyMon.party_pos  = 0;
    wEnemyMon.status     = 0;
    wEnemyMon.type1      = b->type1;
    wEnemyMon.type2      = b->type2;
    wEnemyMon.catch_rate = b->catch_rate;
    memcpy(wEnemyMon.moves, b->start_moves, 4);
    wEnemyMon.dvs        = (uint16_t)((dv_atk << 12) | (dv_def << 8) | (dv_spd << 4) | dv_spc);
    wEnemyMon.level      = lv;
    wEnemyMon.max_hp     = CalcStat(b->hp,  dv_hp,  0, lv, 1);
    wEnemyMon.atk        = CalcStat(b->atk, dv_atk, 0, lv, 0);
    wEnemyMon.def        = CalcStat(b->def, dv_def, 0, lv, 0);
    wEnemyMon.spd        = CalcStat(b->spd, dv_spd, 0, lv, 0);
    wEnemyMon.spc        = CalcStat(b->spc, dv_spc, 0, lv, 0);
    /* PP = max PP from move table (no PP_UPs on wild mons) */
    for (int i = 0; i < 4; i++) {
        uint8_t mid = wEnemyMon.moves[i];
        wEnemyMon.pp[i] = mid ? gMoves[mid].pp : 0;
    }
}
