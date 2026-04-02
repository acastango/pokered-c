/* viridian_mart_scripts.c — Oak's Parcel quest (Viridian Mart clerk).
 *
 * Exact port of scripts/ViridianMart.asm:
 *   Script 0 (ViridianMartDefaultScript):
 *     Fires on map entry.  Clerk says "Hey! You came from PALLET TOWN?"
 *     then player auto-walks LEFT 1, UP 2 to the counter.
 *   Script 1 (ViridianMartOaksParcelScript):
 *     Waits for walk to finish, shows parcel quest text, gives OAK's PARCEL.
 *   Script 2 (noop):
 *     Normal mart operation — clerk callback handles shop.
 */
#include "viridian_mart_scripts.h"
#include "text.h"
#include "npc.h"
#include "pokemart.h"
#include "inventory.h"
#include "player.h"
#include "../data/event_constants.h"
#include "../platform/hardware.h"
#include "../platform/audio.h"

#include <stdio.h>

/* ---- Constants ---------------------------------------------------------- */

#define MAP_VIRIDIAN_MART  0x2a

/* ---- Text strings (text/ViridianMart.asm) ------------------------------- */

/* ViridianMartClerkYouCameFromPalletTownText */
static const char kYouCameFromPallet[] =
    "Hey! You came\nfrom PALLET TOWN?";

/* ViridianMartClerkParcelQuestText */
static const char kParcelQuestText[] =
    "You know PROF.\nOAK, right?\f"
    "His order came in.\nWill you take it\nto him?";

/* "{PLAYER} got OAK's PARCEL!" */
static const char kParcelGotText[] =
    "{PLAYER} got\nOAK's PARCEL!";

/* ViridianMartClerkSayHiToOakText — before parcel delivered, normal talk */
static const char kSayHiToOak[] =
    "Okay! Say hi to\nPROF.OAK for me!";

/* ---- State machine ------------------------------------------------------ */

typedef enum {
    VMS_IDLE = 0,

    /* Script 0: ViridianMartDefaultScript */
    VMS_ENTRY_TEXT,         /* "Hey! You came from PALLET TOWN?" shown */
    VMS_ENTRY_WALK,         /* Auto-walk: LEFT 1, UP 2 */

    /* Script 1: ViridianMartOaksParcelScript */
    VMS_PARCEL_TEXT,        /* "You know PROF. OAK..." */
    VMS_PARCEL_GIVE,        /* "{PLAYER} got OAK's PARCEL!" */
    VMS_PARCEL_DONE,        /* Text closed, back to idle */
} VirtMartState;

static VirtMartState gVMState = VMS_IDLE;

/* Auto-walk directions: LEFT, UP, UP (ASM: PAD_LEFT 1, PAD_UP 2) */
#define DIR_LEFT  2
#define DIR_UP    1

static const int kWalkDirs[] = { DIR_LEFT, DIR_UP, DIR_UP };
#define WALK_LEN 3
static int gWalkStep = 0;

/* ---- Public API --------------------------------------------------------- */

int ViridianMartScripts_IsActive(void) {
    return gVMState != VMS_IDLE;
}

/* Called on map load — mirrors ViridianMartDefaultScript (script 0).
 * Fires once: if parcel not yet obtained, clerk calls player over. */
void ViridianMartScripts_OnMapLoad(void) {
    if (wCurMap != MAP_VIRIDIAN_MART) return;
    if (CheckEvent(EVENT_GOT_OAKS_PARCEL)) return;

    /* ASM: immediately show text and freeze player */
    gVMState = VMS_ENTRY_TEXT;
    Text_ShowASCII(kYouCameFromPallet);
    printf("[viridian_mart] parcel script triggered on entry\n");
}

/* NPC callback for clerk (A-press interaction). */
void ViridianMart_ClerkCallback(void) {
    /* During scripted sequence, ignore A-press */
    if (gVMState != VMS_IDLE) return;

    if (CheckEvent(EVENT_OAK_GOT_PARCEL)) {
        /* Parcel delivered — normal mart shop (ViridianMart_TextPointers2).
         * ASM: script_mart with POKE_BALL, ANTIDOTE, PARLYZ_HEAL, BURN_HEAL. */
        ViridianMart_Start();
        return;
    }
    /* Before parcel delivered — "Say hi to PROF.OAK for me!"
     * (ViridianMart_TextPointers entry 1: ViridianMartClerkSayHiToOakText) */
    Text_ShowASCII(kSayHiToOak);
}

void ViridianMartScripts_Tick(void) {
    switch (gVMState) {
    case VMS_IDLE:
        return;

    /* ---- Script 0: Entry text then auto-walk ------------------------ */
    case VMS_ENTRY_TEXT:
        if (Text_IsOpen()) return;
        /* Text closed — start auto-walk to counter */
        gWalkStep = 0;
        gVMState = VMS_ENTRY_WALK;
        return;

    case VMS_ENTRY_WALK:
        if (Player_IsMoving()) return;
        if (gWalkStep >= WALK_LEN) {
            /* Walk done — ASM: Delay3 then show parcel quest text.
             * Face player LEFT toward clerk, clerk RIGHT toward player. */
            gPlayerFacing = 2;   /* LEFT */
            NPC_FacePlayer(0);   /* clerk turns toward player */
            NPC_BuildView(gScrollPxX, gScrollPxY); /* flush OAM positions before text */
            gVMState = VMS_PARCEL_TEXT;
            Text_ShowASCII(kParcelQuestText);
            return;
        }
        Player_DoScriptedStep(kWalkDirs[gWalkStep]);
        gWalkStep++;
        return;

    /* ---- Script 1: Give parcel -------------------------------------- */
    case VMS_PARCEL_TEXT:
        if (Text_IsOpen()) return;
        /* Give OAK's PARCEL — sound_get_key_item plays with the text */
        Inventory_Add(ITEM_OAKS_PARCEL, 1);
        SetEvent(EVENT_GOT_OAKS_PARCEL);
        Audio_PlaySFX_GetKeyItem();
        Text_ShowASCII(kParcelGotText);
        gVMState = VMS_PARCEL_GIVE;
        return;

    case VMS_PARCEL_GIVE:
        if (Text_IsOpen()) return;
        gVMState = VMS_PARCEL_DONE;
        return;

    case VMS_PARCEL_DONE:
        gVMState = VMS_IDLE;
        return;

    }
}
