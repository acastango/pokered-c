/* vermilion_gym_scripts.c — Vermilion Gym trashcan puzzle + NPC callbacks.
 *
 * Puzzle flow (mirrors engine/events/hidden_events/vermilion_gym_trash.asm):
 *   - wFirstLockTrashCanIndex starts at 0 (WRAM default).
 *   - Player finds can matching gFirstLockCan → EVENT_1ST_LOCK_OPENED set;
 *     gSecondLockCan computed from neighbor table + BattleRandom.
 *   - Player finds can matching gSecondLockCan → EVENT_2ND_LOCK_OPENED set;
 *     door block replaced (block 2,2 → id 0x05).
 *   - Failure (wrong can on second try) → EVENT_1ST_LOCK_OPENED cleared;
 *     gFirstLockCan re-randomised via BattleRandom() & 0x0E.
 *   - Gen 1 bug faithfully replicated: if (random & mask)==0, offset wraps
 *     to 0xFF, reading beyond table → second lock resolves to 0.
 *
 * Door block: Map_SetBlock(2, 2, block_id).
 *   Closed: block_id = 0x24   (locked gate)
 *   Open:   block_id = 0x05   (floor)
 * Reference: scripts/VermilionGym.asm VermilionGymSetDoorTile
 */
#include "vermilion_gym_scripts.h"
#include "gym_scripts.h"
#include "overworld.h"
#include "text.h"
#include "../platform/audio.h"
#include "../platform/hardware.h"
#include "../data/event_constants.h"
#include <stdint.h>

/* Trainer class/party constants (from pokered data/maps/objects/VermilionGym.asm) */
#define VG_GENTLEMAN_CLASS  41   /* OPP_GENTLEMAN = $29 */
#define VG_GENTLEMAN_NO      3
#define VG_ROCKER_CLASS     20   /* OPP_ROCKER    = $14 */
#define VG_ROCKER_NO         1
#define VG_SAILOR_CLASS     19   /* OPP_SAILOR    = $13 */
#define VG_SAILOR_NO         8

/* ---- Puzzle state ------------------------------------------------- */
/* Mirrors wFirstLockTrashCanIndex / wSecondLockTrashCanIndex in WRAM.
 * Default 0 = WRAM zero-initialisation at game start. */
static uint8_t gFirstLockCan  = 0;
static uint8_t gSecondLockCan = 0;

/* GymTrashCans neighbor table (verbatim from vermilion_gym_trash.asm):
 *   byte 0: mask for random number
 *   bytes 1-4: valid second-lock can indices (0-padded) */
static const uint8_t kGymTrashCans[15 * 5] = {
    2,  1,  3,  0,  0, /* 0 */
    3,  0,  2,  4,  0, /* 1 */
    2,  1,  5,  0,  0, /* 2 */
    3,  0,  4,  6,  0, /* 3 */
    4,  1,  3,  5,  7, /* 4 */
    3,  2,  4,  8,  0, /* 5 */
    3,  3,  7,  9,  0, /* 6 */
    4,  4,  6,  8, 10, /* 7 */
    3,  5,  7, 11,  0, /* 8 */
    3,  6, 10, 12,  0, /* 9 */
    4,  7,  9, 11, 13, /* 10 */
    3,  8, 10, 14,  0, /* 11 */
    2,  9, 13,  0,  0, /* 12 */
    3, 10, 12, 14,  0, /* 13 */
    2, 11, 13,  0,  0, /* 14 */
};

/* ---- Dialogue strings --------------------------------------------- */
static const char kTrashText[]    = "Nope, there's\nonly trash here.";
static const char kLock1Text[]    = "Hey! There's a\nswitch under the\ntrash!\nTurn it on!"
                                    "\fThe 1st electric\nlock opened!";
static const char kLock2Text[]    = "The 2nd electric\nlock opened!"
                                    "\fThe motorized door\nopened!";
static const char kFailText[]     = "Nope! There's\nonly trash here.\nHey! The electric\nlocks were reset!";

static const char kGentlemanPre[]   = "When I was in the\nArmy, LT.SURGE\nwas my strict CO!";
static const char kGentlemanEnd[]   = "Stop!\nYou're very good!";
static const char kGentlemanAfter[] = "The door won't\nopen?"
                                      "\fLT.SURGE always\nwas cautious!";

static const char kRockerPre[]   = "I'm a lightweight,\nbut I'm good with\nelectricity!";
static const char kRockerEnd[]   = "Fried!";
static const char kRockerAfter[] = "OK, I'll talk!"
                                   "\fLT.SURGE said he\nhid door switches\ninside something!";

static const char kSailorPre[]   = "This is no place\nfor kids!";
static const char kSailorEnd[]   = "Wow!\nSurprised me!";
static const char kSailorAfter[] = "LT.SURGE set up\ndouble locks!\nHere's a hint!"
                                   "\fWhen you open the\n1st lock, the 2nd\nlock is right\nnext to it!";

/* ---- Door helper -------------------------------------------------- */
static void open_door(void) {
    Map_SetBlock(2, 2, 0x05);
    Map_BuildScrollView();
}

/* ---- Core puzzle logic -------------------------------------------- */
static void gym_trash_script(int can_index) {
    /* If puzzle already complete, trash cans just say "only trash" */
    if (CheckEvent(EVENT_2ND_LOCK_OPENED)) {
        Text_ShowASCII(kTrashText);
        return;
    }

    if (!CheckEvent(EVENT_1ST_LOCK_OPENED)) {
        /* First lock: check if this is the designated can */
        if ((uint8_t)can_index != gFirstLockCan) {
            Text_ShowASCII(kTrashText);
            return;
        }
        SetEvent(EVENT_1ST_LOCK_OPENED);

        /* Compute second lock index using neighbor table + Gen 1 bug.
         * ASM: mask = table[can*5+0]; rand = swap(BattleRandom());
         *      offset = (rand & mask) - 1;  (wraps to 0xFF if result=0)
         *      second_lock = table[can*5+1+offset] & 0x0F */
        uint8_t mask       = kGymTrashCans[can_index * 5];
        uint8_t raw        = BattleRandom();
        uint8_t swapped    = (uint8_t)(((raw & 0x0F) << 4) | ((raw >> 4) & 0x0F));
        uint8_t combined   = swapped & mask;
        uint8_t offset     = (uint8_t)(combined - 1u);  /* wraps to 0xFF if combined==0 */
        if (combined == 0) {
            /* Gen 1 bug: reads beyond table → zero byte → second lock = 0 */
            gSecondLockCan = 0;
        } else {
            gSecondLockCan = kGymTrashCans[can_index * 5 + 1 + offset] & 0x0Fu;
        }

        Text_ShowASCII(kLock1Text);
        Text_SetPendingSFX(Audio_PlaySFX_Switch);   /* SFX_SWITCH after text */
        return;
    }

    /* Second lock: check if this is the designated can */
    if ((uint8_t)can_index != gSecondLockCan) {
        /* Wrong can — reset puzzle */
        ClearEvent(EVENT_1ST_LOCK_OPENED);
        gFirstLockCan = BattleRandom() & 0x0Eu;
        Text_ShowASCII(kFailText);
        Text_SetPendingSFX(Audio_PlaySFX_Denied);   /* SFX_DENIED after text */
        return;
    }

    /* Both locks open — puzzle solved */
    SetEvent(EVENT_2ND_LOCK_OPENED);
    open_door();
    Text_ShowASCII(kLock2Text);
    Text_SetPendingSFX(Audio_PlaySFX_GoInside);     /* SFX_GO_INSIDE after text */
}

/* ---- OnMapLoad ---------------------------------------------------- */
void VermilionGymScripts_OnMapLoad(void) {
    if (wCurMap != 0x5C) return;  /* VERMILION_GYM */
    /* Mirrors VermilionGymSetDoorTile: always set the correct block on entry.
     * Default .blk data has 0x05 (open floor) at (2,2), so we must always
     * explicitly set the gate block when the puzzle is unsolved. */
    if (CheckEvent(EVENT_2ND_LOCK_OPENED))
        open_door();
    else
        Map_SetBlock(2, 2, 0x24);  /* closed gate */
}

/* ---- NPC callbacks ------------------------------------------------ */
void VermilionGymScripts_GentlemanInteract(void) {
    if (CheckEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_0)) {
        Text_ShowASCII(kGentlemanAfter);
        return;
    }
    GymScripts_SetTrainerPending(VG_GENTLEMAN_CLASS, VG_GENTLEMAN_NO,
                                  EVENT_BEAT_VERMILION_GYM_TRAINER_0,
                                  kGentlemanEnd, kGentlemanAfter, kGentlemanPre);
}

void VermilionGymScripts_RockerInteract(void) {
    if (CheckEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_1)) {
        Text_ShowASCII(kRockerAfter);
        return;
    }
    GymScripts_SetTrainerPending(VG_ROCKER_CLASS, VG_ROCKER_NO,
                                  EVENT_BEAT_VERMILION_GYM_TRAINER_1,
                                  kRockerEnd, kRockerAfter, kRockerPre);
}

void VermilionGymScripts_SailorInteract(void) {
    if (CheckEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_2)) {
        Text_ShowASCII(kSailorAfter);
        return;
    }
    GymScripts_SetTrainerPending(VG_SAILOR_CLASS, VG_SAILOR_NO,
                                  EVENT_BEAT_VERMILION_GYM_TRAINER_2,
                                  kSailorEnd, kSailorAfter, kSailorPre);
}

/* ---- Trash can hidden event callbacks ----------------------------- */
void VermilionGymScripts_Trash0(void)  { gym_trash_script(0);  }
void VermilionGymScripts_Trash1(void)  { gym_trash_script(1);  }
void VermilionGymScripts_Trash2(void)  { gym_trash_script(2);  }
void VermilionGymScripts_Trash3(void)  { gym_trash_script(3);  }
void VermilionGymScripts_Trash4(void)  { gym_trash_script(4);  }
void VermilionGymScripts_Trash5(void)  { gym_trash_script(5);  }
void VermilionGymScripts_Trash6(void)  { gym_trash_script(6);  }
void VermilionGymScripts_Trash7(void)  { gym_trash_script(7);  }
void VermilionGymScripts_Trash8(void)  { gym_trash_script(8);  }
void VermilionGymScripts_Trash9(void)  { gym_trash_script(9);  }
void VermilionGymScripts_Trash10(void) { gym_trash_script(10); }
void VermilionGymScripts_Trash11(void) { gym_trash_script(11); }
void VermilionGymScripts_Trash12(void) { gym_trash_script(12); }
void VermilionGymScripts_Trash13(void) { gym_trash_script(13); }
void VermilionGymScripts_Trash14(void) { gym_trash_script(14); }
