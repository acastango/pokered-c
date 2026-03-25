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

extern uint16_t wPlayerID;

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
