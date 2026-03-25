/* battle_loop.c — Gen 1 battle main loop.
 *
 * Ports of engine/battle/core.asm (bank $0F):
 *   SelectEnemyMove     (core.asm:2915)
 *   CheckNumAttacksLeft (core.asm:683)
 *   MainInBattleLoop    (core.asm:280)  — pure state, no UI
 *
 * UI calls (DisplayBattleMenu, DrawHUDs, SaveScreenTiles, TrainerAI) omitted.
 * ALWAYS refer to pokered-master ASM before modifying anything here.
 */

#include "battle_loop.h"
#include "battle_core.h"
#include "battle.h"
#include "../../platform/hardware.h"

/* ============================================================
 * Battle_TurnPrepare / Battle_TurnPlayerFirst
 * First half of MainInBattleLoop (core.asm:280-412).
 * ============================================================ */
static int s_turn_player_first = 1;

battle_result_t Battle_TurnPrepare(void) {
    wFirstMonsNotOutYet = 0;

    /* Top-of-loop HP checks (core.asm:281-289) */
    if (wBattleMon.hp == 0) {
        Battle_HandlePlayerMonFainted();
        return BATTLE_RESULT_PLAYER_FAINTED;
    }
    if (wEnemyMon.hp == 0) {
        Battle_HandleEnemyMonFainted();
        return BATTLE_RESULT_ENEMY_FAINTED;
    }

    /* Clear flinch bits when player not locked (core.asm:294-303) */
    if (!(wPlayerBattleStatus2 & ((1u << BSTAT2_NEEDS_TO_RECHARGE) |
                                   (1u << BSTAT2_USING_RAGE))) &&
        !(wPlayerBattleStatus1 & ((1u << BSTAT1_THRASHING_ABOUT) |
                                   (1u << BSTAT1_CHARGING_UP)))) {
        wEnemyBattleStatus1  &= ~(1u << BSTAT1_FLINCHED);
        wPlayerBattleStatus1 &= ~(1u << BSTAT1_FLINCHED);
    }

    /* Enemy trapping move: player cannot act (core.asm:316-322) */
    if (wEnemyBattleStatus1 & (1u << BSTAT1_USING_TRAPPING))
        wPlayerSelectedMove = CANNOT_MOVE;

    /* Select enemy move (core.asm:339) */
    Battle_SelectEnemyMove();

    BLOG("--- Turn: %s Lv%d %d/%d HP vs %s Lv%d %d/%d HP",
         BMON_P(), wBattleMon.level, wBattleMon.hp, wBattleMon.max_hp,
         BMON_E(), wEnemyMon.level,  wEnemyMon.hp,  wEnemyMon.max_hp);
    BLOG("    Player -> %-12s | Enemy -> %s",
         BMOVE(wPlayerSelectedMove), BMOVE(wEnemySelectedMove));

    /* Move order determination (core.asm:370-412) */
    {
        int p_quick   = (wPlayerSelectedMove == MOVE_QUICK_ATTACK);
        int e_quick   = (wEnemySelectedMove  == MOVE_QUICK_ATTACK);
        int p_counter = (wPlayerSelectedMove == MOVE_COUNTER);
        int e_counter = (wEnemySelectedMove  == MOVE_COUNTER);

        if      (p_quick   && !e_quick)   s_turn_player_first = 1;
        else if (e_quick   && !p_quick)   s_turn_player_first = 0;
        else if (p_counter && !e_counter) s_turn_player_first = 0;
        else if (e_counter && !p_counter) s_turn_player_first = 1;
        else {
            if      (wBattleMon.spd > wEnemyMon.spd) s_turn_player_first = 1;
            else if (wEnemyMon.spd > wBattleMon.spd) s_turn_player_first = 0;
            else    s_turn_player_first = (BattleRandom() < 128);
        }
    }

    return BATTLE_RESULT_CONTINUE;
}

int Battle_TurnPlayerFirst(void) {
    return s_turn_player_first;
}

/* ============================================================
 * Battle_SelectEnemyMove — SelectEnemyMove (core.asm:2915)
 *
 * Link battle logic omitted (wLinkState == LINK_STATE_BATTLING path).
 * Trainer AI (AIEnemyTrainerChooseMoves) omitted — random selection used
 * for both wild and trainer battles.
 * ============================================================ */
void Battle_SelectEnemyMove(void) {
    /* Early return: enemy locked into current move (core.asm:2938-2950) */
    if (wEnemyBattleStatus2 & ((1u << BSTAT2_NEEDS_TO_RECHARGE) |
                                (1u << BSTAT2_USING_RAGE)))
        return;
    if (wEnemyBattleStatus1 & ((1u << BSTAT1_CHARGING_UP) |
                                (1u << BSTAT1_THRASHING_ABOUT)))
        return;
    if (wEnemyMon.status & (STATUS_FRZ | STATUS_SLP_MASK))
        return;
    if (wEnemyBattleStatus1 & ((1u << BSTAT1_USING_TRAPPING) |
                                (1u << BSTAT1_STORING_ENERGY)))
        return;

    /* Caught in player's trapping move (core.asm:2951-2956) */
    if (wPlayerBattleStatus1 & (1u << BSTAT1_USING_TRAPPING)) {
        wEnemySelectedMove = CANNOT_MOVE;
        return;
    }

    /* Only 1 move available (core.asm:2958-2965):
     * moves[1]==0 means only slot 0 exists.  If that slot is disabled,
     * use Struggle; otherwise fall through to random (will always pick 0). */
    if (wEnemyMon.moves[1] == 0) {
        if (wEnemyDisabledMove != 0) {
            wEnemySelectedMove = MOVE_STRUGGLE;
            return;
        }
        /* fall through — random loop will always choose slot 0 */
    }

    /* Random slot selection (core.asm:2972-2999).
     * Thresholds from `percent` macro: N * $FF / 100 (truncated).
     *   25 percent = 63, 50 percent = 127, 75 percent - 1 = 190.
     * Slot probabilities: ~24.6% / 25.0% / 24.6% / 25.8%.
     * Retry if slot is disabled or move is 0 (empty). */
    {
        uint8_t slot, move;
        do {
            uint8_t r = BattleRandom();
            if      (r < 63)  slot = 0;
            else if (r < 127) slot = 1;
            else if (r < 190) slot = 2;
            else              slot = 3;

            /* Disabled slot: high nibble of wEnemyDisabledMove = 1-based slot */
            uint8_t dis = (wEnemyDisabledMove >> 4) & 0x0F;
            if (dis != 0 && dis == (uint8_t)(slot + 1)) continue;

            move = wEnemyMon.moves[slot];
        } while (move == 0);

        wEnemyMoveListIndex = slot;
        wEnemySelectedMove  = move;
    }
}

/* ============================================================
 * Battle_CheckNumAttacksLeft — CheckNumAttacksLeft (core.asm:683)
 * ============================================================ */
void Battle_CheckNumAttacksLeft(void) {
    if (wPlayerNumAttacksLeft == 0)
        wPlayerBattleStatus1 &= ~(1u << BSTAT1_USING_TRAPPING);
    if (wEnemyNumAttacksLeft == 0)
        wEnemyBattleStatus1  &= ~(1u << BSTAT1_USING_TRAPPING);
}

/* ============================================================
 * Battle_RunTurn — MainInBattleLoop state logic (core.asm:280)
 *
 * One full turn: order determination, execute, residual, faint check.
 * Calls Battle_TurnPrepare internally; kept for battle_driver / tests.
 * ============================================================ */
battle_result_t Battle_RunTurn(void) {
    battle_result_t prep = Battle_TurnPrepare();
    if (prep != BATTLE_RESULT_CONTINUE) return prep;

    /* ---- Execute in order (core.asm:413-468) ---- */
    if (!s_turn_player_first) {
        /* .enemyMovesFirst (core.asm:413) */
        hWhoseTurn = 1;
        Battle_ExecuteEnemyMove();
        if (wEscapedFromBattle) return BATTLE_RESULT_ESCAPED;
        if (wBattleMon.hp == 0) {
            Battle_HandlePlayerMonFainted();
            return BATTLE_RESULT_PLAYER_FAINTED;
        }
        /* Enemy residual after enemy's move (core.asm:425-427, .AIActionUsedEnemyFirst) */
        if (!Battle_HandlePoisonBurnLeechSeed()) {
            Battle_HandleEnemyMonFainted();
            return BATTLE_RESULT_ENEMY_FAINTED;
        }
        Battle_ExecutePlayerMove();  /* sets hWhoseTurn = 0 internally */
        if (wEscapedFromBattle) return BATTLE_RESULT_ESCAPED;
        if (wEnemyMon.hp == 0) {
            Battle_HandleEnemyMonFainted();
            return BATTLE_RESULT_ENEMY_FAINTED;
        }
        /* Player residual after player's move (core.asm:434-436, .AIActionUsedPlayerFirst) */
        if (!Battle_HandlePoisonBurnLeechSeed()) {
            Battle_HandlePlayerMonFainted();
            return BATTLE_RESULT_PLAYER_FAINTED;
        }
    } else {
        /* .playerMovesFirst (core.asm:441) */
        Battle_ExecutePlayerMove();  /* sets hWhoseTurn = 0 internally */
        if (wEscapedFromBattle) return BATTLE_RESULT_ESCAPED;
        if (wEnemyMon.hp == 0) {
            Battle_HandleEnemyMonFainted();
            return BATTLE_RESULT_ENEMY_FAINTED;
        }
        /* Player residual after player's move (core.asm:449-450) */
        if (!Battle_HandlePoisonBurnLeechSeed()) {
            Battle_HandlePlayerMonFainted();
            return BATTLE_RESULT_PLAYER_FAINTED;
        }
        hWhoseTurn = 1;
        Battle_ExecuteEnemyMove();
        if (wEscapedFromBattle) return BATTLE_RESULT_ESCAPED;
        if (wBattleMon.hp == 0) {
            Battle_HandlePlayerMonFainted();
            return BATTLE_RESULT_PLAYER_FAINTED;
        }
        /* Enemy residual after enemy's move (core.asm:464-465, .AIActionUsedPlayerFirst) */
        if (!Battle_HandlePoisonBurnLeechSeed()) {
            Battle_HandleEnemyMonFainted();
            return BATTLE_RESULT_ENEMY_FAINTED;
        }
    }

    Battle_CheckNumAttacksLeft();
    return BATTLE_RESULT_CONTINUE;
}
