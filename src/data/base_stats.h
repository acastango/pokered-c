#pragma once
/* base_stats.h -- Generated from pokered-master/data/pokemon/base_stats/<all>.asm
 *
 * gBaseStats      — indexed by POKEDEX NUMBER (1-based, 0 unused).
 * gSpeciesToDex   — internal species ID (0-255) → Pokédex number (1-151).
 * gDexToSpecies   — Pokédex number (1-151) → internal species ID.
 *                   Inverse of gSpeciesToDex; derived from dex_order.asm.
 */
#include <stdint.h>
#include "game/types.h"

extern const base_stats_t gBaseStats[152];
extern const uint8_t gSpeciesToDex[256];
extern const uint8_t gDexToSpecies[152];
