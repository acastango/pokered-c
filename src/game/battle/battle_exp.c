/* battle_exp.c — Experience gain and level-up system.
 *
 * Ports:
 *   engine/battle/experience.asm  → Battle_GainExperience, DivideExpDataByNumMonsGainingExp
 *   engine/pokemon/experience.asm → Battle_CalcLevelFromExp
 *   engine/pokemon/evos_moves.asm → Battle_LearnMoveFromLevelUp
 *
 * UI calls (PrintText, DrawHUD, WaitForButton) are replaced with printf().
 * ALWAYS refer to pokered-master source before modifying.
 */

#include "battle_exp.h"
#include "../../platform/hardware.h"
#include "../../data/base_stats.h"
#include "../../data/moves_data.h"
#include "../../data/evos_moves_data.h"
#include "../pokemon.h"
#include "../constants.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Debug exp rate multiplier (100 = 1×, 150 = 1.5×, 200 = 2×, 300 = 3×).
 * Set via game_cli.py "exprate" command.  Applied in Battle_GainExperience. */
int gDebugExpRate = 100;

/* ---- Pending text queue (drained by battle_ui.c BUI_EXP_DRAIN) ---- */
#define EXP_QUEUE_MAX 20
#define EXP_TEXT_LEN  80

static char           s_exp_queue[EXP_QUEUE_MAX][EXP_TEXT_LEN];
static levelup_stats_t s_exp_stats[EXP_QUEUE_MAX];
static int            s_exp_queue_count = 0;
static int            s_exp_queue_idx   = 0;

static void exp_queue_clear(void) {
    s_exp_queue_count = 0;
    s_exp_queue_idx   = 0;
}

static void exp_queue_push(const char *text) {
    if (s_exp_queue_count < EXP_QUEUE_MAX) {
        strncpy(s_exp_queue[s_exp_queue_count], text, EXP_TEXT_LEN - 1);
        s_exp_queue[s_exp_queue_count][EXP_TEXT_LEN - 1] = '\0';
        s_exp_stats[s_exp_queue_count].valid = 0;
        s_exp_queue_count++;
    }
}

static void exp_queue_push_levelup(const char *text, uint16_t atk, uint16_t def,
                                   uint16_t spd, uint16_t spc) {
    if (s_exp_queue_count < EXP_QUEUE_MAX) {
        strncpy(s_exp_queue[s_exp_queue_count], text, EXP_TEXT_LEN - 1);
        s_exp_queue[s_exp_queue_count][EXP_TEXT_LEN - 1] = '\0';
        s_exp_stats[s_exp_queue_count] = (levelup_stats_t){ 1, atk, def, spd, spc };
        s_exp_queue_count++;
    }
}

const char *BattleExp_TakeNextText(levelup_stats_t *stats_out) {
    if (s_exp_queue_idx >= s_exp_queue_count) return NULL;
    int idx = s_exp_queue_idx++;
    if (stats_out) *stats_out = s_exp_stats[idx];
    return s_exp_queue[idx];
}

/* ============================================================
 * Battle_CalcLevelFromExp — CalcLevelFromExperience (pokemon/experience.asm:2)
 *
 * Iterates d = 2..MAX_LEVEL. Returns the last d-1 where CalcExpForLevel(d) > exp.
 * Matches the ASM loop: "if exp_for_d > current_exp, return d-1".
 * ============================================================ */
uint8_t Battle_CalcLevelFromExp(uint8_t growth_rate, uint32_t exp) {
    for (uint8_t d = 2; d <= MAX_LEVEL; d++) {
        if (CalcExpForLevel(growth_rate, d) > exp) {
            return (uint8_t)(d - 1);
        }
    }
    return MAX_LEVEL;
}

/* ============================================================
 * Battle_LearnMoveFromLevelUp — LearnMoveFromLevelUp (evos_moves.asm:323)
 *
 * Scans gEvosMoves[species_id], skips evo block, finds move at new_level.
 * Adds to first empty slot; if all full, replaces last slot (smoke-test fallback).
 * Syncs to wBattleMon if slot == wPlayerMonNumber.
 * ============================================================ */
void Battle_LearnMoveFromLevelUp(uint8_t slot, uint8_t new_level) {
    uint8_t species_id = wPartyMons[slot].base.species;
    if (species_id == 0 || species_id >= EVOS_MOVES_TABLE_SIZE) return;

    const uint8_t *data = gEvosMoves[species_id];
    if (!data) return;

    /* Skip evolution data — read until 0 byte (evos_moves.asm:.skipEvolutionDataLoop) */
    while (*data != 0) data++;
    data++;  /* skip the 0 terminator */

    /* Scan learnset pairs (level, move_id) until 0 terminator */
    while (*data != 0) {
        uint8_t learn_level = *data++;
        uint8_t move_id     = *data++;

        if (learn_level != new_level) continue;

        /* Check if move already known (evos_moves.asm:.checkCurrentMovesLoop) */
        uint8_t *moves = wPartyMons[slot].base.moves;
        uint8_t *pp    = wPartyMons[slot].base.pp;
        int already = 0;
        for (int i = 0; i < NUM_MOVES; i++) {
            if (moves[i] == move_id) { already = 1; break; }
        }
        if (already) return;

        uint8_t dex = gSpeciesToDex[species_id];

        /* Find first empty slot */
        int learn_slot = -1;
        for (int i = 0; i < NUM_MOVES; i++) {
            if (moves[i] == 0) { learn_slot = i; break; }
        }
        if (learn_slot < 0) {
            /* All slots full — replace last slot (no UI in smoke-test) */
            learn_slot = NUM_MOVES - 1;
        }

        moves[learn_slot] = move_id;
        pp[learn_slot]    = (move_id < NUM_MOVE_DEFS) ? gMoves[move_id].pp : 0;

        /* Sync to wBattleMon if active mon */
        if (slot == wPlayerMonNumber) {
            wBattleMon.moves[learn_slot] = move_id;
            wBattleMon.pp[learn_slot]    = pp[learn_slot];
        }

        {
            char _mv_buf[EXP_TEXT_LEN];
            const char *mn = (move_id < NUM_MOVE_DEFS && gMoveNames[move_id])
                             ? gMoveNames[move_id] : "a new move";
            snprintf(_mv_buf, sizeof(_mv_buf), "%s\nlearned %s!",
                     Pokemon_GetName(dex), mn);
            exp_queue_push(_mv_buf);
        }
        return;
    }
}

/* ============================================================
 * Battle_GainExperience — GainExperience (experience.asm:1)
 *
 * Awards exp + stat exp to party mons flagged in wPartyGainExpFlags.
 * Handles level-up: recalcs stats, increases HP, calls LearnMoveFromLevelUp.
 * At end: resets wPartyGainExpFlags to the currently-active mon (experience.asm:279-291).
 * ============================================================ */
void Battle_GainExperience(void) {
    /* Clear pending text queue for this exp award */
    exp_queue_clear();

    /* Link battle: no exp (experience.asm:2-4) */
    if (wLinkState == LINK_STATE_BATTLING) return;

    /* DivideExpDataByNumMonsGainingExp (experience.asm:294-325):
     * Count set bits in wPartyGainExpFlags. */
    int num_gaining = 0;
    for (int i = 0; i < PARTY_LENGTH; i++) {
        if (wPartyGainExpFlags & (1u << i)) num_gaining++;
    }
    if (num_gaining == 0) return;

    /* Get enemy data from base stats (mirrors wEnemyMonBaseStats in WRAM) */
    uint8_t e_dex = gSpeciesToDex[wEnemyMon.species];
    if (e_dex == 0 || e_dex > NUM_POKEMON) goto done;
    const base_stats_t *eb = &gBaseStats[e_dex];

    /* Per-mon divisor for shared exp (DivideExpDataByNumMonsGainingExp) */
    uint8_t e_stats[5] = {eb->hp, eb->atk, eb->def, eb->spd, eb->spc};
    uint8_t base_exp   = eb->base_exp;
    if (num_gaining > 1) {
        for (int i = 0; i < 5; i++) e_stats[i] /= (uint8_t)num_gaining;
        base_exp /= (uint8_t)num_gaining;
    }

    /* Loop over party mons (experience.asm:.partyMonLoop) */
    for (wWhichPokemon = 0; wWhichPokemon < wPartyCount; wWhichPokemon++) {
        /* Skip if HP == 0 (fainted) */
        if (wPartyMons[wWhichPokemon].base.hp == 0) continue;

        /* Skip if gain-exp flag not set for this slot */
        if (!(wPartyGainExpFlags & (1u << wWhichPokemon))) continue;

        party_mon_t *p = &wPartyMons[wWhichPokemon];

        /* Add enemy base stats to stat_exp, capped at 0xFFFF (experience.asm:.gainStatExpLoop) */
        uint16_t *sexp[5] = {
            &p->base.stat_exp_hp,  &p->base.stat_exp_atk,
            &p->base.stat_exp_def, &p->base.stat_exp_spd,
            &p->base.stat_exp_spc
        };
        for (int i = 0; i < 5; i++) {
            uint32_t v = (uint32_t)*sexp[i] + e_stats[i];
            *sexp[i]   = (v > 0xFFFF) ? 0xFFFF : (uint16_t)v;
        }

        /* exp = floor(base_exp × enemy_level / 7) (experience.asm:.statExpDone)
         * Use wEnemyMon.level — wCurEnemyLevel is only set for wild encounters
         * and is NOT updated when a trainer sends out a replacement pokemon. */
        uint32_t gained = (uint32_t)base_exp * wEnemyMon.level / 7;

        /* BoostExp ×1.5 if traded mon (OT ID ≠ player ID) */
        wGainBoostedExp = 0;
        if (p->base.ot_id != 0 && p->base.ot_id != wPlayerID) {
            gained += gained / 2;
            wGainBoostedExp = 1;
        }

        /* BoostExp ×1.5 if trainer battle (wIsInBattle != 1) */
        if (wIsInBattle != 1) {
            gained += gained / 2;
        }

        /* Debug exp rate multiplier (100 = 1×, 200 = 2×, etc.) */
        if (gDebugExpRate != 100)
            gained = gained * (uint32_t)gDebugExpRate / 100;

        wExpAmountGained = (uint16_t)(gained > 0xFFFF ? 0xFFFF : gained);

        /* Add to party mon's 3-byte exp (experience.asm:.noCarry) */
        uint32_t cur_exp = exp_to_u32(p->base.exp);
        cur_exp += gained;

        /* Cap at max-level exp (experience.asm:.next2) */
        uint8_t dex = gSpeciesToDex[p->base.species];
        uint8_t gr  = gBaseStats[dex].growth_rate;
        uint32_t max_exp = CalcExpForLevel(gr, MAX_LEVEL);
        if (cur_exp > max_exp) cur_exp = max_exp;
        u32_to_exp(cur_exp, p->base.exp);

        {
            char _exp_buf[EXP_TEXT_LEN];
            snprintf(_exp_buf, sizeof(_exp_buf), "%s\ngained %u Exp. Pts!",
                     Pokemon_GetName(dex), (unsigned)gained);
            exp_queue_push(_exp_buf);
        }

        /* CalcLevelFromExperience (experience.asm:farcall CalcLevelFromExperience) */
        uint8_t old_level = p->level;
        uint8_t new_level = Battle_CalcLevelFromExp(gr, cur_exp);
        if (new_level == old_level) continue;

        /* ---- Level-up path (experience.asm:.printGrewLevelText and before) ---- */
        p->level         = new_level;
        p->base.box_level = new_level;

        /* Recalculate stats with stat_exp (CalcStats port, experience.asm:call CalcStats) */
        const base_stats_t *bs = &gBaseStats[dex];
        uint8_t atk_dv  = (uint8_t)((p->base.dvs >> 12) & 0x0F);
        uint8_t def_dv  = (uint8_t)((p->base.dvs >>  8) & 0x0F);
        uint8_t spd_dv  = (uint8_t)((p->base.dvs >>  4) & 0x0F);
        uint8_t spc_dv  = (uint8_t)( p->base.dvs        & 0x0F);
        uint8_t hp_dv   = (uint8_t)(((atk_dv & 1) << 3) | ((def_dv & 1) << 2) |
                                     ((spd_dv & 1) << 1) |  (spc_dv & 1));

        uint16_t old_max_hp = p->max_hp;
        p->max_hp = CalcStat(bs->hp,  hp_dv,  p->base.stat_exp_hp,  new_level, 1);
        p->atk    = CalcStat(bs->atk, atk_dv, p->base.stat_exp_atk, new_level, 0);
        p->def    = CalcStat(bs->def, def_dv, p->base.stat_exp_def, new_level, 0);
        p->spd    = CalcStat(bs->spd, spd_dv, p->base.stat_exp_spd, new_level, 0);
        p->spc    = CalcStat(bs->spc, spc_dv, p->base.stat_exp_spc, new_level, 0);

        /* Add max_hp gain to current HP (experience.asm:196-204).
         * Party HP lags wBattleMon.hp during battle — use the live value for
         * the active mon so we add the gain to actual current HP, not stale HP. */
        uint16_t cur_hp  = (wWhichPokemon == wPlayerMonNumber) ? wBattleMon.hp : p->base.hp;
        uint16_t hp_gain = (p->max_hp > old_max_hp) ? (p->max_hp - old_max_hp) : 0;
        uint32_t new_hp  = (uint32_t)cur_hp + hp_gain;
        p->base.hp = (uint16_t)(new_hp > p->max_hp ? p->max_hp : new_hp);

        /* Sync wBattleMon if this is the active mon (experience.asm:205-237) */
        if (wWhichPokemon == wPlayerMonNumber) {
            wBattleMon.hp     = p->base.hp;
            wBattleMon.level  = new_level;
            wBattleMon.max_hp = p->max_hp;
            wBattleMon.atk    = p->atk;
            wBattleMon.def    = p->def;
            wBattleMon.spd    = p->spd;
            wBattleMon.spc    = p->spc;
        }

        {
            char _lvl_buf[EXP_TEXT_LEN];
            snprintf(_lvl_buf, sizeof(_lvl_buf), "%s grew\nto level %d!",
                     Pokemon_GetName(dex), new_level);
            exp_queue_push_levelup(_lvl_buf, p->atk, p->def, p->spd, p->spc);
        }

        /* LearnMoveFromLevelUp (experience.asm:256 predef LearnMoveFromLevelUp) */
        Battle_LearnMoveFromLevelUp((uint8_t)wWhichPokemon, new_level);

        /* Mark this mon as eligible to evolve after battle (experience.asm:257-261) */
        wCanEvolveFlags |= (uint8_t)(1u << wWhichPokemon);
    }

done:
    /* Reset flags: clear wPartyGainExpFlags, set bit for current active mon.
     * Also clear wPartyFoughtCurrentEnemyFlags, set bit for current active mon.
     * (experience.asm:279-291) */
    wPartyGainExpFlags = (uint8_t)(1u << wPlayerMonNumber);
    wPartyFoughtCurrentEnemyFlags = (uint8_t)(1u << wPlayerMonNumber);
}

/* ============================================================
 * Evolution helpers — split out from EvolutionAfterBattle so that
 * battle_ui.c can drive the animation frame-by-frame via BUI_EVOLUTION.
 * ============================================================ */

/* Scan evo data for party slot i, return new_species (0 = no evo). */
static uint8_t evo_check_one_mon(uint8_t i) {
    party_mon_t *p = &wPartyMons[i];
    uint8_t old_species = p->base.species;
    if (old_species == 0 || old_species >= EVOS_MOVES_TABLE_SIZE) return 0;
    const uint8_t *data = gEvosMoves[old_species];
    if (!data) return 0;

    uint8_t new_species = 0;
    int skip_mon = 0;
    while (!skip_mon && *data != 0) {
        uint8_t evo_type = *data++;
        if (evo_type == EVOLVE_TRADE) {
            if (wLinkState != LINK_STATE_TRADING) { data += 2; continue; }
            uint8_t lr = *data++; uint8_t ns = *data++;
            if (p->level < lr) { skip_mon = 1; break; }
            new_species = ns; break;
        }
        if (wLinkState == LINK_STATE_TRADING) { skip_mon = 1; break; }
        if (evo_type == EVOLVE_ITEM) {
            uint8_t item_id = *data++;
            if (wCurPartySpecies != item_id) { data += 2; continue; }
            uint8_t lr = *data++; uint8_t ns = *data++;
            if (p->level < lr) continue;
            new_species = ns; break;
        }
        if (wForceEvolution) { skip_mon = 1; break; }
        if (evo_type == EVOLVE_LEVEL) {
            uint8_t lr = *data++; uint8_t ns = *data++;
            if (p->level < lr) continue;
            new_species = ns; break;
        }
        data += 2;
    }
    return new_species;
}

/* Battle_CheckNextEvolution — find the first party mon with wCanEvolveFlags
 * set whose evo conditions are met.  Returns 1 and fills *slot_out /
 * *new_species_out; returns 0 if nothing is pending.
 * Called by battle_ui.c to decide whether to enter BUI_EVOLUTION. */
int Battle_CheckNextEvolution(uint8_t *slot_out, uint8_t *new_species_out) {
    for (uint8_t i = 0; i < wPartyCount; i++) {
        if (!(wCanEvolveFlags & (1u << i))) continue;
        uint8_t ns = evo_check_one_mon(i);
        if (ns) {
            *slot_out        = i;
            *new_species_out = ns;
            return 1;
        }
    }
    return 0;
}

/* Battle_ApplyEvolution — commit species change, recalc stats, learn move,
 * update Pokédex.  Clears the wCanEvolveFlags bit for slot.
 * Mirrors the .doEvolution block of EvolveMon (evos_moves.asm:231+). */
void Battle_ApplyEvolution(uint8_t slot, uint8_t new_species) {
    wEvolutionOccurred = 1;
    wWhichPokemon      = slot;
    party_mon_t *p     = &wPartyMons[slot];
    uint8_t old_species = p->base.species;
    wEvoOldSpecies     = old_species;
    wEvoNewSpecies     = new_species;

    uint8_t new_dex = gSpeciesToDex[new_species];
    if (new_dex == 0 || new_dex > NUM_POKEMON) {
        wCanEvolveFlags &= (uint8_t)~(1u << slot);
        return;
    }
    const base_stats_t *bs = &gBaseStats[new_dex];

    p->base.species      = new_species;
    wPartySpecies[slot]  = new_species;

    uint8_t atk_dv = (uint8_t)((p->base.dvs >> 12) & 0x0F);
    uint8_t def_dv = (uint8_t)((p->base.dvs >>  8) & 0x0F);
    uint8_t spd_dv = (uint8_t)((p->base.dvs >>  4) & 0x0F);
    uint8_t spc_dv = (uint8_t)( p->base.dvs        & 0x0F);
    uint8_t hp_dv  = (uint8_t)(((atk_dv & 1) << 3) | ((def_dv & 1) << 2) |
                                ((spd_dv & 1) << 1) |  (spc_dv & 1));

    uint16_t old_max_hp = p->max_hp;
    p->max_hp = CalcStat(bs->hp,  hp_dv,  p->base.stat_exp_hp,  p->level, 1);
    p->atk    = CalcStat(bs->atk, atk_dv, p->base.stat_exp_atk, p->level, 0);
    p->def    = CalcStat(bs->def, def_dv, p->base.stat_exp_def, p->level, 0);
    p->spd    = CalcStat(bs->spd, spd_dv, p->base.stat_exp_spd, p->level, 0);
    p->spc    = CalcStat(bs->spc, spc_dv, p->base.stat_exp_spc, p->level, 0);

    uint16_t cur_hp  = (slot == wPlayerMonNumber) ? wBattleMon.hp : p->base.hp;
    uint16_t hp_gain = (p->max_hp > old_max_hp) ? (p->max_hp - old_max_hp) : 0;
    uint32_t new_hp  = (uint32_t)cur_hp + hp_gain;
    p->base.hp = (uint16_t)(new_hp > p->max_hp ? p->max_hp : new_hp);

    p->base.type1 = bs->type1;
    p->base.type2 = bs->type2;

    if (slot == wPlayerMonNumber) {
        wBattleMon.species = new_species;
        wBattleMon.hp      = p->base.hp;
        wBattleMon.max_hp  = p->max_hp;
        wBattleMon.atk     = p->atk;
        wBattleMon.def     = p->def;
        wBattleMon.spd     = p->spd;
        wBattleMon.spc     = p->spc;
        wBattleMon.type1   = bs->type1;
        wBattleMon.type2   = bs->type2;
    }

    Battle_LearnMoveFromLevelUp(slot, p->level);

    if (new_dex >= 1 && new_dex <= NUM_POKEMON) {
        uint8_t bidx = (uint8_t)((new_dex - 1) % 8);
        uint8_t boff = (uint8_t)((new_dex - 1) / 8);
        if (boff < 19) {
            wPokedexOwned[boff] |= (uint8_t)(1u << bidx);
            wPokedexSeen[boff]  |= (uint8_t)(1u << bidx);
        }
    }

    printf("[battle]   %s evolved into %s!\n",
           Pokemon_GetName(gSpeciesToDex[old_species]),
           Pokemon_GetName(new_dex));

    wCanEvolveFlags &= (uint8_t)~(1u << slot);
}

/* Battle_CancelEvolution — clear the can-evolve flag without applying. */
void Battle_CancelEvolution(uint8_t slot) {
    wCanEvolveFlags &= (uint8_t)~(1u << slot);
}

/* ============================================================
 * Battle_EvolutionAfterBattle — EvolutionAfterBattle (evos_moves.asm:13)
 *
 * Silent fallback used outside the main battle-UI path (e.g. debug/tests).
 * battle_ui.c drives the animated path via BUI_EVOLUTION instead.
 * ============================================================ */
void Battle_EvolutionAfterBattle(void) {
    wEvolutionOccurred = 0;
    uint8_t slot, new_species;
    while (Battle_CheckNextEvolution(&slot, &new_species))
        Battle_ApplyEvolution(slot, new_species);
}

/* ============================================================
 * Battle_AddExpDirect — add raw exp to one party slot (debug/testing).
 * Handles level-up: stat recalc, HP delta, move learning, wBattleMon sync.
 * Does NOT queue text, trigger evolutions, or touch wCanEvolveFlags.
 * ============================================================ */
void Battle_AddExpDirect(uint8_t slot, uint32_t amount) {
    if (slot >= wPartyCount) return;
    party_mon_t *p = &wPartyMons[slot];
    uint8_t dex    = gSpeciesToDex[p->base.species];
    if (dex == 0 || dex > NUM_POKEMON) return;
    const base_stats_t *bs = &gBaseStats[dex];

    uint32_t cur_exp = exp_to_u32(p->base.exp);
    cur_exp += amount;
    uint32_t max_exp = CalcExpForLevel(bs->growth_rate, MAX_LEVEL);
    if (cur_exp > max_exp) cur_exp = max_exp;
    u32_to_exp(cur_exp, p->base.exp);

    uint8_t old_level = p->level;
    uint8_t new_level = Battle_CalcLevelFromExp(bs->growth_rate, cur_exp);
    if (new_level > MAX_LEVEL) new_level = MAX_LEVEL;

    if (new_level > old_level) {
        p->level          = new_level;
        p->base.box_level = new_level;

        uint8_t atk_dv = (uint8_t)((p->base.dvs >> 12) & 0x0F);
        uint8_t def_dv = (uint8_t)((p->base.dvs >>  8) & 0x0F);
        uint8_t spd_dv = (uint8_t)((p->base.dvs >>  4) & 0x0F);
        uint8_t spc_dv = (uint8_t)( p->base.dvs        & 0x0F);
        uint8_t hp_dv  = (uint8_t)(((atk_dv & 1) << 3) | ((def_dv & 1) << 2) |
                                    ((spd_dv & 1) << 1) |  (spc_dv & 1));

        uint16_t old_max_hp = p->max_hp;
        p->max_hp = CalcStat(bs->hp,  hp_dv,  p->base.stat_exp_hp,  new_level, 1);
        p->atk    = CalcStat(bs->atk, atk_dv, p->base.stat_exp_atk, new_level, 0);
        p->def    = CalcStat(bs->def, def_dv, p->base.stat_exp_def, new_level, 0);
        p->spd    = CalcStat(bs->spd, spd_dv, p->base.stat_exp_spd, new_level, 0);
        p->spc    = CalcStat(bs->spc, spc_dv, p->base.stat_exp_spc, new_level, 0);

        uint16_t hp_gain = (p->max_hp > old_max_hp) ? (p->max_hp - old_max_hp) : 0;
        uint32_t new_hp  = (uint32_t)p->base.hp + hp_gain;
        p->base.hp = (uint16_t)(new_hp > p->max_hp ? p->max_hp : new_hp);

        /* Sync wBattleMon if this is the active battle mon */
        if (slot == wPlayerMonNumber && wIsInBattle) {
            wBattleMon.max_hp = p->max_hp;
            wBattleMon.hp     = p->base.hp;
            wBattleMon.atk    = p->atk;
            wBattleMon.def    = p->def;
            wBattleMon.spd    = p->spd;
            wBattleMon.spc    = p->spc;
        }

        /* Learn moves for each level gained */
        for (uint8_t lv = (uint8_t)(old_level + 1); lv <= new_level; lv++)
            Battle_LearnMoveFromLevelUp(slot, lv);

        printf("[battle] %s grew to Lv%d!\n", Pokemon_GetName(dex), new_level);
    }
    printf("[cli] addexp: slot %d +%u exp → %u total, Lv%d\n",
           slot + 1, (unsigned)amount, (unsigned)cur_exp, (int)p->level);
}
