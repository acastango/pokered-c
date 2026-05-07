#pragma once

#include <stdint.h>

/* Resolve the rival's starter species.
 * Primary source is wRivalStarter; if invalid, infer from the player's
 * current party starter family (Charmander/Squirtle/Bulbasaur lines). */
uint8_t RivalStarter_Get(void);
