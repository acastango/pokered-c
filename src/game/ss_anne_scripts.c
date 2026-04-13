/* ss_anne_scripts.c — SS Anne story scripts.
 *
 * Ports scripts/SSAnne2F.asm (rival battle) and
 * scripts/SSAnneCaptainsRoom.asm (HM01 Cut sequence).
 *
 * Rival trigger: player steps to x∈{36,37}, y=8 on SS_ANNE_2F (map 0x60)
 *   (ASM dbmapcoord 36,8 and 37,8 — wXCoord/wYCoord are 1:1 with ASM)
 * while EVENT_BEAT_SS_ANNE_RIVAL is not set.
 *
 * Captain trigger: player presses A on captain NPC at (4,2) in
 * SS_ANNE_CAPTAINS_ROOM (map 0x65) — fires SSAnne_CaptainScript().
 */
#include "ss_anne_scripts.h"
#include "text.h"
#include "npc.h"
#include "player.h"
#include "overworld.h"
#include "warp.h"
#include "music.h"
#include "inventory.h"
#include "trainer_sight.h"
#include "../platform/hardware.h"
#include "../platform/audio.h"
#include "../platform/display.h"
#include "../data/event_constants.h"
#include "constants.h"
#include "types.h"
#include "../platform/display.h"
#include <stdio.h>
#include <string.h>

/* ---- Map IDs ---------------------------------------------------------- */
#define MAP_SS_ANNE_2F            0x60
#define MAP_SS_ANNE_CAPTAINS_ROOM 0x65
#define MAP_VERMILION_CITY        5
#define MAP_VERMILION_DOCK        0x5E

/* ---- Externs ---------------------------------------------------------- */
extern uint8_t wPlayerName[];

/* ---- NPC indices ------------------------------------------------------- */
/* kNpcs_SSAnne2F: 0=Waiter, 1=Rival */
#define RIVAL_NPC_IDX   1

/* ---- Trainer class ----------------------------------------------------- */
/* Rival2 = class 42 in gTrainerPartyData */
#define RIVAL2_CLASS    42

/* ---- Direction codes --------------------------------------------------- */
#define DIR_DOWN  0
#define DIR_UP    1
#define DIR_LEFT  2
#define DIR_RIGHT 3

/* ---- HM item ID -------------------------------------------------------- */
#define HM01  0xC4

/* ============================================================
 * RIVAL STATE MACHINE
 * ============================================================ */

typedef enum {
    RS_IDLE = 0,
    RS_RIVAL_APPROACH,    /* rival walking down to meet player */
    RS_PRE_BATTLE_TEXT,   /* showing pre-battle dialogue */
    RS_BATTLE_PENDING,    /* text done; waiting for game.c to start battle */
    RS_POST_BATTLE_TEXT,  /* battle over; showing post-battle text */
    RS_WALK_AWAY,         /* rival walking off screen */
    RS_CLEANUP,           /* hide rival, restore music */
} RivalState;

static RivalState g_rival_state         = RS_IDLE;
static int        g_pending_battle      = 0;
static int        g_rival_battle_active = 0;
static uint8_t    g_rival_tr_no         = 0;
static int        g_walk_steps_left     = 0;
static int        g_post_text_shown     = 0;
static int        g_trigger_x           = 0;  /* wXCoord when trigger fired */
static int        g_exit_right_pending  = 0;  /* exit walk: do RIGHT step first */
static int        g_rival_saved_tx      = 0;  /* rival tile pos saved before battle */
static int        g_rival_saved_ty      = 0;

/* ---- Rival text ------------------------------------------------------- */

/* SSAnne2FRivalText (pre-battle) — matches _SSAnne2FRivalText in text/SSAnne2F.asm */
static const char kRivalPreBattle[] =
    "{RIVAL}: Bonjour!\n{PLAYER}!\f"
    "Imagine seeing\nyou here!\f"
    "{PLAYER}, were you\nreally invited?\f"
    "So how's your\n#DEX coming?\f"
    "I already caught\n40 kinds, pal!\f"
    "Different kinds\nare everywhere!\f"
    "Crawl around in\ngrassy areas!";

/* SSAnne2FRivalDefeatedText (post-battle) */
static const char kRivalPostBattle[] =
    "{RIVAL}: Hah!\nI won't lose\nnext time!\f"
    "I'm going to\nsee the CAPTAIN\nnow!\f"
    "Later, {PLAYER}!";

/* In-battle defeat quote — _SSAnne2FRivalDefeatedText */
static const char kRivalDefeated[] =
    "Humph!\f"
    "At least you're\nraising your\n#MON!";

/* ============================================================
 * CAPTAIN STATE MACHINE
 * ============================================================ */

typedef enum {
    CS_IDLE = 0,
    CS_CAPTAIN_SICK_TEXT,    /* showing "captain is sick + rub back" text */
    CS_CAPTAIN_JINGLE,       /* text dismissed, PKMN_HEALED jingle playing */
    CS_CAPTAIN_BETTER_TEXT,  /* jingle done, showing "feel better + have this" text */
    CS_CAPTAIN_GIVE_HM,      /* better text dismissed, give HM01 + key item jingle + recv text */
    CS_CAPTAIN_RECV_TEXT,    /* showing "{PLAYER} got HM01!" */
    CS_CAPTAIN_DONE,         /* wait for text + jingle, then idle */
} CaptainState;

static CaptainState g_captain_state = CS_IDLE;

/* ============================================================
 * SS ANNE DEPARTURE — simplified port of VermilionDockSSAnneLeavesScript
 * Fires once on entering Vermilion City after EVENT_GOT_HM01 is set
 * and before EVENT_SS_ANNE_LEFT is set.
 * ============================================================ */
typedef enum {
    DEP_IDLE = 0,
    DEP_WAIT1,    /* ~60 frame pause before first horn */
    DEP_HORN1,    /* horn playing */
    DEP_WAIT2,    /* ~60 frame pause between horns */
    DEP_HORN2,    /* second horn playing */
    DEP_DONE,     /* cleanup; transition to walk-out */
    DEP_WALK_OUT, /* player auto-walks north off dock into Vermilion */
} DepartureState;

/* Water tile in SHIP_PORT tileset (matches $14 in VermilionDock.asm FillMemory) */
#define DEP_WATER_TILE   20
/* First screen row covered by the ship scroll.
 * ASM: hlcoord 0, 10  → row 10 (0-indexed from top). */
#define DEP_SHIP_ROW     10
/* Number of rows animated.
 * ASM: SCREEN_WIDTH * 6  → 6 rows (rows 10-15). */
#define DEP_SHIP_ROWS    6
/* Frames between each 1-tile column shift.
 * Must equal TILE_PX (8): g_dep_scroll_tick doubles as the sub-tile pixel
 * offset (0..7) fed to Display_SetBandXPx, giving smooth 1-px-per-frame
 * scrolling for departure rows — matching the original's SCX raster trick. */
#define DEP_COL_RATE     8

/* OAM slots reserved for smoke puffs (4 sprites per puff, up to 8 puffs = 32 sprites).
 * Must be AFTER player (0-3), NPCs (4-67), shadow (68-71) so NPC_BuildView doesn't
 * overwrite them — slots 72..103 are unused in the overworld. */
#define SMOKE_OAM_BASE   72
#define SMOKE_OAM_MAX    104  /* slots 72..103 */
/* Sprite tile indices for smoke (LoadSmokeTileFourTimes → vChars1 tile $7c = sprite $FC)
 * VermilionDockOAMBlock uses tiles $fc-$ff (same smoke tile repeated 4 times) */
#define SMOKE_TILE       0xFC
/* Smoke OAM positions (GB OAM: Y offset=16, X offset=8).
 * wSSAnneSmokeX=88 (screen px=80), Y=100 (screen px=84 = row 10.5, in ship band). */
#define SMOKE_OAM_Y      100
#define SMOKE_OAM_X_INIT  88
#define SMOKE_OAM_X_STEP  16  /* X decrements by 16 per new puff */

/* SSAnneSmokePuffTile from gfx/overworld/smoke.2bpp — 16 bytes of GB 2bpp.
 * Generated from smoke.png (8×8, 2-bit grayscale). */
static const uint8_t kSmokeTileGfx[16] = {
    0x00, 0x18,  /* row 0 */
    0x1A, 0x66,  /* row 1 */
    0x04, 0x42,  /* row 2 */
    0x0B, 0x81,  /* row 3 */
    0x56, 0x89,  /* row 4 */
    0x1A, 0x2E,  /* row 5 */
    0x4C, 0x12,  /* row 6 */
    0x38, 0x38,  /* row 7 */
};

static DepartureState g_dep_state       = DEP_IDLE;
static int            g_dep_timer       = 0;
static int            g_dep_scroll_col  = 0;  /* tiles shifted so far (0..SCREEN_WIDTH) */
static int            g_dep_scroll_tick = 0;  /* frame counter for current column */
static int            g_dep_slow_tick   = 0;  /* 0..2 gate: advance only 2 of 3 frames (1/3 slower) */
static int            g_dep_smoke_x     = SMOKE_OAM_X_INIT;  /* X for next puff */
static int            g_dep_smoke_count = 0;  /* number of 4-sprite puffs emitted so far */
static int            g_dep_prev_col    = 0;  /* g_dep_scroll_col on previous tick */
static int            g_dep_walk_steps  = 0;  /* steps remaining in DEP_WALK_OUT */
/* Snapshot of the ship rows taken at departure start, doubled in width.
 * Left half [0..SCREEN_WIDTH-1]  = ship tiles from gScrollTileMap.
 * Right half [SCREEN_WIDTH..2*SCREEN_WIDTH-1] = water tiles (pre-filled).
 * As g_dep_scroll_col advances, the visible window shifts right through this
 * buffer, making water tiles that enter from the right also scroll left —
 * matching the original's full-band SCX scroll. */
static uint8_t        g_dep_ship_snap[DEP_SHIP_ROWS][SCREEN_WIDTH * 2];

/* ---- Ship-gone block override ----------------------------------------- */
/* Replace all SS Anne block positions on VermilionDock with water (block 0x0D).
 * Uses Map_SetBlock so the overrides survive until the next Map_Load, matching
 * the ASM's VermilionDock_EraseSSAnne which writes water directly to VRAM.
 * Call in DEP_DONE (animation end) and in OnMapLoad for subsequent visits. */
static void dep_erase_ship_blocks(void) {
    /* Replace gangplank (by=1, bx=5..8, IDs 04-07) and hull (by=2, bx=5..8, IDs 08-0B)
     * with water.  Both rows are visible during DEP_WALK_OUT as the camera pans north. */
    Map_SetBlock(5, 1, 0x0D);
    Map_SetBlock(6, 1, 0x0D);
    Map_SetBlock(7, 1, 0x0D);
    Map_SetBlock(8, 1, 0x0D);
    Map_SetBlock(5, 2, 0x0D);
    Map_SetBlock(6, 2, 0x0D);
    Map_SetBlock(7, 2, 0x0D);
    Map_SetBlock(8, 2, 0x0D);
}

/* ---- Smoke helpers ---------------------------------------------------- */

/* Load SSAnneSmokePuffTile into sprite tile slots $FC..$FF (same tile, 4 copies).
 * Mirrors farcall LoadSmokeTileFourTimes in engine/overworld/dust_smoke.asm. */
static void dep_load_smoke_tiles(void) {
    Display_LoadSpriteTile(0xFC, kSmokeTileGfx);
    Display_LoadSpriteTile(0xFD, kSmokeTileGfx);
    Display_LoadSpriteTile(0xFE, kSmokeTileGfx);
    Display_LoadSpriteTile(0xFF, kSmokeTileGfx);
}

/* Emit one 16×16 smoke puff (4 OAM sprites) at the next free smoke slot.
 * Mirrors VermilionDock_EmitSmokePuff (scripts/VermilionDock.asm:142-155). */
static void dep_emit_smoke_puff(void) {
    int slot = SMOKE_OAM_BASE + g_dep_smoke_count * 4;
    if (slot + 3 >= SMOKE_OAM_MAX) return;  /* cap: don't overrun NPC sprites */
    uint8_t oam_y = (uint8_t)SMOKE_OAM_Y;
    uint8_t oam_x = (uint8_t)(g_dep_smoke_x < 0 ? 0 : (g_dep_smoke_x > 255 ? 255 : g_dep_smoke_x));
    wShadowOAM[slot+0] = (oam_entry_t){ oam_y,   oam_x,   SMOKE_TILE+0, OAM_FLAG_PALETTE };
    wShadowOAM[slot+1] = (oam_entry_t){ oam_y,   oam_x+8, SMOKE_TILE+1, OAM_FLAG_PALETTE };
    wShadowOAM[slot+2] = (oam_entry_t){ oam_y+8, oam_x,   SMOKE_TILE+2, OAM_FLAG_PALETTE };
    wShadowOAM[slot+3] = (oam_entry_t){ oam_y+8, oam_x+8, SMOKE_TILE+3, OAM_FLAG_PALETTE };
    g_dep_smoke_x    -= SMOKE_OAM_X_STEP;
    g_dep_smoke_count++;
}

/* Drift all emitted smoke puffs 2 pixels to the right.
 * Mirrors VermilionDock_AnimSmokePuffDriftRight (VermilionDock.asm:124-140). */
static void dep_drift_smoke(void) {
    int total_sprites = g_dep_smoke_count * 4;
    for (int i = 0; i < total_sprites; i++) {
        int slot = SMOKE_OAM_BASE + i;
        if (slot >= SMOKE_OAM_MAX) break;
        wShadowOAM[slot].x += 2;
    }
}

/* Clear all smoke OAM entries back to hidden. */
static void dep_clear_smoke(void) {
    for (int i = SMOKE_OAM_BASE; i < SMOKE_OAM_MAX; i++)
        wShadowOAM[i] = (oam_entry_t){0, 0, 0, 0};
    g_dep_smoke_count = 0;
    g_dep_smoke_x     = SMOKE_OAM_X_INIT;
}

/* ---- Captain text (verbatim from text/SSAnneCaptainsRoom.asm) ---------- */

/* _SSAnneCaptainsRoomRubCaptainsBackText */
static const char kCaptainSick[] =
    "CAPTAIN: Ooargh...\nI feel hideous...\nUrrp! Seasick...\f"
    "{PLAYER} rubbed\nthe CAPTAIN's\nback!\f"
    "Rub-rub...\nRub-rub...";

/* _SSAnneCaptainsRoomCaptainIFeelMuchBetterText */
static const char kCaptainBetter[] =
    "CAPTAIN: Whew!\nThank you! I\nfeel much better!\f"
    "You want to see\nmy CUT technique?\f"
    "I could show you\nif I wasn't ill...\f"
    "I know! You can\nhave this!\f"
    "Teach it to your\n#MON and you\ncan see it CUT\nany time!";

/* _SSAnneCaptainsRoomCaptainReceivedHM01Text */
static const char kCaptainReceivedHM[] =
    "{PLAYER} got\nHM01!";


/* ============================================================
 * PUBLIC API
 * ============================================================ */

void SSAnneScripts_OnMapLoad(void) {
    if (wCurMap == MAP_SS_ANNE_2F) {
        /* Rival is hidden until triggered (or gone after defeat). */
        if (g_rival_state == RS_POST_BATTLE_TEXT ||
            g_rival_state == RS_WALK_AWAY        ||
            g_rival_state == RS_CLEANUP) {
            /* NPC_Load just reset Gary to spawn — restore his pre-battle pos.
             * Mirrors GetSpritePosition1/SetSpritePosition1 in OaksLab ASM. */
            NPC_SetTilePos(RIVAL_NPC_IDX, g_rival_saved_tx, g_rival_saved_ty);
            NPC_ShowSprite(RIVAL_NPC_IDX);
            return;
        }
        g_rival_state = RS_IDLE;
        if (!CheckEvent(EVENT_BEAT_SS_ANNE_RIVAL))
            NPC_ShowSprite(RIVAL_NPC_IDX);
        else
            NPC_HideSprite(RIVAL_NPC_IDX);
    }

    /* VermilionDock arrival — mirrors VermilionDock_Script (scripts/VermilionDock.asm).
     * Fires when player arrives on the dock (from SS Anne warp) with HM01 obtained
     * and ship not yet departed.  Starts the departure cutscene immediately. */
    if (wCurMap == MAP_VERMILION_DOCK) {
        /* If ship already left, re-apply block overrides (Map_Load cleared them). */
        if (CheckEvent(EVENT_SS_ANNE_LEFT))
            dep_erase_ship_blocks();
        if (CheckEvent(EVENT_GOT_HM01) && !CheckEvent(EVENT_SS_ANNE_LEFT)) {
            SetEvent(EVENT_SS_ANNE_LEFT);
            Music_Play(MUSIC_SURFING);
            gPlayerFacing = DIR_DOWN;  /* face south toward the departing ship */
            g_dep_state = DEP_WAIT1;
            g_dep_timer = 60;
        }
    }
}

void SSAnneScripts_StepCheck(void) {
    /* Rival battle trigger on SS_ANNE_2F */
    if (wCurMap != MAP_SS_ANNE_2F) return;
    if (g_rival_state != RS_IDLE) return;
    if (CheckEvent(EVENT_BEAT_SS_ANNE_RIVAL)) return;

    /* ASM trigger: dbmapcoord 36,8 and 37,8 (SSAnne2F.asm).
     * wXCoord/wYCoord are 1:1 with ASM coords. */
    if ((int)wYCoord != 8) return;
    if ((int)wXCoord != 36 && (int)wXCoord != 37) return;

    printf("[ss_anne] rival trigger at (%d,%d)\n", (int)wXCoord, (int)wYCoord);

    g_trigger_x = (int)wXCoord;
    NPC_ShowSprite(RIVAL_NPC_IDX);
    Music_Play(MUSIC_MEET_RIVAL);
    /* Rival walks DOWN 3 steps (x=36) or 4 steps (x=37) to meet player.
     * Mirrors SSAnne2FDefaultScript: MoveSprite before text. */
    g_walk_steps_left = (g_trigger_x == 37) ? 4 : 3;
    g_rival_state = RS_RIVAL_APPROACH;
}

int SSAnneScripts_IsActive(void) {
    return g_rival_state != RS_IDLE || g_captain_state != CS_IDLE || g_dep_state != DEP_IDLE;
}

int SSAnneScripts_IsWalkingOut(void) {
    return g_dep_state == DEP_WALK_OUT;
}

/* Called in game.c immediately after Player_Update(), before Warp_JustHappened()
 * processing.  If the player just stepped onto the VermilionDock exit warp with
 * HM01 and the ship not yet departed, cancels the warp and starts the departure
 * cutscene on the dock map.
 * Mirrors VermilionDockSSAnneLeavesScript (scripts/VermilionDock.asm). */
void SSAnneScripts_WarpCheck(void) {
    /* Departure is now triggered in SSAnneScripts_OnMapLoad on dock arrival.
     * This function is kept for the call site in game.c but does nothing. */
    (void)0;
}

int SSAnneScripts_GetPendingBattle(uint8_t *class_out, uint8_t *no_out) {
    if (!g_pending_battle) return 0;
    g_pending_battle = 0;
    *class_out = RIVAL2_CLASS;
    *no_out    = g_rival_tr_no;
    g_rival_battle_active = 1;
    return 1;
}

int SSAnneScripts_ConsumeRivalBattle(void) {
    if (!g_rival_battle_active) return 0;
    g_rival_battle_active = 0;
    return 1;
}

void SSAnneScripts_OnVictory(void) {
    SetEvent(EVENT_BEAT_SS_ANNE_RIVAL);
    /* Do NOT restore music or NPC position here — NPC_Load() runs after this
     * call and would reset Gary back to spawn.  Position restore happens in
     * RS_POST_BATTLE_TEXT on first tick (after NPC_Load has run). */
    g_post_text_shown = 0;
    g_rival_state = RS_POST_BATTLE_TEXT;
    printf("[ss_anne] rival defeated\n");
}

void SSAnneScripts_OnDefeat(void) {
    g_rival_state = RS_IDLE;
}

/* Captain A-press callback — wired into event_data.c */
void SSAnne_CaptainScript(void) {
    if (g_captain_state != CS_IDLE) return;  /* already running */
    /* _SSAnneCaptainsRoomCaptainNotSickAnymoreText — shown after HM01 given */
    if (CheckEvent(EVENT_GOT_HM01)) {
        Text_ShowASCII("CAPTAIN: Whew!\f"
                       "Now that I'm not\nsick any more, I\nguess it's time.");
        return;
    }
    g_captain_state = CS_CAPTAIN_SICK_TEXT;
    Text_ShowASCII(kCaptainSick);
}

void SSAnneScripts_Tick(void) {
    /* ---- Rival state machine ---- */
    switch (g_rival_state) {

    case RS_IDLE:
        break;

    case RS_RIVAL_APPROACH:
        /* Wait for each step, then issue the next.
         * Mirrors SSAnne2FDefaultScript MoveSprite (3 or 4 steps DOWN). */
        if (NPC_IsWalking(RIVAL_NPC_IDX)) return;
        if (g_walk_steps_left > 0) {
            NPC_DoScriptedStep(RIVAL_NPC_IDX, DIR_DOWN);
            g_walk_steps_left--;
            return;
        }
        /* Walk done — save tile pos so we can restore it after battle. */
        NPC_GetTilePos(RIVAL_NPC_IDX, &g_rival_saved_tx, &g_rival_saved_ty);
        /* Set facing (SSAnne2FSetFacingDirectionScript).
         * x=37: rival faces RIGHT, player faces LEFT (face-to-face).
         * x=36: rival faces DOWN. */
        if (g_trigger_x == 37) {
            NPC_SetFacing(RIVAL_NPC_IDX, DIR_RIGHT);
            gPlayerFacing = DIR_LEFT;
        } else {
            NPC_SetFacing(RIVAL_NPC_IDX, DIR_DOWN);
        }
        Text_ShowASCII(kRivalPreBattle);
        g_rival_state = RS_PRE_BATTLE_TEXT;
        return;

    case RS_PRE_BATTLE_TEXT:
        if (Text_IsOpen()) { Text_Update(); return; }
        /* Text dismissed — select party based on rival's starter.
         * Mirrors SSAnne2FRivalStartBattleScript wTrainerNo selection. */
        if      (wRivalStarter == STARTER2) g_rival_tr_no = 1;
        else if (wRivalStarter == STARTER3) g_rival_tr_no = 2;
        else                                g_rival_tr_no = 3;
        /* SaveEndBattleTextPointers in SSAnne2F.asm: show the short defeat
         * quote in-battle before the longer overworld follow-up text. */
        gTrainerAfterText = kRivalDefeated;
        g_pending_battle  = 1;
        g_rival_state     = RS_BATTLE_PENDING;
        return;

    case RS_BATTLE_PENDING:
        /* game.c polls GetPendingBattle() and starts the battle */
        return;

    case RS_POST_BATTLE_TEXT:
        if (!g_post_text_shown) {
            /* Set facing again before post-battle text (SSAnne2FRivalAfterBattleScript) */
            if (g_trigger_x == 37) {
                NPC_SetFacing(RIVAL_NPC_IDX, DIR_RIGHT);
                gPlayerFacing = DIR_LEFT;
            } else {
                NPC_SetFacing(RIVAL_NPC_IDX, DIR_DOWN);
            }
            Text_ShowASCII(kRivalPostBattle);
            g_post_text_shown = 1;
            return;
        }
        if (Text_IsOpen()) { Text_Update(); return; }
        /* Post-battle text done — stop music, play rival alternate start.
         * Mirrors SSAnne2FRivalAfterBattleScript: farcall Music_RivalAlternateStart. */
        Music_Play(MUSIC_MEET_RIVAL);
        /* Exit walk: x=37 → DOWN×4; x=36 → RIGHT then DOWN×4. */
        g_walk_steps_left  = 4;
        g_exit_right_pending = (g_trigger_x == 36) ? 1 : 0;
        g_rival_state = RS_WALK_AWAY;
        return;

    case RS_WALK_AWAY:
        /* Mirrors SSAnne2FRivalAfterBattleScript movement tables. */
        if (NPC_IsWalking(RIVAL_NPC_IDX)) return;
        if (g_exit_right_pending) {
            NPC_DoScriptedStep(RIVAL_NPC_IDX, DIR_RIGHT);
            g_exit_right_pending = 0;
            return;
        }
        if (g_walk_steps_left > 0) {
            NPC_DoScriptedStep(RIVAL_NPC_IDX, DIR_DOWN);
            g_walk_steps_left--;
            return;
        }
        g_rival_state = RS_CLEANUP;
        return;

    case RS_CLEANUP:
        /* Mirrors SSAnne2FRivalExitScript: HideObject, PlayDefaultMusic. */
        NPC_HideSprite(RIVAL_NPC_IDX);
        Music_Play(Music_GetMapID(wCurMap));
        g_rival_state = RS_IDLE;
        printf("[ss_anne] rival script complete\n");
        return;
    }

    /* ---- Captain state machine ---- */
    switch (g_captain_state) {

    case CS_IDLE:
        break;

    case CS_CAPTAIN_SICK_TEXT:
        if (Text_IsOpen()) { Text_Update(); return; }
        /* Rub text dismissed — mirror SSAnneCaptainsRoomRubCaptainsBackText text_asm:
         * play PKMN_HEALED jingle, wait, play default music, set rubbed event. */
        SetEvent(EVENT_RUBBED_CAPTAINS_BACK);
        Music_Play(MUSIC_PKMN_HEALED);
        g_captain_state = CS_CAPTAIN_JINGLE;
        return;

    case CS_CAPTAIN_JINGLE:
        /* Wait for PKMN_HEALED jingle to finish */
        if (Music_IsPlaying()) return;
        Music_Play(Music_GetMapID(wCurMap));
        Text_ShowASCII(kCaptainBetter);
        g_captain_state = CS_CAPTAIN_BETTER_TEXT;
        return;

    case CS_CAPTAIN_BETTER_TEXT:
        if (Text_IsOpen()) { Text_Update(); return; }
        /* Better text dismissed — give HM01, play key item jingle, show received text.
         * Mirrors SSAnneCaptainsRoomCaptainReceivedHM01Text: sound_get_key_item. */
        Inventory_Add(HM01, 1);
        SetEvent(EVENT_GOT_HM01);
        Audio_PlaySFX_GetKeyItem();
        Text_ShowASCII(kCaptainReceivedHM);
        g_captain_state = CS_CAPTAIN_RECV_TEXT;
        printf("[ss_anne] HM01 Cut received\n");
        return;

    case CS_CAPTAIN_RECV_TEXT:
        if (Text_IsOpen()) { Text_Update(); return; }
        if (Audio_IsSFXPlaying_GetKeyItem()) return;
        g_captain_state = CS_CAPTAIN_DONE;
        return;

    case CS_CAPTAIN_DONE:
        g_captain_state = CS_IDLE;
        return;
    }

    /* ---- Departure state machine ---- */
    /* DEP_ADVANCE_COL: advance scroll column counter, emit smoke on new column. */
    #define DEP_ADVANCE_COL() do {                                  \
    if (g_dep_scroll_col < SCREEN_WIDTH) {                      \
        if (++g_dep_scroll_tick >= DEP_COL_RATE) {              \
            g_dep_scroll_tick = 0;                               \
            g_dep_scroll_col++;                                  \
        }                                                        \
    }                                                            \
} while (0)

    switch (g_dep_state) {
    case DEP_IDLE:
        break;

    case DEP_WAIT1:
        if (--g_dep_timer > 0) return;
        /* Snapshot the visible ship rows from gScrollTileMap, plus a full screen-
         * width of water tiles to the right.  Both halves are part of the scrolling
         * buffer: as g_dep_scroll_col advances, ship tiles slide left AND water tiles
         * slide left behind them — matching the original's full-band BG scroll.
         * gScrollTileMap[(row+2)*SCROLL_MAP_W + (col+2)] is screen col 0..19. */
        for (int r = 0; r < DEP_SHIP_ROWS; r++) {
            for (int c = 0; c < SCREEN_WIDTH; c++)
                g_dep_ship_snap[r][c] =
                    gScrollTileMap[(DEP_SHIP_ROW + r + 2) * SCROLL_MAP_W + (c + 2)];
            for (int c = SCREEN_WIDTH; c < SCREEN_WIDTH * 2; c++)
                g_dep_ship_snap[r][c] = DEP_WATER_TILE;
        }
        g_dep_scroll_col  = 0;
        g_dep_scroll_tick = 0;
        g_dep_slow_tick   = 0;
        g_dep_prev_col    = 0;
        dep_load_smoke_tiles();
        dep_clear_smoke();
        Display_SetBandXPx(-1, 0, 0);  /* clear any stale band offset */
        /* OBP1 = 0x00: all-white smoke palette.
         * Mirrors: xor a; ldh [rOBP1], a  (VermilionDock.asm departure init). */
        Display_SetOBP1(0x00);
        /* First horn blast — matches SFX_SS_ANNE_HORN in VermilionDock.asm */
        Audio_PlaySFX_SSAnneHorn();
        g_dep_state = DEP_HORN1;
        return;

    case DEP_HORN1:
        /* Ship slides left while first horn plays.
         * g_dep_slow_tick gates advancement to 2 of every 3 frames (≈1/3 slower). */
        if (++g_dep_slow_tick >= 3) g_dep_slow_tick = 0;
        if (g_dep_slow_tick < 2) {
            DEP_ADVANCE_COL();
            if (g_dep_scroll_col != g_dep_prev_col) {
                dep_emit_smoke_puff();
                g_dep_prev_col = g_dep_scroll_col;
            }
            dep_drift_smoke();
        }
        if (Audio_IsSFXPlaying_SSAnneHorn()) return;
        g_dep_timer = 80;  /* inter-horn pause: 60 frames × 4/3 ≈ 80 */
        g_dep_state = DEP_WAIT2;
        return;

    case DEP_WAIT2:
        /* Continue sliding between horn blasts. */
        if (++g_dep_slow_tick >= 3) g_dep_slow_tick = 0;
        if (g_dep_slow_tick < 2) {
            DEP_ADVANCE_COL();
            if (g_dep_scroll_col != g_dep_prev_col) {
                dep_emit_smoke_puff();
                g_dep_prev_col = g_dep_scroll_col;
            }
            dep_drift_smoke();
        }
        if (--g_dep_timer > 0) return;
        Audio_PlaySFX_SSAnneHorn();
        g_dep_state = DEP_HORN2;
        return;

    case DEP_HORN2:
        /* Finish any remaining columns during second horn. */
        if (++g_dep_slow_tick >= 3) g_dep_slow_tick = 0;
        if (g_dep_slow_tick < 2) {
            DEP_ADVANCE_COL();
            if (g_dep_scroll_col != g_dep_prev_col) {
                dep_emit_smoke_puff();
                g_dep_prev_col = g_dep_scroll_col;
            }
            dep_drift_smoke();
        }
        if (Audio_IsSFXPlaying_SSAnneHorn()) return;
        g_dep_state = DEP_DONE;
        return;

    case DEP_DONE:
        dep_clear_smoke();
        Display_SetBandXPx(-1, 0, 0);
        Display_SetOBP1(0xE4);
        dep_erase_ship_blocks();
        gPlayerFacing    = DIR_UP;
        g_dep_walk_steps = 3;      /* 2 on dock + warp + 1 in Vermilion */
        g_dep_state      = DEP_WALK_OUT;
        return;

    case DEP_WALK_OUT:
        /* Drive the player north, step by step.
         * Player_Update is called by game.c when IsWalkingOut() returns 1.
         * Steps span the dock→Vermilion warp naturally: dock (14,2)→(14,1)→(14,0)
         * triggers the exit warp; the final step finishes in Vermilion at (18,30). */
        if (Player_IsMoving()) return;  /* wait for current step to animate */
        if (g_dep_walk_steps > 0) {
            g_dep_walk_steps--;
            Player_DoScriptedStep(DIR_UP);
        } else {
            g_dep_state = DEP_IDLE;
        }
        return;
    }
}

/* ============================================================
 * VERMILION DOCK GUARD — SS Ticket step check
 * Ports VermilionCityDefaultScript from scripts/VermilionCity.asm.
 *
 * Trigger: player steps to (18,30) facing DOWN on MAP_VERMILION_CITY.
 * SSAnneTicketCheckCoords: dbmapcoord 18, 30
 *
 * Logic (mirrors ASM):
 *   1. EVENT_SS_ANNE_LEFT set → "The ship set sail." → push player back north.
 *   2. Player has S_S_TICKET → show greeting + flash text → allow through.
 *   3. No ticket → show greeting + denial text → push player back north.
 * ============================================================ */

static int s_dock_push_pending = 0;
static char kDockFlashedText[192];
static char kDockNoTicketText[256];

static void build_dock_texts(void) {
    char player_ascii[12] = "RED";
    int i;
    for (i = 0; i < 11; i++) {
        uint8_t c = wPlayerName[i];
        if (c == 0x50) break;
        if      (c >= 0x80 && c <= 0x99) player_ascii[i] = (char)('A' + c - 0x80);
        else if (c >= 0xA0 && c <= 0xB9) player_ascii[i] = (char)('a' + c - 0xA0);
        else if (c >= 0xF6 && c <= 0xFF) player_ascii[i] = (char)('0' + c - 0xF6);
        else                             player_ascii[i] = '?';
        player_ascii[i + 1] = '\0';
    }

    /* _VermilionCitySailor1DoYouHaveATicketText +
     * _VermilionCitySailor1FlashedTicketText (verbatim from text/VermilionCity.asm) */
    snprintf(kDockFlashedText, sizeof(kDockFlashedText),
        "Welcome to S.S.\nANNE!\f"
        "Excuse me, do you\nhave a ticket?\f"
        "%s flashed\nthe S.S.TICKET!\f"
        "Great! Welcome to\nS.S.ANNE!",
        player_ascii);

    /* _VermilionCitySailor1DoYouHaveATicketText +
     * _VermilionCitySailor1YouNeedATicketText (verbatim from text/VermilionCity.asm) */
    snprintf(kDockNoTicketText, sizeof(kDockNoTicketText),
        "Welcome to S.S.\nANNE!\f"
        "Excuse me, do you\nhave a ticket?\f"
        "%s doesn't\nhave the needed\nS.S.TICKET.\f"
        "Sorry!\f"
        "You need a ticket\nto get aboard.",
        player_ascii);
}

void VermilionScripts_StepCheck(void) {
    if (wCurMap != MAP_VERMILION_CITY) return;
    if (s_dock_push_pending) return;
    if ((gPlayerFacing & 3) != DIR_DOWN) return;
    if (wXCoord != 18 || wYCoord != 30) return;

    if (CheckEvent(EVENT_SS_ANNE_LEFT)) {
        /* _VermilionCitySailor1ShipSetSailText */
        Text_ShowASCII("The ship set sail.");
        s_dock_push_pending = 1;
        return;
    }

    build_dock_texts();

    if (Inventory_GetQty(ITEM_SS_TICKET) > 0) {
        /* Player has ticket — flash text, allow through (no push) */
        Text_ShowASCII(kDockFlashedText);
        return;
    }

    /* No ticket — show denial and push back north */
    Text_ShowASCII(kDockNoTicketText);
    s_dock_push_pending = 1;
}

/* Called after Map_BuildScrollView() during departure.
 * Overwrites the ship-band rows of gScrollTileMap with water tile $14, matching
 * the original's "hlcoord 0,10 / SCREEN_WIDTH*6 / $14 / FillMemory" that fills
 * BGMap0 with water before the SCX scroll starts.  The window layer then paints
 * the animated ship on top; transparent positions reveal this BG water. */
void SSAnneScripts_PostBuildScrollView(void) {
    /* Skip during IDLE/WAIT1: departure not yet active.
     * From HORN1 onwards, overwrite the ship band in gScrollTileMap directly.
     * For each scroll-map column bx, the corresponding snap column is:
     *   snap_col = (bx - 2) + g_dep_scroll_col
     * (bx=2 is screen col 0; border tiles at bx=0,1 are off-screen).
     * In-range snap tiles show the departing ship; out-of-range → water.
     * All tiles are in gScrollTileMap, so they receive the same gScrollPxX
     * sub-pixel offset as surrounding water — no window/BG seam.
     *
     * Per-band X offset: g_dep_scroll_tick (0..TILE_PX-1) is used as a
     * negative pixel shift for departure rows, giving smooth 1-px-per-frame
     * scrolling that mirrors the original's SCX raster trick.  Negative
     * means the band shifts LEFT (ship departs left); border tiles in the
     * scroll buffer cover the sub-tile gap on both edges. */
    if (g_dep_state == DEP_IDLE || g_dep_state == DEP_WAIT1) return;

    if (g_dep_state == DEP_WALK_OUT) {
        /* Gangplank (by=1, bx=5..8) world tiles: ty=4..7, tx=20..35.
         * Hull     (by=2, bx=5..8) world tiles: ty=8..11, tx=20..35.
         * Both rows are visible as the camera pans north during walk-out.
         * Write DEP_WATER_TILE directly so Map_BuildScrollView's output
         * (which may still carry ship tile IDs) is overridden every frame. */
        for (int ty = 4; ty <= 11; ty++) {
            int sy = ty - (gCamY - 2);
            if (sy < 0 || sy >= SCROLL_MAP_H) continue;
            for (int tx2 = 20; tx2 <= 35; tx2++) {
                int sx = tx2 - (gCamX - 2);
                if (sx < 0 || sx >= SCROLL_MAP_W) continue;
                gScrollTileMap[sy * SCROLL_MAP_W + sx] = DEP_WATER_TILE;
            }
        }
        return;
    }

    /* Animation phases (HORN1, WAIT2, HORN2, DEP_DONE):
     * DEP_DONE is NOT skipped — snap buffer (all-water at col=SCREEN_WIDTH)
     * covers the one-frame gap before DEP_WALK_OUT takes over. */
    for (int r = 0; r < DEP_SHIP_ROWS; r++) {
        for (int bx = 0; bx < SCROLL_MAP_W; bx++) {
            int snap_col = (bx - 2) + g_dep_scroll_col;
            uint8_t t = (snap_col >= 0 && snap_col < SCREEN_WIDTH * 2)
                        ? g_dep_ship_snap[r][snap_col]
                        : DEP_WATER_TILE;
            gScrollTileMap[(DEP_SHIP_ROW + r + 2) * SCROLL_MAP_W + bx] = t;
        }
    }
    Display_SetBandXPx(DEP_SHIP_ROW, DEP_SHIP_ROWS, -g_dep_scroll_tick);
}

void VermilionScripts_DoPush(void) {
    if (!s_dock_push_pending) return;
    if (Text_IsOpen()) return;
    s_dock_push_pending = 0;
    Player_DoScriptedStep(DIR_UP);
}
