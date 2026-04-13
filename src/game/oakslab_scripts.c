/* oakslab_scripts.c — Oak's Lab map scripts.
 *
 * Exact reference: scripts/OaksLab.asm
 * Implemented:
 *   OaksLabDefaultScript / OaksLabOakEntersLabScript / OaksLabToggleOaksScript
 *   OaksLabPlayerEntersLabScript / OaksLabFollowedOakScript
 *   OaksLabOakChooseMonSpeechScript
 *   OaksLabChoseStarterScript / OaksLabRivalChoosesStarterScript
 *   OaksLabRivalChallengesPlayerScript / OaksLabRivalStartBattleScript
 *   OaksLabRivalEndBattleScript / OaksLabRivalStartsExitScript
 *
 * ALWAYS refer to pokered-master ASM before modifying anything here.
 */
#include "oakslab_scripts.h"
#include "npc.h"
#include "player.h"
#include "text.h"
#include "music.h"
#include "warp.h"
#include "pokemon.h"
#include "overworld.h"
#include "../platform/hardware.h"
#include "../platform/display.h"
#include "../platform/audio.h"
#include "../data/event_constants.h"
#include "../data/base_stats.h"
#include "../data/font_data.h"
#include "inventory.h"
#include "pokedex.h"
#include "trainer_sight.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAP_OAKS_LAB 0x28

/* ---- Pokered charmap encoding ----------------------------------------- */
/* Encode a single ASCII character to its pokered charmap byte value. */
static uint8_t encode_char(char c) {
    if (c >= 'A' && c <= 'Z') return 0x80 + (c - 'A');
    if (c >= 'a' && c <= 'z') return 0xA0 + (c - 'a');
    if (c == '#') return 0x54;  /* POKé */
    if (c == ' ') return 0x7F;
    if (c == '.') return 0xE8;
    if (c == '-') return 0xE3;
    if (c == '!') return 0xE7;
    if (c == '?') return 0xE6;
    if (c == '\n') return 0x4E;  /* <LINE> */
    return 0x7F;  /* default to space */
}

/* Encode ASCII string to pokered charmap, store in output buffer. */
static void encode_string(const char *src, uint8_t *dst, int max_len) {
    int i = 0;
    while (i < max_len - 1 && *src) {
        *dst++ = encode_char(*src++);
        i++;
    }
    *dst = 0x50;  /* CHAR_TERM */
}

/* NPC slot indices for kNpcs_OaksLab (see event_data.c):
 *   0 Rival, 1 Charmander ball, 2 Squirtle ball, 3 Bulbasaur ball,
 *   4 Oak at table, 5 Pokedex1, 6 Pokedex2,
 *   7 Oak at entrance, 8 Girl, 9 Scientist1, 10 Scientist2. */
#define OAKSLAB_RIVAL_IDX       0
#define OAKSLAB_CHARMANDER_IDX  1
#define OAKSLAB_SQUIRTLE_IDX    2
#define OAKSLAB_BULBASAUR_IDX   3
#define OAKSLAB_OAK1_IDX        4
#define OAKSLAB_POKEDEX1_IDX    5
#define OAKSLAB_POKEDEX2_IDX    6
#define OAKSLAB_OAK2_IDX        7

#define DIR_DOWN  0
#define DIR_UP    1
#define DIR_LEFT  2
#define DIR_RIGHT 3

/* ASM y=6: player must be this far south to trigger rival challenge.
 * Check is wYCoord < CHALLENGE_TRIGGER_Y → fires when wYCoord >= 7. */
#define CHALLENGE_TRIGGER_Y  7

/* OaksLabRivalStartsExitScript uses five downward steps in the ASM. */
#define RIVAL_EXIT_STEPS  5

typedef enum {
    OLS_IDLE = 0,

    /* Oak enters lab and swaps with Oak at table */
    OLS_OAK_ENTER,
    OLS_OAK_SWAP,

    /* Player walks into lab (scripted auto-walk) */
    OLS_PLAYER_ENTER_SETUP,
    OLS_PLAYER_ENTER_WALK,
    OLS_FOLLOWED,

    /* Opening speech */
    OLS_SPEECH_SHOW,
    OLS_SPEECH_WAIT,

    /* Ball interaction — player pressed A on a Pokéball NPC */
    OLS_BALL_CANT_YET,      /* EVENT_OAK_ASKED_TO_CHOOSE_MON not set */
    OLS_BALL_CANT_WAIT,
    OLS_BALL_LAST_MON,      /* Already picked a starter */
    OLS_BALL_LAST_WAIT,
    OLS_DEX_SHOW,           /* StarterDex: show Pokédex entry for selected ball */
    OLS_DEX_WAIT,           /* Wait for dex entry screen to close */
    OLS_DEX_RESTORE,        /* Reload map GFX after dex screen */
    OLS_BALL_CONFIRM,       /* "Do you want X?" text shown */
    OLS_BALL_CONFIRM_WAIT,
    OLS_BALL_YESNO,         /* YES/NO box active */
    OLS_BALL_DECLINED,      /* "Take your time" shown after player says NO */
    OLS_BALL_DECLINED_WAIT,
    OLS_BALL_ENERGETIC,     /* "It's energetic!" text */
    OLS_BALL_ENERGETIC_WAIT,
    OLS_BALL_RECEIVED,      /* "You received X!" text */
    OLS_BALL_RECEIVED_WAIT,

    /* Rival picks his starter */
    OLS_RIVAL_WALK,         /* Rival walks toward his ball */
    OLS_RIVAL_WALK_WAIT,
    OLS_RIVAL_TAKES,        /* "I'll take this one!" */
    OLS_RIVAL_TAKES_WAIT,
    OLS_RIVAL_RCVD,         /* "Rival received X!" */
    OLS_RIVAL_RCVD_WAIT,

    /* Player walks south → rival challenges */
    OLS_AWAIT_CHALLENGE,
    OLS_CHALLENGE_TEXT,     /* "Wait, Player! Let's check out our #MON!" */
    OLS_CHALLENGE_WAIT,
    OLS_RIVAL_APPROACH,     /* Rival walks toward player */
    OLS_RIVAL_APPROACH_WAIT,
    OLS_BATTLE_TRIGGER,     /* Signal game.c to start battle */

    /* Post-battle: rival exits */
    OLS_POST_BATTLE_SETUP,  /* battle cleanup, heal, lock input */
    OLS_POST_BATTLE_DELAY,  /* 20-frame pause before exit line */
    OLS_POST_BATTLE_TEXT,   /* "Okay! I'll make my #MON fight to toughen it up!" */
    OLS_POST_BATTLE_WAIT,   /* wait for exit line to finish */
    OLS_RIVAL_EXIT_WALK,    /* Rival walks to door */
    OLS_RIVAL_EXIT_WAIT,    /* Player watches rival exit */
    OLS_RIVAL_GONE,         /* Hide rival, restore music, return to IDLE */

    /* Oak's Parcel delivery → Pokédex giving (OaksLabOak1Text / OaksLabOakGivesPokedexScript) */
    OLS_PARCEL_TEXT1,       /* "OAK: Oh, {PLAYER}!…" */
    OLS_PARCEL_TEXT2,       /* "Ah! This is the custom BALL I ordered! Thank you!" */
    OLS_RIVAL_ARRIVE_TEXT,  /* "{RIVAL}: What did you call me for?" */
    OLS_OAK_REQUEST_TEXT,   /* "OAK: Oh right! I have a request of you two." */
    OLS_POKEDEX_TEXT,       /* "On the desk there is my invention, #DEX!…" */
    OLS_GIVE_POKEDEX,       /* Give Pokédex item + set events */
    OLS_GAVE_POKEDEX_TEXT,  /* "{PLAYER} got #DEX from OAK!" */
    OLS_OAK_DREAM_TEXT,     /* "OAK: To make a complete guide on all #MON…" */
    OLS_RIVAL_LEAVE_TEXT,   /* "{RIVAL}: Alright Gramps! Leave it all to me!" */
    OLS_RIVAL_LEAVE_WALK,   /* Rival walks down and out */
    OLS_RIVAL_LEAVE_DONE,   /* Rival gone, restore state */

    OLS_PARCEL_SHOW_RIVAL,  /* Wait for "Gramps!" text, then position+show rival at entrance */
    OLS_RIVAL_WALK_IN,      /* Rival walks up from entrance toward Oak */
} oakslab_state_t;

static oakslab_state_t gState          = OLS_IDLE;
static int gOakEnterSteps              = 0;
static int gPlayerEnterSteps           = 0;
static int gSpeechIdx                  = 0;
static int gDelay                      = 0;

/* Starter selection state */
static uint8_t gSelectedSpecies        = 0;  /* player's chosen species */
static uint8_t gRivalStarterSpecies    = 0;  /* rival's species */
static int     gSelectedBallIdx        = 0;  /* NPC slot of player's ball */
static int     gRivalBallIdx           = 0;  /* NPC slot of rival's ball */
static int     gYesNoCursor            = 0;  /* 0=YES, 1=NO */
static int     gShowDataDelay          = 0;  /* ReloadMapData delay after dex screen */

/* Rival walk-to-ball state */
static int gRivalWalkDirs[8];
static int gRivalWalkLen               = 0;
static int gRivalWalkStep              = 0;

/* Rival approach (walk to player) state */
#define MAX_APPROACH_STEPS 16
static int gRivalApproachDirs[MAX_APPROACH_STEPS];
static int gRivalApproachLen           = 0;
static int gRivalApproachStep          = 0;

/* Rival exit walk */
static int gRivalExitStep              = 0;

/* Saved rival position — ASM: GetSpritePosition1 / SetSpritePosition1 */
static int gRivalSavedX                = 0;
static int gRivalSavedY                = 0;

/* Pending battle to signal game.c */
static int     gPendingBattle          = 0;
static uint8_t gPendingBattleClass     = 0;
static uint8_t gPendingBattleNo        = 0;

/* Set before battle, checked on re-entry to trigger post-battle sequence */
static int gRivalExitPending           = 0;

/* Pokédex scene rival exit */
static int gRivalPokedexExitStep       = 0;

/* Parcel delivery rival walk-in */
#define RIVAL_WALK_IN_STEPS 7  /* entrance y=10 → near Oak y=3 */
static int gRivalWalkInStep            = 0;

static void set_player_facing(int dir) {
    gPlayerFacing = dir & 3;
    Player_SyncOAM();
}

static void update_player_watch_rival_exit(void) {
    /* Mirror OaksLabPlayerWatchRivalExitScript (OaksLab.asm):
     * Step 0 = horizontal dodge: player faces LEFT/RIGHT to watch rival sidestep.
     * Step 1 = rival starts walking down: player faces DOWN. */
    if (gRivalExitStep == 0) {
        if ((int)wXCoord == 4) set_player_facing(DIR_RIGHT);
        else                   set_player_facing(DIR_LEFT);
    } else if (gRivalExitStep == 1) {
        set_player_facing(DIR_DOWN);
    }
}

/* ---- Speech strings ------------------------------------------------------- */

static const char kRivalFedUp[] =
    "{RIVAL}: Gramps!\nI'm fed up with\nwaiting!";

static const char kOakChooseMon[] =
    "OAK: {RIVAL}?\nLet me think..."
    "\fOh, that's right,\nI told you to\ncome! Just wait!"
    "\fHere, {PLAYER}!"
    "\fThere are 3\n#MON here!"
    "\fHaha!"
    "\fThey are inside\nthe POKE BALLs."
    "\fWhen I was young,\nI was a serious\n#MON trainer!"
    "\fIn my old age, I\nhave only 3 left,\nbut you can have\none! Choose!";

static const char kRivalWhatAboutMe[] =
    "{RIVAL}: Hey!\nGramps! What\nabout me?";

static const char kOakBePatient[] =
    "OAK: Be patient!\n{RIVAL}, you can\nhave one too!";

static const char *const kIntroSpeech[] = {
    kRivalFedUp,
    kOakChooseMon,
    kRivalWhatAboutMe,
    kOakBePatient,
};
#define INTRO_SPEECH_COUNT ((int)(sizeof(kIntroSpeech) / sizeof(kIntroSpeech[0])))

static const char kThoseArePokeballs[] =
    "Those are #\nBALLs. They\ncontain #MON!";

static const char kLastMon[] =
    "That's PROF.OAK's\nlast #MON!";

static const char kRivalIllTakeThisOne[] =
    "{RIVAL}: I'll take\nthis one, then!";

static const char kRivalIllTakeYouOn[] =
    "{RIVAL}: Wait\n{PLAYER}!\nLet's check out\nour #MON!";

/* _OaksLabRivalIPickedTheWrongPokemonText — defeat quote shown in battle UI */
static const char kRivalIPickedTheWrongPokemon[] =
    "WHAT?\nUnbelievable!\nI picked the\nwrong #MON!";

static const char kRivalSmellYouLater[] =
    "{RIVAL}: Okay!\nI'll make my\n#MON fight to\ntoughen it up!"
    "\f{PLAYER}! Gramps!\nSmell you later!";

/* ---- Oak's Parcel / Pokédex sequence (OaksLabOak1Text, OaksLabOakGivesPokedexScript) --- */

/* _OaksLabRivalGrampsText — rival calls from offscreen before walking in */
static const char kRivalGramps[] = "{RIVAL}: Gramps!";

/* _OaksLabOak1DeliverParcelText */
static const char kOakParcelText1[] =
    "OAK: Oh, {PLAYER}!"
    "\fHow is my old\n#MON?"
    "\fWell, it seems to\nlike you a lot."
    "\fYou must be\ntalented as a\n#MON trainer!"
    "\fWhat? You have\nsomething for me?"
    "\f{PLAYER} delivered\nOAK's PARCEL.";

/* _OaksLabOak1ParcelThanksText */
static const char kOakParcelText2[] =
    "Ah! This is the\ncustom # BALL\nI ordered!\nThank you!";

/* _OaksLabRivalWhatDidYouCallMeForText */
static const char kRivalWhatDidYouCall[] =
    "{RIVAL}: What did\nyou call me for?";

/* _OaksLabOakIHaveARequestText */
static const char kOakRequest[] =
    "OAK: Oh right! I\nhave a request\nof you two.";

/* _OaksLabOakMyInventionPokedexText */
static const char kOakPokedexText[] =
    "On the desk there\nis my invention,\n#DEX!"
    "\fIt automatically\nrecords data on\n#MON you've\nseen or caught!";

/* _OaksLabOakGotPokedexText */
static const char kOakGivesPokedex[] =
    "OAK: {PLAYER} and\n{RIVAL}! Take\nthese with you!"
    "\f{PLAYER} got\n#DEX from OAK!";

/* _OaksLabOakThatWasMyDreamText */
static const char kOakDreamText[] =
    "To make a complete\nguide on all the\n#MON in the\nworld..."
    "\fThat was my dream!"
    "\fBut, I'm too old!\nI can't do it!"
    "\fSo, I want you two\nto fulfill my\ndream for me!"
    "\fGet moving, you\ntwo!"
    "\fThis is a great\nundertaking in\n#MON history!";

/* _OaksLabRivalLeaveItAllToMeText */
static const char kRivalLeaveText[] =
    "{RIVAL}: Alright\nGramps! Leave it\nall to me!"
    "\f{PLAYER}, I hate to\nsay it, but I\ndon't need you!"
    "\fI know! I'll\nborrow a TOWN MAP\nfrom my sis!"
    "\fI'll tell her not\nto lend you one,\n{PLAYER}! Hahaha!";

/* Steps rival walks down to exit after Pokédex scene */
#define RIVAL_POKEDEX_EXIT_STEPS 5

/* ---- Dynamic text buffers (built per interaction) ------------------------- */
static uint8_t gConfirmText[80];   /* pokered-encoded: "So! You want the fire #MON, CHARMANDER?" */
static uint8_t gEnergeticText[80]; /* pokered-encoded: "This #MON is really energetic!" */
static uint8_t gPlayerRcvdText[80];/* pokered-encoded: "{PLAYER} received a CHARMANDER!" */
static uint8_t gRivalRcvdText[80]; /* pokered-encoded: "{RIVAL} received a SQUIRTLE!" */

/* ---- YES/NO box ----------------------------------------------------------- */
/* Identical layout to pokecenter's YesNoChoicePokeCenter.
 * Box at cols 14-19, rows 8-11.  Cursor 0=YES, 1=NO. */
#define YESNO_COL  14
#define YESNO_ROW   8
#define YESNO_W     6
#define YESNO_H     4

#define BC_TL  0x79u
#define BC_H   0x7Au
#define BC_TR  0x7Bu
#define BC_V   0x7Cu
#define BC_BL  0x7Du
#define BC_BR  0x7Eu
#define BC_SP  0x7Fu
#define BC_CUR 0xEDu

static void yesno_set_tile(int col, int row, uint8_t tile) {
    if (col < 0 || col >= SCREEN_WIDTH || row < 0 || row >= SCREEN_HEIGHT) return;
    gScrollTileMap[(row + 2) * SCROLL_MAP_W + (col + 2)] = tile;
}

static uint8_t yesno_char(char c) {
    if (c >= 'A' && c <= 'Z') return (uint8_t)Font_CharToTile((unsigned char)(0x80 + (c - 'A')));
    if (c >= 'a' && c <= 'z') return (uint8_t)Font_CharToTile((unsigned char)(0xA0 + (c - 'a')));
    if (c >= '0' && c <= '9') return (uint8_t)Font_CharToTile((unsigned char)(0xF6 + (c - '0')));
    return (uint8_t)Font_CharToTile(BC_SP);
}

static void yesno_draw(void) {
    /* Top: ┌────┐ */
    yesno_set_tile(YESNO_COL,               YESNO_ROW,     (uint8_t)Font_CharToTile(BC_TL));
    for (int c = 1; c < YESNO_W - 1; c++)
        yesno_set_tile(YESNO_COL + c,       YESNO_ROW,     (uint8_t)Font_CharToTile(BC_H));
    yesno_set_tile(YESNO_COL + YESNO_W - 1, YESNO_ROW,    (uint8_t)Font_CharToTile(BC_TR));
    /* YES row */
    yesno_set_tile(YESNO_COL,               YESNO_ROW + 1, (uint8_t)Font_CharToTile(BC_V));
    yesno_set_tile(YESNO_COL + 1,           YESNO_ROW + 1,
                   gYesNoCursor == 0 ? (uint8_t)Font_CharToTile(BC_CUR)
                                     : (uint8_t)Font_CharToTile(BC_SP));
    yesno_set_tile(YESNO_COL + 2,           YESNO_ROW + 1, yesno_char('Y'));
    yesno_set_tile(YESNO_COL + 3,           YESNO_ROW + 1, yesno_char('E'));
    yesno_set_tile(YESNO_COL + 4,           YESNO_ROW + 1, yesno_char('S'));
    yesno_set_tile(YESNO_COL + YESNO_W - 1, YESNO_ROW + 1, (uint8_t)Font_CharToTile(BC_V));
    /* NO row */
    yesno_set_tile(YESNO_COL,               YESNO_ROW + 2, (uint8_t)Font_CharToTile(BC_V));
    yesno_set_tile(YESNO_COL + 1,           YESNO_ROW + 2,
                   gYesNoCursor == 1 ? (uint8_t)Font_CharToTile(BC_CUR)
                                     : (uint8_t)Font_CharToTile(BC_SP));
    yesno_set_tile(YESNO_COL + 2,           YESNO_ROW + 2, yesno_char('N'));
    yesno_set_tile(YESNO_COL + 3,           YESNO_ROW + 2, yesno_char('O'));
    yesno_set_tile(YESNO_COL + 4,           YESNO_ROW + 2, (uint8_t)Font_CharToTile(BC_SP));
    yesno_set_tile(YESNO_COL + YESNO_W - 1, YESNO_ROW + 2, (uint8_t)Font_CharToTile(BC_V));
    /* Bottom: └────┘ */
    yesno_set_tile(YESNO_COL,               YESNO_ROW + 3, (uint8_t)Font_CharToTile(BC_BL));
    for (int c = 1; c < YESNO_W - 1; c++)
        yesno_set_tile(YESNO_COL + c,       YESNO_ROW + 3, (uint8_t)Font_CharToTile(BC_H));
    yesno_set_tile(YESNO_COL + YESNO_W - 1, YESNO_ROW + 3, (uint8_t)Font_CharToTile(BC_BR));
}

static void yesno_clear(void) {
    for (int r = 0; r < YESNO_H; r++)
        for (int c = 0; c < YESNO_W; c++)
            yesno_set_tile(YESNO_COL + c, YESNO_ROW + r, (uint8_t)Font_CharToTile(BC_SP));
}

/* ---- Pathfinding helper --------------------------------------------------- */
/* Mirrors FindPathToPlayer (engine/overworld/pathfinding.asm).
 * Generates a direction list from (from_x,from_y) toward one step
 * short of (to_x,to_y).  Returns step count written to dirs[]. */
static int find_path(int from_x, int from_y, int to_x, int to_y, int *dirs) {
    int ydist = abs(from_y - to_y);
    int xdist = abs(from_x - to_x);
    if (ydist > 0) ydist--;  /* stop 1 tile short */
    int ydir = (to_y < from_y) ? DIR_UP   : DIR_DOWN;
    int xdir = (to_x < from_x) ? DIR_LEFT : DIR_RIGHT;
    int yprog = 0, xprog = 0, count = 0;
    while (yprog < ydist || xprog < xdist) {
        int yrem = ydist - yprog, xrem = xdist - xprog;
        if (xrem >= yrem && xprog < xdist) { dirs[count++] = xdir; xprog++; }
        else                               { dirs[count++] = ydir; yprog++; }
        if (count >= MAX_APPROACH_STEPS - 1) break;
    }
    return count;
}

/* ---- Hide the balls that have been claimed -------------------------------- */
/* Mirrors ASM TOGGLE_STARTER_BALL_1/2/3 — persistent event flags that survive
 * save/load, checked on every map re-entry via OnMapLoad. */
static void hide_chosen_balls(void) {
    if (CheckEvent(EVENT_HIDE_STARTER_BALL_1)) NPC_HideSprite(OAKSLAB_CHARMANDER_IDX);
    if (CheckEvent(EVENT_HIDE_STARTER_BALL_2)) NPC_HideSprite(OAKSLAB_SQUIRTLE_IDX);
    if (CheckEvent(EVENT_HIDE_STARTER_BALL_3)) NPC_HideSprite(OAKSLAB_BULBASAUR_IDX);
}

/* Set the persistent toggle event for a ball NPC slot. */
static void set_ball_toggle(int ball_idx) {
    switch (ball_idx) {
    case OAKSLAB_CHARMANDER_IDX: SetEvent(EVENT_HIDE_STARTER_BALL_1); break;
    case OAKSLAB_SQUIRTLE_IDX:   SetEvent(EVENT_HIDE_STARTER_BALL_2); break;
    case OAKSLAB_BULBASAUR_IDX:  SetEvent(EVENT_HIDE_STARTER_BALL_3); break;
    }
}

/* ---- Public API ----------------------------------------------------------- */

int OaksLabScripts_IsActive(void) {
    return gState != OLS_IDLE;
}

void OaksLabScripts_PostRender(void) {
    if (gState == OLS_BALL_YESNO) yesno_draw();
}

int OaksLabScripts_GetPendingBattle(uint8_t *class_out, uint8_t *no_out) {
    if (!gPendingBattle) return 0;
    *class_out   = gPendingBattleClass;
    *no_out      = gPendingBattleNo;
    gPendingBattle = 0;
    return 1;
}

/* ---- NPC callbacks (called by game.c when player presses A on a ball) ----- */

static void ball_callback(uint8_t player_species, uint8_t rival_species,
                          int player_ball_idx, int rival_ball_idx) {
    /* In the original, EVENT_GOT_STARTER flips only after the rival
     * receives their mon. Until then, other balls can still be examined. */
    if (CheckEvent(EVENT_GOT_STARTER)) {
        NPC_SetFacing(OAKSLAB_OAK1_IDX, DIR_DOWN);
        gState = OLS_BALL_LAST_MON;
        return;
    }
    /* If Oak hasn't asked yet show generic "those are Pokéballs" */
    if (!CheckEvent(EVENT_OAK_ASKED_TO_CHOOSE_MON)) {
        gState = OLS_BALL_CANT_YET;
        return;
    }
    /* Oak asked → show the selection prompt */
    gSelectedSpecies     = player_species;
    gRivalStarterSpecies = rival_species;
    gSelectedBallIdx     = player_ball_idx;
    gRivalBallIdx        = rival_ball_idx;

    /* Build choice text (approximate of _OaksLabYouWantCharmanderText etc.) */
    const char *type_str = "fire";
    const char *name_str = "CHARMANDER";
    if (player_species == STARTER2) { type_str = "water"; name_str = "SQUIRTLE";  }
    if (player_species == STARTER3) { type_str = "plant"; name_str = "BULBASAUR"; }
    snprintf((char*)gConfirmText, sizeof(gConfirmText),
             "So! You want the\n%s #MON,\n%s?", type_str, name_str);

    /* Build "received" texts */
    uint8_t p_dex = gSpeciesToDex[player_species];
    uint8_t r_dex = gSpeciesToDex[rival_species];
    snprintf((char*)gPlayerRcvdText, sizeof(gPlayerRcvdText),
             "{PLAYER} received\na %s!", Pokemon_GetName(p_dex));
    snprintf((char*)gRivalRcvdText, sizeof(gRivalRcvdText),
             "{RIVAL} received\na %s!", Pokemon_GetName(r_dex));

    /* "This #MON is really energetic!" */
    snprintf((char*)gEnergeticText, sizeof(gEnergeticText),
             "This #MON is\nreally energetic!");

    /* Oak / rival face into the interaction (ASM: OaksLabShowPokeBallPokemonScript) */
    NPC_SetFacing(OAKSLAB_OAK1_IDX,  DIR_DOWN);
    NPC_SetFacing(OAKSLAB_RIVAL_IDX, DIR_RIGHT);

    gScriptedMovement = 1;  /* freeze player during selection */

    /* ASM flow: StarterDex (show dex entry) → then confirm text.
     * StarterDex temporarily marks starters as owned so full data shows. */
    gState = OLS_DEX_SHOW;
}

void OaksLab_CharmanderCallback(void) {
    ball_callback(STARTER1, STARTER2,
                  OAKSLAB_CHARMANDER_IDX, OAKSLAB_SQUIRTLE_IDX);
}
void OaksLab_SquirtleCallback(void) {
    ball_callback(STARTER2, STARTER3,
                  OAKSLAB_SQUIRTLE_IDX, OAKSLAB_BULBASAUR_IDX);
}
void OaksLab_BulbasaurCallback(void) {
    ball_callback(STARTER3, STARTER1,
                  OAKSLAB_BULBASAUR_IDX, OAKSLAB_CHARMANDER_IDX);
}

/* ---- Oak's Parcel / Pokédex callback -------------------------------------- */
/* Registered as the talk callback for Oak slot 4 (OAK1 at table).
 * Reference: OaksLabOak1Text in scripts/OaksLab.asm lines 965-1069. */
void OaksLab_OakCallback(void) {
    if (wCurMap != MAP_OAKS_LAB) return;
    if (gState != OLS_IDLE) return;

    /* Has parcel but hasn't delivered it yet → trigger delivery sequence.
     * ASM: OaksLabOak1Text.got_parcel / OaksLabRivalArrivesAtOaksRequestScript.
     * Rival stays hidden until after the "Gramps!" offscreen text fires. */
    if (Inventory_GetQty(ITEM_OAKS_PARCEL) > 0 && !CheckEvent(EVENT_OAK_GOT_PARCEL)) {
        gScriptedMovement = 1;
        NPC_SetFacing(OAKSLAB_OAK1_IDX, DIR_DOWN);
        Text_ShowASCII(kOakParcelText1);
        gState = OLS_PARCEL_TEXT1;
        return;
    }

    /* Already have Pokédex — generic post-quest Oak text */
    if (CheckEvent(EVENT_GOT_POKEDEX)) {
        Text_ShowASCII("OAK: If a #MON\nis weak, raise\nit patiently!");
        return;
    }

    /* Normal Oak text depending on quest stage */
    if (CheckEvent(EVENT_OAK_ASKED_TO_CHOOSE_MON) && !CheckEvent(EVENT_GOT_STARTER)) {
        Text_ShowASCII("OAK: Now,\n{PLAYER}, which\n#MON do you\nwant?");
        return;
    }

    Text_ShowASCII("OAK: If a #MON\nis weak, raise\nit patiently!");
}

/* ---- Map load ------------------------------------------------------------- */

void OaksLabScripts_OnMapLoad(void) {
    if (wCurMap != MAP_OAKS_LAB) return;

    Warp_HasDoorStep();

    /* Preserve gRivalExitPending across enter_overworld() — checked below. */
    gState             = OLS_IDLE;
    gOakEnterSteps     = 0;
    gPlayerEnterSteps  = 0;
    gSpeechIdx         = 0;
    gDelay             = 0;

    /* Show rival unless they have already exited */
    if (CheckEvent(EVENT_BATTLED_RIVAL_IN_OAKS_LAB) && !gRivalExitPending) {
        NPC_HideSprite(OAKSLAB_RIVAL_IDX);
    } else {
        NPC_ShowSprite(OAKSLAB_RIVAL_IDX);
    }

    if (!CheckEvent(EVENT_OAK_APPEARED_IN_PALLET)) {
        NPC_ShowSprite(OAKSLAB_OAK1_IDX);
        NPC_HideSprite(OAKSLAB_OAK2_IDX);
        return;
    }

    if (CheckEvent(EVENT_FOLLOWED_OAK_INTO_LAB)) {
        NPC_ShowSprite(OAKSLAB_OAK1_IDX);
        NPC_HideSprite(OAKSLAB_OAK2_IDX);
        hide_chosen_balls();

        /* Post-battle rival exit sequence */
        if (gRivalExitPending) {
            NPC_ShowSprite(OAKSLAB_RIVAL_IDX);
            gState = OLS_POST_BATTLE_SETUP;
            return;
        }

        /* After rival exits, nothing more to do */
        if (CheckEvent(EVENT_BATTLED_RIVAL_IN_OAKS_LAB)) {
            NPC_HideSprite(OAKSLAB_RIVAL_IDX);
            /* Hide Pokédex table sprites once the player has received the Pokédex.
             * ASM: HideObject TOGGLE_POKEDEX_1/2 in OaksLabOakGivesPokedexScript. */
            if (CheckEvent(EVENT_GOT_POKEDEX)) {
                NPC_HideSprite(OAKSLAB_POKEDEX1_IDX);
                NPC_HideSprite(OAKSLAB_POKEDEX2_IDX);
            }
            return;
        }

        /* Still waiting for player to pick a starter / rival to challenge */
        if (CheckEvent(EVENT_OAK_ASKED_TO_CHOOSE_MON)) {
            if (CheckEvent(EVENT_GOT_STARTER)) {
                /* Rival has starter too — resume awaiting challenge */
                if (wRivalStarter != 0) {
                    gState = OLS_AWAIT_CHALLENGE;
                }
            }
        }
        return;
    }

    /* Oak is walking in from entrance.
     * Freeze player immediately — mirrors the simulated joypad taking over
     * in the original (OaksLabPlayerEntersLabScript: wJoypad replaced by RLE). */
    NPC_HideSprite(OAKSLAB_OAK1_IDX);
    NPC_ShowSprite(OAKSLAB_OAK2_IDX);
    gScriptedMovement = 1;
    gState = OLS_OAK_ENTER;
}

/* ---- Main tick ------------------------------------------------------------ */

void OaksLabScripts_Tick(void) {
    if (wCurMap != MAP_OAKS_LAB) return;

    switch (gState) {
    /* ------------------------------------------------------------------ */
    case OLS_IDLE:
        if (gRivalExitPending) {
            gState = OLS_POST_BATTLE_SETUP;
        }
        return;

    /* ---- Oak enters lab ------------------------------------------------ */
    case OLS_OAK_ENTER:
        if (NPC_IsWalking(OAKSLAB_OAK2_IDX)) return;
        if (gOakEnterSteps >= 3) { gState = OLS_OAK_SWAP; return; }
        NPC_DoScriptedStep(OAKSLAB_OAK2_IDX, DIR_UP);
        gOakEnterSteps++;
        return;

    case OLS_OAK_SWAP:
        if (NPC_IsWalking(OAKSLAB_OAK2_IDX)) return;
        NPC_HideSprite(OAKSLAB_OAK2_IDX);
        NPC_ShowSprite(OAKSLAB_OAK1_IDX);
        gDelay = 3;
        gState = OLS_PLAYER_ENTER_SETUP;
        return;

    /* ---- Player auto-walks into lab ------------------------------------ */
    case OLS_PLAYER_ENTER_SETUP:
        if (gDelay-- > 0) return;
        gScriptedMovement = 1;
        gPlayerEnterSteps = 8;
        NPC_SetFacing(OAKSLAB_RIVAL_IDX, DIR_DOWN);
        NPC_SetFacing(OAKSLAB_OAK1_IDX,  DIR_DOWN);
        gState = OLS_PLAYER_ENTER_WALK;
        return;

    case OLS_PLAYER_ENTER_WALK:
        if (Player_IsMoving()) return;
        if (gPlayerEnterSteps <= 0) { gState = OLS_FOLLOWED; return; }
        Player_DoScriptedStep(DIR_UP);
        gPlayerEnterSteps--;
        return;

    case OLS_FOLLOWED:
        if (Player_IsMoving()) return;
        SetEvent(EVENT_FOLLOWED_OAK_INTO_LAB);
        SetEvent(EVENT_FOLLOWED_OAK_INTO_LAB_2);
        NPC_ShowSprite(OAKSLAB_RIVAL_IDX);
        NPC_SetFacing(OAKSLAB_RIVAL_IDX, DIR_UP);
        Music_Play(Music_GetMapID(wCurMap));
        gSpeechIdx = 0;
        gState = OLS_SPEECH_SHOW;
        return;

    /* ---- Opening speech ----------------------------------------------- */
    case OLS_SPEECH_SHOW:
        if (gSpeechIdx >= INTRO_SPEECH_COUNT) {
            SetEvent(EVENT_OAK_ASKED_TO_CHOOSE_MON);
            gScriptedMovement = 0;
            gState = OLS_IDLE;
            return;
        }
        Text_ShowASCII(kIntroSpeech[gSpeechIdx]);
        gState = OLS_SPEECH_WAIT;
        return;

    case OLS_SPEECH_WAIT:
        if (Text_IsOpen()) return;
        gSpeechIdx++;
        gState = OLS_SPEECH_SHOW;
        return;

    /* ---- Ball: no event yet ------------------------------------------- */
    case OLS_BALL_CANT_YET:
        Text_ShowASCII(kThoseArePokeballs);
        gState = OLS_BALL_CANT_WAIT;
        return;

    case OLS_BALL_CANT_WAIT:
        if (Text_IsOpen()) return;
        gState = OLS_IDLE;
        return;

    /* ---- Ball: already picked ----------------------------------------- */
    case OLS_BALL_LAST_MON:
        Text_ShowASCII(kLastMon);
        gState = OLS_BALL_LAST_WAIT;
        return;

    case OLS_BALL_LAST_WAIT:
        if (Text_IsOpen()) return;
        gState = OLS_IDLE;
        return;

    /* ---- StarterDex: show Pokédex entry before confirm ---------------
     * ASM: OaksLabShowPokeBallPokemonScript → predef StarterDex
     *   StarterDex temporarily marks starters as owned, calls ShowPokedexData,
     *   then clears ownership. After dismiss: ReloadMapData + 10 frame delay. */
    case OLS_DEX_SHOW: {
        /* StarterDex: temporarily mark starters as owned so full data shows.
         * ASM: ld a, 1<<(DEX_BULBASAUR-1) | 1<<(DEX_IVYSAUR-1) |
         *         1<<(DEX_CHARMANDER-1) | 1<<(DEX_SQUIRTLE-1)
         * Dex numbers: Bulbasaur=1, Ivysaur=2, Charmander=4, Squirtle=7 */
        uint8_t saved0 = wPokedexOwned[0];
        wPokedexOwned[0] |= (1u << 0) | (1u << 1) | (1u << 3); /* dex 1,2,4 */
        wPokedexOwned[0] |= (1u << 6);                          /* dex 7 */
        uint8_t dex = gSpeciesToDex[gSelectedSpecies];
        Pokedex_ShowData(dex);
        /* Restore ownership (StarterDex clears after ShowPokedexData) */
        wPokedexOwned[0] = saved0;
        gShowDataDelay = 10;
        gState = OLS_DEX_WAIT;
        return;
    }

    case OLS_DEX_WAIT:
        /* Wait for dex entry screen to be dismissed (A/B press) */
        if (Pokedex_IsShowingData()) return;
        gState = OLS_DEX_RESTORE;
        return;

    case OLS_DEX_RESTORE:
        /* ASM: call ReloadMapData; DelayFrames 10 */
        if (gShowDataDelay > 0) {
            if (gShowDataDelay == 10) {
                /* Restore overworld: tileset, font, NPC sprite tiles, palette */
                Map_ReloadGfx();
                Font_Load();
                NPC_ReloadTiles();  /* reload NPC sprite tiles (overwritten by dex sprite).
                                 * NPC_Load() would reset npc_hidden[], making Oak2 visible
                                 * at the bottom of the lab. NPC_ReloadTiles() restores
                                 * GFX only, without touching positions or hidden state. */
                Display_SetPalette(0xE4, 0xD0, 0xE0);
            }
            gShowDataDelay--;
            return;
        }
        /* Now show confirm text */
        Text_KeepTilesOnClose();
        Text_ShowASCII((char*)gConfirmText);
        gState = OLS_BALL_CONFIRM;
        return;

    /* ---- Ball: confirm text shown ------------------------------------- */
    case OLS_BALL_CONFIRM:
        /* Text was shown in the callback; wait for it to close. */
        if (Text_IsOpen()) return;
        /* Text closed — draw YES/NO box on top of the kept tiles. */
        gYesNoCursor = 0;
        yesno_draw();
        gState = OLS_BALL_YESNO;
        return;

    case OLS_BALL_CONFIRM_WAIT:
        /* (unused — kept for symmetry) */
        gState = OLS_BALL_YESNO;
        return;

    /* ---- YES/NO cursor ------------------------------------------------ */
    case OLS_BALL_YESNO:
        if (hJoyPressed & PAD_UP) {
            if (gYesNoCursor > 0) { gYesNoCursor--; yesno_draw(); }
        }
        if (hJoyPressed & PAD_DOWN) {
            if (gYesNoCursor < 1) { gYesNoCursor++; yesno_draw(); }
        }
        if (hJoyPressed & (PAD_A | PAD_B)) {
            yesno_clear();
            Map_BuildScrollView();  /* restore map tiles under box */
            if ((hJoyPressed & PAD_A) && gYesNoCursor == 0) {
                /* YES — give starter */
                NPC_HideSprite(gSelectedBallIdx);
                set_ball_toggle(gSelectedBallIdx);
                Pokemon_AddToParty(gSelectedSpecies, 5);
                Pokedex_SetOwned(gSelectedSpecies);
                Text_ShowASCII(gEnergeticText);
                gState = OLS_BALL_ENERGETIC;
            } else {
                /* NO or B — Oak says "take your time" and return to picking */
                Text_ShowASCII("Take your time.");
                gState = OLS_BALL_DECLINED;
            }
        }
        return;

    /* ---- Ball: declined ------------------------------------------------ */
    case OLS_BALL_DECLINED:
        /* "Take your time." was shown; wait for it to close. */
        if (Text_IsOpen()) return;
        /* Text closed — unfreeze player and return to idle to select another ball. */
        gScriptedMovement = 0;
        gState = OLS_IDLE;
        return;

    case OLS_BALL_DECLINED_WAIT:
        /* (unused — kept for symmetry) */
        gState = OLS_IDLE;
        return;

    /* ---- Give starter texts ------------------------------------------- */
    case OLS_BALL_ENERGETIC:
        if (Text_IsOpen()) return;
        /* ASM: OaksLabReceivedMonText with sound_get_key_item */
        Audio_PlaySFX_GetKeyItem();
        Text_ShowASCII((char*)gPlayerRcvdText);
        gState = OLS_BALL_ENERGETIC_WAIT;
        return;

    case OLS_BALL_ENERGETIC_WAIT:
        if (Text_IsOpen()) return;
        gState = OLS_BALL_RECEIVED;
        return;

    case OLS_BALL_RECEIVED:
        /* Transition: start rival walk-to-ball sequence */
        /* Rival starts at (8,7); destination = one tile below rival's ball.
         * Rival's ball positions in port coords:
         *   Squirtle  ball (x=14,y=7) → rival walks to (14,9)
         *   Bulbasaur ball (x=16,y=7) → rival walks to (16,9)
         *   Charmander ball (x=12,y=7) → rival walks to (12,9)
         *
         * Movement sequences (each step = 2 port units, matching NPC_DoScriptedStep):
         *   Charmander choice → rival to Squirtle ball: DOWN RIGHT RIGHT RIGHT
         *   Squirtle choice   → rival to Bulbasaur ball: DOWN RIGHT RIGHT RIGHT RIGHT
         *   Bulbasaur choice  → rival to Charmander ball: DOWN RIGHT RIGHT
         */
        {
            /* Exact movement variants from OaksLab.asm. The rival's path
             * depends on which ball the player chose and where the player
             * was standing when they interacted with it. */
            static const int middle_ball_1[] = {
                DIR_DOWN, DIR_DOWN, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_UP
            };
            static const int middle_ball_2[] = {
                DIR_DOWN, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT
            };
            static const int right_ball_1[] = {
                DIR_DOWN, DIR_DOWN, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_UP
            };
            static const int right_ball_2[] = {
                DIR_DOWN, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT
            };
            static const int left_ball_1[] = {
                DIR_DOWN, DIR_RIGHT, DIR_RIGHT
            };
            static const int left_ball_2[] = {
                DIR_RIGHT
            };
            const int *src = NULL;

            if (gSelectedSpecies == STARTER1) {
                if ((int)wYCoord == 4) {
                    src = middle_ball_1;
                    gRivalWalkLen = (int)(sizeof(middle_ball_1) / sizeof(middle_ball_1[0]));
                } else {
                    src = middle_ball_2;
                    gRivalWalkLen = (int)(sizeof(middle_ball_2) / sizeof(middle_ball_2[0]));
                }
            } else if (gSelectedSpecies == STARTER2) {
                if ((int)wYCoord == 4) {
                    src = right_ball_1;
                    gRivalWalkLen = (int)(sizeof(right_ball_1) / sizeof(right_ball_1[0]));
                } else {
                    src = right_ball_2;
                    gRivalWalkLen = (int)(sizeof(right_ball_2) / sizeof(right_ball_2[0]));
                }
            } else {
                if ((int)wXCoord == 9) {
                    src = left_ball_2;
                    gRivalWalkLen = (int)(sizeof(left_ball_2) / sizeof(left_ball_2[0]));
                } else {
                    src = left_ball_1;
                    gRivalWalkLen = (int)(sizeof(left_ball_1) / sizeof(left_ball_1[0]));
                }
            }

            for (int i = 0; i < gRivalWalkLen; i++) {
                gRivalWalkDirs[i] = src[i];
            }
            gRivalWalkStep = 0;
        }
        NPC_SetFacing(OAKSLAB_RIVAL_IDX, DIR_DOWN);
        gState = OLS_RIVAL_WALK;
        return;

    case OLS_BALL_RECEIVED_WAIT:
        /* (unused — fall through) */
        gState = OLS_RIVAL_WALK;
        return;

    /* ---- Rival walks to his ball --------------------------------------- */
    case OLS_RIVAL_WALK:
        if (NPC_IsWalking(OAKSLAB_RIVAL_IDX)) return;
        if (gRivalWalkStep >= gRivalWalkLen) {
            gState = OLS_RIVAL_TAKES;
            return;
        }
        NPC_DoScriptedStep(OAKSLAB_RIVAL_IDX, gRivalWalkDirs[gRivalWalkStep]);
        gRivalWalkStep++;
        return;

    case OLS_RIVAL_WALK_WAIT:
        /* (unused — merged into OLS_RIVAL_WALK) */
        gState = OLS_RIVAL_TAKES;
        return;

    /* ---- Rival picks his starter -------------------------------------- */
    case OLS_RIVAL_TAKES:
        if (NPC_IsWalking(OAKSLAB_RIVAL_IDX)) return;
        NPC_SetFacing(OAKSLAB_RIVAL_IDX, DIR_UP);
        Text_ShowASCII(kRivalIllTakeThisOne);
        gState = OLS_RIVAL_TAKES_WAIT;
        return;

    case OLS_RIVAL_TAKES_WAIT:
        if (Text_IsOpen()) return;
        /* Hide rival's ball and give rival their starter */
        NPC_HideSprite(gRivalBallIdx);
        set_ball_toggle(gRivalBallIdx);
        wRivalStarter = gRivalStarterSpecies;
        SetEvent(EVENT_GOT_STARTER);
        Text_ShowASCII((char*)gRivalRcvdText);
        gState = OLS_RIVAL_RCVD;
        return;

    case OLS_RIVAL_RCVD:
        if (Text_IsOpen()) return;
        gState = OLS_RIVAL_RCVD_WAIT;
        return;

    case OLS_RIVAL_RCVD_WAIT:
        /* Rival has their starter.  Player must walk south to trigger challenge. */
        gScriptedMovement = 0;
        gState = OLS_AWAIT_CHALLENGE;
        return;

    /* ---- Await challenge position ------------------------------------- */
    case OLS_AWAIT_CHALLENGE:
        /* OaksLabRivalChallengesPlayerScript: fires when wYCoord == 6 (ASM).
         * In port: y=6 (ASM) → y=6*2+1=13. */
        if (wYCoord < CHALLENGE_TRIGGER_Y) return;
        gScriptedMovement = 1;  /* freeze player for challenge sequence */
        NPC_SetFacing(OAKSLAB_RIVAL_IDX, DIR_DOWN);
        NPC_SetFacing(OAKSLAB_OAK1_IDX,  DIR_DOWN);
        Music_Play(MUSIC_MEET_RIVAL);
        Text_ShowASCII(kRivalIllTakeYouOn);
        gState = OLS_CHALLENGE_TEXT;
        return;

    case OLS_CHALLENGE_TEXT:
        if (Text_IsOpen()) return;
        gState = OLS_CHALLENGE_WAIT;
        return;

    case OLS_CHALLENGE_WAIT: {
        /* Compute rival approach path toward player (one step short). */
        int rx, ry;
        NPC_GetTilePos(OAKSLAB_RIVAL_IDX, &rx, &ry);
        gRivalApproachLen  = find_path(rx, ry, (int)wXCoord, (int)wYCoord,
                                       gRivalApproachDirs);
        gRivalApproachStep = 0;
        gState = OLS_RIVAL_APPROACH;
        return;
    }

    /* ---- Rival walks to player ---------------------------------------- */
    case OLS_RIVAL_APPROACH:
        if (NPC_IsWalking(OAKSLAB_RIVAL_IDX)) return;
        if (gRivalApproachStep >= gRivalApproachLen) {
            gState = OLS_RIVAL_APPROACH_WAIT;
            return;
        }
        NPC_DoScriptedStep(OAKSLAB_RIVAL_IDX, gRivalApproachDirs[gRivalApproachStep]);
        gRivalApproachStep++;
        return;

    case OLS_RIVAL_APPROACH_WAIT:
        if (NPC_IsWalking(OAKSLAB_RIVAL_IDX)) return;
        gState = OLS_BATTLE_TRIGGER;
        return;

    /* ---- Signal battle to game.c -------------------------------------- */
    case OLS_BATTLE_TRIGGER: {
        /* OaksLabRivalStartBattleScript:
         *   wTrainerNo depends on rival's starter:
         *     STARTER2 (Squirtle) → trainer no. 1
         *     STARTER3 (Bulbasaur) → trainer no. 2
         *     else (Charmander) → trainer no. 3
         */
        uint8_t trainer_no;
        if      (wRivalStarter == STARTER2) trainer_no = 1;
        else if (wRivalStarter == STARTER3) trainer_no = 2;
        else                                trainer_no = 3;

        /* ASM: GetSpritePosition1 — save rival position before battle */
        NPC_GetTilePos(OAKSLAB_RIVAL_IDX, &gRivalSavedX, &gRivalSavedY);
        gRivalExitPending    = 1;
        /* ASM: SaveEndBattleTextPointers hl=IPickedTheWrongPokemon de=AmIGreatOrWhat */
        gTrainerAfterText    = kRivalIPickedTheWrongPokemon;
        /* Battle_StartTrainer expects the raw 1-based trainer class, not OPP_ID space. */
        gPendingBattleClass  = OPP_RIVAL1 - OPP_ID_OFFSET;
        gPendingBattleNo     = trainer_no;
        gPendingBattle       = 1;
        gState               = OLS_IDLE;
        return;
    }

    /* ---- Post-battle rival exit --------------------------------------- */
    case OLS_POST_BATTLE_SETUP:
        /* OaksLabRivalEndBattleScript: restore post-battle stance, heal, and lock input.
         * ASM: SetSpritePosition1 — restore rival to pre-battle position. */
        NPC_SetTilePos(OAKSLAB_RIVAL_IDX, gRivalSavedX, gRivalSavedY);
        gScriptedMovement = 1;
        set_player_facing(DIR_UP);
        NPC_SetFacing(OAKSLAB_RIVAL_IDX, DIR_DOWN);
        Pokemon_HealParty();
        SetEvent(EVENT_BATTLED_RIVAL_IN_OAKS_LAB);
        gDelay = 20;
        gState = OLS_POST_BATTLE_DELAY;
        return;

    case OLS_POST_BATTLE_DELAY:
        if (gDelay-- > 0) return;
        Text_ShowASCII(kRivalSmellYouLater);
        gRivalExitStep = 0;
        gState = OLS_POST_BATTLE_TEXT;
        return;

    case OLS_POST_BATTLE_TEXT:
        if (Text_IsOpen()) return;
        Music_PlayFromLoop(MUSIC_MEET_RIVAL);  /* ASM: Music_RivalAlternateStart */
        NPC_SetFacing(OAKSLAB_RIVAL_IDX, DIR_DOWN);
        update_player_watch_rival_exit();
        gState = OLS_RIVAL_EXIT_WAIT;
        return;

    case OLS_RIVAL_EXIT_WAIT:
        /* ASM .RivalExitMovement: MoveSprite writes NPC_CHANGE_FACING + 5× DOWN,
         * then overwrites wNPCMovementDirections[0] with LEFT or RIGHT.
         * Actual execution: 1× LEFT/RIGHT, then 5× DOWN.
         * Step 0 = horizontal, steps 1-5 = DOWN. */
        if (NPC_IsWalking(OAKSLAB_RIVAL_IDX)) return;
        if (gRivalExitStep == 0) {
            /* Step 0: horizontal dodge — ASM wXCoord cp 4 (port: x==9) */
            int exit_dir = ((int)wXCoord == 4) ? DIR_RIGHT : DIR_LEFT;
            NPC_DoScriptedStep(OAKSLAB_RIVAL_IDX, exit_dir);
            gRivalExitStep++;
            update_player_watch_rival_exit();
        } else if (gRivalExitStep <= RIVAL_EXIT_STEPS) {
            /* Steps 1-5: walk down toward exit */
            NPC_DoScriptedStep(OAKSLAB_RIVAL_IDX, DIR_DOWN);
            gRivalExitStep++;
            update_player_watch_rival_exit();
        } else {
            gState = OLS_RIVAL_GONE;
        }
        return;

    case OLS_RIVAL_EXIT_WALK:
    case OLS_POST_BATTLE_WAIT:
        /* Unused legacy states retained for enum stability. */
        gState = OLS_RIVAL_EXIT_WAIT;
        return;

    case OLS_RIVAL_GONE:
        if (NPC_IsWalking(OAKSLAB_RIVAL_IDX)) return;
        NPC_HideSprite(OAKSLAB_RIVAL_IDX);
        gRivalExitPending = 0;
        gScriptedMovement = 0;
        Music_Play(Music_GetMapID(wCurMap));
        gState = OLS_IDLE;
        return;

    /* ---- Oak's Parcel delivery → Pokédex giving ----------------------- */

    case OLS_PARCEL_TEXT1:
        if (Text_IsOpen()) return;
        /* Remove parcel from bag */
        Inventory_Remove(ITEM_OAKS_PARCEL, 1);
        Text_ShowASCII(kOakParcelText2);
        gState = OLS_PARCEL_TEXT2;
        return;

    case OLS_PARCEL_TEXT2:
        /* ASM: OaksLabRivalArrivesAtOaksRequestScript — stop music, play rival
         * theme, then show TEXT_OAKSLAB_RIVAL_GRAMPS while rival is still offscreen. */
        if (Text_IsOpen()) return;
        Music_PlayFromLoop(MUSIC_MEET_RIVAL);  /* ASM: Music_RivalAlternateStart */
        Text_ShowASCII(kRivalGramps);          /* "{RIVAL}: Gramps!" — rival offscreen */
        gState = OLS_PARCEL_SHOW_RIVAL;
        return;

    case OLS_PARCEL_SHOW_RIVAL:
        /* ASM: OaksLabCalcRivalMovementScript + ShowObject(TOGGLE_OAKS_LAB_RIVAL)
         * + MoveSprite.  Rival appears at the entrance and walks up to Oak. */
        if (Text_IsOpen()) return;
        NPC_SetTilePos(OAKSLAB_RIVAL_IDX, 4, 10);  /* entrance position (y=10) */
        NPC_SetFacing(OAKSLAB_RIVAL_IDX, DIR_UP);
        NPC_ShowSprite(OAKSLAB_RIVAL_IDX);
        gRivalWalkInStep = 0;
        gState = OLS_RIVAL_WALK_IN;
        return;

    case OLS_RIVAL_WALK_IN:
        /* Walk rival up toward Oak one step at a time. */
        if (NPC_IsWalking(OAKSLAB_RIVAL_IDX)) return;
        if (gRivalWalkInStep < RIVAL_WALK_IN_STEPS) {
            NPC_DoScriptedStep(OAKSLAB_RIVAL_IDX, DIR_UP);
            gRivalWalkInStep++;
        } else {
            gState = OLS_RIVAL_ARRIVE_TEXT;
        }
        return;

    case OLS_RIVAL_ARRIVE_TEXT:
        /* ASM: OaksLabOakGivesPokedexScript — rival has arrived, now speak. */
        if (NPC_IsWalking(OAKSLAB_RIVAL_IDX)) return;
        NPC_SetFacing(OAKSLAB_RIVAL_IDX, DIR_UP);
        Text_ShowASCII(kRivalWhatDidYouCall);
        gState = OLS_OAK_REQUEST_TEXT;
        return;

    case OLS_OAK_REQUEST_TEXT:
        if (Text_IsOpen()) return;
        Text_ShowASCII(kOakRequest);  /* "OAK: Oh right! I have a request of you two." */
        gState = OLS_POKEDEX_TEXT;
        return;

    case OLS_POKEDEX_TEXT:
        /* ASM: OaksLabOakGivesPokedexScript — "On the desk is my invention, #DEX!" */
        if (Text_IsOpen()) return;
        Text_ShowASCII(kOakPokedexText);
        gState = OLS_GIVE_POKEDEX;
        return;

    case OLS_GIVE_POKEDEX:
        /* ASM: OAK_GOT_POKEDEX text + give item */
        if (Text_IsOpen()) return;
        Inventory_Add(ITEM_POKEDEX, 1);
        SetEvent(EVENT_GOT_POKEDEX);
        SetEvent(EVENT_OAK_GOT_PARCEL);
        Text_ShowASCII(kOakGivesPokedex);
        gState = OLS_GAVE_POKEDEX_TEXT;
        return;

    case OLS_GAVE_POKEDEX_TEXT:
        /* ASM: after OAK_GOT_POKEDEX text + Delay3, HideObject TOGGLE_POKEDEX_1/2,
         * then TEXT_OAKSLAB_OAK_THAT_WAS_MY_DREAM. */
        if (Text_IsOpen()) return;
        NPC_HideSprite(OAKSLAB_POKEDEX1_IDX);
        NPC_HideSprite(OAKSLAB_POKEDEX2_IDX);
        Text_ShowASCII(kOakDreamText);
        gState = OLS_OAK_DREAM_TEXT;
        return;

    case OLS_OAK_DREAM_TEXT:
        /* ASM: RIVAL_LEAVE_IT_ALL_TO_ME text */
        if (Text_IsOpen()) return;
        Text_ShowASCII(kRivalLeaveText);
        gState = OLS_RIVAL_LEAVE_TEXT;
        return;

    case OLS_RIVAL_LEAVE_TEXT:
        /* Rival turns and walks down to exit after his leave text. */
        if (Text_IsOpen()) return;
        NPC_SetFacing(OAKSLAB_RIVAL_IDX, DIR_DOWN);
        gRivalPokedexExitStep = 0;
        gState = OLS_RIVAL_LEAVE_WALK;
        return;

    case OLS_RIVAL_LEAVE_WALK:
        if (NPC_IsWalking(OAKSLAB_RIVAL_IDX)) return;
        if (gRivalPokedexExitStep >= RIVAL_POKEDEX_EXIT_STEPS) {
            NPC_HideSprite(OAKSLAB_RIVAL_IDX);
            gState = OLS_RIVAL_LEAVE_DONE;
            return;
        }
        NPC_DoScriptedStep(OAKSLAB_RIVAL_IDX, DIR_DOWN);
        gRivalPokedexExitStep++;
        return;

    case OLS_RIVAL_LEAVE_DONE:
        gScriptedMovement = 0;
        Music_Play(Music_GetMapID(wCurMap));
        /* OaksLabRivalLeavesWithPokedexScript: arm the Route 22 rival encounter */
        SetEvent(EVENT_1ST_ROUTE22_RIVAL_BATTLE);
        ClearEvent(EVENT_2ND_ROUTE22_RIVAL_BATTLE);
        SetEvent(EVENT_ROUTE22_RIVAL_WANTS_BATTLE);
        gState = OLS_IDLE;
        return;
    }
}
