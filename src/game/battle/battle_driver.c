/* battle_driver.c — Full battle loop orchestrator.
 *
 * Drives Battle_RunTurn() in a blocking loop, dispatching all result codes:
 *   - ENEMY_FAINTED  → check wBattleResult for wild/trainer victory, blackout,
 *                       or NONE (enemy replaced; handle simultaneous player faint)
 *   - PLAYER_FAINTED → AnyPartyAlive check, simultaneous-faint exp + outcome,
 *                       ChooseNextMon for next alive party slot
 *   - ESCAPED        → clean exit
 *
 * Auto-selects the player's first available move each turn.  Prints state to
 * stdout.  Caller must set wIsInBattle (1=wild, 2=trainer), load wBattleMon
 * and wEnemyMon, and populate wEnemyMons/wEnemyPartyCount before calling.
 *
 * ALWAYS refer to pokered-master ASM before modifying anything here.
 */

#include "battle_driver.h"
#include "battle_loop.h"
#include "battle_exp.h"
#include "battle_switch.h"
#include "battle_trainer.h"
#include "../../platform/hardware.h"
#include "../../data/base_stats.h"
#include "../../data/moves_data.h"
#include "../constants.h"
#include "../pokemon.h"
#include <stdio.h>

/* find_next_alive_party_slot
 *
 * Scans wPartyMons[0..wPartyCount-1] for the first slot with HP > 0 that
 * is not the current active mon.  Returns the slot index, or -1 if none.
 * Approximates the party menu selection from ChooseNextMon (core.asm:1086). */
static int find_next_alive_party_slot(void) {
    for (uint8_t i = 0; i < wPartyCount; i++) {
        if (i != wPlayerMonNumber && wPartyMons[i].base.hp > 0) {
            return (int)i;
        }
    }
    return -1;
}

void Battle_RunLoop(void) {
    uint8_t p_dex = gSpeciesToDex[wBattleMon.species];
    uint8_t e_dex = gSpeciesToDex[wEnemyMon.species];

    if (wIsInBattle == 2) {
        printf("[battle] Trainer sent out %s Lv.%d!\n",
               Pokemon_GetName(e_dex), wEnemyMon.level);
    } else {
        printf("[battle] Wild %s Lv.%d (HP %d/%d) appeared!\n",
               Pokemon_GetName(e_dex), wEnemyMon.level,
               wEnemyMon.hp, wEnemyMon.max_hp);
    }
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

        /* Refresh dex indices — mon may have changed if a switch-in occurred
         * inside battle state (e.g. enemy already replaced in HandleEnemyMonFainted). */
        p_dex = gSpeciesToDex[wBattleMon.species];
        e_dex = gSpeciesToDex[wEnemyMon.species];
        printf("[battle]   %s HP: %d/%d  |  %s HP: %d/%d\n",
               Pokemon_GetName(p_dex), wBattleMon.hp, wBattleMon.max_hp,
               Pokemon_GetName(e_dex), wEnemyMon.hp,  wEnemyMon.max_hp);

        /* ── Enemy mon fainted (Battle_HandleEnemyMonFainted already ran) ── */
        if (r == BATTLE_RESULT_ENEMY_FAINTED) {
            /* wBattleResult carries the final outcome set by the handler. */
            if (wBattleResult == BATTLE_OUTCOME_WILD_VICTORY) {
                printf("[battle] Wild %s fainted!\n", Pokemon_GetName(e_dex));
                break;
            }
            if (wBattleResult == BATTLE_OUTCOME_TRAINER_VICTORY) {
                printf("[battle] Trainer is out of usable Pokemon!\n");
                break;
            }
            if (wBattleResult == BATTLE_OUTCOME_BLACKOUT) {
                printf("[battle] You have no more usable Pokemon!\n");
                break;
            }

            /* BATTLE_OUTCOME_NONE: enemy was replaced, battle continues.
             * Simultaneous faint: player mon also has 0 HP — choose next
             * (core.asm:863-868: wForcePlayerToChooseMon was set by handler). */
            if (wForcePlayerToChooseMon) {
                wForcePlayerToChooseMon = 0;
                int next = find_next_alive_party_slot();
                if (next < 0) {
                    Battle_HandlePlayerBlackOut();
                    printf("[battle] You have no more usable Pokemon!\n");
                    break;
                }
                Battle_ChooseNextMon((uint8_t)next);
                p_dex = gSpeciesToDex[wBattleMon.species];
                printf("[battle] Go! %s Lv.%d!\n",
                       Pokemon_GetName(p_dex), wBattleMon.level);
            }
            e_dex = gSpeciesToDex[wEnemyMon.species];
            printf("[battle] Trainer sent out %s Lv.%d!\n",
                   Pokemon_GetName(e_dex), wEnemyMon.level);
            continue;
        }

        /* ── Player mon fainted (Battle_HandlePlayerMonFainted already ran) ─ */
        if (r == BATTLE_RESULT_PLAYER_FAINTED) {
            /* HandlePlayerMonFainted did state cleanup + simultaneous enemy
             * faint state (faint_enemy_pokemon_state).  High-level outcomes
             * are our responsibility (core.asm:973-1000). */
            printf("[battle] %s fainted!\n", Pokemon_GetName(p_dex));

            /* AnyPartyAlive → blackout (core.asm:973-976) */
            if (!Battle_AnyPartyAlive()) {
                Battle_HandlePlayerBlackOut();
                printf("[battle] You blacked out!\n");
                break;
            }

            /* Simultaneous faint: enemy also has 0 HP (core.asm:977-1000).
             * faint_enemy_pokemon_state() already ran; now award exp + resolve. */
            if (wEnemyMon.hp == 0) {
                /* Award exp (mirrors FaintEnemyPokemon exp path, core.asm:838-840) */
                wBoostExpByExpAll = 0;
                Battle_GainExperience();

                if (wIsInBattle != 2) {
                    /* Wild simultaneous faint — battle ends (core.asm:983-985) */
                    printf("[battle] Wild %s fainted too!\n", Pokemon_GetName(e_dex));
                    break;
                }
                /* Trainer: any enemy mons left? (core.asm:986-987) */
                if (!Battle_AnyEnemyPokemonAliveCheck()) {
                    Battle_TrainerBattleVictory();
                    printf("[battle] Trainer is out of usable Pokemon!\n");
                    break;
                }
                /* Replace fainted enemy; then fall through to choose player mon */
                Battle_ReplaceFaintedEnemyMon();
                e_dex = gSpeciesToDex[wEnemyMon.species];
                printf("[battle] Trainer sent out %s Lv.%d!\n",
                       Pokemon_GetName(e_dex), wEnemyMon.level);
            }

            /* Choose next alive player mon (core.asm:991 call ChooseNextMon) */
            int next = find_next_alive_party_slot();
            if (next < 0) {
                Battle_HandlePlayerBlackOut();
                printf("[battle] You blacked out!\n");
                break;
            }
            Battle_ChooseNextMon((uint8_t)next);
            p_dex = gSpeciesToDex[wBattleMon.species];
            printf("[battle] Go! %s Lv.%d!\n",
                   Pokemon_GetName(p_dex), wBattleMon.level);
            continue;
        }

        /* ── Escaped ──────────────────────────────────────────────────── */
        if (r == BATTLE_RESULT_ESCAPED) {
            printf("[battle] Got away safely!\n");
            break;
        }

        /* Safety valve: stop after 50 turns to prevent infinite loops */
        if (turn >= 50) {
            printf("[battle] Turn limit reached — ending battle.\n");
            break;
        }
    }

    /* Sync wBattleMon HP back to party so damage persists between battles */
    wPartyMons[wPlayerMonNumber].base.hp = wBattleMon.hp;
    wIsInBattle = 0;

    /* EvolutionAfterBattle (evos_moves.asm:13) — check post-battle evolutions */
    Battle_EvolutionAfterBattle();
}
