#pragma once
/* trainer_data.h — Trainer party data tables.
 *
 * Ports from data/trainers/parties.asm.
 *
 * Party format A (first byte != 0xFF):
 *   level, species..., 0
 *   All mons in the party share the same level.
 *
 * Party format B (first byte == 0xFF):
 *   0xFF, level1, species1, level2, species2, ..., 0
 *   Each mon has its own level.
 *
 * gTrainerPartyData[trainer_class - 1] points to the first party for that
 * trainer class.  Multiple trainer_no values are delimited by 0x00.
 * Battle_ReadTrainer scans to find the trainer_no-th party.
 *
 * ALWAYS refer to pokered-master ASM before modifying anything here.
 */
#include <stdint.h>
#include "game/constants.h"

#define TRAINER_PARTY_FMT_B  0xFF   /* individual-level format marker */

/* Pointer table indexed by [trainer_class - 1] (0-based). */
extern const uint8_t * const gTrainerPartyData[NUM_TRAINERS];
