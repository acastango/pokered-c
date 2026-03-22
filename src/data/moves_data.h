#pragma once
/* moves_data.h -- Generated from pokered-master/data/moves/moves.asm */
#include <stdint.h>
#include "game/types.h"

#define NUM_MOVE_DEFS 166   /* total entries in gMoves[] — distinct from NUM_MOVES (4 per mon) */

/* Indexed by move ID (0..165) */
extern const move_t gMoves[NUM_MOVE_DEFS];
