#pragma once
/* type_chart.h -- Type effectiveness lookup.
 *
 * TypeEffectiveness(attacker_type, defender_type) returns:
 *   0  = no effect
 *   5  = not very effective
 *  10  = normal (default)
 *  20  = super effective
 *
 * Type byte values: NORMAL=0x00 FIGHTING=0x01 FLYING=0x02 POISON=0x03
 *   GROUND=0x04 ROCK=0x05 BUG=0x07 GHOST=0x08 FIRE=0x14 WATER=0x15
 *   GRASS=0x16 ELECTRIC=0x17 PSYCHIC=0x18 ICE=0x19 DRAGON=0x1A
 */
#include <stdint.h>

uint8_t TypeEffectiveness(uint8_t attacker, uint8_t defender);
