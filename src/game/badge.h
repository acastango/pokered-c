#pragma once
#include <stdint.h>

/* Badge bit indices in wObtainedBadges (1 byte).
 * Mirrors BIT_*BADGE constants from pokered constants/ram_constants.asm. */
#define BADGE_BOULDER  0   /* Pewter  — Brock */
#define BADGE_CASCADE  1   /* Cerulean — Misty */
#define BADGE_THUNDER  2   /* Vermilion — Lt. Surge */
#define BADGE_RAINBOW  3   /* Celadon — Erika */
#define BADGE_SOUL     4   /* Fuchsia — Koga */
#define BADGE_MARSH    5   /* Saffron — Sabrina */
#define BADGE_VOLCANO  6   /* Cinnabar — Blaine */
#define BADGE_EARTH    7   /* Viridian — Giovanni */

int  Badge_Has(int bit);
void Badge_Set(int bit);
