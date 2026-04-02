/* pallet_scripts.c — Pallet Town map scripts.
 *
 * Exact port of scripts/PalletTown.asm (PalletTown_Script through
 * PalletTownPlayerFollowsOakScript) and engine/overworld/auto_movement.asm
 * (PalletMovementScriptPointerTable — OakMoveLeft through Done).
 *
 * Script sequence (mirrors wPalletTownCurScript states 0-4):
 *   0 DefaultScript         — trigger check (wYCoord<=1, north edge)
 *   1 OakHeyWaitScript      — "Hey! Wait!" text, show Oak
 *   2 OakWalksToPlayer      — FindPathToPlayer, MoveSprite
 *   3 NotSafeComeWithMe     — wait for Oak walk, "It's unsafe..." text
 *   4 PlayerFollowsOak      — PalletMovementScript functions 0-4
 *   5 DaisyScript/Noop      — hide Oak / end movement; OaksLab sets the
 *                             followed-Oak event after the lab-entry handoff
 *
 * ALWAYS refer to pokered-master ASM before modifying anything here.
 */
#include "pallet_scripts.h"
#include "text.h"
#include "music.h"
#include "npc.h"
#include "player.h"
#include "trainer_sight.h"
#include "../platform/hardware.h"
#include "../platform/display.h"
#include "../data/event_constants.h"

#include <stdlib.h>
#include <stdio.h>

/* ---- Constants ---------------------------------------------------- */

#define MAP_PALLET_TOWN     0x00
#define OAK_NPC_IDX         0       /* first NPC in Pallet Town objects */

/* Direction constants matching our facing system (npc.h / player.h). */
#define DIR_DOWN    0
#define DIR_UP      1
#define DIR_LEFT    2
#define DIR_RIGHT   3
#define DIR_CHANGE_FACING  4  /* NPC_CHANGE_FACING: change facing, no step */

/* ---- Text strings (text/PalletTown.asm) --------------------------- */

/* PalletTownOakHeyWaitDontGoOutText — shown with auto-dismiss in original
 * (wDoNotWaitForButtonPressAfterDisplayingText = 1).
 * We show it normally (A-press to dismiss) — close enough. */
static const char kOakHeyWait[] =
    "OAK: Hey! Wait!\nDon't go out!";

/* PalletTownOakItsUnsafeText */
static const char kOakItsUnsafe[] =
    "OAK: It's unsafe!\nWild POKEMON live\nin tall grass!"
    "\fYou need your own\nPOKEMON for your\nprotection.\nI know!"
    "\fHere, come with\nme!";

/* ---- RLE walk sequences (auto_movement.asm) ----------------------- */
/* {direction, count} pairs, terminated by {-1, 0}.
 * Exact values from RLEList_ProfOakWalkToLab / RLEList_PlayerWalkToLab. */

typedef struct { int dir; int count; } rle_entry_t;

static const rle_entry_t kOakWalkToLab[] = {
    { DIR_DOWN,  5 },
    { DIR_LEFT,  1 },
    { DIR_DOWN,  5 },
    { DIR_RIGHT, 3 },
    { DIR_UP,    1 },
    { DIR_CHANGE_FACING, 1 },   /* face lab entrance */
    { -1, 0 }
};

/* The original stores the player path in source order, then executes it from the
 * end because simulated joypad playback decrements wSimulatedJoypadStatesIndex.
 * Our port dispatches steps directly via Player_DoScriptedStep() in forward order,
 * so this table must stay in the ASM's authored order. */
static const rle_entry_t kPlayerWalkToLab[] = {
    { DIR_UP,    2 },
    { DIR_RIGHT, 3 },
    { DIR_DOWN,  5 },
    { DIR_LEFT,  1 },
    { DIR_DOWN,  6 },
    { -1, 0 }
};

/* ---- FindPathToPlayer (engine/overworld/pathfinding.asm) ---------- */
/* Generates a direction list from Oak's position to one step south of
 * the player.  Manhattan pathfinding: prioritizes the larger remaining
 * axis each iteration, exactly matching the original's interleave logic.
 *
 * Returns the number of steps written to dirs[]. */
static int find_path_to_player(int oak_x, int oak_y,
                               int player_x, int player_y,
                               int *dirs)
{
    /* Distances in steps (game coords are 1:1 with ASM block coords). */
    int ydist = abs(oak_y - player_y);
    int xdist = abs(oak_x - player_x);

    /* Stop one step short (hNPCPlayerYDistance dec'd by 1). */
    if (ydist > 0) ydist--;

    /* Direction to walk: Oak south of player → UP; Oak north → DOWN.
     * Perspective flip (hNPCPlayerRelativePosPerspective = 1) is applied:
     * CalcPositionOfPlayerRelativeToNPC computes from NPC's perspective,
     * then cpl+and flips the flags so FindPathToPlayer generates the
     * correct direction for the NPC to walk TOWARD the player. */
    int ydir = (player_y < oak_y) ? DIR_UP : DIR_DOWN;
    int xdir = (player_x < oak_x) ? DIR_LEFT : DIR_RIGHT;

    int yprog = 0, xprog = 0;
    int count = 0;

    while (yprog < ydist || xprog < xdist) {
        int yrem = ydist - yprog;
        int xrem = xdist - xprog;
        if (yrem == 0 && xrem == 0) break;

        /* Compare remaining distances — larger axis gets priority.
         * Matches: ld a,e / cp d / jr c,.yDistanceGreater */
        if (xrem >= yrem && xprog < xdist) {
            dirs[count++] = xdir;
            xprog++;
        } else {
            dirs[count++] = ydir;
            yprog++;
        }
    }
    return count;
}

/* ---- Decode RLE into flat direction array ------------------------- */
static int decode_rle(const rle_entry_t *rle, int *dirs) {
    int n = 0;
    for (int i = 0; rle[i].dir != -1; i++) {
        for (int j = 0; j < rle[i].count; j++)
            dirs[n++] = rle[i].dir;
    }
    return n;
}

static void reverse_dirs(int *dirs, int count) {
    for (int i = 0; i < count / 2; i++) {
        int tmp = dirs[i];
        dirs[i] = dirs[count - 1 - i];
        dirs[count - 1 - i] = tmp;
    }
}

/* ---- State machine ------------------------------------------------ */
typedef enum {
    PS_IDLE = 0,

    /* Script 0: DefaultScript — trigger check */
    PS_TRIGGER,

    /* Script 1: OakHeyWaitScript */
    PS_TEXT_HEY_WAIT,           /* show "Hey! Wait!" text */
    PS_TEXT_HEY_WAIT_CLOSE,     /* wait for text to close */
    PS_EMOTE,                   /* exclamation bubble + delay */
    PS_SHOW_OAK,                /* ShowObject(Oak) */

    /* Script 2: OakWalksToPlayerScript */
    PS_OAK_FACE_UP,             /* face Oak UP + 3-frame delay */
    PS_OAK_WALK_PATH,           /* dispatch next step from path */
    PS_OAK_WALK_WAIT,           /* wait for Oak to finish step */

    /* Script 3: OakNotSafeComeWithMeScript */
    PS_OAK_ARRIVED,             /* face player DOWN */
    PS_TEXT_UNSAFE,              /* "It's unsafe ... come with me!" */
    PS_TEXT_UNSAFE_CLOSE,        /* wait for text to close */

    /* Script 4: PlayerFollowsOak — PalletMovementScript functions 0-4 */
    PS_MOVE_OAK_LEFT,           /* function 0: if player on right tile, Oak left */
    PS_MOVE_OAK_LEFT_WAIT,      /* wait for Oak left steps */
    PS_MOVE_PLAYER_LEFT,        /* function 1: player walks left same steps */
    PS_MOVE_PLAYER_LEFT_WAIT,   /* function 2: wait for player */
    PS_WALK_TO_LAB,             /* function 3: start simultaneous RLE walks */
    PS_WALK_TO_LAB_STEP,        /* advance both queues step by step */
    PS_WALK_DONE_WAIT,          /* function 4: wait for player to finish */

    /* Script 5: Done */
    PS_DONE,
} PalletState;

static PalletState gState = PS_IDLE;
static int gTimer = 0;

/* Oak pathfinding path */
static int sOakPath[32];
static int sOakPathLen = 0;
static int sOakPathIdx = 0;

/* OakMoveLeft step count */
static int sMoveLeftSteps = 0;
static int sMoveLeftIdx = 0;

/* Walk-to-lab RLE decoded queues */
static int sOakLabDirs[32];
static int sOakLabLen = 0;
static int sOakLabIdx = 0;

static int sPlayerLabDirs[32];
static int sPlayerLabLen = 0;
static int sPlayerLabIdx = 0;

/* ---- Public API --------------------------------------------------- */

int PalletScripts_IsActive(void) { return gState != PS_IDLE; }

void PalletScripts_OnMapLoad(void) {
    /* Mirrors toggleable_objects.asm: TOGGLE_PALLET_TOWN_OAK starts OFF.
     * Oak is hidden until EVENT_OAK_APPEARED_IN_PALLET fires. */
    if (wCurMap != MAP_PALLET_TOWN) {
        /* Loading a non-Pallet map: always reset the Pallet state machine.
         * Without this, PS_WALK_DONE_WAIT would fire on the first frame in
         * the new map and call NPC_HideSprite(OAK_NPC_IDX=0), which hides
         * NPC slot 0 of the new map (the rival in Oak's Lab). */
        gState = PS_IDLE;
        return;
    }
    if (!CheckEvent(EVENT_OAK_APPEARED_IN_PALLET)
        || CheckEvent(EVENT_FOLLOWED_OAK_INTO_LAB)) {
        NPC_HideSprite(OAK_NPC_IDX);
    }
    /* If the Oak walk sequence was interrupted (map reload mid-script),
     * reset to idle. */
    gState = PS_IDLE;
}

void PalletScripts_Tick(void) {
    switch (gState) {

    /* ================================================================
     * Script 0: PalletTownDefaultScript — trigger check
     * ================================================================ */
    case PS_IDLE: {
        if (wCurMap != MAP_PALLET_TOWN) return;
        if (CheckEvent(EVENT_FOLLOWED_OAK_INTO_LAB)) return;
        /* ASM PalletTownDefaultScript: triggers when wYCoord == 1.
         * Game coords now match ASM directly. Require idle because this port
         * commits coords at step start rather than step end. */
        if (Player_IsMoving()) return;
        if ((int)wYCoord != 1) return;

        /* Player reached north edge — mirrors PalletTownDefaultScript. */
        printf("[pallet] Oak encounter triggered at y=%d\n", wYCoord);

        /* xor a / ldh [hJoyHeld], a — clear input */
        /* ld a, PLAYER_DIR_DOWN / ld [wPlayerMovingDirection], a */
        gPlayerFacing = DIR_DOWN;

        /* PlaySound(SFX_STOP_ALL_MUSIC) + PlayMusic(MUSIC_OAKS_LAB) */
        Music_Play(MUSIC_OAKS_LAB);

        /* ld a, PAD_SELECT | PAD_START | PAD_CTRL_PAD / ld [wJoyIgnore], a */
        gScriptedMovement = 1;

        /* SetEvent EVENT_OAK_APPEARED_IN_PALLET */
        SetEvent(EVENT_OAK_APPEARED_IN_PALLET);

        gState = PS_TEXT_HEY_WAIT;
        return;
    }

    /* ================================================================
     * Script 1: PalletTownOakHeyWaitScript
     * ================================================================ */
    case PS_TEXT_HEY_WAIT:
        /* PalletTownOakText with wOakWalkedToPlayer=0 path:
         * wDoNotWaitForButtonPressAfterDisplayingText = 1
         * → shows "Hey! Wait! Don't go out!" then auto-closes.
         * We use normal text (requires A) — close enough. */
        Text_ShowASCII(kOakHeyWait);
        gState = PS_TEXT_HEY_WAIT_CLOSE;
        return;

    case PS_TEXT_HEY_WAIT_CLOSE:
        if (Text_IsOpen()) return;
        /* After text closes: DelayFrames(10), EmotionBubble (exclamation
         * on player sprite), PLAYER_DIR_DOWN. */
        Emote_ShowOnPlayer();
        gTimer = 60;   /* ~1 second, matches original emote display time */
        gState = PS_EMOTE;
        return;

    case PS_EMOTE:
        if (--gTimer > 0) return;
        Emote_Hide();
        gPlayerFacing = DIR_DOWN;
        /* ShowObject(TOGGLE_PALLET_TOWN_OAK) — reveal Oak */
        NPC_ShowSprite(OAK_NPC_IDX);
        gState = PS_OAK_FACE_UP;
        gTimer = 3;    /* Delay3 in original */
        return;

    case PS_SHOW_OAK:
        /* Absorbed into PS_EMOTE above */
        gState = PS_OAK_FACE_UP;
        gTimer = 3;
        return;

    /* ================================================================
     * Script 2: PalletTownOakWalksToPlayerScript
     * ================================================================ */
    case PS_OAK_FACE_UP:
        /* SetSpriteFacingDirectionAndDelay — face Oak UP + Delay3 */
        if (gTimer > 0) {
            if (gTimer == 3) NPC_SetFacing(OAK_NPC_IDX, DIR_UP);
            gTimer--;
            return;
        }
        /* CalcPositionOfPlayerRelativeToNPC + dec hNPCPlayerYDistance
         * + FindPathToPlayer → fill path from Oak to one step south of player */
        {
            int oak_x, oak_y;
            NPC_GetTilePos(OAK_NPC_IDX, &oak_x, &oak_y);
            sOakPathLen = find_path_to_player(
                oak_x, oak_y, (int)wXCoord, (int)wYCoord, sOakPath);
            sOakPathIdx = 0;
        }
        printf("[pallet] Oak path: %d steps\n", sOakPathLen);
        gState = PS_OAK_WALK_PATH;
        return;

    case PS_OAK_WALK_PATH:
        /* MoveSprite: dispatch one step at a time from the path.
         * Wait for NPC to finish before dispatching next. */
        if (NPC_IsWalking(OAK_NPC_IDX)) return;
        if (sOakPathIdx >= sOakPathLen) {
            /* Oak arrived one step south of player. */
            gState = PS_OAK_ARRIVED;
            return;
        }
        NPC_DoScriptedStep(OAK_NPC_IDX, sOakPath[sOakPathIdx++]);
        return;

    case PS_OAK_WALK_WAIT:
        /* Merged into PS_OAK_WALK_PATH */
        return;

    /* ================================================================
     * Script 3: PalletTownOakNotSafeComeWithMeScript
     * ================================================================ */
    case PS_OAK_ARRIVED:
        /* Wait for any residual Oak walk to finish. */
        if (NPC_IsWalking(OAK_NPC_IDX)) return;
        /* xor a / ld [wSpritePlayerStateData1FacingDirection], a — face player DOWN */
        gPlayerFacing = DIR_DOWN;
        gState = PS_TEXT_UNSAFE;
        return;

    case PS_TEXT_UNSAFE:
        /* PalletTownOakText with wOakWalkedToPlayer=TRUE → "It's unsafe..." */
        Text_ShowASCII(kOakItsUnsafe);
        gState = PS_TEXT_UNSAFE_CLOSE;
        return;

    case PS_TEXT_UNSAFE_CLOSE:
        if (Text_IsOpen()) return;
        /* Set up PalletMovementScript — function 0: OakMoveLeft.
         * Check if player is on the right tile of the north path (wXCoord > 0x0a).
         * Game coords now match ASM: wXCoord > 10 means right tile.
         * OakMoveLeft steps = wXCoord - 10. */
        {
            int px = (int)wXCoord;
            sMoveLeftSteps = (px > 10) ? (px - 10) : 0;
            sMoveLeftIdx = 0;
        }
        if (sMoveLeftSteps > 0) {
            gState = PS_MOVE_OAK_LEFT;
        } else {
            /* Player on left tile — skip to WalkToLab (function 3). */
            gState = PS_WALK_TO_LAB;
        }
        return;

    /* ================================================================
     * Script 4: PalletMovementScript functions 0-4
     * ================================================================ */

    /* Function 0: PalletMovementScript_OakMoveLeft */
    case PS_MOVE_OAK_LEFT:
        if (NPC_IsWalking(OAK_NPC_IDX)) return;
        if (sMoveLeftIdx >= sMoveLeftSteps) {
            gState = PS_MOVE_PLAYER_LEFT;
            return;
        }
        NPC_DoScriptedStep(OAK_NPC_IDX, DIR_LEFT);
        sMoveLeftIdx++;
        return;

    case PS_MOVE_OAK_LEFT_WAIT:
        /* Merged into PS_MOVE_OAK_LEFT */
        return;

    /* Function 1: PalletMovementScript_PlayerMoveLeft */
    case PS_MOVE_PLAYER_LEFT:
        if (Player_IsMoving()) return;
        if (sMoveLeftIdx > sMoveLeftSteps) {
            /* Done moving player left. */
            gState = PS_WALK_TO_LAB;
            return;
        }
        /* ConvertNPCMovementDirectionsToJoypadMasks + StartSimulatingJoypadStates.
         * In our port: use Player_DoScriptedStep for each left step. */
        if (sMoveLeftSteps > 0) {
            /* Re-use sMoveLeftIdx for player steps. */
            sMoveLeftIdx = 0;
            gState = PS_MOVE_PLAYER_LEFT_WAIT;
        } else {
            gState = PS_WALK_TO_LAB;
        }
        return;

    /* Function 2: PalletMovementScript_WaitAndWalkToLab */
    case PS_MOVE_PLAYER_LEFT_WAIT:
        if (Player_IsMoving()) return;
        if (sMoveLeftIdx >= sMoveLeftSteps) {
            gState = PS_WALK_TO_LAB;
            return;
        }
        Player_DoScriptedStep(DIR_LEFT);
        sMoveLeftIdx++;
        return;

    /* Function 3: PalletMovementScript_WalkToLab */
    case PS_WALK_TO_LAB: {
        /* Decode RLE sequences into flat direction arrays. */
        sOakLabLen = decode_rle(kOakWalkToLab, sOakLabDirs);
        sOakLabIdx = 0;
        sPlayerLabLen = decode_rle(kPlayerWalkToLab, sPlayerLabDirs);
        /* In the original, DecodeRLEList fills wSimulatedJoypadStatesEnd in
         * source order, then StartSimulatingJoypadStates consumes that buffer
         * by decrementing wSimulatedJoypadStatesIndex. The player's authored
         * path therefore runs in reverse execution order. Oak's NPC movement
         * list does not use that reverse-consuming joypad buffer, so only the
         * player list is reversed here. */
        reverse_dirs(sPlayerLabDirs, sPlayerLabLen);
        sPlayerLabIdx = 0;
        int ox, oy;
        NPC_GetTilePos(OAK_NPC_IDX, &ox, &oy);
        printf("[pallet] Walk to lab start: Oak=(%d,%d) Player=(%d,%d)\n",
               ox, oy, (int)wXCoord, (int)wYCoord);
        printf("[pallet] Oak %d steps, Player %d steps\n",
               sOakLabLen, sPlayerLabLen);
        printf("[pallet] Lab warp is at (24,23)\n");
        /* Display_SaveScreenshot("walk_start.bmp"); */
        gState = PS_WALK_TO_LAB_STEP;
        return;
    }

    case PS_WALK_TO_LAB_STEP: {
        static const char *dname[] = {"DOWN","UP","LEFT","RIGHT","FACE"};
        int oak_busy = NPC_IsWalking(OAK_NPC_IDX);
        int player_busy = Player_IsMoving();

        /* Dispatch Oak's next step if idle and queue not empty. */
        if (!oak_busy && sOakLabIdx < sOakLabLen) {
            int dir = sOakLabDirs[sOakLabIdx++];
            if (dir == DIR_CHANGE_FACING) {
                NPC_SetFacing(OAK_NPC_IDX, DIR_DOWN);
                printf("[pallet] Oak step %d: CHANGE_FACING\n", sOakLabIdx-1);
            } else {
                NPC_DoScriptedStep(OAK_NPC_IDX, dir);
                int ox, oy; NPC_GetTilePos(OAK_NPC_IDX, &ox, &oy);
                printf("[pallet] Oak step %d/%d: %s → (%d,%d)\n",
                       sOakLabIdx-1, sOakLabLen, dname[dir], ox, oy);
            }
        }

        /* Dispatch player's next step only when BOTH are idle — forces lockstep.
         * In the ASM, both are driven by the same frame tick so they stay in sync. */
        if (!player_busy && !oak_busy && sPlayerLabIdx < sPlayerLabLen) {
            int dir = sPlayerLabDirs[sPlayerLabIdx++];
            Player_DoScriptedStep(dir);
            printf("[pallet] Player step %d/%d: %s → (%d,%d)\n",
                   sPlayerLabIdx-1, sPlayerLabLen, dname[dir],
                   (int)wXCoord, (int)wYCoord);
        }

        /* Check if both are done. */
        if (sOakLabIdx >= sOakLabLen && sPlayerLabIdx >= sPlayerLabLen &&
            !NPC_IsWalking(OAK_NPC_IDX) && !Player_IsMoving()) {
            printf("[pallet] Walk to lab complete. Player=(%d,%d)\n",
                   (int)wXCoord, (int)wYCoord);
            gState = PS_WALK_DONE_WAIT;
        }
        return;
    }

    /* Function 4: PalletMovementScript_Done */
    case PS_WALK_DONE_WAIT:
        /* Wait for player to finish final step. */
        if (Player_IsMoving()) return;

        /* HideObject(TOGGLE_PALLET_TOWN_OAK) */
        NPC_HideSprite(OAK_NPC_IDX);

        /* EndNPCMovementScript — clear scripted movement state */
        gScriptedMovement = 0;

        printf("[pallet] Oak encounter complete, awaiting lab handoff\n");
        gState = PS_IDLE;
        return;

    /* ================================================================
     * Default / PS_DONE
     * ================================================================ */
    case PS_DONE:
        gScriptedMovement = 0;
        gState = PS_IDLE;
        return;

    default:
        return;
    }
}
