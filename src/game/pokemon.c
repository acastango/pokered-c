/* pokemon.c — Pokémon data utilities.
 *
 * Ports:
 *   data/growth_rates.asm      → CalcExpForLevel
 *   home/pokemon.asm InitPlayerData path → Pokemon_InitMon
 *   data/pokemon/names.asm     → Pokemon_GetName (via pokemon_names_gen.h)
 */
#include "pokemon.h"
#include "constants.h"
#include "../data/base_stats.h"
#include "../data/moves_data.h"
#include "../data/pokemon_names_gen.h"
#include "../platform/hardware.h"

#include <string.h>

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
void Pokemon_InitMon(party_mon_t *mon, uint8_t dex, uint8_t level) {
    if (dex == 0 || dex > NUM_POKEMON) return;
    const base_stats_t *bs = &gBaseStats[dex];

    memset(mon, 0, sizeof(*mon));

    /* Box fields */
    mon->base.species    = dex;          /* stored as dex number in this port */
    mon->base.box_level  = level;
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
