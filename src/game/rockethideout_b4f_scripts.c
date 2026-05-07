#include "rockethideout_b4f_scripts.h"
#include "text.h"
#include "npc.h"
#include "overworld.h"
#include "trainer_sight.h"
#include "../platform/display.h"
#include "../platform/hardware.h"
#include "../data/event_constants.h"

#define MAP_ROCKET_HIDEOUT_B4F 0xca
#define GIOVANNI_NPC_IDX 0
#define SILPH_SCOPE_NPC_SLOT 7
#define LIFT_KEY_NPC_SLOT 8

#define GIOVANNI_CLASS 29
#define GIOVANNI_NO    1
#define ROCKET_CLASS   30
#define ROCKET3_NO     1

typedef enum {
    RH4_IDLE = 0,
    RH4_GIO_PRE_TEXT,
    RH4_GIO_BATTLE_PENDING,
    RH4_R3_PRE_TEXT,
    RH4_R3_BATTLE_PENDING,
    RH4_GIO_POST_TEXT,
    RH4_POST_BATTLE_TEXT,
    RH4_GIO_FADE_OUT,
    RH4_GIO_FADE_IN,
} RH4State;

typedef enum {
    RH4B_NONE = 0,
    RH4B_GIOVANNI,
    RH4B_ROCKET3,
} RH4BattleKind;

static RH4State g_state = RH4_IDLE;
static RH4BattleKind g_pending_kind = RH4B_NONE;
static RH4BattleKind g_active_kind = RH4B_NONE;
static int g_pending_battle = 0;
static const char *g_post_battle_text = NULL;
static RH4BattleKind g_post_kind = RH4B_NONE;
static int g_restore_gio_facing = 0;
static int g_saved_gio_facing = 0;
static int g_fade_step = 0;
static int g_fade_timer = 0;

#define RH4_FADE_STEP_TICKS 4
#define RH4_FADE_STEPS 4

static const uint8_t kFadeOut[RH4_FADE_STEPS][3] = {
    {0xE4, 0xD0, 0xE0},
    {0xF9, 0xE4, 0xE4},
    {0xFE, 0xFE, 0xF8},
    {0xFF, 0xFF, 0xFF},
};

static const uint8_t kFadeInFromWhite[RH4_FADE_STEPS][3] = {
    {0x00, 0x00, 0x00},
    {0x0A, 0x01, 0x08},
    {0x28, 0x04, 0x20},
    {0xE4, 0xD0, 0xE0},
};

static const char kGiovanniPre[] =
    "So! I must say, I\nam impressed you\ngot here!";

static const char kGiovanniDefeated[] =
    "WHAT! This cannot\nbe!";

static const char kGiovanniAfter[] =
    "I see that you\nraise POKEMON\nwith utmost care.\f"
    "A child like you\nwould never\nunderstand what\nI hope to achieve.\f"
    "I shall step aside\nthis time!\f"
    "I hope we meet\nagain...";

static const char kRocket3Pre[] =
    "Why did you come\nhere?";

static const char kRocket3Defeated[] =
    "No!@";

static const char kRocket3After[] =
    "Oh no! I dropped\nthe LIFT KEY!";

void RocketHideoutB4FScripts_OnMapLoad(void) {
    if (wCurMap != MAP_ROCKET_HIDEOUT_B4F) return;

    /* Battle return path re-fires OnMapLoad; preserve the in-progress
     * Giovanni post-battle sequence so text -> fade -> hide/show can finish. */
    if (g_state == RH4_GIO_POST_TEXT ||
        g_state == RH4_GIO_FADE_OUT  ||
        g_state == RH4_GIO_FADE_IN) {
        return;
    }

    g_state = RH4_IDLE;
    g_pending_kind = RH4B_NONE;
    g_active_kind = RH4B_NONE;
    g_pending_battle = 0;
    g_post_battle_text = NULL;
    g_post_kind = RH4B_NONE;
    g_restore_gio_facing = 0;
    g_saved_gio_facing = 0;
    g_fade_step = 0;
    g_fade_timer = 0;

    if (CheckEvent(EVENT_BEAT_ROCKET_HIDEOUT_GIOVANNI))
        NPC_HideSprite(GIOVANNI_NPC_IDX);

    /* ASM toggle-object behavior: Silph Scope appears only after Giovanni sequence. */
    if (!CheckEvent(EVENT_BEAT_ROCKET_HIDEOUT_GIOVANNI))
        NPC_HideSprite(SILPH_SCOPE_NPC_SLOT);

    /* ASM behavior: Lift Key object appears only after Rocket3 drops it. */
    if (!CheckEvent(EVENT_ROCKET_DROPPED_LIFT_KEY))
        NPC_HideSprite(LIFT_KEY_NPC_SLOT);

    printf("[rh4-gio] onload beat_giovanni=%d dropped_lift_key=%d\n",
           CheckEvent(EVENT_BEAT_ROCKET_HIDEOUT_GIOVANNI),
           CheckEvent(EVENT_ROCKET_DROPPED_LIFT_KEY));
}

void RocketHideoutB4FScripts_Tick(void) {
    if (g_state == RH4_IDLE || g_state == RH4_GIO_BATTLE_PENDING || g_state == RH4_R3_BATTLE_PENDING)
        return;

    switch (g_state) {
    case RH4_GIO_PRE_TEXT:
        if (Text_IsOpen()) return;
        /* Preserve pre-battle orientation; trainer reload after battle resets facings. */
        g_saved_gio_facing = NPC_GetFacing(GIOVANNI_NPC_IDX);
        gTrainerAfterText = kGiovanniDefeated;
        g_pending_kind = RH4B_GIOVANNI;
        g_pending_battle = 1;
        g_state = RH4_GIO_BATTLE_PENDING;
        return;
    case RH4_R3_PRE_TEXT:
        if (Text_IsOpen()) return;
        gTrainerAfterText = kRocket3Defeated;
        g_pending_kind = RH4B_ROCKET3;
        g_pending_battle = 1;
        g_state = RH4_R3_BATTLE_PENDING;
        return;
    case RH4_GIO_POST_TEXT:
        if (g_restore_gio_facing) {
            NPC_SetFacing(GIOVANNI_NPC_IDX, g_saved_gio_facing);
            g_restore_gio_facing = 0;
        }
        if (g_post_battle_text && !Text_IsOpen()) {
            Text_ShowASCII(g_post_battle_text);
            g_post_battle_text = NULL;
            return;
        }
        if (Text_IsOpen()) return;
        g_fade_step = 0;
        g_fade_timer = RH4_FADE_STEP_TICKS;
        Display_SetPalette(kFadeOut[0][0], kFadeOut[0][1], kFadeOut[0][2]);
        g_state = RH4_GIO_FADE_OUT;
        return;
    case RH4_POST_BATTLE_TEXT:
        if (g_post_battle_text && !Text_IsOpen()) {
            Text_ShowASCII(g_post_battle_text);
            g_post_battle_text = NULL;
        } else if (!Text_IsOpen()) {
            g_post_kind = RH4B_NONE;
            g_state = RH4_IDLE;
        }
        return;
    case RH4_GIO_FADE_OUT:
        if (--g_fade_timer > 0) return;
        g_fade_timer = RH4_FADE_STEP_TICKS;
        g_fade_step++;
        if (g_fade_step < RH4_FADE_STEPS) {
            Display_SetPalette(kFadeOut[g_fade_step][0], kFadeOut[g_fade_step][1], kFadeOut[g_fade_step][2]);
            return;
        }
        /* At full black: hide Giovanni, reveal Silph Scope, then fade back in. */
        NPC_HideSprite(GIOVANNI_NPC_IDX);
        NPC_ShowSprite(SILPH_SCOPE_NPC_SLOT);
        g_fade_step = 0;
        g_fade_timer = RH4_FADE_STEP_TICKS;
        Display_SetPalette(kFadeInFromWhite[0][0], kFadeInFromWhite[0][1], kFadeInFromWhite[0][2]);
        g_state = RH4_GIO_FADE_IN;
        return;
    case RH4_GIO_FADE_IN:
        if (--g_fade_timer > 0) return;
        g_fade_timer = RH4_FADE_STEP_TICKS;
        g_fade_step++;
        if (g_fade_step < RH4_FADE_STEPS) {
            Display_SetPalette(kFadeInFromWhite[g_fade_step][0], kFadeInFromWhite[g_fade_step][1], kFadeInFromWhite[g_fade_step][2]);
            return;
        }
        g_state = RH4_IDLE;
        return;
    default:
        g_state = RH4_IDLE;
        return;
    }
}

int RocketHideoutB4FScripts_IsActive(void) {
    return wCurMap == MAP_ROCKET_HIDEOUT_B4F && g_state != RH4_IDLE;
}

int RocketHideoutB4FScripts_GetPendingBattle(uint8_t *class_out, uint8_t *no_out) {
    if (wCurMap != MAP_ROCKET_HIDEOUT_B4F) return 0;
    if (!g_pending_battle) return 0;
    g_pending_battle = 0;
    if (g_pending_kind == RH4B_GIOVANNI) {
        *class_out = GIOVANNI_CLASS;
        *no_out = GIOVANNI_NO;
    } else if (g_pending_kind == RH4B_ROCKET3) {
        *class_out = ROCKET_CLASS;
        *no_out = ROCKET3_NO;
    } else {
        return 0;
    }
    g_active_kind = g_pending_kind;
    g_pending_kind = RH4B_NONE;
    return 1;
}

int RocketHideoutB4FScripts_ConsumeBattle(void) {
    if (g_active_kind == RH4B_NONE) return 0;
    return 1;
}

void RocketHideoutB4FScripts_OnVictory(void) {
    if (g_active_kind == RH4B_GIOVANNI) {
        SetEvent(EVENT_BEAT_ROCKET_HIDEOUT_GIOVANNI);
        g_post_battle_text = kGiovanniAfter;
        g_post_kind = RH4B_GIOVANNI;
        g_restore_gio_facing = 1;
        g_state = RH4_GIO_POST_TEXT;
    } else if (g_active_kind == RH4B_ROCKET3) {
        SetEvent(EVENT_BEAT_ROCKET_HIDEOUT_4_TRAINER_2);
        if (!CheckEvent(EVENT_ROCKET_DROPPED_LIFT_KEY)) {
            SetEvent(EVENT_ROCKET_DROPPED_LIFT_KEY);
            NPC_ShowSprite(LIFT_KEY_NPC_SLOT);
        }
        g_post_battle_text = kRocket3After;
        g_post_kind = RH4B_ROCKET3;
        g_state = RH4_POST_BATTLE_TEXT;
    } else {
        g_state = RH4_IDLE;
    }
    g_active_kind = RH4B_NONE;
}

void RocketHideoutB4FScripts_OnDefeat(void) {
    g_active_kind = RH4B_NONE;
    g_state = RH4_IDLE;
}

void RocketHideoutB4F_GiovanniScript(void) {
    if (wCurMap != MAP_ROCKET_HIDEOUT_B4F) return;
    if (g_state != RH4_IDLE) {
        /* Recover from stale state machine stalls that would otherwise
         * swallow A-button interaction with no visible feedback. */
        if (!Text_IsOpen() && g_pending_kind == RH4B_NONE && g_active_kind == RH4B_NONE && !g_pending_battle) {
            g_state = RH4_IDLE;
        } else {
            printf("[rh4-gio] interact blocked state=%d pending=%d active=%d pending_battle=%d text_open=%d\n",
                   (int)g_state, (int)g_pending_kind, (int)g_active_kind, g_pending_battle, Text_IsOpen());
            return;
        }
    }

    if (CheckEvent(EVENT_BEAT_ROCKET_HIDEOUT_GIOVANNI)) {
        printf("[rh4-gio] interact while beat flag set\n");
        Text_ShowASCII(kGiovanniAfter);
        return;
    }
    printf("[rh4-gio] interact pre-battle path\n");
    Text_ShowASCII(kGiovanniPre);
    g_state = RH4_GIO_PRE_TEXT;
}

void RocketHideoutB4F_Rocket3Script(void) {
    if (wCurMap != MAP_ROCKET_HIDEOUT_B4F) return;
    if (g_state != RH4_IDLE) return;

    if (CheckEvent(EVENT_BEAT_ROCKET_HIDEOUT_4_TRAINER_2)) {
        Text_ShowASCII(kRocket3After);
        return;
    }
    Text_ShowASCII(kRocket3Pre);
    g_state = RH4_R3_PRE_TEXT;
}
