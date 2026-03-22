#pragma once
/* base_stats.h -- Generated from pokered-master/data/pokemon/base_stats/<all>.asm
 *
 * gBaseStats is indexed by POKEDEX NUMBER (1-based, 0 unused).
 * gSpeciesToDex converts internal species ID to Pokedex number (1..151).
 */
#include <stdint.h>
#include "game/types.h"

extern const base_stats_t gBaseStats[152];
extern const uint8_t gSpeciesToDex[256];
