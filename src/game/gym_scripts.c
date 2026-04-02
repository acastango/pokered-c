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
#include "../data/event_constants.h"
#include "../platform/hardware.h"

/* ---- Trainer class/no for each gym leader (1-based party index) ---- */
#define BROCK_CLASS              34   /* trainer_const BROCK = $22 */
#define BROCK_NO                  1
#define PEWTER_TRAINER_CLASS      5   /* JR_TRAINER_M = $05 */
#define PEWTER_TRAINER_NO         1

#define MISTY_CLASS              35   /* trainer_const MISTY = $23 */
#define MISTY_NO                  1
#define CERULEAN_TRAINER0_CLASS   6   /* JR_TRAINER_F = $06 */
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

    /* Misty (Cerulean) */
    GS_MISTY_PRE_TEXT,
    GS_MISTY_PRE_WAIT,
    GS_MISTY_POST_TEXT,
    GS_MISTY_POST_WAIT,

    /* Generic gym trainer NPC (class/no/event set at interact time) */
    GS_GYM_TRAINER_PRE_TEXT,
    GS_GYM_TRAINER_PRE_WAIT,
    GS_GYM_TRAINER_POST_TEXT,
    GS_GYM_TRAINER_POST_WAIT,

    /* Generic guide / info text */
    GS_GUIDE_TEXT,
    GS_GUIDE_WAIT,
} GymState;

static GymState     gState                  = GS_IDLE;
static int          gPendingBattle          = 0;
static uint8_t      gPendingClass           = 0;
static uint8_t      gPendingNo              = 0;
static int          gGymTrainerBattlePending = 0;

/* Generic gym trainer NPC state — populated by each gym's trainer interact fn */
static uint8_t      gGymTrainerClass        = 0;
static uint8_t      gGymTrainerNo           = 0;
static uint32_t     gGymTrainerVictoryEvent = 0;
static const char  *gGymTrainerEndText      = NULL;
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
static const char kBrockPost[] =
    "Hm... I was\ndefeated. I can't\nbelieve it!"
    "\fI, BROCK, hereby\nacknowledge your\nskill."
    "\fAs proof, here's\nthe BOULDERBADGE.";

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

static const char kMistyPost[] =
    "Wow!\nYou're too much!"
    "\fThe CASCADEBADGE\nmakes all #MON\nup to L30 obey!"
    "\fThat includes\neven outsiders!";

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
        gState = GS_BROCK_POST_WAIT;
        return;

    case GS_BROCK_POST_WAIT:
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
        wGymLeaderNo   = 2;
        gPendingClass  = MISTY_CLASS;
        gPendingNo     = MISTY_NO;
        gPendingBattle = 1;
        gState         = GS_IDLE;
        return;

    case GS_MISTY_POST_TEXT:
        gState = GS_MISTY_POST_WAIT;
        return;

    case GS_MISTY_POST_WAIT:
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

void GymScripts_OnVictory(void) {
    if (wGymLeaderNo == 1) {
        SetEvent(EVENT_BEAT_BROCK);
        wObtainedBadges |= (1u << BADGE_BOULDER);
        wGymLeaderNo = 0;
        Text_ShowASCII(kBrockPost);
        gState = GS_BROCK_POST_TEXT;
    } else if (wGymLeaderNo == 2) {
        SetEvent(EVENT_BEAT_MISTY);
        /* Also deactivate gym trainers (mirrors CeruleanGymMistyPostBattleScript) */
        SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_0);
        SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_1);
        wObtainedBadges |= (1u << BADGE_CASCADE);
        wGymLeaderNo = 0;
        Text_ShowASCII(kMistyPost);
        gState = GS_MISTY_POST_TEXT;
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

/* Called by game.c after gym trainer battle ends in TRAINER_VICTORY.
 * Sets the event flag and starts the post-battle text. */
void GymScripts_OnGymTrainerVictory(void) {
    if (gGymTrainerVictoryEvent) SetEvent(gGymTrainerVictoryEvent);
    Text_ShowASCII(gGymTrainerEndText ? gGymTrainerEndText : kGymTrainerEnd);
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
