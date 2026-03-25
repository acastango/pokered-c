#pragma once
/* evos_moves_data.h — Evolution and learnset data for all species. */
#include <stdint.h>
#include <stddef.h>

/* Evolution type constants (mirrors EVOLVE_* in pokemon_data_constants.asm) */
#define EVOLVE_LEVEL  1
#define EVOLVE_ITEM   2
#define EVOLVE_TRADE  3

/* Table size: 191 entries (index 0 unused, 1..190 = internal species IDs) */
#define EVOS_MOVES_TABLE_SIZE  191

extern const uint8_t * const gEvosMoves[EVOS_MOVES_TABLE_SIZE];
