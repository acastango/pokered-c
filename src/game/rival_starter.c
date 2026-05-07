#include "rival_starter.h"
#include "constants.h"
#include "../platform/hardware.h"
#include "../data/event_constants.h"
#include <stdio.h>

uint8_t RivalStarter_Get(void) {
    if (wRivalStarter == STARTER1 || wRivalStarter == STARTER2 || wRivalStarter == STARTER3) {
        return wRivalStarter;
    }

    /* Fallback to story-state truth from Oak's Lab starter ball toggles.
     * Exactly two of the three balls are hidden after selection:
     *   hide(2,3) => player picked Squirtle, rival took Bulbasaur
     *   hide(1,2) => player picked Charmander, rival took Squirtle
     *   hide(1,3) => player picked Bulbasaur, rival took Charmander
     */
    if (CheckEvent(EVENT_GOT_STARTER)) {
        const int hide1 = CheckEvent(EVENT_HIDE_STARTER_BALL_1);
        const int hide2 = CheckEvent(EVENT_HIDE_STARTER_BALL_2);
        const int hide3 = CheckEvent(EVENT_HIDE_STARTER_BALL_3);
        if (hide2 && hide3 && !hide1) return STARTER3;
        if (hide1 && hide2 && !hide3) return STARTER2;
        if (hide1 && hide3 && !hide2) return STARTER1;
    }

    printf("[rival] warning: invalid wRivalStarter=0x%02X; defaulting STARTER1\n",
           (unsigned)wRivalStarter);
    return STARTER1;
}
