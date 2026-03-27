/* battle_init.c — Battle setup: port of StartBattle (engine/battle/core.asm).
 *
 * Covers the state-initialization path only (wild and trainer battle).
 * UI (transition animation, text) omitted.
 *
 * ALWAYS refer to pokered-master engine/battle/core.asm before modifying.
 */

#include "battle_init.h"
#include "battle_trainer.h"
#include "../../platform/hardware.h"
#include "../../data/base_stats.h"
#include "../../data/moves_data.h"
#include "../../data/trainer_data.h"
#include "../constants.h"
#include "../pokemon.h"
#include "../music.h"
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
    wPartyGainExpFlags     = (1u << 0);   /* slot 0 is fighting (core.asm:2429 send-out path) */
    wPartyFoughtCurrentEnemyFlags = (1u << 0);
    wMoveDidntMiss         = 0;
    wDamage                = 0;
    wDamageMultipliers     = DAMAGE_MULT_EFFECTIVE;
    wFirstMonsNotOutYet    = 1;
    wIsInBattle            = 1;   /* 1 = wild battle */
    wBattleType            = 0;   /* 0 = normal */
    hWhoseTurn             = 0;

    /* ---- Copy player mon (slot 0) → wBattleMon (core.asm:LoadPlayerMon) ---- */
    wPlayerMonNumber = 0;

    /* Populate wPartySpecies (wram.asm:wPartyDataStart + 1): species list used by
     * GainExperience to find each mon's growth rate. Mirrors the 7-byte array that
     * wPartyCount species IDs + 0xFF terminator occupy in the original. */
    for (int i = 0; i < wPartyCount && i < PARTY_LENGTH; i++)
        wPartySpecies[i] = wPartyMons[i].base.species;
    wPartySpecies[wPartyCount < PARTY_LENGTH ? wPartyCount : PARTY_LENGTH] = 0xFF;

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

    /* Unmodified stats — base for CalculateModifiedStats after stat changes */
    wPlayerMonUnmodifiedAttack  = wBattleMon.atk;
    wPlayerMonUnmodifiedDefense = wBattleMon.def;
    wPlayerMonUnmodifiedSpeed   = wBattleMon.spd;
    wPlayerMonUnmodifiedSpecial = wBattleMon.spc;

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
    wEnemyMon.dvs        = (uint16_t)((dv_atk << 12) | (dv_def << 8) | (dv_spd << 4) | dv_spc);
    wEnemyMon.level      = lv;
    wEnemyMon.max_hp     = CalcStat(b->hp,  dv_hp,  0, lv, 1);
    wEnemyMon.atk        = CalcStat(b->atk, dv_atk, 0, lv, 0);
    wEnemyMon.def        = CalcStat(b->def, dv_def, 0, lv, 0);
    wEnemyMon.spd        = CalcStat(b->spd, dv_spd, 0, lv, 0);
    wEnemyMon.spc        = CalcStat(b->spc, dv_spc, 0, lv, 0);
    /* Moves: start_moves first, then add level-up moves (mirrors _AddPartyMon / WriteMonMoves). */
    for (int i = 0; i < 4; i++) {
        uint8_t mid = b->start_moves[i];
        wEnemyMon.moves[i] = mid;
        wEnemyMon.pp[i] = (mid && mid < NUM_MOVE_DEFS) ? gMoves[mid].pp : 0;
    }
    Pokemon_WriteMovesForLevel(wEnemyMon.moves, wEnemyMon.pp, wCurPartySpecies, lv);

    wEnemyMonUnmodifiedAttack  = wEnemyMon.atk;
    wEnemyMonUnmodifiedDefense = wEnemyMon.def;
    wEnemyMonUnmodifiedSpeed   = wEnemyMon.spd;
    wEnemyMonUnmodifiedSpecial = wEnemyMon.spc;

    /* Music is started before the battle transition in game.c (check_wild_encounter /
     * check_trainer_encounter), matching PlayBattleMusic in init_battle_variables.asm.
     * Do NOT call Music_Play here — it would restart the music mid-intro. */
}

/* ---- add_enemy_mon_to_party -------------------------------------------- *
 * Called by Battle_ReadTrainer for each mon in a trainer's party.            *
 * Ports _AddPartyMon (engine/pokemon/add_mon.asm) for the enemy-party case:  *
 *   - Uses fixed trainer DVs (ATKDEFDV=$98 → atk=9,def=8; SPDSPCDV=$88)     *
 *   - Calls Pokemon_WriteMovesForLevel to fill last-4-learned moves           *
 *   - Calls CalcStat for all 6 stats                                          *
 *   - Sets HP = max HP (full health)                                          */
static void add_enemy_mon_to_party(uint8_t slot, uint8_t species, uint8_t level) {
    if (slot >= PARTY_LENGTH) return;
    uint8_t dex = gSpeciesToDex[species];
    if (dex == 0) return;
    const base_stats_t *b = &gBaseStats[dex];

    party_mon_t *m = &wEnemyMons[slot];
    memset(m, 0, sizeof(*m));

    /* Fixed trainer DVs (battle_constants.asm: ATKDEFDV=$98, SPDSPCDV=$88) */
    uint8_t dv_atk = TRAINER_ATK_DV;
    uint8_t dv_def = TRAINER_DEF_DV;
    uint8_t dv_spd = TRAINER_SPD_DV;
    uint8_t dv_spc = TRAINER_SPC_DV;
    uint8_t dv_hp  = (uint8_t)(((dv_atk & 1) << 3) | ((dv_def & 1) << 2) |
                                ((dv_spd & 1) << 1) |  (dv_spc & 1));

    m->base.species    = species;
    m->base.type1      = b->type1;
    m->base.type2      = b->type2;
    m->base.catch_rate = b->catch_rate;
    m->base.dvs        = (uint16_t)((dv_atk << 12) | (dv_def << 8) | (dv_spd << 4) | dv_spc);
    m->level           = level;

    /* Moves: start from base level-1 moves, then add higher-level ones.
     * Mirrors _AddPartyMon (add_mon.asm): copies start_moves, then WriteMonMoves. */
    for (int i = 0; i < 4; i++) {
        uint8_t mid = b->start_moves[i];
        m->base.moves[i] = mid;
        m->base.pp[i] = (mid && mid < NUM_MOVE_DEFS) ? gMoves[mid].pp : 0;
    }
    Pokemon_WriteMovesForLevel(m->base.moves, m->base.pp, species, level);

    /* Stats */
    uint16_t max_hp    = CalcStat(b->hp,  dv_hp,  0, level, 1);
    m->base.hp         = max_hp;
    m->max_hp          = max_hp;
    m->atk             = CalcStat(b->atk, dv_atk, 0, level, 0);
    m->def             = CalcStat(b->def, dv_def, 0, level, 0);
    m->spd             = CalcStat(b->spd, dv_spd, 0, level, 0);
    m->spc             = CalcStat(b->spc, dv_spc, 0, level, 0);
}

/* ---- Battle_ReadTrainer ------------------------------------------------- *
 * Port of ReadTrainer (engine/battle/read_trainer_party.asm).                *
 *                                                                             *
 * Reads party data for trainer_class / trainer_no from gTrainerPartyData,   *
 * builds wEnemyMons[], and sets wEnemyPartyCount.                            *
 *                                                                             *
 * trainer_class: 1..NUM_TRAINERS                                             *
 * trainer_no:    1-based index within that class                             */
void Battle_ReadTrainer(uint8_t trainer_class, uint8_t trainer_no) {
    if (trainer_class < 1 || trainer_class > NUM_TRAINERS) return;
    if (trainer_no < 1) return;

    const uint8_t *data = gTrainerPartyData[trainer_class - 1];

    /* Advance to the trainer_no-th party (scan past null terminators). */
    for (uint8_t skip = 1; skip < trainer_no; skip++) {
        /* Skip one party entry — scan to 0x00 terminator. */
        if (*data == TRAINER_PARTY_FMT_B) {
            data++;  /* skip 0xFF marker */
            while (*data != 0) data += 2;  /* skip (level, species) pairs */
        } else {
            data++;  /* skip uniform level byte */
            while (*data != 0) data++;     /* skip species bytes */
        }
        data++;  /* skip 0x00 terminator */
    }

    /* Parse the selected party entry into wEnemyMons[]. */
    wEnemyPartyCount = 0;
    int is_fmt_b = 0;
    if (*data == TRAINER_PARTY_FMT_B) {
        is_fmt_b = 1;
        /* Format B: 0xFF, {level, species}..., 0 */
        data++;  /* skip 0xFF marker */
        while (*data != 0 && wEnemyPartyCount < PARTY_LENGTH) {
            uint8_t lv      = *data++;
            uint8_t species = *data++;
            add_enemy_mon_to_party(wEnemyPartyCount, species, lv);
            wEnemyPartyCount++;
        }
    } else {
        /* Format A: level, {species}..., 0 */
        uint8_t lv = *data++;
        while (*data != 0 && wEnemyPartyCount < PARTY_LENGTH) {
            uint8_t species = *data++;
            add_enemy_mon_to_party(wEnemyPartyCount, species, lv);
            wEnemyPartyCount++;
        }
    }

    /* Compute prize money: base_money × level of LAST enemy mon.
     * Per read_trainer_party.asm: after loading all mons, wCurEnemyLevel holds
     * the last mon's level and is multiplied by wTrainerBaseMoney once. */
    {
        uint16_t base = (trainer_class >= 1 && trainer_class <= NUM_TRAINERS)
                        ? gTrainerBaseMoney[trainer_class - 1] : 0;
        wAmountMoneyWon = (uint32_t)base * wEnemyMons[wEnemyPartyCount - 1].level;
    }

    /* ---- Signature move injection (read_trainer_party.asm / special_moves.asm) ----
     * Only format B trainers participate (LoneMoves path is gated behind SpecialTrainer).
     * PP is intentionally NOT updated — faithful to Gen 1 behaviour. */
    if (!is_fmt_b) return;

    if (wLoneAttackNo != 0) {
        /* LoneMoves: 1-based index → {0-indexed party slot, move written to moves[2]} */
        static const uint8_t lone_moves[8][2] = {
            {1, MOVE_BIDE},        /* 1: Brock    — Onix        */
            {1, MOVE_BUBBLEBEAM},  /* 2: Misty    — Starmie     */
            {2, MOVE_THUNDERBOLT}, /* 3: Lt. Surge — Raichu     */
            {2, MOVE_MEGA_DRAIN},  /* 4: Erika    — Vileplume   */
            {3, MOVE_TOXIC},       /* 5: Koga     — Weezing     */
            {3, MOVE_PSYWAVE},     /* 6: Sabrina  — Alakazam    */
            {3, MOVE_FIRE_BLAST},  /* 7: Blaine   — Arcanine    */
            {4, MOVE_FISSURE},     /* 8: Giovanni — Rhydon      */
        };
        uint8_t idx = wLoneAttackNo - 1;
        if (idx < 8) {
            uint8_t slot = lone_moves[idx][0];
            uint8_t mid  = lone_moves[idx][1];
            if (slot < wEnemyPartyCount)
                wEnemyMons[slot].base.moves[2] = mid;
        }
        return;  /* LoneMoves and TeamMoves are mutually exclusive */
    }

    /* TeamMoves: scan for matching trainer class (Elite 4 only).
     * Writes the move to party slot 4 (the trainer's last/signature mon). */
    static const uint8_t team_moves[5][2] = {
        {OPP_LORELEI - OPP_ID_OFFSET, MOVE_BLIZZARD},
        {OPP_BRUNO   - OPP_ID_OFFSET, MOVE_FISSURE},
        {OPP_AGATHA  - OPP_ID_OFFSET, MOVE_TOXIC},
        {OPP_LANCE   - OPP_ID_OFFSET, MOVE_BARRIER},
        {0xFF, 0},  /* terminator */
    };
    for (int i = 0; team_moves[i][0] != 0xFF; i++) {
        if (team_moves[i][0] == trainer_class) {
            if (4 < wEnemyPartyCount)
                wEnemyMons[4].base.moves[2] = team_moves[i][1];
            return;
        }
    }

    /* RIVAL3 special case (special_moves.asm):
     * Pidgeot (slot 0) gets Sky Attack; starter counter-mon (slot 5) gets
     * a coverage move based on which starter the rival received. */
    if (trainer_class == (OPP_RIVAL3 - OPP_ID_OFFSET)) {
        if (wEnemyPartyCount > 0)
            wEnemyMons[0].base.moves[2] = MOVE_SKY_ATTACK;
        uint8_t rival_move;
        if      (wRivalStarter == STARTER3) rival_move = MOVE_MEGA_DRAIN;
        else if (wRivalStarter == STARTER1) rival_move = MOVE_FIRE_BLAST;
        else                                rival_move = MOVE_BLIZZARD;   /* STARTER2 */
        if (wEnemyPartyCount > 5)
            wEnemyMons[5].base.moves[2] = rival_move;
    }
}

/* ---- Battle_StartTrainer ----------------------------------------------- *
 * Port of InitBattle/InitOpponent/InitBattleCommon (engine/battle/core.asm:  *
 * InitBattle=6642, InitOpponent=6647, InitBattleCommon=6666).                *
 *                                                                             *
 * Sets up a trainer battle: same combat state init as Battle_Start(), plus:  *
 *   wIsInBattle = 2 (trainer)                                                *
 *   wAICount    = 0xFF (trainer AI marker — core.asm:6686)                   *
 *   Calls Battle_ReadTrainer to build wEnemyMons[]                           *
 *   Sets wEnemyMonPartyPos = 0xFF so Battle_EnemySendOut_State scans from 0  *
 *   Calls Battle_EnemySendOut_State to load the first alive enemy mon        */
void Battle_StartTrainer(uint8_t trainer_class, uint8_t trainer_no) {
    /* ---- Same combat state init as Battle_Start() ---- */
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
    wPartyGainExpFlags     = (1u << 0);
    wPartyFoughtCurrentEnemyFlags = (1u << 0);
    wMoveDidntMiss         = 0;
    wDamage                = 0;
    wDamageMultipliers     = DAMAGE_MULT_EFFECTIVE;
    wFirstMonsNotOutYet    = 1;
    wBattleType            = 0;
    hWhoseTurn             = 0;

    /* ---- Trainer-specific init (core.asm:InitBattleCommon) ---- */
    wIsInBattle = 2;       /* 2 = trainer battle */
    wAICount    = 0xFF;    /* trainer AI init marker (core.asm:6686) */

    /* ---- Load player mon (slot 0) → wBattleMon ---- */
    wPlayerMonNumber = 0;
    for (int i = 0; i < wPartyCount && i < PARTY_LENGTH; i++)
        wPartySpecies[i] = wPartyMons[i].base.species;
    wPartySpecies[wPartyCount < PARTY_LENGTH ? wPartyCount : PARTY_LENGTH] = 0xFF;

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
    memcpy(wBattleMon.pp, p->base.pp, 4);

    wPlayerMonUnmodifiedAttack  = wBattleMon.atk;
    wPlayerMonUnmodifiedDefense = wBattleMon.def;
    wPlayerMonUnmodifiedSpeed   = wBattleMon.spd;
    wPlayerMonUnmodifiedSpecial = wBattleMon.spc;

    /* ---- Build enemy party from trainer data ---- */
    Battle_ReadTrainer(trainer_class, trainer_no);

    /* ---- Find and load first alive enemy mon ---- */
    /* wEnemyMonPartyPos = 0xFF makes EnemySendOut_State scan from slot 0
     * (no current slot to skip — mirrors EnemySendOutFirstMon, core.asm:1292) */
    wEnemyMonPartyPos = 0xFF;
    Battle_EnemySendOut_State();
}
