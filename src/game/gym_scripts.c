/* gym_scripts.c — Gym battle state machine.
 *
 * Reference: scripts/PewterGym.asm (and other gym ASM scripts).
 *
 * Flow (Brock example, mirrors PewterGymBrockText/PewterGymBrockPostBattle):
 *   1. Player presses A on Brock → GymScripts_BrockInteract() fires
 *   2. If EVENT_BEAT_BROCK not set: show pre-battle speech
 *   3. After text closes: set wGymLeaderNo=1, signal pending trainer battle
 *   4. game.c detects pending battle → plays MUSIC_GYM_LEADER_BATTLE, starts transition
 *   5. Battle ends with TRAINER_VICTORY → game.c calls GymScripts_OnVictory()
 *   6. SetEvent(EVENT_BEAT_BROCK), award BADGE_BOULDER, show post-battle text
 *   7. After text closes: state returns to GS_IDLE
 */
#include "gym_scripts.h"
#include "badge.h"
#include "text.h"
#include "music.h"
#include "trainer_sight.h"  /* gTrainerAfterText */
#include "battle/battle_ui.h"  /* BattleUI_SetBadgeInfoText */
#include "inventory.h"
#include "../data/event_constants.h"
#include "../platform/hardware.h"
#include "../platform/audio.h"
#include <stdio.h>

extern uint8_t wPlayerName[];

/* Built at battle-start time so gTrainerAfterText can point into it.
 * Pages 1-2 of _PewterGymBrockReceivedBoulderBadgeText. */
static char kBrockDefeatQuote[96];

/* Page 3 — shown simultaneously with key-item jingle. */
static char kBrockRecvText[48];

/* Shown after page 3 is dismissed: _PewterGymBrockBoulderBadgeInfoText (3 pages). */
static const char kBrockBadgeInfo[] =
    "That's an official\nPOKEMON LEAGUE\nBADGE!"
    "\fIts bearer's\nPOKEMON become\nmore powerful!"
    "\fThe technique\nFLASH can now be\nused any time!";

/* ---- Trainer class/no for each gym leader (1-based party index) ---- */
#define BROCK_CLASS              34   /* trainer_const BROCK = $22 */
#define BROCK_NO                  1
#define PEWTER_TRAINER_CLASS      5   /* JR_TRAINER_M = $05 */
#define PEWTER_TRAINER_NO         1

#define MISTY_CLASS              35   /* trainer_const MISTY = $23 */
#define MISTY_NO                  1
#define CERULEAN_TRAINER0_CLASS   6   /* JR_TRAINER_F = $06 */
#define SURGE_CLASS              36   /* trainer_const LT_SURGE = $24 */
#define SURGE_NO                  1
#define CERULEAN_TRAINER0_NO      1
#define CERULEAN_TRAINER1_CLASS  15   /* SWIMMER = $0F */
#define CERULEAN_TRAINER1_NO      1

/* ---- State machine ----------------------------------------------- */
typedef enum {
    GS_IDLE = 0,

    /* Brock (Pewter) */
    GS_BROCK_PRE_TEXT,
    GS_BROCK_PRE_WAIT,
    GS_BROCK_START_BATTLE,
    GS_BROCK_POST_TEXT,
    GS_BROCK_POST_WAIT,
    GS_BROCK_TM_TEXT,     /* "[PLAYER] received TM34!" with GetItem1 jingle */
    GS_BROCK_TM_WAIT,
    GS_BROCK_TM_EXPLAIN,  /* TM34 explanation text */
    GS_BROCK_TM_EXP_WAIT,

    /* Misty (Cerulean) */
    GS_MISTY_PRE_TEXT,
    GS_MISTY_PRE_WAIT,
    GS_MISTY_POST_TEXT,
    GS_MISTY_POST_WAIT,
    GS_MISTY_TM_WAIT,      /* wait for "[PLAYER] received TM11!" */
    GS_MISTY_TM_EXPLAIN,   /* show TM11 explanation */
    GS_MISTY_TM_EXP_WAIT,

    /* Generic gym trainer NPC (class/no/event set at interact time) */
    GS_GYM_TRAINER_PRE_TEXT,
    GS_GYM_TRAINER_PRE_WAIT,
    GS_GYM_TRAINER_POST_TEXT,
    GS_GYM_TRAINER_POST_WAIT,

    /* Lt. Surge (Vermilion) */
    GS_SURGE_PRE_TEXT,
    GS_SURGE_PRE_WAIT,
    GS_SURGE_POST_TEXT,
    GS_SURGE_POST_WAIT,
    GS_SURGE_TM_WAIT,      /* wait for "[PLAYER] received TM24!" */
    GS_SURGE_TM_EXPLAIN,   /* show TM24 explanation */
    GS_SURGE_TM_EXP_WAIT,

    /* Generic guide / info text */
    GS_GUIDE_TEXT,
    GS_GUIDE_WAIT,
} GymState;

static GymState     gState                  = GS_IDLE;
static int          gPostFadeTimer          = 0;  /* ticks to wait for fade-in before showing post-battle text */
static int          gPendingBattle          = 0;
static uint8_t      gPendingClass           = 0;
static uint8_t      gPendingNo              = 0;
       int          gGymTrainerBattlePending = 0;

/* Generic gym trainer NPC state — populated by each gym's trainer interact fn */
static uint8_t      gGymTrainerClass        = 0;
static uint8_t      gGymTrainerNo           = 0;
       uint32_t     gGymTrainerVictoryEvent = 0;
       const char  *gGymTrainerEndText      = NULL;
static const char  *gGymTrainerAfterText    = NULL;

/* ---- Brock dialogue ----------------------------------------------- */
/* Pre-battle speech (mirrors _PewterGymBrockPreBattleText) */
static const char kBrockPre[] =
    "I'm BROCK!\nI'm PEWTER's GYM\nLEADER!"
    "\fI believe in rock\nhard defense\nand determination!"
    "\fThat's why my\nPOKEMON are all\nthe rock-type!"
    "\fDo you still want\nto challenge me?\nFine then! Show\nme your best!";

/* Post-victory speech (mirrors _PewterGymBrockWaitTakeThisText +
 * _PewterGymBrockReceivedBoulderBadgeText) */
/* Mirrors _PewterGymBrockWaitTakeThisText (PewterGym_2.asm) */
static const char kBrockPost[] =
    "Wait! Take this\nwith you!";

/* Built at post-battle time: "[PLAYER] received\nTM34!" */
static char kBrockTMText[48];

/* _TM34ExplanationText (PewterGym_2.asm:25-47) */
static const char kBrockTMExplain[] =
    "A TM contains a\ntechnique that\ncan be taught to\nPOKEMON!"
    "\fA TM is good only\nonce! So when you\nuse one to teach\na new technique,\npick the POKEMON\ncarefully!"
    "\fTM34 contains\nBIDE!"
    "\fYour POKEMON will\nabsorb damage in\nbattle then pay\nit back double!";

/* Post-beat advice (mirrors _PewterGymBrockPostBattleAdviceText) */
static const char kBrockAfter[] =
    "The BOULDERBADGE\nmakes your POKEMON\nobey you."
    "\fIt also boosts\nATTACK strength\noutside battle.";

/* ---- Gym trainer dialogue ----------------------------------------- */
/* Pre-battle (mirrors _PewterGymCooltrainerMBattleText) */
static const char kGymTrainerPre[] =
    "Stop right there,\nkid!"
    "\fYou're still light\nyears from\nfacing BROCK!";

/* After player wins (mirrors _PewterGymCooltrainerMEndBattleText) */
static const char kGymTrainerEnd[] =
    "Darn!"
    "\fLight years isn't\ntime! It measures\ndistance!";

/* Repeated talks after defeat (mirrors _PewterGymCooltrainerMAfterBattleText) */
static const char kGymTrainerAfter[] =
    "You're pretty hot,\nbut not as hot\nas BROCK!";

/* ---- Misty dialogue ----------------------------------------------- */
static const char kMistyPre[] =
    "Hi, you're a new\nface!"
    "\fTrainers who want\nto turn pro have\nto have a policy\nabout POKEMON!"
    "\fWhat is your\napproach when you\ncatch POKEMON?"
    "\fMy policy is an\nall-out offensive\nwith water-type\nPOKEMON!";

/* Battle defeat quote pages 1-3 (→ gTrainerAfterText):
 * mirrors _CeruleanGymMistyReceivedCascadeBadgeText pages 1-3 */
static const char kMistyDefeatQuote[] =
    "Wow!\nYou're too much!"
    "\fAll right!"
    "\fYou can have the\nCASCADEBADGE to\nshow you beat me!";

/* Page 4 shown simultaneously with GetKeyItem jingle (→ BattleUI_SetBadgeRecvText):
 * "[PLAYER] received the CASCADEBADGE!" — built at battle-start time */
static char kMistyBadgeRecv[48];

/* Badge info shown in overworld post-battle (5 pages):
 * mirrors _CeruleanGymMistyCascadeBadgeInfoText */
static const char kMistyBadgeInfo[] =
    "The CASCADEBADGE\nmakes all #MON\nup to L30 obey!"
    "\fThat includes\neven outsiders!"
    "\fThere's more, you\ncan now use CUT\nany time!"
    "\fYou can CUT down\nsmall bushes to\nopen new paths!"
    "\fYou can also have\nmy favorite TM!";

/* "[PLAYER] received TM11!" — filled at runtime */
static char kMistyTMText[48];

/* TM11 explanation (2 pages) — also shown on re-interaction after badge:
 * mirrors _CeruleanGymMistyTM11ExplanationText */
static const char kMistyAfter[] =
    "TM11 teaches\nBUBBLEBEAM!"
    "\fUse it on an\naquatic #MON!";

/* ---- Cerulean gym trainer dialogue -------------------------------- */
static const char kCeruleanTrainer0Pre[]   = "I'm more than\ngood enough for\nyou!";
static const char kCeruleanTrainer0End[]   = "You\noverwhelmed me!";
static const char kCeruleanTrainer0After[] = "You have to face\nother trainers to\nfind out how good\nyou really are!";

static const char kCeruleanTrainer1Pre[]   = "Splash!\nI'm first up!";
static const char kCeruleanTrainer1End[]   = "That\ncan't be!";
static const char kCeruleanTrainer1After[] = "MISTY is going\nto keep\nimproving!";

/* ---- Cerulean gym guide dialogue ---------------------------------- */
static const char kCeruleanGuidePre[] =
    "Yo! Champ in\nmaking!"
    "\fHere's my advice!"
    "\fThe LEADER, MISTY,\nis a pro who uses\nwater POKEMON!"
    "\fYou can drain all\ntheir water with\nplant POKEMON!"
    "\fOr, zap them with\nelectricity!";

static const char kCeruleanGuidePost[] =
    "You beat MISTY!\nWhat'd I tell ya?";

/* ---- Vermilion gym / Lt. Surge dialogue --------------------------- */
static const char kSurgePre[] =
    "Hey, kid! What do\nyou think you're\ndoing here?"
    "\fYou won't live\nlong in combat!\nThat's for sure!"
    "\fI tell you kid,\nelectric #MON\nsaved me during\nthe war!"
    "\fThey zapped my\nenemies into\nparalysis!"
    "\fThe same as I'll\ndo to you!";

static const char kSurgePost[] =
    "The THUNDERBADGE\ncranks up your\n#MON's SPEED!"
    "\fIt also lets your\n#MON FLY any\ntime, kid!"
    "\fYou're special,\nkid! Take this!";

static const char kSurgeAfter[] =
    "A little word of\nadvice, kid!"
    "\fElectricity is\nsure powerful!"
    "\fBut, it's useless\nagainst ground-\ntype #MON!";

/* Defeat quote — mirrors _VermilionGymLTSurgeReceivedThunderBadgeText */
static char kSurgeDefeatQuote[96];
/* Badge received — "[PLAYER] received the THUNDERBADGE!" */
static char kSurgeBadgeRecv[48];
/* TM received — "[PLAYER] received TM24!" */
static char kSurgeTMText[48];
/* TM24 explanation — mirrors _TM24ExplanationText (2 pages) */
static const char kSurgeTMExplain[] =
    "TM24 contains\nTHUNDERBOLT!"
    "\fTeach it to an\nelectric #MON!";

/* ---- Pewter gym guide dialogue ------------------------------------ */
/* Pre-beat: advice about party order (mirrors PewterGymGuideText path) */
static const char kGuidePre[] =
    "Hiya! I can tell\nyou have what it\ntakes to become a\nPOKEMON champ!"
    "\fI'm no trainer,\nbut I can tell\nyou how to win!"
    "\fIt's a free\nservice! Let's\nget happening!"
    "\fThe 1st POKEMON\nout in a match is\nat the top of the\nPOKEMON LIST!"
    "\fBy changing the\norder of POKEMON,\nmatches could be\nmade easier!";

/* Post-beat (mirrors _PewterGymGuidePostBattleText) */
static const char kGuidePost[] =
    "Just as I thought!\nYou're POKEMON\nchamp material!";

/* ---- Public API --------------------------------------------------- */

void GymScripts_Tick(void) {
    switch (gState) {
    case GS_IDLE:
        return;

    case GS_BROCK_PRE_TEXT:
        /* Text was shown in BrockInteract; wait for it to open */
        gState = GS_BROCK_PRE_WAIT;
        return;

    case GS_BROCK_PRE_WAIT:
        if (Text_IsOpen()) return;
        /* Build defeat quote ("I took you for granted..." — mirrors
         * _PewterGymBrockReceivedBoulderBadgeText shown by PrintEndBattleText) */
        {
            char player_ascii[12] = "RED";
            for (int i = 0; i < 11; i++) {
                uint8_t c = wPlayerName[i];
                if (c == 0x50) break;
                if (c >= 0x80 && c <= 0x99)      player_ascii[i] = (char)('A' + c - 0x80);
                else if (c >= 0xA0 && c <= 0xB9) player_ascii[i] = (char)('a' + c - 0xA0);
                else { player_ascii[i] = '?'; }
                player_ascii[i + 1] = '\0';
            }
            /* Pages 1-2 of _PewterGymBrockReceivedBoulderBadgeText */
            snprintf(kBrockDefeatQuote, sizeof(kBrockDefeatQuote),
                     "I took\nyou for granted."
                     "\fAs proof of your\nvictory, here's\nthe BOULDERBADGE!",
                     player_ascii);
            /* Page 3 — shown simultaneously with key-item jingle */
            snprintf(kBrockRecvText, sizeof(kBrockRecvText),
                     "%s received\nthe BOULDERBADGE!", player_ascii);
            gTrainerAfterText = kBrockDefeatQuote;
            BattleUI_SetBadgeRecvText(kBrockRecvText);
            BattleUI_SetBadgeInfoText(kBrockBadgeInfo);
        }
        /* Text closed — start the battle */
        wGymLeaderNo    = 1;   /* non-zero → gym leader music in game.c */
        gPendingClass   = BROCK_CLASS;
        gPendingNo      = BROCK_NO;
        gPendingBattle  = 1;
        gState          = GS_IDLE;
        return;

    case GS_BROCK_START_BATTLE:
        /* Unused: battle is triggered via GymScripts_GetPendingBattle */
        gState = GS_IDLE;
        return;

    case GS_BROCK_POST_TEXT:
        if (gPostFadeTimer > 0) { gPostFadeTimer--; return; }
        Text_ShowASCII(kBrockPost);
        gState = GS_BROCK_POST_WAIT;
        return;

    case GS_BROCK_POST_WAIT:
        if (Text_IsOpen()) return;
        /* Give TM34 (BIDE). Mirrors GiveItem TM_BIDE in PewterGymScriptReceiveTM34. */
        {
            char player_ascii[12] = "RED";
            for (int i = 0; i < 11; i++) {
                uint8_t c = wPlayerName[i];
                if (c == 0x50) break;
                if (c >= 0x80 && c <= 0x99)      player_ascii[i] = (char)('A' + c - 0x80);
                else if (c >= 0xA0 && c <= 0xB9) player_ascii[i] = (char)('a' + c - 0xA0);
                else { player_ascii[i] = '?'; }
                player_ascii[i + 1] = '\0';
            }
            snprintf(kBrockTMText, sizeof(kBrockTMText),
                     "%s received\nTM34!", player_ascii);
        }
        Inventory_Add(TM01 + 33, 1);   /* TM34 = TM01 + 33 = 0xEA */
        Audio_PlaySFX_GetItem1();
        Text_ShowASCII(kBrockTMText);
        gState = GS_BROCK_TM_WAIT;
        return;

    case GS_BROCK_TM_WAIT:
        if (Text_IsOpen()) return;
        gState = GS_BROCK_TM_EXPLAIN;
        return;

    case GS_BROCK_TM_EXPLAIN:
        Text_ShowASCII(kBrockTMExplain);
        gState = GS_BROCK_TM_EXP_WAIT;
        return;

    case GS_BROCK_TM_EXP_WAIT:
        if (Text_IsOpen()) return;
        gState = GS_IDLE;
        return;

    case GS_GYM_TRAINER_PRE_TEXT:
        gState = GS_GYM_TRAINER_PRE_WAIT;
        return;

    case GS_MISTY_PRE_TEXT:
        gState = GS_MISTY_PRE_WAIT;
        return;

    case GS_MISTY_PRE_WAIT:
        if (Text_IsOpen()) return;
        /* Set up defeat sequence (mirrors CeruleanGymMistyReceivedCascadeBadgeText).
         * Pages 1-3 via gTrainerAfterText; page 4 shown with GetKeyItem jingle. */
        {
            char player_ascii[12] = "RED";
            for (int i = 0; i < 11; i++) {
                uint8_t c = wPlayerName[i];
                if (c == 0x50) break;
                if (c >= 0x80 && c <= 0x99)      player_ascii[i] = (char)('A' + c - 0x80);
                else if (c >= 0xA0 && c <= 0xB9) player_ascii[i] = (char)('a' + c - 0xA0);
                else { player_ascii[i] = '?'; }
                player_ascii[i + 1] = '\0';
            }
            snprintf(kMistyBadgeRecv, sizeof(kMistyBadgeRecv),
                     "%s received\nthe CASCADEBADGE!", player_ascii);
        }
        gTrainerAfterText = kMistyDefeatQuote;
        BattleUI_SetBadgeRecvText(kMistyBadgeRecv);
        /* Badge info goes in overworld post-battle; no badge info text in battle UI */
        wGymLeaderNo   = 2;
        gPendingClass  = MISTY_CLASS;
        gPendingNo     = MISTY_NO;
        gPendingBattle = 1;
        gState         = GS_IDLE;
        return;

    case GS_MISTY_POST_TEXT:
        /* Badge info (5 pages ending "You can also have my favorite TM!"):
         * mirrors _CeruleanGymMistyCascadeBadgeInfoText */
        if (gPostFadeTimer > 0) { gPostFadeTimer--; return; }
        Text_ShowASCII(kMistyBadgeInfo);
        gState = GS_MISTY_POST_WAIT;
        return;

    case GS_MISTY_POST_WAIT:
        /* Wait for badge info, then give TM11 (mirrors CeruleanGymReceiveTM11) */
        if (Text_IsOpen()) return;
        {
            char playerName[12] = "RED";
            for (int i = 0; i < 11; i++) {
                uint8_t c = wPlayerName[i];
                if (c == 0x50) break;
                if (c >= 0x80 && c <= 0x99)      playerName[i] = (char)('A' + c - 0x80);
                else if (c >= 0xA0 && c <= 0xB9) playerName[i] = (char)('a' + c - 0xA0);
                else { playerName[i] = '?'; }
                playerName[i + 1] = '\0';
            }
            snprintf(kMistyTMText, sizeof(kMistyTMText),
                "%s received\nTM11!", playerName);
        }
        Inventory_Add(TM01 + 10, 1);   /* TM11 = TM_BUBBLEBEAM */
        SetEvent(EVENT_GOT_TM11);
        Audio_PlaySFX_GetItem1();
        Text_ShowASCII(kMistyTMText);
        gState = GS_MISTY_TM_WAIT;
        return;

    case GS_MISTY_TM_WAIT:
        if (Text_IsOpen()) return;
        gState = GS_MISTY_TM_EXPLAIN;
        return;

    case GS_MISTY_TM_EXPLAIN:
        /* 2-page TM explanation: mirrors _CeruleanGymMistyTM11ExplanationText */
        Text_ShowASCII(kMistyAfter);
        gState = GS_MISTY_TM_EXP_WAIT;
        return;

    case GS_MISTY_TM_EXP_WAIT:
        if (Text_IsOpen()) return;
        gState = GS_IDLE;
        return;

    case GS_SURGE_PRE_TEXT:
        gState = GS_SURGE_PRE_WAIT;
        return;

    case GS_SURGE_PRE_WAIT:
        if (Text_IsOpen()) return;
        /* Build defeat quote (mirrors _VermilionGymLTSurgeReceivedThunderBadgeText)
         * and badge received text shown with key-item jingle in battle UI. */
        {
            char playerName[12] = "RED";
            for (int i = 0; i < 11; i++) {
                uint8_t c = wPlayerName[i];
                if (c == 0x50) break;
                if (c >= 0x80 && c <= 0x99)      playerName[i] = (char)('A' + c - 0x80);
                else if (c >= 0xA0 && c <= 0xB9) playerName[i] = (char)('a' + c - 0xA0);
                else { playerName[i] = '?'; }
                playerName[i + 1] = '\0';
            }
            snprintf(kSurgeDefeatQuote, sizeof(kSurgeDefeatQuote),
                "Whoa!"
                "\fYou're the real\ndeal, kid!"
                "\fFine then, take\nthe THUNDERBADGE!");
            snprintf(kSurgeBadgeRecv, sizeof(kSurgeBadgeRecv),
                "%s received\nthe THUNDERBADGE!", playerName);
            gTrainerAfterText = kSurgeDefeatQuote;
            BattleUI_SetBadgeRecvText(kSurgeBadgeRecv);
            /* Badge info shown in overworld (GS_SURGE_POST_TEXT); not wired here. */
        }
        wGymLeaderNo   = 3;
        gPendingClass  = SURGE_CLASS;
        gPendingNo     = SURGE_NO;
        gPendingBattle = 1;
        gState         = GS_IDLE;
        return;

    case GS_SURGE_POST_TEXT:
        if (gPostFadeTimer > 0) { gPostFadeTimer--; return; }
        Text_ShowASCII(kSurgePost);
        gState = GS_SURGE_POST_WAIT;
        return;

    case GS_SURGE_POST_WAIT:
        /* Badge info text closed — give TM24 (mirrors VermilionGymLTSurgeReceiveTM24Script) */
        if (Text_IsOpen()) return;
        {
            char playerName[12] = "RED";
            for (int i = 0; i < 11; i++) {
                uint8_t c = wPlayerName[i];
                if (c == 0x50) break;
                if (c >= 0x80 && c <= 0x99)      playerName[i] = (char)('A' + c - 0x80);
                else if (c >= 0xA0 && c <= 0xB9) playerName[i] = (char)('a' + c - 0xA0);
                else { playerName[i] = '?'; }
                playerName[i + 1] = '\0';
            }
            snprintf(kSurgeTMText, sizeof(kSurgeTMText),
                "%s received\nTM24!", playerName);
        }
        Inventory_Add(TM01 + 23, 1);   /* TM24 = TM_THUNDERBOLT */
        SetEvent(EVENT_GOT_TM24);
        Audio_PlaySFX_GetItem1();
        Text_ShowASCII(kSurgeTMText);
        gState = GS_SURGE_TM_WAIT;
        return;

    case GS_SURGE_TM_WAIT:
        if (Text_IsOpen()) return;
        gState = GS_SURGE_TM_EXPLAIN;
        return;

    case GS_SURGE_TM_EXPLAIN:
        /* 2-page TM explanation — mirrors _TM24ExplanationText */
        Text_ShowASCII(kSurgeTMExplain);
        gState = GS_SURGE_TM_EXP_WAIT;
        return;

    case GS_SURGE_TM_EXP_WAIT:
        if (Text_IsOpen()) return;
        gState = GS_IDLE;
        return;

    case GS_GYM_TRAINER_PRE_WAIT:
        if (Text_IsOpen()) return;
        gPendingClass            = gGymTrainerClass;
        gPendingNo               = gGymTrainerNo;
        gPendingBattle           = 1;
        gGymTrainerBattlePending = 1;
        gState                   = GS_IDLE;
        return;

    case GS_GYM_TRAINER_POST_TEXT:
        if (gPostFadeTimer > 0) { gPostFadeTimer--; return; }
        Text_ShowASCII(gGymTrainerEndText ? gGymTrainerEndText : kGymTrainerEnd);
        gState = GS_GYM_TRAINER_POST_WAIT;
        return;

    case GS_GYM_TRAINER_POST_WAIT:
        if (Text_IsOpen()) return;
        gState = GS_IDLE;
        return;

    case GS_GUIDE_TEXT:
        gState = GS_GUIDE_WAIT;
        return;

    case GS_GUIDE_WAIT:
        if (Text_IsOpen()) return;
        gState = GS_IDLE;
        return;
    }
}

int GymScripts_IsActive(void) {
    return gState != GS_IDLE;
}

int GymScripts_GetPendingBattle(uint8_t *class_out, uint8_t *no_out) {
    if (!gPendingBattle) return 0;
    gPendingBattle = 0;
    *class_out = gPendingClass;
    *no_out    = gPendingNo;
    return 1;
}

/* Fade-in after battle is 4 steps × 4 ticks = 16 overworld ticks.
 * Wait 17 to be safe before showing post-battle text. */
#define POST_FADE_WAIT 17

void GymScripts_OnVictory(void) {
    gPostFadeTimer = POST_FADE_WAIT;
    if (wGymLeaderNo == 1) {
        SetEvent(EVENT_BEAT_BROCK);
        SetEvent(EVENT_BEAT_PEWTER_GYM_TRAINER_0); /* deactivate gym trainers (mirrors PewterGymBrockPostBattle) */
        wObtainedBadges |= (1u << BADGE_BOULDER);
        wGymLeaderNo = 0;
        gState = GS_BROCK_POST_TEXT;
    } else if (wGymLeaderNo == 2) {
        SetEvent(EVENT_BEAT_MISTY);
        SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_0);
        SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_1);
        wObtainedBadges |= (1u << BADGE_CASCADE);
        wGymLeaderNo = 0;
        gState = GS_MISTY_POST_TEXT;
    } else if (wGymLeaderNo == 3) {
        SetEvent(EVENT_BEAT_LT_SURGE);
        SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_0);
        SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_1);
        SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_2);
        wObtainedBadges |= (1u << BADGE_THUNDER);
        wGymLeaderNo = 0;
        gState = GS_SURGE_POST_TEXT;
    } else {
        wGymLeaderNo = 0;
    }
}

/* ---- NPC callbacks ------------------------------------------------ */

void GymScripts_BrockInteract(void) {
    if (CheckEvent(EVENT_BEAT_BROCK)) {
        Text_ShowASCII(kBrockAfter);
        return;
    }
    Text_ShowASCII(kBrockPre);
    gState = GS_BROCK_PRE_TEXT;
}

void GymScripts_GymTrainerInteract(void) {
    if (CheckEvent(EVENT_BEAT_PEWTER_GYM_TRAINER_0)) {
        Text_ShowASCII(kGymTrainerAfter);
        return;
    }
    gGymTrainerClass        = PEWTER_TRAINER_CLASS;
    gGymTrainerNo           = PEWTER_TRAINER_NO;
    gGymTrainerVictoryEvent = EVENT_BEAT_PEWTER_GYM_TRAINER_0;
    gGymTrainerEndText      = kGymTrainerEnd;
    gGymTrainerAfterText    = kGymTrainerAfter;
    Text_ShowASCII(kGymTrainerPre);
    gState = GS_GYM_TRAINER_PRE_TEXT;
}

/* Public helper: configure generic trainer state and start pre-battle text.
 * Used by gym-specific scripts in other translation units. */
void GymScripts_SetTrainerPending(uint8_t cls, uint8_t no, uint32_t flag,
                                   const char *end_text, const char *after_text,
                                   const char *pre_text) {
    gGymTrainerClass        = cls;
    gGymTrainerNo           = no;
    gGymTrainerVictoryEvent = flag;
    gGymTrainerEndText      = end_text;
    gGymTrainerAfterText    = after_text;
    Text_ShowASCII(pre_text);
    gState = GS_GYM_TRAINER_PRE_TEXT;
}

/* Called by game.c after gym trainer battle ends in TRAINER_VICTORY.
 * Defers end text until after WARP_FADE_IN completes (via gPostFadeTimer),
 * mirroring the pattern used for gym leader post-battle text. */
void GymScripts_OnGymTrainerVictory(void) {
    if (gGymTrainerVictoryEvent) SetEvent(gGymTrainerVictoryEvent);
    gPostFadeTimer = POST_FADE_WAIT;
    gState = GS_GYM_TRAINER_POST_TEXT;
}

/* Read-and-clear the gym trainer battle flag.  Call once after any battle
 * that was initiated through gym scripts (victory or not) to keep state clean.
 * Returns non-zero if the battle was a gym trainer NPC battle. */
int GymScripts_ConsumeGymTrainer(void) {
    int v = gGymTrainerBattlePending;
    gGymTrainerBattlePending = 0;
    return v;
}

void GymScripts_GuideInteract(void) {
    if (CheckEvent(EVENT_BEAT_BROCK)) {
        Text_ShowASCII(kGuidePost);
    } else {
        Text_ShowASCII(kGuidePre);
    }
    gState = GS_GUIDE_TEXT;
}

/* ---- Cerulean Gym callbacks --------------------------------------- */

void GymScripts_MistyInteract(void) {
    if (CheckEvent(EVENT_BEAT_MISTY)) {
        Text_ShowASCII(kMistyAfter);
        gState = GS_GUIDE_TEXT;   /* reuse generic text-wait */
        return;
    }
    Text_ShowASCII(kMistyPre);
    gState = GS_MISTY_PRE_TEXT;
}

void GymScripts_CeruleanTrainer0Interact(void) {
    if (CheckEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_0)) {
        Text_ShowASCII(kCeruleanTrainer0After);
        gState = GS_GUIDE_TEXT;
        return;
    }
    gGymTrainerClass        = CERULEAN_TRAINER0_CLASS;
    gGymTrainerNo           = CERULEAN_TRAINER0_NO;
    gGymTrainerVictoryEvent = EVENT_BEAT_CERULEAN_GYM_TRAINER_0;
    gGymTrainerEndText      = kCeruleanTrainer0End;
    gGymTrainerAfterText    = kCeruleanTrainer0After;
    Text_ShowASCII(kCeruleanTrainer0Pre);
    gState = GS_GYM_TRAINER_PRE_TEXT;
}

void GymScripts_CeruleanTrainer1Interact(void) {
    if (CheckEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_1)) {
        Text_ShowASCII(kCeruleanTrainer1After);
        gState = GS_GUIDE_TEXT;
        return;
    }
    gGymTrainerClass        = CERULEAN_TRAINER1_CLASS;
    gGymTrainerNo           = CERULEAN_TRAINER1_NO;
    gGymTrainerVictoryEvent = EVENT_BEAT_CERULEAN_GYM_TRAINER_1;
    gGymTrainerEndText      = kCeruleanTrainer1End;
    gGymTrainerAfterText    = kCeruleanTrainer1After;
    Text_ShowASCII(kCeruleanTrainer1Pre);
    gState = GS_GYM_TRAINER_PRE_TEXT;
}

void GymScripts_CeruleanGuideInteract(void) {
    if (CheckEvent(EVENT_BEAT_MISTY)) {
        Text_ShowASCII(kCeruleanGuidePost);
    } else {
        Text_ShowASCII(kCeruleanGuidePre);
    }
    gState = GS_GUIDE_TEXT;
}

/* ---- Vermilion Gym callbacks -------------------------------------- */

void GymScripts_SurgeInteract(void) {
    if (CheckEvent(EVENT_BEAT_LT_SURGE)) {
        Text_ShowASCII(kSurgeAfter);
        gState = GS_GUIDE_TEXT;
        return;
    }
    Text_ShowASCII(kSurgePre);
    gState = GS_SURGE_PRE_TEXT;
}
