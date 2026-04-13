/* pokemon.c — Pokémon data utilities.
 *
 * Ports:
 *   data/growth_rates.asm      → CalcExpForLevel
 *   home/pokemon.asm InitPlayerData path → Pokemon_InitMon
 *   data/pokemon/names.asm     → Pokemon_GetName (via pokemon_names_gen.h)
 *   engine/pokemon/evos_moves.asm:383 → Pokemon_WriteMovesForLevel
 */
#include "pokemon.h"
#include "constants.h"
#include "../data/base_stats.h"
#include "../data/moves_data.h"
#include "../data/evos_moves_data.h"
#include "../data/pokemon_names_gen.h"
#include "../platform/hardware.h"

#include <string.h>
#include <stdlib.h>

extern uint16_t wPlayerID;

static uint8_t encode_name_char(char c) {
    if (c >= 'A' && c <= 'Z') return (uint8_t)(0x80 + (c - 'A'));
    if (c >= 'a' && c <= 'z') return (uint8_t)(0xA0 + (c - 'a'));
    if (c >= '0' && c <= '9') return (uint8_t)(0xF6 + (c - '0'));
    if (c == ' ')  return 0x7F;
    if (c == '.')  return 0xE8;
    if (c == '-')  return 0xE3;
    if (c == '\'') return 0xE0;
    return 0x7F;
}

void Pokemon_EncodeNameString(const char *src, uint8_t *dst) {
    int i;
    for (i = 0; i < NAME_LENGTH - 1 && src[i]; i++)
        dst[i] = encode_name_char(src[i]);
    dst[i] = 0x50;
    for (i = i + 1; i < NAME_LENGTH; i++)
        dst[i] = 0x50;
}

static uint32_t exp_bytes_to_u32(const uint8_t exp[3]) {
    return ((uint32_t)exp[0] << 16) | ((uint32_t)exp[1] << 8) | (uint32_t)exp[2];
}

static uint8_t infer_level_from_exp(uint8_t species, const uint8_t exp[3]) {
    uint8_t dex = gSpeciesToDex[species];
    uint8_t growth_rate;
    uint32_t cur_exp;
    uint8_t best = 1;

    if (dex == 0 || dex > NUM_POKEMON) return 1;
    growth_rate = gBaseStats[dex].growth_rate;
    cur_exp = exp_bytes_to_u32(exp);

    for (uint8_t lv = 1; lv <= 100; lv++) {
        uint32_t need = CalcExpForLevel(growth_rate, lv);
        if (need > cur_exp) break;
        best = lv;
    }
    return best;
}

static uint8_t hp_dv_from_dvs(uint16_t dvs) {
    uint8_t atk = (uint8_t)((dvs >> 12) & 0xF);
    uint8_t def = (uint8_t)((dvs >> 8) & 0xF);
    uint8_t spd = (uint8_t)((dvs >> 4) & 0xF);
    uint8_t spc = (uint8_t)(dvs & 0xF);
    return (uint8_t)(((atk & 1) << 3) | ((def & 1) << 2) | ((spd & 1) << 1) | (spc & 1));
}

static void fill_party_from_box(party_mon_t *dst, const box_mon_t *src) {
    uint8_t dex;
    const base_stats_t *bs;
    uint8_t level;
    uint8_t atk_dv, def_dv, spd_dv, spc_dv, hp_dv;

    memset(dst, 0, sizeof(*dst));
    memcpy(&dst->base, src, sizeof(*src));

    dex = gSpeciesToDex[src->species];
    if (dex == 0 || dex > NUM_POKEMON) return;
    bs = &gBaseStats[dex];
    level = src->box_level;
    if (level == 0) {
        /* Recover tainted saves where box_level was left zeroed by older logic. */
        level = infer_level_from_exp(src->species, src->exp);
        dst->base.box_level = level;
    }
    atk_dv = (uint8_t)((src->dvs >> 12) & 0xF);
    def_dv = (uint8_t)((src->dvs >> 8) & 0xF);
    spd_dv = (uint8_t)((src->dvs >> 4) & 0xF);
    spc_dv = (uint8_t)(src->dvs & 0xF);
    hp_dv = hp_dv_from_dvs(src->dvs);

    dst->level  = level;
    dst->max_hp = CalcStat(bs->hp,  hp_dv,  src->stat_exp_hp,  level, 1);
    dst->atk    = CalcStat(bs->atk, atk_dv, src->stat_exp_atk, level, 0);
    dst->def    = CalcStat(bs->def, def_dv, src->stat_exp_def, level, 0);
    dst->spd    = CalcStat(bs->spd, spd_dv, src->stat_exp_spd, level, 0);
    dst->spc    = CalcStat(bs->spc, spc_dv, src->stat_exp_spc, level, 0);
    if (dst->base.hp > dst->max_hp)
        dst->base.hp = dst->max_hp;
}

static void shift_party_left(int start_slot) {
    for (int i = start_slot; i < PARTY_LENGTH - 1; i++) {
        wPartyMons[i] = wPartyMons[i + 1];
        memcpy(wPartyMonOT[i], wPartyMonOT[i + 1], NAME_LENGTH);
        memcpy(wPartyMonNicks[i], wPartyMonNicks[i + 1], NAME_LENGTH);
    }
    memset(&wPartyMons[PARTY_LENGTH - 1], 0, sizeof(wPartyMons[PARTY_LENGTH - 1]));
    memset(wPartyMonOT[PARTY_LENGTH - 1], 0x50, NAME_LENGTH);
    memset(wPartyMonNicks[PARTY_LENGTH - 1], 0x50, NAME_LENGTH);
}

static void shift_box_left(uint8_t box, int start_slot) {
    for (int i = start_slot; i < BOX_CAPACITY - 1; i++) {
        wBoxMons[box][i] = wBoxMons[box][i + 1];
        memcpy(wBoxMonOT[box][i], wBoxMonOT[box][i + 1], NAME_LENGTH);
        memcpy(wBoxMonNicks[box][i], wBoxMonNicks[box][i + 1], NAME_LENGTH);
        wBoxSpecies[box][i] = wBoxSpecies[box][i + 1];
    }
    memset(&wBoxMons[box][BOX_CAPACITY - 1], 0, sizeof(wBoxMons[box][BOX_CAPACITY - 1]));
    memset(wBoxMonOT[box][BOX_CAPACITY - 1], 0x50, NAME_LENGTH);
    memset(wBoxMonNicks[box][BOX_CAPACITY - 1], 0x50, NAME_LENGTH);
    wBoxSpecies[box][BOX_CAPACITY - 1] = 0xFF;
    wBoxSpecies[box][BOX_CAPACITY] = 0xFF;
}

/* ---- CalcExpForLevel ---------------------------------------------------- *
 * Mirrors data/growth_rates.asm formula table.                              *
 * growth_rate values (GROWTH_* constants in constants.h):                   *
 *   0 = MEDIUM_FAST  : n³                                                   *
 *   1 = SLIGHTLY_FAST: 3n³/4 + 10n² - 30                                   *
 *   2 = SLIGHTLY_SLOW: 3n³/4 + 20n² - 70                                   *
 *   3 = MEDIUM_SLOW  : 6n³/5 - 15n² + 100n - 140                           *
 *   4 = FAST         : 4n³/5                                                *
 *   5 = SLOW         : 5n³/4                                                *
 * All integer arithmetic uses the same truncating division as the original. */
uint32_t CalcExpForLevel(uint8_t growth_rate, uint8_t level) {
    if (level < 2) return 0;
    uint32_t n  = level;
    uint32_t n2 = n * n;
    uint32_t n3 = n2 * n;
    switch (growth_rate) {
        case GROWTH_MEDIUM_FAST:   return n3;
        case GROWTH_SLIGHTLY_FAST: return (3 * n3 / 4) + 10 * n2 - 30;
        case GROWTH_SLIGHTLY_SLOW: return (3 * n3 / 4) + 20 * n2 - 70;
        case GROWTH_MEDIUM_SLOW:   return (6 * n3 / 5) - 15 * n2 + 100 * n - 140;
        case GROWTH_FAST:          return 4 * n3 / 5;
        case GROWTH_SLOW:          return 5 * n3 / 4;
        default:                   return n3;
    }
}

/* ---- Pokemon_InitMon ---------------------------------------------------- *
 * Mirrors the InitPlayerData path in home/pokemon.asm + engine init code.   *
 *                                                                            *
 * DVs: all 0 (gender-neutral; wild DVs are randomised at encounter time).   *
 * Stat EXP: all 0 (freshly obtained mon).                                   *
 * HP: set to max_hp (full health).                                           *
 * PP: look up base PP from gMoves table; no PP Ups applied.                  */
void Pokemon_InitMon(party_mon_t *mon, uint8_t species, uint8_t level) {
    uint8_t dex = gSpeciesToDex[species];
    if (dex == 0 || dex > NUM_POKEMON) return;
    const base_stats_t *bs = &gBaseStats[dex];

    memset(mon, 0, sizeof(*mon));

    /* Box fields */
    mon->base.species    = species;
    mon->base.box_level  = level;
    mon->base.ot_id      = wPlayerID;
    mon->base.type1      = bs->type1;
    mon->base.type2      = bs->type2;
    mon->base.catch_rate = bs->catch_rate;

    /* Starting moves + PP from move table */
    for (int i = 0; i < NUM_MOVES; i++) {
        uint8_t move_id = bs->start_moves[i];
        mon->base.moves[i] = move_id;
        if (move_id && move_id < NUM_MOVE_DEFS)
            mon->base.pp[i] = gMoves[move_id].pp;
    }

    /* Experience for this level */
    uint32_t exp = CalcExpForLevel(bs->growth_rate, level);
    u32_to_exp(exp, mon->base.exp);

    /* DVs: 0 (Atk=0, Def=0, Spd=0, Spc=0, all packed into dvs) */
    mon->base.dvs = 0;

    /* Party-only fields */
    mon->level  = level;
    mon->max_hp = CalcStat(bs->hp,  0, 0, level, 1);
    mon->atk    = CalcStat(bs->atk, 0, 0, level, 0);
    mon->def    = CalcStat(bs->def, 0, 0, level, 0);
    mon->spd    = CalcStat(bs->spd, 0, 0, level, 0);
    mon->spc    = CalcStat(bs->spc, 0, 0, level, 0);

    /* Current HP = max HP (full health) */
    mon->base.hp = mon->max_hp;
}

/* ---- Pokemon_GetName ---------------------------------------------------- */
const char *Pokemon_GetName(uint8_t dex) {
    if (dex == 0 || dex >= NUM_POKEMON_NAMES) return "";
    return kPokemonNames[dex];
}

/* ---- Pokemon_WriteMovesForLevel ----------------------------------------- *
 * Port of WriteMonMoves (engine/pokemon/evos_moves.asm:383).                 *
 *                                                                             *
 * Data format in gEvosMoves[species_id]:                                     *
 *   evo block: {type, param, species} repeated, terminated by 0x00           *
 *   move block: {learn_level, move_id} repeated, terminated by 0x00          *
 *                                                                             *
 * Algorithm: skip each evo entry (3 bytes) until 0x00 terminator.            *
 * Then walk move pairs.  If the mon already knows the move: skip.            *
 * Empty slot available: fill it.  No empty slots: shift [1..3]→[0..2],      *
 * place new move at slot 3 (keeps the last 4 learned). */
void Pokemon_WriteMovesForLevel(uint8_t *moves, uint8_t *pp,
                                uint8_t species_id, uint8_t level) {
    if (species_id == 0 || species_id >= EVOS_MOVES_TABLE_SIZE) return;
    const uint8_t *data = gEvosMoves[species_id];
    if (!data) return;

    /* Skip evo block: each entry is 3 bytes {type, param, species}.
     * type==0 means end of evo block. */
    while (*data != 0) data += 3;
    data++;  /* skip 0x00 terminator */

    /* Walk (learn_level, move_id) pairs up to target level. */
    while (*data != 0) {
        uint8_t learn_level = *data++;
        uint8_t move_id     = *data++;
        if (learn_level > level) break;

        /* Skip if already known */
        int already = 0;
        for (int i = 0; i < NUM_MOVES; i++) {
            if (moves[i] == move_id) { already = 1; break; }
        }
        if (already) continue;

        /* Find first empty slot */
        int slot = -1;
        for (int i = 0; i < NUM_MOVES; i++) {
            if (moves[i] == 0) { slot = i; break; }
        }

        if (slot >= 0) {
            moves[slot] = move_id;
            pp[slot] = (move_id < NUM_MOVE_DEFS) ? gMoves[move_id].pp : 0;
        } else {
            /* All slots full: shift left, place at slot 3 */
            moves[0] = moves[1]; pp[0] = pp[1];
            moves[1] = moves[2]; pp[1] = pp[2];
            moves[2] = moves[3]; pp[2] = pp[3];
            moves[3] = move_id;
            pp[3] = (move_id < NUM_MOVE_DEFS) ? gMoves[move_id].pp : 0;
        }
    }
}

/* ---- Pokemon_AddToParty ------------------------------------------------- *
 * Mirrors _AddPartyMon (engine/pokemon/add_mon.asm).                         *
 * Appends a new mon at wPartyMons[wPartyCount] with random DVs, then         *
 * increments wPartyCount.                                                    */
void Pokemon_AddToParty(uint8_t species, uint8_t level) {
    if (wPartyCount >= PARTY_LENGTH) return;
    uint8_t dex = gSpeciesToDex[species];
    if (dex == 0 || dex > NUM_POKEMON) return;
    const base_stats_t *bs = &gBaseStats[dex];

    party_mon_t *m = &wPartyMons[wPartyCount];
    memset(m, 0, sizeof(*m));

    /* Random DVs matching Gen 1 _AddPartyMon (Random call per pair). */
    uint8_t dv_atk = (uint8_t)(rand() & 0xF);
    uint8_t dv_def = (uint8_t)(rand() & 0xF);
    uint8_t dv_spd = (uint8_t)(rand() & 0xF);
    uint8_t dv_spc = (uint8_t)(rand() & 0xF);
    uint8_t dv_hp  = (uint8_t)(((dv_atk & 1) << 3) | ((dv_def & 1) << 2) |
                                ((dv_spd & 1) << 1) |  (dv_spc & 1));

    m->base.species    = species;
    m->base.box_level  = level;
    m->base.ot_id      = wPlayerID;
    m->base.type1      = bs->type1;
    m->base.type2      = bs->type2;
    m->base.catch_rate = bs->catch_rate;
    m->base.dvs        = (uint16_t)((dv_atk << 12) | (dv_def << 8) | (dv_spd << 4) | dv_spc);

    uint32_t exp = CalcExpForLevel(bs->growth_rate, level);
    u32_to_exp(exp, m->base.exp);

    for (int i = 0; i < 4; i++) {
        uint8_t mid = bs->start_moves[i];
        m->base.moves[i] = mid;
        m->base.pp[i]    = (mid && mid < NUM_MOVE_DEFS) ? gMoves[mid].pp : 0;
    }
    Pokemon_WriteMovesForLevel(m->base.moves, m->base.pp, species, level);

    m->level  = level;
    m->max_hp = CalcStat(bs->hp,  dv_hp,  0, level, 1);
    m->atk    = CalcStat(bs->atk, dv_atk, 0, level, 0);
    m->def    = CalcStat(bs->def, dv_def, 0, level, 0);
    m->spd    = CalcStat(bs->spd, dv_spd, 0, level, 0);
    m->spc    = CalcStat(bs->spc, dv_spc, 0, level, 0);
    m->base.hp = m->max_hp;

    wPartyCount++;
}

int Pokemon_SendBattleMonToBox(const battle_mon_t *mon) {
    uint8_t box;
    uint8_t slot;
    uint8_t dex;
    const base_stats_t *bs;
    const char *name;
    box_mon_t *dst;
    uint32_t exp;

    if (!mon) return 0;

    box = (uint8_t)(wCurrentBoxNum % NUM_BOXES);
    if (box >= NUM_BOXES) return 0;
    if (wBoxCount[box] >= BOX_CAPACITY) return 0;

    dex = gSpeciesToDex[mon->species];
    if (dex == 0 || dex > NUM_POKEMON) return 0;
    bs = &gBaseStats[dex];

    slot = wBoxCount[box];
    dst = &wBoxMons[box][slot];
    memset(dst, 0, sizeof(*dst));

    dst->species    = mon->species;
    dst->hp         = mon->hp;
    dst->box_level  = mon->level;
    dst->status     = mon->status;
    dst->type1      = mon->type1;
    dst->type2      = mon->type2;
    dst->catch_rate = mon->catch_rate;
    memcpy(dst->moves, mon->moves, sizeof(dst->moves));
    dst->ot_id      = wPlayerID;
    exp = CalcExpForLevel(bs->growth_rate, mon->level);
    u32_to_exp(exp, dst->exp);
    dst->dvs        = mon->dvs;
    memcpy(dst->pp, mon->pp, sizeof(dst->pp));

    memcpy(wBoxMonOT[box][slot], wPlayerName, NAME_LENGTH);
    name = Pokemon_GetName(dex);
    Pokemon_EncodeNameString(name ? name : "", wBoxMonNicks[box][slot]);

    wBoxCount[box]++;
    wBoxSpecies[box][slot] = mon->species;
    wBoxSpecies[box][slot + 1] = 0xFF;
    return 1;
}

int Pokemon_DepositPartyMonToBox(int party_slot) {
    uint8_t box;
    uint8_t slot;

    if (party_slot < 0 || party_slot >= wPartyCount) return 0;

    box = (uint8_t)(wCurrentBoxNum % NUM_BOXES);
    if (wBoxCount[box] >= BOX_CAPACITY) return 0;

    slot = wBoxCount[box];
    memcpy(&wBoxMons[box][slot], &wPartyMons[party_slot].base, sizeof(box_mon_t));
    /* Keep box level authoritative from party level field.
     * This prevents stale/zero base.box_level from corrupting boxed mons. */
    wBoxMons[box][slot].box_level = wPartyMons[party_slot].level;
    memcpy(wBoxMonOT[box][slot], wPartyMonOT[party_slot], NAME_LENGTH);
    memcpy(wBoxMonNicks[box][slot], wPartyMonNicks[party_slot], NAME_LENGTH);
    wBoxCount[box]++;
    wBoxSpecies[box][slot] = wPartyMons[party_slot].base.species;
    wBoxSpecies[box][slot + 1] = 0xFF;

    shift_party_left(party_slot);
    if (wPartyCount > 0) wPartyCount--;
    return 1;
}

int Pokemon_WithdrawBoxMonToParty(int box_slot) {
    uint8_t box;
    int party_slot;

    if (wPartyCount >= PARTY_LENGTH) return 0;

    box = (uint8_t)(wCurrentBoxNum % NUM_BOXES);
    if (box_slot < 0 || box_slot >= wBoxCount[box]) return 0;

    party_slot = wPartyCount;
    fill_party_from_box(&wPartyMons[party_slot], &wBoxMons[box][box_slot]);
    memcpy(wPartyMonOT[party_slot], wBoxMonOT[box][box_slot], NAME_LENGTH);
    memcpy(wPartyMonNicks[party_slot], wBoxMonNicks[box][box_slot], NAME_LENGTH);
    wPartyCount++;

    shift_box_left(box, box_slot);
    if (wBoxCount[box] > 0) wBoxCount[box]--;
    wBoxSpecies[box][wBoxCount[box]] = 0xFF;
    return 1;
}

int Pokemon_ReleaseBoxMon(int box_slot) {
    uint8_t box = (uint8_t)(wCurrentBoxNum % NUM_BOXES);

    if (box_slot < 0 || box_slot >= wBoxCount[box]) return 0;
    shift_box_left(box, box_slot);
    if (wBoxCount[box] > 0) wBoxCount[box]--;
    wBoxSpecies[box][wBoxCount[box]] = 0xFF;
    return 1;
}

/* ---- Pokemon_HealParty -------------------------------------------------- *
 * Mirrors HealParty (engine/events/heal_party.asm).                          *
 * Restores HP, PP, and clears status for all party Pokémon.                  */
void Pokemon_HealParty(void) {
    for (int i = 0; i < wPartyCount && i < PARTY_LENGTH; i++) {
        party_mon_t *mon = &wPartyMons[i];
        mon->base.status = 0;
        for (int m = 0; m < 4; m++) {
            uint8_t move_id = mon->base.moves[m];
            if (move_id == 0 || move_id >= NUM_MOVE_DEFS) continue;
            uint8_t pp_ups  = (mon->base.pp[m] >> 6) & 0x03;
            uint8_t base_pp = gMoves[move_id].pp;
            uint16_t new_pp = (uint16_t)base_pp + (uint16_t)(pp_ups * (base_pp / 5));
            if (new_pp > 63) new_pp = 63;
            mon->base.pp[m] = (uint8_t)((pp_ups << 6) | (uint8_t)new_pp);
        }
        mon->base.hp = mon->max_hp;
    }
}
