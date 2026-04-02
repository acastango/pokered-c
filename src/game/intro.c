/* intro.c — New-game intro sequence (Professor Oak's speech).
 *
 * Mirrors engine/movie/oak_speech/oak_speech.asm:OakSpeech.
 *
 * Text is a faithful ASCII rendering of the original pokered text files
 * (data/text/text_2.asm).  #MON → POKEMON, <PLAYER> → player name.
 *
 * InitPlayerData equivalent (init_player_data.asm):
 *   - Random 16-bit player ID
 *   - wPlayerName = "ASH"  (pokered debug default: DebugNewGamePlayerName)
 *   - wRivalName  = "GARY" (pokered debug default: DebugNewGameRivalName)
 *   - Empty party, empty bag (+ 1 POTION as Oak gives in OakSpeech)
 *   - Starting money: ¥3000 (START_MONEY EQU $3000 in init_player_data.asm)
 *
 * On completion wCurMap / wXCoord / wYCoord are set to the bedroom spawn
 * (REDS_HOUSE_2F = 0x26, special_warp_spec x=3,y=6 → our x=6,y=13).
 * game.c detects Intro_IsActive() → 0 and calls the enter-map path.
 */
#include "intro.h"
#include "text.h"
#include "inventory.h"
#include "pokemon.h"
#include "music.h"
#include "../platform/hardware.h"
#include "../data/event_constants.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ---- Starting position in REDS_HOUSE_2F ---------------------- */
/* pokered special_warp_spec: x=3, y=6 (ASM block coords, 1:1). */
#define MAP_REDS_HOUSE_2F  0x26
#define BEDROOM_X           3
#define BEDROOM_Y           6

/* ---- Player/rival default names (pokéred-encoded) ------------ */
/* Mirrors DebugNewGamePlayerName / DebugNewGameRivalName in oak_speech.asm.
 * A=0x80 B=0x81 … Z=0x99, terminator=0x50. */
static const uint8_t kNameAsh[]  = {0x80,0x92,0x87, 0x50}; /* ASH  */
static const uint8_t kNameGary[] = {0x86,0x80,0x91,0x98, 0x50}; /* GARY */

/* ---- State machine ------------------------------------------- */
typedef enum {
    IS_IDLE = 0,
    IS_SPEECH1,       /* Hello there! / My name is OAK!          */
    IS_SPEECH2,       /* This world is inhabited by POKEMON…      */
    IS_NAME_PROMPT,   /* First, what is your name? / You are ASH  */
    IS_RIVAL_PROMPT,  /* This is my grandson … His name is GARY!  */
    IS_SPEECH3,       /* ASH! Your very own POKEMON legend…       */
    IS_DONE,
} IntroState;

static IntroState gState   = IS_IDLE;
static int        gActive  = 0;
static int        gWaiting = 0; /* 1 = text shown, waiting for it to close */

/* ---- Oak speech texts (ASCII, \n=line \f=paragraph) ---------- */
static const char kSpeech1[] =
    "Hello there!\nWelcome to the\nworld of POKEMON!"
    "\fMy name is OAK!\nPeople call me\nthe POKEMON PROF!";

static const char kSpeech2[] =
    "This world is\ninhabited by\ncreatures called\nPOKEMON!"
    "\fFor some people,\nPOKEMON are\npets. Others use\nthem for fights."
    "\fMyself..."
    "\fI study POKEMON\nas a profession.";

static const char kNamePrompt[] =
    "First, what is\nyour name?"
    "\fYou shall be\ncalled ASH!";

static const char kRivalPrompt[] =
    "This is my grand-\nson. He's been\nyour rival since\nyou were a baby."
    "\fHis name is GARY!";

static const char kSpeech3[] =
    "ASH!\nYour very own\nPOKEMON legend is\nabout to unfold!"
    "\fA world of dreams\nand adventures\nwith POKEMON\nawaits! Let's go!";

/* ---- InitPlayerData (mirrors init_player_data.asm) ----------- */
static void init_player_data(void) {
    /* Random 16-bit player ID */
    wPlayerID = (uint16_t)(rand() & 0xFFFF);

    /* Names */
    memset(wPlayerName, 0x50, NAME_LENGTH);
    memset(wRivalName,  0x50, NAME_LENGTH);
    memcpy(wPlayerName, kNameAsh,  sizeof(kNameAsh));
    memcpy(wRivalName,  kNameGary, sizeof(kNameGary));

    /* Empty party, empty bag */
    wPartyCount    = 0;
    wNumBagItems   = 0;

    /* Starting money: ¥3000 BCD (START_MONEY = $3000) */
    wPlayerMoney[0] = 0x00;
    wPlayerMoney[1] = 0x30;
    wPlayerMoney[2] = 0x00;

    /* Oak gives 1 POTION at start (OakSpeech: AddItemToInventory POTION,1) */
    Inventory_Add(ITEM_POTION, 1);

    /* Clear badges and coins */
    wObtainedBadges = 0;

    printf("[intro] Player data initialized: ASH / GARY, ¥3000, 1 POTION\n");
}

/* ---- Public API ---------------------------------------------- */
void Intro_Start(void) {
    gActive  = 1;
    gWaiting = 0;
    gState   = IS_DONE; /* TEMP: skip Oak speech for testing */
    init_player_data();
    /* Play Routes2 music like the original (Music_Routes2 during OakSpeech) */
    Music_Play(MUSIC_ROUTES2);
}

int Intro_IsActive(void) { return gActive; }

void Intro_Tick(void) {
    if (!gActive) return;

    /* If text is open, let it tick — game.c already returns early for text,
     * but Intro_Tick is called from game.c after the text early-return,
     * so we just need to wait until it closes. */
    if (gWaiting) {
        if (Text_IsOpen()) return;  /* still displaying */
        gWaiting = 0;
        /* Advance state */
        switch (gState) {
            case IS_SPEECH1:     gState = IS_SPEECH2;      break;
            case IS_SPEECH2:     gState = IS_NAME_PROMPT;  break;
            case IS_NAME_PROMPT: gState = IS_RIVAL_PROMPT; break;
            case IS_RIVAL_PROMPT:gState = IS_SPEECH3;      break;
            case IS_SPEECH3:     gState = IS_DONE;         break;
            default: break;
        }
    }

    switch (gState) {
        case IS_SPEECH1:
            Text_ShowASCII(kSpeech1);
            gWaiting = 1;
            break;
        case IS_SPEECH2:
            Text_ShowASCII(kSpeech2);
            gWaiting = 1;
            break;
        case IS_NAME_PROMPT:
            Text_ShowASCII(kNamePrompt);
            gWaiting = 1;
            break;
        case IS_RIVAL_PROMPT:
            Text_ShowASCII(kRivalPrompt);
            gWaiting = 1;
            break;
        case IS_SPEECH3:
            Text_ShowASCII(kSpeech3);
            gWaiting = 1;
            break;
        case IS_DONE:
            /* Set up bedroom warp destination */
            wCurMap  = MAP_REDS_HOUSE_2F;
            wXCoord  = BEDROOM_X;
            wYCoord  = BEDROOM_Y;

            Music_Play(MUSIC_PALLET_TOWN);
            gActive = 0;
            gState  = IS_IDLE;
            break;
        default:
            break;
    }
}
