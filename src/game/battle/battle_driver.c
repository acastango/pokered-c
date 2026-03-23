/* battle_driver.c — Skeleton battle loop driver.
 *
 * Drives Battle_RunTurn() in a blocking loop, auto-selecting the player's
 * first non-empty move each turn.  Prints state to stdout.
 *
 * This is a smoke-test harness; replace with real UI once battle rendering
 * is implemented.
 */

#include "battle_driver.h"
#include "battle_loop.h"
#include "../../platform/hardware.h"
#include "../../data/base_stats.h"
#include "../../data/moves_data.h"
#include "../constants.h"
#include "../pokemon.h"
#include <stdio.h>

void Battle_RunLoop(void) {
    /* Print battle opening line */
    uint8_t p_dex = gSpeciesToDex[wBattleMon.species];  /* internal ID → dex for name lookup */
    uint8_t e_dex = gSpeciesToDex[wEnemyMon.species];  /* internal ID → dex for name lookup */
    printf("[battle] Wild %s Lv.%d (HP %d/%d) appeared!\n",
           Pokemon_GetName(e_dex), wEnemyMon.level,
           wEnemyMon.hp, wEnemyMon.max_hp);
    printf("[battle] Go! %s Lv.%d (HP %d/%d)\n",
           Pokemon_GetName(p_dex), wBattleMon.level,
           wBattleMon.hp, wBattleMon.max_hp);

    int turn = 0;
    for (;;) {
        turn++;

        /* Auto-select: first non-zero move slot, or Struggle */
        wPlayerSelectedMove = MOVE_STRUGGLE;
        for (int i = 0; i < 4; i++) {
            if (wBattleMon.moves[i]) {
                wPlayerSelectedMove = wBattleMon.moves[i];
                break;
            }
        }

        printf("[battle] --- Turn %d ---\n", turn);

        battle_result_t r = Battle_RunTurn();

        printf("[battle]   %s HP: %d/%d  |  %s HP: %d/%d\n",
               Pokemon_GetName(p_dex), wBattleMon.hp, wBattleMon.max_hp,
               Pokemon_GetName(e_dex), wEnemyMon.hp,  wEnemyMon.max_hp);

        if (r == BATTLE_RESULT_ENEMY_FAINTED) {
            printf("[battle] Wild %s fainted!\n", Pokemon_GetName(e_dex));
            break;
        }
        if (r == BATTLE_RESULT_PLAYER_FAINTED) {
            printf("[battle] %s fainted!\n", Pokemon_GetName(p_dex));
            break;
        }
        if (r == BATTLE_RESULT_ESCAPED) {
            printf("[battle] Got away safely!\n");
            break;
        }

        /* Safety valve: stop after 50 turns to prevent infinite loops
         * if both mons somehow never KO each other (e.g. all misses). */
        if (turn >= 50) {
            printf("[battle] Turn limit reached — ending battle.\n");
            break;
        }
    }

    wIsInBattle = 0;
}
