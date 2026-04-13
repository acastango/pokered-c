#pragma once
#include <stdint.h>

#define NUM_TMS   50
#define NUM_HMS    5
#define NUM_TM_HM 55

/* gTMHMMoves[0..49] = TM01–TM50 move IDs; [50..54] = HM01–HM05 move IDs.
 * Mirrors TechnicalMachines table in data/moves/tmhm_moves.asm. */
extern const uint8_t gTMHMMoves[NUM_TM_HM];
