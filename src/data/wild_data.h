#pragma once
/* wild_data.h -- Generated from pokered-master wild pokemon data. */
#include <stdint.h>

typedef struct {
    uint8_t level;
    uint8_t species;  /* internal species ID */
} wild_slot_t;

typedef struct {
    uint8_t      rate;      /* encounter rate (0 = no encounters) */
    wild_slot_t  slots[10];
} wild_mons_t;

#define NUM_MAPS 248
extern const wild_mons_t gWildGrass[NUM_MAPS];
