/* battle_ui.c — Non-blocking battle scene state machine.
 *
 * Drives wild / trainer battles one frame at a time from GameTick.
 * gScene == SCENE_BATTLE routes every tick here.
 *
 * Key invariant: BattleUI_Tick() is NOT called while Text_IsOpen().
 * This lets us use "show text then set next state" transitions —
 * the text box stays up until A is pressed, then the next Tick runs
 * in whatever state we pre-set.
 *
 * Turn execution flow (mirrors MainInBattleLoop / core.asm:280):
 *   BUI_MOVE_SELECT  → player presses A → Battle_TurnPrepare
 *                    → execute FIRST mover's move → show "[X] used [Y]!"
 *   BUI_EXEC_MOVE_B  → (after A dismissed) check faint/escape
 *                    → execute SECOND mover's move → show "[X] used [Y]!"
 *   BUI_TURN_END     → (after A dismissed) check faint/escape → loop
 *
 * Screen layout (matches original pokered tile coordinates exactly):
 *   Rows  0– 3 : enemy HUD   cols 0-11  (name r0, level r1, HP bar r2)
 *   Rows  0– 6 : enemy sprite cols 11-17 (placeholder — no sprite yet)
 *   Rows  7–11 : player sprite cols 0-8  (placeholder — no sprite yet)
 *   Rows  7–11 : player HUD  cols 9-19  (name r7, level r8, HP bar r9, nums r10)
 *   Rows 12–17 : dialog box  full-width (text) / right-half (battle menu)
 */

#include "battle_ui.h"
#include "battle_init.h"
#include "battle_loop.h"
#include "battle_core.h"
#include "battle_exp.h"
#include "battle_switch.h"
#include "battle_trainer.h"
#include "battle_catch.h"
#include "battle_items.h"
#include "../party_menu.h"
#include "../bag_menu.h"
#include "../inventory.h"
#include "../../platform/hardware.h"
#include "../../platform/display.h"
#include "../../platform/audio.h"
#include "../music.h"
#include "../../data/font_data.h"
#include "../../data/moves_data.h"
#include "../../data/base_stats.h"
#include "../../data/event_constants.h"
#include "../../data/pokemon_sprites.h"
#include "../../data/trainer_sprites.h"
#include "../constants.h"
#include "../text.h"
#include "../pokedex.h"
#include "../pokemon.h"
#include "../overworld.h"   /* gScrollTileMap, SCROLL_MAP_W */
#include "../trainer_sight.h" /* gEngagedTrainerClass */
#include "../player.h"      /* gScrollPxX, gScrollPxY */
#include <stdio.h>
#include <string.h>

/* HP bar and HUD tiles are loaded from gHudTiles ($62–$78) via Font_LoadHudTiles()
 * called in BattleUI_Enter. Tile slots are dedicated (HUD_TILE_BASE + $62 offset),
 * so no save/restore is needed around the battle. */

/* ---- State --------------------------------------------------------- */

typedef enum {
    BUI_INACTIVE      = 0,
    BUI_SLIDE_IN,       /* enemy + player silhouettes slide from right (72 frames) */
    BUI_APPEARED,       /* "Wild X appeared!" + pokeball party display  */
    BUI_SEND_OUT,          /* trigger: start slide-outs                        */
    BUI_ENEMY_SLIDE_OUT,   /* trainer sprite slides right off enemy side       */
    BUI_TRAINER_SLIDE_OUT, /* Red's OAM slides left off screen (4px/tick)      */
    BUI_ENEMY_SEND_OUT,    /* trainer throws pokeball → enemy grows in         */
    BUI_POKEMON_APPEAR, /* pokeball → 3×3 → 5×5 → full 7×7 grow animation  */
    BUI_INTRO,          /* (legacy placeholder — skipped via BUI_SLIDE_IN entry) */
    BUI_DRAW_HUD,       /* refresh HUD + draw main menu             */
    BUI_MENU,           /* FIGHT / RUN input                        */
    BUI_MOVE_SELECT,    /* move list input                          */
    BUI_MOVE_ANIM,      /* multi-frame shake/blink after move exec  */
    BUI_HP_ANIM,        /* animated HP bar scroll (1px/2f)          */
    BUI_EXEC_MOVE_B,    /* after first move text: check + exec 2nd  */
    BUI_EXEC_SECOND,    /* after residual-A message: exec 2nd move  */
    BUI_TURN_END,       /* after second move text: check + loop     */
    BUI_TURN_FINISH,    /* after residual-B message: cleanup + loop */
    BUI_EXP_DRAIN,      /* drain exp/level-up texts after faint     */
    BUI_TRAINER_VICTORY_SLIDE, /* trainer sprite slides in from right (56px, 4px/tick) */
    BUI_TRAINER_VICTORY_PAUSE, /* 40-frame hold after slide completes                  */
    BUI_TRAINER_VICTORY_TEXT,  /* defeat quote dismissed: wait for SFX                 */
    BUI_GYM_BADGE_JINGLE,      /* wait for key-item jingle, then show badge info text  */
    BUI_WAIT_CRY,              /* WaitForSoundToFinish for a cry before continuing     */
    BUI_WAIT_SOUND,            /* WaitForSoundToFinish then show money text            */
    BUI_FADE_WHITE,            /* white palette hold ~30f then BUI_END                 */
    BUI_LEVELUP_STATS,  /* wait for A on the level-up stats box     */
    BUI_ENEMY_FAINT_ANIM, /* enemy sprite slides down off-screen (14f) */
    BUI_PLAYER_FAINTED, /* player mon fainted: checks + open menu   */
    BUI_USE_NEXT_MON,   /* wild only: "Use next Pokemon?" YES/NO    */
    BUI_PARTY_SELECT,   /* party menu: choose next mon              */
    BUI_SWITCH_SELECT,  /* voluntary: party menu for mid-turn switch */
    BUI_RETREAT_ANIM,   /* voluntary: player sprite shrink (7×7→3×3) */
    BUI_SWITCH_ENEMY_TURN, /* voluntary: enemy executes their pre-selected move */
    BUI_BAG_BATTLE,     /* bag menu open during battle                       */
    BUI_ITEM_TARGET,    /* party menu: choose which mon to use item on       */
    BUI_BALL_THROW,     /* pokéball arc animation (player → enemy)           */
    BUI_BALL_POOF,      /* pokéball poof cloud (24f, Subanim_0BallPoofEnemy) */
    BUI_BALL_SHAKE,     /* pokéball shake animation (0–3 rocks)              */
    BUI_CAUGHT,         /* add caught mon to party / box, maybe show dex     */
    BUI_CAUGHT_DEX_WAIT,/* wait through "new dex data" text + dex page       */
    BUI_CAUGHT_BOX_TEXT,/* wait for post-dex PC transfer text                */
    BUI_EVOLUTION,      /* post-battle evolution screen (flicker + B-cancel) */
    BUI_END,            /* battle over                              */
} bui_state_t;

static bui_state_t bui_state        = BUI_INACTIVE;
static int         bui_cursor       = 0;
static int         s_slide_cx       = 0;  /* BUI_SLIDE_IN: current X offset (144→0, step 2) */

/* Turn execution state */
static int  s_player_first;   /* 1 = player moves first this turn */
static char s_name_a[20];     /* first mover's mon name  */
static char s_pfx_a[8];       /* "Wild " or ""           */
static char s_name_b[20];     /* second mover's mon name */
static char s_pfx_b[8];

/* Shared message buffer — never modified while Text_IsOpen() */
static char s_msg_buf[384];

/* Item selected from bag that requires a party-slot target (medicine, revive, etc.) */
static uint8_t s_pending_item = 0;

/* BUI_EXP_DRAIN destination and optional suffix messages (2 pages, like original) */
static bui_state_t   bui_exp_dest    = BUI_END;
static char          s_exp_suffix[64];
static char          s_exp_suffix2[64];  /* second page shown after s_exp_suffix */

/* BUI_ENEMY_FAINT_ANIM state
 * Mirrors SlideDownFaintedMonPic (engine/battle/core.asm:1181):
 *   7 steps × 2 frames = 14 frames.  Each step shifts the sprite's OAM
 *   rows down by 8px and hides the bottommost row (it scrolled off the
 *   sprite area in the original BG-tile implementation). */
static int s_faint_step  = 0;  /* 0=inactive, 1-7=animation progress */
static int s_faint_timer = 0;
/* Pending level-up stats box — drawn after the "grew to level N!" text closes */
static levelup_stats_t s_pending_lvl_stats;

/* Trainer victory sequence (BUI_TRAINER_VICTORY_SLIDE/PAUSE/TEXT/FADE_WHITE) */
static int         s_victory_timer      = 0;
static char        s_trainer_money_text[80];
/* Gym-leader badge receipt sequence:
 *   s_badge_recv_text  — "[PLAYER] received the BOULDERBADGE!" (page shown while jingle plays)
 *   s_badge_info_text  — badge info pages shown after received text is dismissed */
static const char *s_badge_recv_text    = NULL;
static const char *s_badge_info_text    = NULL;
static bui_state_t s_wait_cry_next_state = BUI_END;
static char        s_wait_cry_text[64];

void BattleUI_SetBadgeRecvText(const char *text) { s_badge_recv_text = text; }
void BattleUI_SetBadgeInfoText(const char *text) { s_badge_info_text = text; }

/* Pokemon appear (grow) animation state — AnimateSendingOutMon equivalent.
 * stage 0 = pokeball OAM at center; 1 = 3×3 crop; 2 = 5×5 crop; 3 → BUI_DRAW_HUD */
static int s_grow_stage;
static int s_grow_frame;
/* 1 = grow animation was triggered by a voluntary mid-turn switch (→ BUI_SWITCH_ENEMY_TURN) */
static int s_grow_after_switch;
/* 1 = BUI_ENEMY_SEND_OUT entered from a mid-battle trainer replacement (not battle intro) */
static int s_mid_battle_send;

/* Retreat animation state (AnimateRetreatingPlayerMon equivalent).
 * stage 0 = full 7×7 (4f); 1 = 5×5 crop (4f); 2 = 3×3 crop (4f); 3 = done */
static int     s_retreat_stage;
static int     s_retreat_frame;
static uint8_t s_retreat_species;  /* old mon species for sprite tile lookup */
static uint8_t s_switch_slot;      /* party slot chosen for voluntary switch */
static int     s_struggle_pending; /* 1 = all PP exhausted, execute Struggle after text */

/* BUI_EVOLUTION state (EvolveMon equivalent) */
static uint8_t s_evo_slot;         /* party slot of the evolving mon */
static uint8_t s_evo_old_species;  /* species before evolution */
static uint8_t s_evo_new_species;  /* species after evolution */
static int     s_evo_phase;        /* 0=entry text, 1=init flicker, 2=flicker, 3=result text, 4=apply */
static int     s_evo_timer;        /* ticks until next flicker swap */
static int     s_evo_blink;        /* 0=old sprite showing, 1=new sprite showing */
static int     s_evo_wave;         /* ASM b-counter surrogate: 1..8 */
static int     s_evo_wave_units;   /* remaining back-and-forth units in current wave */
static int     s_evo_cancelled;    /* 1 if B was pressed during flicker */
static int     s_evo_screen_white; /* 1 if BUI_FADE_WHITE already ran (screen clear + white) */

/* ---- Ball-throw / catch animation state (BUI_BAG_BATTLE → BUI_BALL_THROW → *
 *       BUI_BALL_SHAKE → BUI_CAUGHT)                                          *
 *                                                                             *
 * Animation data sourced directly from pokered-master:                        *
 *                                                                             *
 * THROW ARC — Subanim_0BallTossHigh (subanimations.asm:139)                  *
 *   11 frame-block entries; each entry shown for 3 frames                    *
 *   (BallTossAnim: battle_anim NO_MOVE, SUBANIM_0_BALL_TOSS_HIGH, 0, 3).    *
 *   Waypoints = BASECOORD_30,A2,93,61,73,A7,33,A8,0E,A9,34 (base_coords.asm)*
 *   OAM coords are raw (Y = screen_Y+16, X = screen_X+8).                   *
 *                                                                             *
 * SPRITE — FrameBlock03 (frame_blocks.asm:158): 2×2 tile block (16×16px).   *
 *   TL/TR: tile 2 of move_anim_0, TR is X-flipped.                          *
 *   BL/BR: tile 18 of move_anim_0, BR is X-flipped.                         *
 *   4 OAM entries at THROW_BALL_OAM_BASE (slots 0-3) — must have LOWER OAM  *
 *   index than enemy (4-52) so ball renders on top (lower index = higher    *
 *   visual priority in display.c).  Original game had ball at OAM[0] with  *
 *   enemy on BG tiles; we replicate priority by keeping ball at index 0.   *
 *                                                                             *
 * SHAKE — Subanim_0BallShakeEnemy (subanimations.asm:153)                   *
 *   4 frame-block entries at BASECOORD_21; each shown for 4 frames           *
 *   (BallShakeAnim: battle_anim NO_MOVE, SUBANIM_0_BALL_SHAKE_ENEMY, 0, 4). *
 *   FrameBlock03 = neutral, FrameBlock04 = tilt-right, FrameBlock05 = left.  *
 *   DoBallShakeSpecialEffects (animations.asm:739):                          *
 *     fires at start of each shake → SFX_TINK + DelayFrames 40.             *
 *   Per-shake total: 40 pause + 4×4 visual = 56 frames.                     *
 *                                                                             *
 * SHAKE POSITION — BASECOORD_21 (base_coords.asm:36):                       *
 *   OAM Y = $38 = 56, OAM X = $78 = 120.                                   */

/* 4-OAM ball sprite block: OAM[0..3] — must be lowest index to beat enemy priority */
#define THROW_BALL_OAM_BASE  0
/* Shake rest position (BASECOORD_21, base_coords.asm:36) */
#define BALL_SHAKE_OAM_Y     56   /* OAM Y = $38 */
#define BALL_SHAKE_OAM_X     120  /* OAM X = $78 */
/* Throw timing from BallTossAnim: 3 frames per waypoint */
#define THROW_FPW            3    /* frames per waypoint */
/* Shake timing from DoBallShakeSpecialEffects (animations.asm:739):
 *   40-frame pause (SFX_TINK + DelayFrames 40) + 4 entries × 4 frames = 56 */
#define SHAKE_DELAY          40
#define SHAKE_CYCLE          56   /* frames per shake (40+16) */

/* Sprite tile IDs loaded into sprite_tile_gfx for the ball animation.
 * Sourced from gfx/battle/move_anim_0.png (tiles 0-based):
 *   tile 2  → top half (FrameBlock03/throw + neutral)
 *   tile 18 → bottom half
 *   tiles 6,7,22,23 → tilted-right (FrameBlock04)
 *   tiles 7,6,23,22 + XFLIP → tilted-left (FrameBlock05) */
#define BALL_TILE_NEUT_TOP   114  /* move_anim_0 tile #2  */
#define BALL_TILE_NEUT_BOT   115  /* move_anim_0 tile #18 */
#define BALL_TILE_TILT_TL    116  /* move_anim_0 tile #6  */
#define BALL_TILE_TILT_TR    117  /* move_anim_0 tile #7  */
#define BALL_TILE_TILT_BL    118  /* move_anim_0 tile #22 */
#define BALL_TILE_TILT_BR    119  /* move_anim_0 tile #23 */

/* move_anim_0 tile data — extracted from gfx/battle/move_anim_0.png (128×40, 16 tiles/row) */
static const uint8_t kBallTileNeutTop[16] = /* tile 2  */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x03,0x0c,0x0f,0x11,0x1e,0x11,0x1e};
static const uint8_t kBallTileNeutBot[16] = /* tile 18 */
    {0x20,0x3f,0x20,0x3f,0x38,0x27,0x3f,0x20,0x1f,0x10,0x1f,0x10,0x0f,0x0c,0x03,0x03};
static const uint8_t kBallTileTiltTL[16] = /* tile 6  */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0x07,0x18,0x1f,0x23,0x3c,0x23,0x3c};
static const uint8_t kBallTileTiltTR[16] = /* tile 7  */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x80,0x60,0xe0,0x10,0xf0,0x10,0xf0};
static const uint8_t kBallTileTiltBL[16] = /* tile 22 */
    {0x40,0x7f,0x40,0x7f,0x40,0x7f,0x41,0x7e,0x3f,0x20,0x3f,0x20,0x1f,0x18,0x07,0x07};
static const uint8_t kBallTileTiltBR[16] = /* tile 23 */
    {0x08,0xf8,0x18,0xe8,0x38,0xc8,0xf8,0x08,0xf0,0x10,0xf0,0x10,0xe0,0x60,0x80,0x80};

/* Poof cloud tile data — Subanim_0BallPoofEnemy (subanimations.asm:160).
 * FrameBlocks 06-0A use tiles $20-$25, $30-$34 of move_anim_0.
 * Sprite tile slots POOF_TILE_BASE+0..+9 reuse the player-slide sprite tiles,
 * and the OAM entries reuse the player-slide OAM block once the intro is over. */
#define POOF_TILE_BASE    53   /* sprite_tile_gfx slots 53-62 */
#define POOF_OAM_BASE     4    /* reuse player-slide OAM slots 4-19 (max 16 per frame) */
#define POOF_OAM_COUNT    16
/* tile index helpers: POOF_TILE(anim_id) maps move_anim tile $20-$34 → slot */
#define POOF_T20  (POOF_TILE_BASE + 0)
#define POOF_T21  (POOF_TILE_BASE + 1)
#define POOF_T23  (POOF_TILE_BASE + 2)
#define POOF_T24  (POOF_TILE_BASE + 3)
#define POOF_T25  (POOF_TILE_BASE + 4)
#define POOF_T30  (POOF_TILE_BASE + 5)
#define POOF_T31  (POOF_TILE_BASE + 6)
#define POOF_T32  (POOF_TILE_BASE + 7)
#define POOF_T33  (POOF_TILE_BASE + 8)
#define POOF_T34  (POOF_TILE_BASE + 9)

static const uint8_t kPoofTile20[16] = /* $20 */
    {0x00,0x00,0x00,0x01,0x01,0x02,0x01,0x02,0x03,0x1C,0x1D,0x22,0x1F,0x20,0x3E,0x41};
static const uint8_t kPoofTile21[16] = /* $21 */
    {0x00,0x00,0x00,0xF0,0xF0,0x08,0xF8,0x07,0xFD,0x02,0xF0,0x0F,0xE0,0x1E,0x40,0xBC};
static const uint8_t kPoofTile23[16] = /* $23 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x06,0x39,0x3F,0x40};
static const uint8_t kPoofTile24[16] = /* $24 */
    {0x00,0x00,0x00,0x01,0x01,0x06,0x07,0x08,0x0E,0x11,0x1E,0x21,0x1C,0x22,0x30,0x4C};
static const uint8_t kPoofTile25[16] = /* $25 */
    {0x00,0x70,0x70,0x8C,0xF0,0x0E,0x80,0x7C,0x00,0xC0,0x00,0x80,0x00,0x00,0x00,0x00};
static const uint8_t kPoofTile30[16] = /* $30 */
    {0x3E,0x41,0x1C,0x63,0x00,0x7F,0x00,0x7F,0x00,0x3F,0x00,0x1F,0x00,0x1F,0x00,0x1C};
static const uint8_t kPoofTile31[16] = /* $31 */
    {0x00,0xFC,0x00,0xFC,0x00,0xF8,0x00,0xE0,0x00,0xE0,0x00,0xC0,0x00,0x00,0x00,0x00};
static const uint8_t kPoofTile32[16] = /* $32 */
    {0x00,0x00,0x00,0x01,0x01,0x02,0x01,0x02,0x01,0x02,0x03,0x04,0x03,0x04,0x01,0x02};
static const uint8_t kPoofTile33[16] = /* $33 */
    {0x7F,0x80,0xFE,0x01,0xF8,0x07,0xF8,0x07,0xF0,0x0E,0xC0,0x3E,0xC0,0x3C,0x80,0x70};
static const uint8_t kPoofTile34[16] = /* $34 */
    {0x30,0x4C,0x60,0x98,0x60,0x98,0x60,0x90,0x00,0x70,0x00,0x60,0x00,0x20,0x00,0x00};

static uint8_t        s_ball_item     = 0;
static catch_result_t s_catch_result  = CATCH_RESULT_0_SHAKES;
static int            s_throw_frame   = 0;  /* counts 0..11*THROW_FPW-1 */
static int            s_shake_total   = 0;  /* 0-3 shakes */
static int            s_shake_frame   = 0;
static int            s_poof_frame    = 0;  /* counts 0..23 (6 entries × 4 frames) */
static uint8_t        s_caught_species = 0;
static uint8_t        s_caught_dex = 0;
static int            s_caught_new_entry = 0;
static int            s_caught_sent_to_box = 0;
static int            s_caught_dex_started = 0;

/* Move animation state — PlayApplyingAttackAnimation equivalent.
 * type: 0=none  1=vert-shake(b=8)  2=horiz-heavy(b=8)  4=blink-enemy  5=horiz-light(b=2)
 * s_anim_first: 1=finishing first move, 0=finishing second move */
static int s_anim_type;
static int s_anim_frame;
static int s_anim_total;
static int s_anim_first;

/* HP bar scroll animation state — UpdateHPBar2 equivalent.
 * s_hp_anim_who: 0=enemy bar animated, 1=player bar animated
 * s_hp_pre_hp/max: target HP snapshot taken BEFORE Battle_Execute*Move()
 * s_hp_cur_px: current displayed pixel count (animates toward s_hp_new_px)
 * s_hp_half_frame: sub-frame counter — advance 1px every 2 frames */
static int      s_hp_anim_who;
static int      s_hp_old_px;
static int      s_hp_new_px;
static int      s_hp_cur_px;
static int      s_hp_half_frame;
static uint16_t s_hp_pre_hp;
static uint16_t s_hp_pre_max;

/* Drain-HP text shown as a separate dialog after the move text closes.
 * Mirrors drain_hp.asm: the PrintText call fires AFTER UpdateHPBar2 +
 * DrawPlayerHUDAndHPBar + DrawEnemyHUDAndHPBar, i.e. after the move dialog.
 * Set by bui_show_after_move; consumed and cleared by BUI_EXEC_MOVE_B /
 * BUI_TURN_END before they do any other processing. */
static char s_drain_text[128];

/* Pre-move snapshot: capture target/attacker state before each Execute*Move()
 * so bui_show_after_move can detect newly applied status/stat conditions. */
typedef struct {
    uint8_t target_status;
    uint8_t target_bstat1;
    uint8_t target_stat_mods[6];
    uint8_t attacker_stat_mods[6];
} pre_move_snap_t;
static pre_move_snap_t s_pre;

/* ---- Tile helpers -------------------------------------------------- */

static int bui_char_to_tile(unsigned char c) {
    if (c >= 'A' && c <= 'Z') return Font_CharToTile(0x80 + (c - 'A'));
    if (c >= 'a' && c <= 'z') return Font_CharToTile(0xA0 + (c - 'a'));
    if (c == ' ')              return BLANK_TILE_SLOT;
    if (c >= '0' && c <= '9') return Font_CharToTile(0xF6 + (c - '0'));
    if (c == '!')              return Font_CharToTile(0xE7);
    if (c == '?')              return Font_CharToTile(0xE6);
    if (c == '.')              return Font_CharToTile(0xE8);
    if (c == ',')              return Font_CharToTile(0xF4);
    if (c == '-')              return Font_CharToTile(0xE3);
    if (c == '\'')             return Font_CharToTile(0xE0);
    if (c == ':')              return Font_CharToTile(0x9C);
    if (c == '/')              return Font_CharToTile(0xF3);
    if (c == '>')              return Font_CharToTile(0xED); /* ▶ solid arrow (cursor) */
    if (c == '#')              return Font_CharToTile(0xE0);
    return BLANK_TILE_SLOT;
}

static void bui_place_player_sprite(void);  /* forward decl — defined after bui_load_sprites */
static void bui_ball_load_poof_tiles(void); /* forward decl — defined after poof data */

/* Decode a pokéred-encoded name (0x50-terminated) to null-terminated ASCII.
 * Mirrors the decode table in inventory.c:Inventory_DecodeASCII. */
static void bui_decode_poke_str(const uint8_t *src, char *dst, int max) {
    int out = 0;
    for (int i = 0; out < max - 1 && src[i] != 0x50; i++) {
        uint8_t c = src[i];
        if      (c >= 0x80 && c <= 0x99) dst[out++] = (char)('A' + (c - 0x80));
        else if (c >= 0xA0 && c <= 0xB9) dst[out++] = (char)('a' + (c - 0xA0));
        else if (c == 0x7F)               dst[out++] = ' ';
        else if (c >= 0xF6)               dst[out++] = (char)('0' + (c - 0xF6));
        else if (c == 0xE8)               dst[out++] = '.';
        else if (c == 0xE3)               dst[out++] = '-';
        else                              dst[out++] = '?';
    }
    dst[out] = '\0';
}

/* Returns 1 if item_id is a pokéball (ITEM_MASTER/ULTRA/GREAT/POKE/SAFARI_BALL). */
static int bui_is_ball(uint8_t id) {
    return id == ITEM_MASTER_BALL || id == ITEM_ULTRA_BALL ||
           id == ITEM_GREAT_BALL  || id == ITEM_POKE_BALL  ||
           id == ITEM_SAFARI_BALL;
}

/* Medicine and revive items require the player to select a party slot target. */
static int item_needs_target(uint8_t id) {
    switch (id) {
    case ITEM_ANTIDOTE:    case ITEM_BURN_HEAL:   case ITEM_ICE_HEAL:
    case ITEM_AWAKENING:   case ITEM_PARLYZ_HEAL: case ITEM_FULL_HEAL:
    case ITEM_FULL_RESTORE:case ITEM_MAX_POTION:  case ITEM_HYPER_POTION:
    case ITEM_SUPER_POTION:case ITEM_POTION:      case ITEM_FRESH_WATER:
    case ITEM_SODA_POP:    case ITEM_LEMONADE:
    case ITEM_REVIVE:      case ITEM_MAX_REVIVE:
        return 1;
    default:
        return 0;
    }
}

static void bui_clear_rows(int r0, int r1);
static void bui_draw_enemy_hud(void);
static void bui_draw_player_hud(void);
static void bui_load_sprites(void);
static void bui_hide_pokeballs(void);
static void bui_set_enemy_oam_visible(int visible);
static void bui_draw_box(void);

static int bui_pokedex_owned_num(uint8_t dex_num) {
    int bit;
    if (dex_num < 1 || dex_num > 151) return 0;
    bit = dex_num - 1;
    return (wPokedexOwned[bit >> 3] >> (bit & 7)) & 1;
}

static int bui_current_box_is_full(void) {
    uint8_t box = (uint8_t)(wCurrentBoxNum % NUM_BOXES);
    return wBoxCount[box] >= BOX_CAPACITY;
}

static void bui_restore_catch_screen_after_dex(void) {
    bui_clear_rows(0, SCREEN_HEIGHT - 1);
    bui_draw_enemy_hud();
    bui_draw_player_hud();
    bui_load_sprites();
    bui_hide_pokeballs();
    bui_set_enemy_oam_visible(0);
    bui_draw_box();
    Display_SetPalette(0xE4, 0xE4, 0xE4);
}

/* Load the 6 ball throw/shake tiles + 10 poof cloud tiles into sprite_tile_gfx.
 * Called once when BUI_BAG_BATTLE transitions to BUI_BALL_THROW. */
static void bui_ball_load_tiles(void) {
    Display_LoadSpriteTile(BALL_TILE_NEUT_TOP, kBallTileNeutTop);
    Display_LoadSpriteTile(BALL_TILE_NEUT_BOT, kBallTileNeutBot);
    Display_LoadSpriteTile(BALL_TILE_TILT_TL,  kBallTileTiltTL);
    Display_LoadSpriteTile(BALL_TILE_TILT_TR,  kBallTileTiltTR);
    Display_LoadSpriteTile(BALL_TILE_TILT_BL,  kBallTileTiltBL);
    Display_LoadSpriteTile(BALL_TILE_TILT_BR,  kBallTileTiltBR);
    bui_ball_load_poof_tiles();
}

/* Set the 4-OAM ball sprite (FrameBlock03/04/05) at OAM base position (baseY, baseX).
 * fb: 0 = neutral (FrameBlock03), 1 = tilt-right (FrameBlock04), 2 = tilt-left (FrameBlock05) */
static void bui_ball_set_oam(uint8_t baseY, uint8_t baseX, int fb) {
    int b = THROW_BALL_OAM_BASE;
    switch (fb) {
    case 0: /* FrameBlock03: neutral / throw */
        wShadowOAM[b+0].y = baseY;          wShadowOAM[b+0].x = baseX;
        wShadowOAM[b+0].tile = BALL_TILE_NEUT_TOP; wShadowOAM[b+0].flags = 0;
        wShadowOAM[b+1].y = baseY;          wShadowOAM[b+1].x = (uint8_t)(baseX + 8);
        wShadowOAM[b+1].tile = BALL_TILE_NEUT_TOP; wShadowOAM[b+1].flags = OAM_FLAG_FLIP_X;
        wShadowOAM[b+2].y = (uint8_t)(baseY + 8); wShadowOAM[b+2].x = baseX;
        wShadowOAM[b+2].tile = BALL_TILE_NEUT_BOT; wShadowOAM[b+2].flags = 0;
        wShadowOAM[b+3].y = (uint8_t)(baseY + 8); wShadowOAM[b+3].x = (uint8_t)(baseX + 8);
        wShadowOAM[b+3].tile = BALL_TILE_NEUT_BOT; wShadowOAM[b+3].flags = OAM_FLAG_FLIP_X;
        break;
    case 1: /* FrameBlock04: tilt right */
        wShadowOAM[b+0].y = baseY;          wShadowOAM[b+0].x = baseX;
        wShadowOAM[b+0].tile = BALL_TILE_TILT_TL;  wShadowOAM[b+0].flags = 0;
        wShadowOAM[b+1].y = baseY;          wShadowOAM[b+1].x = (uint8_t)(baseX + 8);
        wShadowOAM[b+1].tile = BALL_TILE_TILT_TR;  wShadowOAM[b+1].flags = 0;
        wShadowOAM[b+2].y = (uint8_t)(baseY + 8); wShadowOAM[b+2].x = baseX;
        wShadowOAM[b+2].tile = BALL_TILE_TILT_BL;  wShadowOAM[b+2].flags = 0;
        wShadowOAM[b+3].y = (uint8_t)(baseY + 8); wShadowOAM[b+3].x = (uint8_t)(baseX + 8);
        wShadowOAM[b+3].tile = BALL_TILE_TILT_BR;  wShadowOAM[b+3].flags = 0;
        break;
    case 2: /* FrameBlock05: tilt left (XFLIP of FrameBlock04, tiles swapped per column) */
        wShadowOAM[b+0].y = baseY;          wShadowOAM[b+0].x = baseX;
        wShadowOAM[b+0].tile = BALL_TILE_TILT_TR;  wShadowOAM[b+0].flags = OAM_FLAG_FLIP_X;
        wShadowOAM[b+1].y = baseY;          wShadowOAM[b+1].x = (uint8_t)(baseX + 8);
        wShadowOAM[b+1].tile = BALL_TILE_TILT_TL;  wShadowOAM[b+1].flags = OAM_FLAG_FLIP_X;
        wShadowOAM[b+2].y = (uint8_t)(baseY + 8); wShadowOAM[b+2].x = baseX;
        wShadowOAM[b+2].tile = BALL_TILE_TILT_BR;  wShadowOAM[b+2].flags = OAM_FLAG_FLIP_X;
        wShadowOAM[b+3].y = (uint8_t)(baseY + 8); wShadowOAM[b+3].x = (uint8_t)(baseX + 8);
        wShadowOAM[b+3].tile = BALL_TILE_TILT_BL;  wShadowOAM[b+3].flags = OAM_FLAG_FLIP_X;
        break;
    }
}

/* Hide the 4-OAM ball sprite by moving all entries off-screen (Y=0). */
static void bui_ball_hide(void) {
    for (int i = 0; i < 4; i++)
        wShadowOAM[THROW_BALL_OAM_BASE + i].y = 0;
}

/* Load 10 poof cloud tiles into sprite_tile_gfx (reuses player-slide sprite slots). */
static void bui_ball_load_poof_tiles(void) {
    Display_LoadSpriteTile(POOF_T20, kPoofTile20);
    Display_LoadSpriteTile(POOF_T21, kPoofTile21);
    Display_LoadSpriteTile(POOF_T23, kPoofTile23);
    Display_LoadSpriteTile(POOF_T24, kPoofTile24);
    Display_LoadSpriteTile(POOF_T25, kPoofTile25);
    Display_LoadSpriteTile(POOF_T30, kPoofTile30);
    Display_LoadSpriteTile(POOF_T31, kPoofTile31);
    Display_LoadSpriteTile(POOF_T32, kPoofTile32);
    Display_LoadSpriteTile(POOF_T33, kPoofTile33);
    Display_LoadSpriteTile(POOF_T34, kPoofTile34);
}

/* Poof cloud sprite entry: column/row tile offsets, tile ID, OAM flags.
 * Mirrors the dbsprite macro in frame_blocks.asm. */
typedef struct { int8_t col; int8_t row; uint8_t tile; uint8_t flags; } poof_spr_t;

/* FrameBlock06 (12 sprites) — BASECOORD_1B (Y=48, X=112), frames 0-1 of poof */
static const poof_spr_t kFB06[12] = {
    {1,0,POOF_T23,0},                        {0,1,POOF_T32,0},  {1,1,POOF_T33,0},
    {2,0,POOF_T23,OAM_FLAG_FLIP_X},          {2,1,POOF_T33,OAM_FLAG_FLIP_X},
    {3,1,POOF_T32,OAM_FLAG_FLIP_X},
    {0,2,POOF_T32,OAM_FLAG_FLIP_Y},          {1,2,POOF_T33,OAM_FLAG_FLIP_Y},
    {1,3,POOF_T23,OAM_FLAG_FLIP_Y},
    {2,2,POOF_T33,OAM_FLAG_FLIP_X|OAM_FLAG_FLIP_Y},
    {3,2,POOF_T32,OAM_FLAG_FLIP_X|OAM_FLAG_FLIP_Y},
    {2,3,POOF_T23,OAM_FLAG_FLIP_X|OAM_FLAG_FLIP_Y},
};
/* FrameBlock07 (16 sprites) — BASECOORD_1B, frame 1 of poof */
static const poof_spr_t kFB07[16] = {
    {0,0,POOF_T20,0}, {1,0,POOF_T21,0}, {0,1,POOF_T30,0}, {1,1,POOF_T31,0},
    {2,0,POOF_T21,OAM_FLAG_FLIP_X}, {3,0,POOF_T20,OAM_FLAG_FLIP_X},
    {2,1,POOF_T31,OAM_FLAG_FLIP_X}, {3,1,POOF_T30,OAM_FLAG_FLIP_X},
    {0,2,POOF_T30,OAM_FLAG_FLIP_Y}, {1,2,POOF_T31,OAM_FLAG_FLIP_Y},
    {0,3,POOF_T20,OAM_FLAG_FLIP_Y}, {1,3,POOF_T21,OAM_FLAG_FLIP_Y},
    {2,2,POOF_T31,OAM_FLAG_FLIP_X|OAM_FLAG_FLIP_Y},
    {3,2,POOF_T30,OAM_FLAG_FLIP_X|OAM_FLAG_FLIP_Y},
    {2,3,POOF_T21,OAM_FLAG_FLIP_X|OAM_FLAG_FLIP_Y},
    {3,3,POOF_T20,OAM_FLAG_FLIP_X|OAM_FLAG_FLIP_Y},
};
/* FrameBlock08 (16 sprites) — BASECOORD_36 (Y=44, X=108), frames 2-3 of poof */
static const poof_spr_t kFB08[16] = {
    {0,0,POOF_T20,0}, {1,0,POOF_T21,0}, {0,1,POOF_T30,0}, {1,1,POOF_T31,0},
    {3,0,POOF_T21,OAM_FLAG_FLIP_X}, {4,0,POOF_T20,OAM_FLAG_FLIP_X},
    {3,1,POOF_T31,OAM_FLAG_FLIP_X}, {4,1,POOF_T30,OAM_FLAG_FLIP_X},
    {0,3,POOF_T30,OAM_FLAG_FLIP_Y}, {1,3,POOF_T31,OAM_FLAG_FLIP_Y},
    {0,4,POOF_T20,OAM_FLAG_FLIP_Y}, {1,4,POOF_T21,OAM_FLAG_FLIP_Y},
    {3,3,POOF_T31,OAM_FLAG_FLIP_X|OAM_FLAG_FLIP_Y},
    {4,3,POOF_T30,OAM_FLAG_FLIP_X|OAM_FLAG_FLIP_Y},
    {3,4,POOF_T21,OAM_FLAG_FLIP_X|OAM_FLAG_FLIP_Y},
    {4,4,POOF_T20,OAM_FLAG_FLIP_X|OAM_FLAG_FLIP_Y},
};
/* FrameBlock09 (12 sprites) — BASECOORD_36, frame 3 of poof */
static const poof_spr_t kFB09[12] = {
    {0,0,POOF_T24,0}, {1,0,POOF_T25,0}, {0,1,POOF_T34,0},
    {3,0,POOF_T25,OAM_FLAG_FLIP_X}, {4,0,POOF_T24,OAM_FLAG_FLIP_X},
    {4,1,POOF_T34,OAM_FLAG_FLIP_X},
    {0,3,POOF_T34,OAM_FLAG_FLIP_Y}, {0,4,POOF_T24,OAM_FLAG_FLIP_Y},
    {1,4,POOF_T25,OAM_FLAG_FLIP_Y},
    {4,3,POOF_T34,OAM_FLAG_FLIP_X|OAM_FLAG_FLIP_Y},
    {3,4,POOF_T25,OAM_FLAG_FLIP_X|OAM_FLAG_FLIP_Y},
    {4,4,POOF_T24,OAM_FLAG_FLIP_X|OAM_FLAG_FLIP_Y},
};
/* FrameBlock0A (12 sprites) — BASECOORD_15 (Y=40, X=104), frames 4-5 of poof */
static const poof_spr_t kFB0A[12] = {
    {0,0,POOF_T24,0}, {1,0,POOF_T25,0}, {0,1,POOF_T34,0},
    {4,0,POOF_T25,OAM_FLAG_FLIP_X}, {5,0,POOF_T24,OAM_FLAG_FLIP_X},
    {5,1,POOF_T34,OAM_FLAG_FLIP_X},
    {0,4,POOF_T34,OAM_FLAG_FLIP_Y}, {0,5,POOF_T24,OAM_FLAG_FLIP_Y},
    {1,5,POOF_T25,OAM_FLAG_FLIP_Y},
    {5,4,POOF_T34,OAM_FLAG_FLIP_X|OAM_FLAG_FLIP_Y},
    {4,5,POOF_T25,OAM_FLAG_FLIP_X|OAM_FLAG_FLIP_Y},
    {5,5,POOF_T24,OAM_FLAG_FLIP_X|OAM_FLAG_FLIP_Y},
};

/* Draw one poof frame block at POOF_OAM_BASE.
 * Subanim_0BallPoofEnemy: 6 entries using FB06,07,08,09,0A,0A
 * at BASECOORD_1B(Y=48,X=112), 1B, 36(Y=44,X=108), 36, 15(Y=40,X=104), 15.
 * Transform = SUBANIMTYPE_NORMAL for player turn (GetSubanimationTransform1
 * returns NORMAL for non-ENEMY type on player's turn) so coords are direct. */
static void bui_draw_poof_frame(int entry) {
    static const uint8_t kBaseY[6] = {48, 48, 44, 44, 40, 40};
    static const uint8_t kBaseX[6] = {112,112,108,108,104,104};
    static const poof_spr_t * const kFB[6]  = {kFB06,kFB07,kFB08,kFB09,kFB0A,kFB0A};
    static const int                kSz[6]  = {12,16,16,12,12,12};

    uint8_t baseY = kBaseY[entry], baseX = kBaseX[entry];
    const poof_spr_t *fb = kFB[entry];
    int n = kSz[entry];
    for (int i = 0; i < n; i++) {
        int idx = POOF_OAM_BASE + i;
        wShadowOAM[idx].y     = (uint8_t)(baseY + fb[i].row * 8);
        wShadowOAM[idx].x     = (uint8_t)(baseX + fb[i].col * 8);
        wShadowOAM[idx].tile  = fb[i].tile;
        wShadowOAM[idx].flags = fb[i].flags;
    }
}

/* Draw one player-side poof frame block at POOF_OAM_BASE.
 * Subanim_0BallPoof (player): 3 entries, FB06→FB07→FB08.
 * BASECOORD_82 (Y=72, X=32) for entries 0-1; BASECOORD_96 (Y=68, X=28) for entry 2. */
static void bui_draw_player_poof_frame(int entry) {
    static const uint8_t kPBaseY[3] = {72, 72, 68};
    static const uint8_t kPBaseX[3] = {32, 32, 28};
    static const poof_spr_t * const kPFB[3]  = {kFB06, kFB07, kFB08};
    static const int                kPSz[3]  = {12, 16, 16};

    uint8_t baseY = kPBaseY[entry], baseX = kPBaseX[entry];
    const poof_spr_t *fb = kPFB[entry];
    int n = kPSz[entry];
    for (int i = 0; i < n; i++) {
        int idx = POOF_OAM_BASE + i;
        wShadowOAM[idx].y     = (uint8_t)(baseY + fb[i].row * 8);
        wShadowOAM[idx].x     = (uint8_t)(baseX + fb[i].col * 8);
        wShadowOAM[idx].tile  = fb[i].tile;
        wShadowOAM[idx].flags = fb[i].flags;
    }
}

static void bui_set_tile(int col, int row, uint8_t tile) {
    if (col >= 0 && col < SCREEN_WIDTH && row >= 0 && row < SCREEN_HEIGHT)
        gScrollTileMap[(row + 2) * SCROLL_MAP_W + (col + 2)] = tile;
}

static void bui_put_str(int col, int row, const char *s) {
    while (*s && col < SCREEN_WIDTH) {
        bui_set_tile(col++, row, (uint8_t)bui_char_to_tile((unsigned char)*s));
        s++;
    }
}

/* Bit-reverse helper for horizontal sprite mirroring in BG tile data. */
static uint8_t bui_reverse_bits(uint8_t v) {
    v = (uint8_t)(((v & 0xF0u) >> 4) | ((v & 0x0Fu) << 4));
    v = (uint8_t)(((v & 0xCCu) >> 2) | ((v & 0x33u) << 2));
    v = (uint8_t)(((v & 0xAAu) >> 1) | ((v & 0x55u) << 1));
    return v;
}

static void bui_clear_rows(int r0, int r1) {
    for (int r = r0; r <= r1; r++)
        for (int c = 0; c < SCREEN_WIDTH; c++)
            bui_set_tile(c, r, BLANK_TILE_SLOT);
}

static void bui_fill_rows_tile(int r0, int r1, uint8_t tile) {
    for (int r = r0; r <= r1; r++)
        for (int c = 0; c < SCREEN_WIDTH; c++)
            bui_set_tile(c, r, tile);
}

/* Mirror writes to both battle tile backbuffers so scene clears happen in one
 * visual step even if a code path samples wTileMap directly this frame. */
static void bui_fill_rows_tile_both(int r0, int r1, uint8_t tile) {
    bui_fill_rows_tile(r0, r1, tile);
    if (r0 < 0) r0 = 0;
    if (r1 >= SCREEN_HEIGHT) r1 = SCREEN_HEIGHT - 1;
    for (int r = r0; r <= r1; r++)
        for (int c = 0; c < SCREEN_WIDTH; c++)
            wTileMap[r * SCREEN_WIDTH + c] = tile;
}

/* Clear a rectangular region of tiles (inclusive). */
static void bui_clear_rect(int c0, int r0, int c1, int r1) {
    for (int r = r0; r <= r1; r++)
        for (int c = c0; c <= c1; c++)
            bui_set_tile(c, r, BLANK_TILE_SLOT);
}

/* Right-align a decimal number in 3 tiles at (col, row). Matches PrintNumber
 * b=2, c=3 used by PrintStatsBox (status_screen.asm PrintStats). */
static void bui_put_num3(int col, int row, uint16_t val) {
    char buf[4];
    snprintf(buf, sizeof(buf), "%3u", (unsigned)val);
    bui_put_str(col, row, buf);
}

/* Level-up stats box — mirrors PrintStatsBox LEVEL_UP_STATS_BOX branch
 * (engine/pokemon/status_screen.asm:250).
 * TextBoxBorder hlcoord(9,2) b=8 c=9: outer cols 9-19, rows 2-11.
 * Labels at (11, 3/5/7/9) via PlaceString with double-spaced <NEXT>.
 * Numbers at col 15, rows 4/6/8/10 (bc=SCREEN_WIDTH+4 offset + 2 rows each). */
static void bui_draw_levelup_stats(const levelup_stats_t *s) {
    /* Box border (same char codes as bui_draw_box: 0x79=┌ 0x7A=─ 0x7B=┐ 0x7C=│ 0x7D=└ 0x7E=┘) */
    const int c0 = 9, c1 = 19, r0 = 2, r1 = 11;
    bui_set_tile(c0, r0, (uint8_t)Font_CharToTile(0x79));
    for (int c = c0+1; c < c1; c++) bui_set_tile(c, r0, (uint8_t)Font_CharToTile(0x7A));
    bui_set_tile(c1, r0, (uint8_t)Font_CharToTile(0x7B));
    for (int r = r0+1; r < r1; r++) {
        bui_set_tile(c0, r, (uint8_t)Font_CharToTile(0x7C));
        for (int c = c0+1; c < c1; c++) bui_set_tile(c, r, BLANK_TILE_SLOT);
        bui_set_tile(c1, r, (uint8_t)Font_CharToTile(0x7C));
    }
    bui_set_tile(c0, r1, (uint8_t)Font_CharToTile(0x7D));
    for (int c = c0+1; c < c1; c++) bui_set_tile(c, r1, (uint8_t)Font_CharToTile(0x7A));
    bui_set_tile(c1, r1, (uint8_t)Font_CharToTile(0x7E));
    /* Labels: col 11, double-spaced rows */
    bui_put_str(11, 3, "ATTACK");
    bui_put_str(11, 5, "DEFENSE");
    bui_put_str(11, 7, "SPEED");
    bui_put_str(11, 9, "SPECIAL");
    /* Numbers: col 15, one row below each label */
    bui_put_num3(15, 4,  s->atk);
    bui_put_num3(15, 6,  s->def);
    bui_put_num3(15, 8,  s->spd);
    bui_put_num3(15, 10, s->spc);
}

/* ---- Sprite rendering ---------------------------------------------- *
 * Enemy front sprite: OBJ tiles (no overlap with any UI region).       *
 * Player back sprite: BG tiles written directly to gScrollTileMap,     *
 *   matching the original game's CopyUncompressedPicToTilemap at       *
 *   hlcoord(1,5). Since it lives on the same tilemap layer as the UI,  *
 *   the PP box / move menu simply overwrites those tiles when drawn —   *
 *   no compositing needed. bui_load_sprites is called in BUI_DRAW_HUD  *
 *   so the sprite is redrawn every time the main battle view restores.  *
 *                                                                       *
 * OBJ tile slots (sprite_tile_gfx):                                    *
 *   0-48  : enemy front sprite (7×7 canvas, 49 tiles)                  *
 * BG tile slots (tile_gfx, safe to overwrite during battle):           *
 *   53-101: player back sprite (7×7 canvas, 49 tiles)                  *
 *                                                                       *
 * OAM entries:                                                          *
 *   0- 3  : pokeball throw sprite (4 tiles, highest priority)          *
 *   4-52  : enemy sprite tiles (7×7 grid, screen pixel 88,0)           */
#define ENEMY_SPR_TILE_BASE   0
#define PLAYER_SPR_BG_BASE    53   /* BG tile slots for back sprite (53-101) */
#define ENEMY_SPR_OAM_BASE    53   /* OAM 53-101: renders below player (higher index = lower priority) */
/* Enemy front pic is drawn at hlcoord(12,0) in the original battle ASM:
 * InitOpponent / InitWildBattle call CopyUncompressedPicToTilemap there,
 * so the 7x7 front sprite's top-left is pixel (96, 0). */
#define ENEMY_SPR_PX_X        96
/* Victory slide final X: col 14 = pixel 112.  Rightmost tile (tx=6) lands at
 * pixel 160 = just off-screen right, matching ASM scroll (6 of 7 cols visible). */
#define VICTORY_TRAINER_PX_X  112
#define ENEMY_SPR_PX_Y        0
/* Player back: hlcoord(1,5) = tile col 1, row 5 = pixel (8, 40) */
#define PLAYER_SPR_COL        1
#define PLAYER_SPR_ROW        5

/* Slide-in intro: player back sprite as OAM during the 72-frame slide animation.
 * Uses sprite tile slots 53-101 (separate from BG tile slots 49-97 for the same gfx).
 * Poof animation (BUI_BALL_POOF) reuses slots 53-68 once the slide is done. */
#define PLAYER_SLIDE_OAM_BASE 4     /* OAM entries 4-52: renders above enemy (lower index = higher priority) */
#define PLAYER_SLIDE_TILE_BASE 53   /* sprite tile VRAM slots 53-101 (independent of OAM index) */
/* Party pokeball OAM (DrawAllPokeballs): 6 entries after the two sprite blocks. */
#define POKEBALL_OAM_BASE         102   /* player party balls: OAM 102-107 */
#define ENEMY_POKEBALL_OAM_BASE   108   /* enemy party balls (trainer only): OAM 108-113 */
#define POKEBALL_TILE_BASE    120   /* sprite tile 120=normal,121=status,122=fainted,123=empty
                                    * Must not overlap player slide tiles 53-101, enemy 0-48,
                                    * or ball throw tiles 114-119. */
/* Pokeball screen position: row 10 = pixel Y 80; OAM Y includes +16 hardware offset. */
#define POKEBALL_OAM_Y        96    /* 80 + 16 = 96 */
/* Pokeball X: screen X starts at 88 (matching wBaseCoordX=$60 in the original).
 * OAM X = screen_x + 8.  Six balls spaced 8px apart: 96,104,112,120,128,136. */
#define POKEBALL_OAM_X_START  96
#define POKEBALL_OAM_X_STEP   8

/* Red's back sprite — 49 tiles (7x7 canvas) extracted from gfx/player/redb.png via
 * the same ScaleSpriteByTwo algorithm used for pokemon back sprites:
 *   crop 32x32 → 28x28, nearest-neighbor 2x scale → 56x56, encode as 7x7 tiles.
 * Matches LoadPlayerBackPic (core.asm:6202) which loads RedPicBack for the slide. */
static const uint8_t kRedBackSprite[49][16] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x03,0x03,0x03},
    {0x00,0x00,0x00,0x00,0x03,0x03,0x03,0x03,0x3F,0x3C,0x3F,0x3C,0xC0,0xFF,0xC0,0xFF},
    {0x3F,0x3F,0x3F,0x3F,0xFF,0xC0,0xFF,0xC0,0xFF,0x00,0xFF,0x00,0xCF,0x30,0xCF,0x30},
    {0x00,0xC0,0x00,0xC0,0xFC,0x3C,0xFC,0x3C,0xFF,0x03,0xFF,0x03,0xFF,0x03,0xFF,0x03},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x0C,0x0F,0x0C,0x0F,0x30,0x3F,0x30,0x3F,0x30,0x3F,0x30,0x3F,0x30,0x3F,0x30,0x3F},
    {0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF},
    {0x33,0xCC,0x33,0xCC,0x0C,0xF3,0x0C,0xF3,0x33,0xCC,0x33,0xCC,0x00,0xFF,0x00,0xFF},
    {0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0x3F,0xC0,0x3F,0xC0,0x00,0xFC,0x00,0xFC},
    {0x00,0xC0,0x00,0xC0,0xC0,0xC0,0xC0,0xC0,0xFC,0xFC,0xFC,0xFC,0x03,0x03,0x03,0x03},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x0F,0x00,0x0F,0x0C,0x0F,0x0C,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F},
    {0xFC,0xFF,0xFC,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
    {0x00,0xFF,0x00,0xFF,0x0F,0xFF,0x0F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
    {0x00,0xC0,0x00,0xC0,0xC3,0xFF,0xC3,0xFF,0x3F,0x33,0x3F,0x33,0xF3,0x33,0xF3,0x33},
    {0x3C,0x3C,0x3C,0x3C,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x03,0x03,0x03,0x03,0x00,0x03,0x00,0x03,0x00,0x00,0x00,0x00,0x0F,0x3F,0x0F,0x3F},
    {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x3F,0xFF,0x3F,0xFF,0xF0,0xFF,0xF0,0xFF},
    {0xFF,0xFF,0xFF,0xFF,0xFC,0xFF,0xFC,0xFF,0xCF,0xF0,0xCF,0xF0,0x3C,0xFF,0x3C,0xFF},
    {0xF0,0x30,0xF0,0x30,0xF0,0xC0,0xF0,0xC0,0xFF,0x03,0xFF,0x03,0x30,0xFC,0x30,0xFC},
    {0x00,0xC0,0x00,0xC0,0xC0,0xC0,0xC0,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x03,0x00,0x03,0x03,0x0C,0x03,0x0C,0x0C,0x0F,0x0C,0x0F,0x30,0x3F,0x30,0x3F},
    {0xFF,0xC0,0xFF,0xC0,0x3F,0xC0,0x3F,0xC0,0xCC,0x33,0xCC,0x33,0xF3,0xFC,0xF3,0xFC},
    {0xFF,0x0F,0xFF,0x0F,0xFF,0x00,0xFF,0x00,0xCF,0x33,0xCF,0x33,0x3C,0xCF,0x3C,0xCF},
    {0xC3,0xFF,0xC3,0xFF,0xF3,0xFF,0xF3,0xFF,0xCF,0xFC,0xCF,0xFC,0x0F,0xFC,0x0F,0xFC},
    {0xC0,0xC0,0xC0,0xC0,0x30,0x3C,0x30,0x3C,0x03,0x03,0x03,0x03,0x0C,0x0C,0x0C,0x0C},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC3,0xC3,0xC3,0xC3},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0xF0,0xC0,0xF0},
    {0x3F,0x3F,0x3F,0x3F,0xC0,0xCF,0xC0,0xCF,0xF0,0xFF,0xF0,0xFF,0xF0,0xFF,0xF0,0xFF},
    {0x0C,0x0F,0x0C,0x0F,0xFF,0xFF,0xFF,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF},
    {0x0C,0xFF,0x0C,0xFF,0xF0,0xFF,0xF0,0xFF,0xF0,0xFF,0xF0,0xFF,0x30,0xFF,0x30,0xFF},
    {0x0F,0xFC,0x0F,0xFC,0x0F,0xFC,0x0F,0xFC,0x3F,0xF0,0x3F,0xF0,0x3F,0xFF,0x3F,0xFF},
    {0xF0,0x30,0xF0,0x30,0xF0,0xC0,0xF0,0xC0,0xFC,0xC0,0xFC,0xC0,0xFF,0x00,0xFF,0x00},
    {0x3C,0x3C,0x3C,0x3C,0x3C,0x30,0x3C,0x30,0x00,0x0F,0x00,0x0F,0x0C,0x0C,0x0C,0x0C},
    {0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x00,0x03,0x00,0x03,0x03,0xC3,0x03,0xC3},
    {0x30,0x3F,0x30,0x3F,0x30,0x3F,0x30,0x3F,0x0F,0x0F,0x0F,0x0F,0x00,0x00,0x00,0x00},
    {0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0xFF,0xFF,0xFF},
    {0x30,0xFF,0x30,0xFF,0x30,0xFF,0x30,0xFF,0x0F,0xFF,0x0F,0xFF,0xF3,0xFF,0xF3,0xFF},
    {0xC3,0xFF,0xC3,0xFF,0xCF,0xFC,0xCF,0xFC,0x3F,0xF3,0x3F,0xF3,0x0C,0xFC,0x0C,0xFC},
    {0xFF,0x00,0xFF,0x00,0xFF,0xC0,0xFF,0xC0,0x3F,0x30,0x3F,0x30,0x0F,0x0F,0x0F,0x0F},
    {0xFC,0x0C,0xFC,0x0C,0xFC,0x0C,0xFC,0x0C,0xFF,0x3F,0xFF,0x3F,0xF0,0xF0,0xF0,0xF0},
    {0x3F,0x33,0x3F,0x33,0xFC,0xFC,0xFC,0xFC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

/* 4 pokeball tile bitmaps (2bpp, 16 bytes each) extracted from gfx/battle/balls.png.
 * Tile indices match the original vSprites $31-$34 assignments in draw_hud_pokeball_gfx.asm. */
static const uint8_t kPokeballTiles[4][16] = {
    /* 110 — normal (full health) */
    {0x00,0x00,0x1c,0x1c,0x22,0x3e,0x51,0x6f,0x41,0x7f,0x7f,0x41,0x3e,0x22,0x1c,0x1c},
    /* 111 — status ailment (solid black ball) */
    {0x00,0x00,0x1c,0x1c,0x3e,0x3e,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x3e,0x3e,0x1c,0x1c},
    /* 112 — fainted (crossed ball) */
    {0x00,0x00,0x41,0x5d,0x3e,0x22,0x3e,0x55,0x3e,0x49,0x3e,0x55,0x3e,0x22,0x41,0x5d},
    /* 113 — empty slot (outline only) */
    {0x00,0x00,0x1c,0x00,0x22,0x00,0x41,0x00,0x41,0x00,0x41,0x00,0x22,0x00,0x1c,0x00},
};

/* DrawAllPokeballs (draw_hud_pokeball_gfx.asm):
 *   SetupOwnPartyPokeballs  — always (wild + trainer)
 *   SetupEnemyPartyPokeballs — trainer battles only (wIsInBattle == 2)
 *
 * Player balls: OAM Y=$60(96), X starts $60(96) stepping +8 left→right.
 * Enemy  balls: OAM Y=$20(32), X starts $48(72) stepping -8 right→left.
 *   Matches the original wBaseCoordX/Y values and wHUDPokeballGfxOffsetX. */
static uint8_t pokeball_tile_for(const party_mon_t *m) {
    if (m->base.hp == 0)     return (uint8_t)(POKEBALL_TILE_BASE + 2); /* fainted */
    if (m->base.status != 0) return (uint8_t)(POKEBALL_TILE_BASE + 1); /* status ailment */
    return (uint8_t)(POKEBALL_TILE_BASE + 0);                           /* healthy */
}

static void bui_draw_pokeballs(void) {
    for (int i = 0; i < 4; i++)
        Display_LoadSpriteTile((uint8_t)(POKEBALL_TILE_BASE + i), kPokeballTiles[i]);

    /* SetupOwnPartyPokeballs — PlacePlayerHUDTiles then OAM.
     * PlacePlayerHUDTiles: hlcoord(18,10) de=-1
     *   $73 at (18,10), $77 at (18,11), $76×8 at (17-10,11), $6F at (9,11) */
    bui_set_tile(18, 10, (uint8_t)Font_CharToTile(0x73));
    bui_set_tile(18, 11, (uint8_t)Font_CharToTile(0x77));
    for (int c = 10; c <= 17; c++)
        bui_set_tile(c, 11, (uint8_t)Font_CharToTile(0x76));
    bui_set_tile(9,  11, (uint8_t)Font_CharToTile(0x6F));

    for (int i = 0; i < PARTY_LENGTH; i++) {
        uint8_t tile = (i >= wPartyCount) ? (uint8_t)(POKEBALL_TILE_BASE + 3) : pokeball_tile_for(&wPartyMons[i]);
        wShadowOAM[POKEBALL_OAM_BASE + i].y     = POKEBALL_OAM_Y;
        wShadowOAM[POKEBALL_OAM_BASE + i].x     = (uint8_t)(POKEBALL_OAM_X_START + i * POKEBALL_OAM_X_STEP);
        wShadowOAM[POKEBALL_OAM_BASE + i].tile  = tile;
        wShadowOAM[POKEBALL_OAM_BASE + i].flags = 0;
    }

    /* SetupEnemyPartyPokeballs — trainer battles only.
     * PlaceEnemyHUDTiles: hlcoord(1,2) de=+1
     *   $73 at (1,2), $74 at (1,3), $76×8 at (2-9,3), $78 at (10,3) */
    if (wIsInBattle == 2) {
        bui_set_tile(1,  2, (uint8_t)Font_CharToTile(0x73));
        bui_set_tile(1,  3, (uint8_t)Font_CharToTile(0x74));
        for (int c = 2; c <= 9; c++)
            bui_set_tile(c, 3, (uint8_t)Font_CharToTile(0x76));
        bui_set_tile(10, 3, (uint8_t)Font_CharToTile(0x78));

        for (int i = 0; i < PARTY_LENGTH; i++) {
            uint8_t tile = (i >= wEnemyPartyCount) ? (uint8_t)(POKEBALL_TILE_BASE + 3) : pokeball_tile_for(&wEnemyMons[i]);
            /* X decrements right→left: $48, $40, $38, $30, $28, $20 */
            wShadowOAM[ENEMY_POKEBALL_OAM_BASE + i].y     = 32;
            wShadowOAM[ENEMY_POKEBALL_OAM_BASE + i].x     = (uint8_t)(72 - i * 8);
            wShadowOAM[ENEMY_POKEBALL_OAM_BASE + i].tile  = tile;
            wShadowOAM[ENEMY_POKEBALL_OAM_BASE + i].flags = 0;
        }
    }
}

/* Hide all pokeball OAM entries. */
static void bui_hide_pokeballs(void) {
    for (int i = 0; i < PARTY_LENGTH; i++) {
        wShadowOAM[POKEBALL_OAM_BASE + i].y = 0;
        wShadowOAM[ENEMY_POKEBALL_OAM_BASE + i].y = 0;
    }
}

static void bui_load_sprites(void) {
    uint8_t e_dex = gSpeciesToDex[wEnemyMon.species];
    uint8_t p_dex = gSpeciesToDex[wBattleMon.species];

    /* Enemy front sprite: 7×7 OBJ tiles */
    if (e_dex > 0 && e_dex <= 151) {
        for (int i = 0; i < POKEMON_FRONT_CANVAS_TILES; i++)
            Display_LoadSpriteTile((uint8_t)(ENEMY_SPR_TILE_BASE + i),
                                   gPokemonFrontSprite[e_dex][i]);
        for (int ty = 0; ty < 7; ty++) {
            for (int tx = 0; tx < 7; tx++) {
                int idx = ENEMY_SPR_OAM_BASE + ty * 7 + tx;
                wShadowOAM[idx].y    = (uint8_t)(ENEMY_SPR_PX_Y + ty * 8 + 16);
                wShadowOAM[idx].x    = (uint8_t)(ENEMY_SPR_PX_X + tx * 8 + 8);
                wShadowOAM[idx].tile = (uint8_t)(ENEMY_SPR_TILE_BASE + ty * 7 + tx);
                wShadowOAM[idx].flags = 0;
            }
        }
    }

    /* Player back sprite: drawn as BG tiles (CopyUncompressedPicToTilemap).
     * Tiles go into tile_gfx slots 49-97 (PLAYER_SPR_BG_BASE), written to gScrollTileMap.
     * PP box / move menu naturally overwrites the overlap area when drawn. */
    if (p_dex > 0 && p_dex <= 151) {
        for (int i = 0; i < POKEMON_BACK_TILES; i++)
            Display_LoadTile((uint8_t)(PLAYER_SPR_BG_BASE + i),
                             gPokemonBackSprite[p_dex][i]);
        bui_place_player_sprite();
    }
}

/* Re-stamp player back sprite tile IDs onto gScrollTileMap.
 * Call after any clear that erases the sprite area (e.g. PP box clear). */
static void bui_place_player_sprite(void) {
    for (int ty = 0; ty < 7; ty++)
        for (int tx = 0; tx < 7; tx++)
            bui_set_tile(PLAYER_SPR_COL + tx, PLAYER_SPR_ROW + ty,
                         (uint8_t)(PLAYER_SPR_BG_BASE + ty * 7 + tx));
}

/* HP bar: 6 tiles wide, 48 pixels total (matches DrawHPBar d=6).
 * Tiles $63 (empty) through $6B (full), with $64–$6A as partial fills (1–7 px).
 * Pixel calculation mirrors the original: hp * 48 / max_hp. */
static void bui_draw_hp_bar(int col, int row, int hp, int max_hp) {
    int pixels = (max_hp > 0) ? ((int)hp * 48 / (int)max_hp) : 0;
    if (pixels < 0) pixels = 0;
    if (pixels > 48) pixels = 48;
    for (int i = 0; i < 6; i++) {
        int seg = pixels - i * 8;
        uint8_t tile;
        if (seg <= 0)       tile = 0x63;
        else if (seg >= 8)  tile = 0x6B;
        else                tile = (uint8_t)(0x63 + seg);  /* $64–$6A partial */
        bui_set_tile(col + i, row, (uint8_t)Font_CharToTile(tile));
    }
}

/* Pixel count for HP bar — mirrors GetHPBarLength: hp*48/max_hp, min 1 if alive. */
static int calc_hp_pixels(int hp, int max_hp) {
    if (max_hp <= 0) return 0;
    int px = hp * 48 / max_hp;
    if (px < 0) px = 0;
    if (px > 48) px = 48;
    if (hp > 0 && px == 0) px = 1;
    return px;
}

/* Draw HP bar at an explicit pixel count (used during scroll animation). */
static void bui_draw_hp_bar_px(int col, int row, int pixels) {
    for (int i = 0; i < 6; i++) {
        int seg = pixels - i * 8;
        uint8_t tile;
        if (seg <= 0)       tile = 0x63;
        else if (seg >= 8)  tile = 0x6B;
        else                tile = (uint8_t)(0x63 + seg);
        bui_set_tile(col + i, row, (uint8_t)Font_CharToTile(tile));
    }
}

/* Level display: matches PrintLevel — <LV> tile + 2 digits, or 3 digits at 100. */
static void bui_put_level(int col, int row, int level) {
    if (level >= 100) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%d", level);
        bui_put_str(col, row, buf);
    } else {
        bui_set_tile(col, row, (uint8_t)Font_CharToTile(0x6E));  /* <LV> ":L" tile */
        char buf[3];
        snprintf(buf, sizeof(buf), "%-2d", level);  /* LEFT_ALIGN: "5 " not " 5" */
        bui_put_str(col + 1, row, buf);
    }
}

static void bui_draw_box(void) {
    const int r0 = 12, r1 = 17;
    bui_set_tile(0,              r0, (uint8_t)Font_CharToTile(0x79));
    for (int c = 1; c < SCREEN_WIDTH - 1; c++)
        bui_set_tile(c,          r0, (uint8_t)Font_CharToTile(0x7A));
    bui_set_tile(SCREEN_WIDTH-1, r0, (uint8_t)Font_CharToTile(0x7B));
    for (int r = r0+1; r < r1; r++) {
        bui_set_tile(0,              r, (uint8_t)Font_CharToTile(0x7C));
        for (int c = 1; c < SCREEN_WIDTH-1; c++)
            bui_set_tile(c,          r, BLANK_TILE_SLOT);
        bui_set_tile(SCREEN_WIDTH-1, r, (uint8_t)Font_CharToTile(0x7C));
    }
    bui_set_tile(0,              r1, (uint8_t)Font_CharToTile(0x7D));
    for (int c = 1; c < SCREEN_WIDTH - 1; c++)
        bui_set_tile(c,          r1, (uint8_t)Font_CharToTile(0x7A));
    bui_set_tile(SCREEN_WIDTH-1, r1, (uint8_t)Font_CharToTile(0x7E));
}

/* ---- HUD drawing --------------------------------------------------- */

/* Enemy HUD: top-left, cols 0-11, rows 0-3.
 * Matches DrawEnemyHUDAndHPBar exactly:
 *   ClearScreenArea hlcoord(0,0) b=4 c=12
 *   PlaceEnemyHUDTiles → $73 at (1,2), $74 at (1,3), $76×8 at (2-9,3), $78 at (10,3)
 *   name centered at hlcoord(1,0)
 *   PrintLevel at hlcoord(4,1): <LV> + digits
 *   DrawHPBar at hlcoord(2,2): $71 HP label, $62 left cap, 6 bar tiles, $6D right cap */
static void bui_draw_enemy_hud(void) {
    bui_clear_rect(0, 0, 11, 3);
    /* PlaceEnemyHUDTiles */
    bui_set_tile(1,  2, (uint8_t)Font_CharToTile(0x73));
    bui_set_tile(1,  3, (uint8_t)Font_CharToTile(0x74));
    for (int c = 2; c <= 9; c++)
        bui_set_tile(c, 3, (uint8_t)Font_CharToTile(0x76));
    bui_set_tile(10, 3, (uint8_t)Font_CharToTile(0x78));
    /* Name at hlcoord(1,0), left-aligned in cols 1-10 */
    const char *name = Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]);
    char nbuf[11];
    snprintf(nbuf, sizeof(nbuf), "%-10s", name);
    bui_put_str(1, 0, nbuf);
    /* Level at hlcoord(4,1) */
    bui_put_level(4, 1, wEnemyMon.level);
    /* DrawHPBar at hlcoord(2,2) */
    bui_set_tile(2,  2, (uint8_t)Font_CharToTile(0x71));  /* HP label */
    bui_set_tile(3,  2, (uint8_t)Font_CharToTile(0x62));  /* left cap */
    bui_draw_hp_bar(4, 2, wEnemyMon.hp, wEnemyMon.max_hp);
    bui_set_tile(10, 2, (uint8_t)Font_CharToTile(0x6C));  /* right cap (wHPBarType=0, enemy) */
}

/* Player HUD: bottom-right, cols 9-19, rows 7-11.
 * Matches DrawPlayerHUDAndHPBar exactly:
 *   ClearScreenArea hlcoord(9,7) b=5 c=11
 *   PlacePlayerHUDTiles → $73 at (18,10), $77 at (18,11), $76×8 at (10-17,11), $6F at (9,11)
 *   name centered at hlcoord(10,7)
 *   PrintLevel at hlcoord(14,8)
 *   DrawHP (predef) at hlcoord(10,9): $71 HP label, $62 left cap, 6 bar tiles, $6D right cap
 *   HP fraction below bar at hlcoord(10,9)+SCREEN_WIDTH+1 = hlcoord(11,10):
 *     PrintNumber 2,3 (curHP) + '/' + PrintNumber 2,3 (maxHP) */
static void bui_draw_player_hud(void) {
    bui_clear_rect(9, 7, 19, 11);
    /* PlacePlayerHUDTiles */
    bui_set_tile(18, 10, (uint8_t)Font_CharToTile(0x73));
    bui_set_tile(18, 11, (uint8_t)Font_CharToTile(0x77));
    for (int c = 10; c <= 17; c++)
        bui_set_tile(c, 11, (uint8_t)Font_CharToTile(0x76));
    bui_set_tile(9,  11, (uint8_t)Font_CharToTile(0x6F));
    /* Name at hlcoord(10,7), left-aligned in cols 10-19 (10 chars, CenterMonName ≥4) */
    const char *name = Pokemon_GetName(gSpeciesToDex[wBattleMon.species]);
    char nbuf[11];
    snprintf(nbuf, sizeof(nbuf), "%-10s", name);
    bui_put_str(10, 7, nbuf);
    /* Level at hlcoord(14,8) */
    bui_put_level(14, 8, wBattleMon.level);
    /* DrawHP at hlcoord(10,9) */
    bui_set_tile(10, 9, (uint8_t)Font_CharToTile(0x71));  /* HP label */
    bui_set_tile(11, 9, (uint8_t)Font_CharToTile(0x62));  /* left cap */
    bui_draw_hp_bar(12, 9, wBattleMon.hp, wBattleMon.max_hp);
    bui_set_tile(18, 9, (uint8_t)Font_CharToTile(0x6D));  /* right cap */
    /* HP fraction at hlcoord(11,10): %3d / %3d right-aligned */
    char hbuf[4];
    snprintf(hbuf, sizeof(hbuf), "%3d", wBattleMon.hp);
    bui_put_str(11, 10, hbuf);
    bui_set_tile(14, 10, (uint8_t)bui_char_to_tile('/'));
    snprintf(hbuf, sizeof(hbuf), "%3d", wBattleMon.max_hp);
    bui_put_str(15, 10, hbuf);
}

/* ---- Menu drawing -------------------------------------------------- */

/* Battle menu sub-box: cols 8-19, rows 12-17.
 * Matches BATTLE_MENU_TEMPLATE (text_boxes.asm:31) which draws on top of the
 * full-width message box, creating a T-junction at col 8 and a dual-section
 * appearance: left message area (cols 0-7) + right menu area (cols 8-19). */
static void bui_draw_battle_menu_box(void) {
    const int c0 = 8, c1 = 19, r0 = 12, r1 = 17;
    bui_set_tile(c0,             r0, (uint8_t)Font_CharToTile(0x79)); /* ┌ */
    for (int c = c0+1; c < c1; c++)
        bui_set_tile(c,          r0, (uint8_t)Font_CharToTile(0x7A)); /* ─ */
    bui_set_tile(c1,             r0, (uint8_t)Font_CharToTile(0x7B)); /* ┐ */
    for (int r = r0+1; r < r1; r++) {
        bui_set_tile(c0,         r, (uint8_t)Font_CharToTile(0x7C)); /* │ */
        for (int c = c0+1; c < c1; c++)
            bui_set_tile(c,      r, BLANK_TILE_SLOT);
        bui_set_tile(c1,         r, (uint8_t)Font_CharToTile(0x7C)); /* │ */
    }
    bui_set_tile(c0,             r1, (uint8_t)Font_CharToTile(0x7D)); /* └ */
    for (int c = c0+1; c < c1; c++)
        bui_set_tile(c,          r1, (uint8_t)Font_CharToTile(0x7A)); /* ─ */
    bui_set_tile(c1,             r1, (uint8_t)Font_CharToTile(0x7E)); /* ┘ */
}

/* Battle main menu.
 * Matches DisplayBattleMenu + BATTLE_MENU_TEMPLATE:
 *   1. PrintEmptyString draws a full-width message box (cols 0-19, rows 12-17).
 *   2. BATTLE_MENU_TEMPLATE overlays cols 8-19 with a separate menu box,
 *      creating a T-junction at col 8 — left half stays as message area.
 * Cursor col 9 (left) / col 15 (right). BattleMenuText placed at col 10, row 14.
 * Right col labels use 3 ASCII chars (cols 16-18) for the original 2-tile <PK><MN>. */
static void bui_draw_main_menu(int cursor) {
    bui_draw_box();              /* full-width message area (PrintEmptyString) */
    bui_draw_battle_menu_box();  /* overlay right half: BATTLE_MENU_TEMPLATE   */
    /* cursor: 0=FIGHT(top-left) 1=PKMN(top-right) 2=ITEM(bot-left) 3=RUN(bot-right) */
    /* Row 14: FIGHT / <PK><MN> */
    bui_put_str(9,  14, cursor == 0 ? ">FIGHT" : " FIGHT");
    bui_set_tile(15, 14, cursor == 1 ? (uint8_t)bui_char_to_tile('>') : BLANK_TILE_SLOT);
    bui_set_tile(16, 14, (uint8_t)Font_CharToTile(0xE1));        /* <PK> tile */
    bui_set_tile(17, 14, (uint8_t)Font_CharToTile(0xE2));        /* <MN> tile */
    /* Row 16: ITEM / RUN */
    bui_put_str(9,  16, cursor == 2 ? ">ITEM" : " ITEM");
    bui_put_str(15, 16, cursor == 3 ? ">RUN" : " RUN");
}

/* Move selection box: cols 4-19, rows 12-17.
 * Matches TextBoxBorder hlcoord(4,12) b=4 c=14, then hlcoord(4,12)='─'
 * and hlcoord(10,12)='┘' to create the T-junction with the info box. */
static void bui_draw_move_box(void) {
    const int c0 = 4, c1 = 19, r0 = 12, r1 = 17;
    /* Top border row — col 4 is '─' (not corner), col 10 is '┘', rest normal */
    bui_set_tile(c0,             r0, (uint8_t)Font_CharToTile(0x7A)); /* ─ */
    for (int c = c0+1; c < c1; c++)
        bui_set_tile(c,          r0, (uint8_t)Font_CharToTile(0x7A)); /* ─ */
    bui_set_tile(10,             r0, (uint8_t)Font_CharToTile(0x7E)); /* ┘ */
    bui_set_tile(c1,             r0, (uint8_t)Font_CharToTile(0x7B)); /* ┐ */
    /* Side borders + clear inner rows 13-16 */
    for (int r = r0+1; r < r1; r++) {
        bui_set_tile(c0,         r, (uint8_t)Font_CharToTile(0x7C)); /* │ */
        for (int c = c0+1; c < c1; c++)
            bui_set_tile(c,      r, BLANK_TILE_SLOT);
        bui_set_tile(c1,         r, (uint8_t)Font_CharToTile(0x7C)); /* │ */
    }
    /* Bottom border row */
    bui_set_tile(c0,             r1, (uint8_t)Font_CharToTile(0x7D)); /* └ */
    for (int c = c0+1; c < c1; c++)
        bui_set_tile(c,          r1, (uint8_t)Font_CharToTile(0x7A)); /* ─ */
    bui_set_tile(c1,             r1, (uint8_t)Font_CharToTile(0x7E)); /* ┘ */
}

/* Type name helper for PrintMenuItem info box. */
static const char *bui_type_name(uint8_t type) {
    switch (type) {
        case TYPE_NORMAL:    return "NORMAL";
        case TYPE_FIGHTING:  return "FIGHTNG";
        case TYPE_FLYING:    return "FLYING";
        case TYPE_POISON:    return "POISON";
        case TYPE_GROUND:    return "GROUND";
        case TYPE_ROCK:      return "ROCK";
        case TYPE_BIRD:      return "BIRD";
        case TYPE_BUG:       return "BUG";
        case TYPE_GHOST:     return "GHOST";
        case TYPE_FIRE:      return "FIRE";
        case TYPE_WATER:     return "WATER";
        case TYPE_GRASS:     return "GRASS";
        case TYPE_ELECTRIC:  return "ELECTRC";
        case TYPE_PSYCHIC:   return "PSYCHIC";
        case TYPE_ICE:       return "ICE";
        case TYPE_DRAGON:    return "DRAGON";
        default:             return "?????";
    }
}

/* PP/type info box: cols 0-10, rows 8-12 (matches PrintMenuItem).
 * Drawn over the player sprite area when move menu is open. */
static void bui_draw_pp_info_box(int cursor) {
    const int c0 = 0, c1 = 10, r0 = 8, r1 = 12;
    /* Top border */
    bui_set_tile(c0,             r0, (uint8_t)Font_CharToTile(0x79)); /* ┌ */
    for (int c = c0+1; c < c1; c++)
        bui_set_tile(c,          r0, (uint8_t)Font_CharToTile(0x7A)); /* ─ */
    bui_set_tile(c1,             r0, (uint8_t)Font_CharToTile(0x7B)); /* ┐ */
    /* Side borders + clear inner rows */
    for (int r = r0+1; r < r1; r++) {
        bui_set_tile(c0,         r, (uint8_t)Font_CharToTile(0x7C)); /* │ */
        for (int c = c0+1; c < c1; c++)
            bui_set_tile(c,      r, BLANK_TILE_SLOT);
        bui_set_tile(c1,         r, (uint8_t)Font_CharToTile(0x7C)); /* │ */
    }
    /* Bottom border — overlaps move box top row 12 at cols 0-10 */
    bui_set_tile(c0,             r1, (uint8_t)Font_CharToTile(0x7D)); /* └ */
    for (int c = c0+1; c < c1; c++)
        bui_set_tile(c,          r1, (uint8_t)Font_CharToTile(0x7A)); /* ─ */
    bui_set_tile(c1,             r1, (uint8_t)Font_CharToTile(0x7E)); /* ┘ */

    int move_id = wBattleMon.moves[cursor];
    if (!move_id) {
        /* Row 9: TYPE/ label */
        bui_put_str(1, 9, "TYPE");
        bui_set_tile(5, 9, (uint8_t)bui_char_to_tile('/'));
        /* Row 11: --/-- */
        bui_put_str(5, 11, "--/--");
        return;
    }
    /* Row 9: "TYPE" + "/" */
    bui_put_str(1, 9, "TYPE");
    bui_set_tile(5, 9, (uint8_t)bui_char_to_tile('/'));
    /* Row 10: type name at col 2 (matches hlcoord 2, 10 PrintMoveType) */
    bui_put_str(2, 10, bui_type_name(gMoves[move_id].type));
    /* Row 11: curPP at cols 5-6, '/' at col 7, maxPP at cols 8-9
     * (matches hlcoord 5,11 PrintNumber + hlcoord 7,11 '/' + hlcoord 8,11 PrintNumber) */
    uint8_t cur_pp = wBattleMon.pp[cursor] & 0x3F;
    uint8_t max_pp = gMoves[move_id].pp;
    char pp[6];
    snprintf(pp, sizeof(pp), "%2d", cur_pp);
    bui_put_str(5, 11, pp);
    bui_set_tile(7, 11, (uint8_t)bui_char_to_tile('/'));
    snprintf(pp, sizeof(pp), "%2d", max_pp);
    bui_put_str(8, 11, pp);
}

/* Move selection menu.
 * Matches MoveSelectionMenu .regularmenu: box hlcoord(4,12) b=4 c=14
 * → outer box rows 12-17, cols 4-19. Moves written at hlcoord(6,13),
 * single-spaced → rows 13-16. Cursor col = wTopMenuItemX = 5.
 * PP/type info drawn in separate box at cols 0-10, rows 8-12 (PrintMenuItem). */
static void bui_draw_move_menu(int cursor) {
    bui_draw_move_box();
    bui_draw_pp_info_box(cursor);
    for (int i = 0; i < 4; i++) {
        char buf[14];
        char prefix = (cursor == i) ? '>' : ' ';
        /* cursor at col 5, name starts at col 6 (hlcoord 6, 13 + i) */
        if (wBattleMon.moves[i]) {
            const char *mn = gMoveNames[wBattleMon.moves[i]];
            snprintf(buf, sizeof(buf), "%c%-12s", prefix, mn ? mn : "-");
        } else {
            /* FormatMovesString uses single '-' for empty slots */
            snprintf(buf, sizeof(buf), "%c-", prefix);
        }
        buf[13] = '\0';
        bui_put_str(5, 13 + i, buf);
    }
}

/* Open the Gen 1 party menu for forced battle selection.
 * Backs up and clears the battle screen; the full Gen 1 party layout is drawn
 * by PartyMenu_Open.  BUI_DRAW_HUD redraws the battle screen after the menu. */
static void bui_open_party_select(void) {
    PartyMenu_Open(1 /* force */);
    bui_state = BUI_PARTY_SELECT;
}

/* ---- Turn execution helpers --------------------------------------- */


/* Build and show the appropriate text after a move execution.
 *
 * whose:    0 = player just moved, 1 = enemy just moved
 * pfx:      "Wild " or "" (enemy/player prefix for wild battle)
 * name:     mover's mon name (no prefix)
 * selected: wPlayerSelectedMove or wEnemySelectedMove
 * move_id:  wPlayerMoveNum or wEnemyMoveNum (may be 0 for bide/no-move)
 *
 * Shows:
 *   CANNOT_MOVE:           "[name]\ncan't move!"
 *   block status msg:      per BSTAT_MSG_* text
 *   pre-move msg + move:   "[pre-msg]!\f[name] used [move]!" (+ crit/miss/eff)
 *   normal move:           "[name] used [move]!" (+ crit/miss/eff)
 *   move_id == 0, no msg:  (nothing shown) */
static void bui_show_after_move(int whose, const char *pfx, const char *name,
                                 uint8_t selected, uint8_t move_id,
                                 uint8_t crit, uint8_t missed, uint8_t eff) {
    battle_status_msg_t smsg = (whose == 0) ? Battle_GetPlayerStatusMsg()
                                            : Battle_GetEnemyStatusMsg();
    battle_status_msg_t pmsg = (whose == 0) ? Battle_GetPlayerPreStatusMsg()
                                            : Battle_GetEnemyPreStatusMsg();

    /* --- CANNOT_MOVE (trapped by opposing multi-turn move) --- */
    if (selected == CANNOT_MOVE) {
        snprintf(s_msg_buf, sizeof(s_msg_buf), "%s\ncan't move!", name);
        Text_ShowASCII(s_msg_buf);
        return;
    }

    /* --- Block message: move not used --- */
    if (smsg != BSTAT_MSG_NONE) {
        switch (smsg) {
        case BSTAT_MSG_FAST_ASLEEP:
            snprintf(s_msg_buf, sizeof(s_msg_buf), "%s\nis fast asleep!", name);
            break;
        case BSTAT_MSG_WOKE_UP:
            snprintf(s_msg_buf, sizeof(s_msg_buf), "%s\nwoke up!", name);
            break;
        case BSTAT_MSG_FROZEN:
            snprintf(s_msg_buf, sizeof(s_msg_buf), "%s\nis frozen solid!", name);
            break;
        case BSTAT_MSG_CANT_MOVE:
            snprintf(s_msg_buf, sizeof(s_msg_buf), "%s\ncan't move!", name);
            break;
        case BSTAT_MSG_FLINCHED:
            snprintf(s_msg_buf, sizeof(s_msg_buf), "%s\nflinched!", name);
            break;
        case BSTAT_MSG_MUST_RECHARGE:
            snprintf(s_msg_buf, sizeof(s_msg_buf), "%s\nmust recharge!", name);
            break;
        case BSTAT_MSG_HURT_ITSELF:
            snprintf(s_msg_buf, sizeof(s_msg_buf),
                     "It hurt itself\nin its confusion!");
            break;
        case BSTAT_MSG_FULLY_PARALYZED:
            snprintf(s_msg_buf, sizeof(s_msg_buf), "%s's\nfully paralyzed!", name);
            break;
        default:
            s_msg_buf[0] = '\0';
            break;
        }
        if (s_msg_buf[0]) Text_ShowASCII(s_msg_buf);
        return;
    }

    /* --- Build pre-move prefix (confused/disable-wore-off/confusion-wore-off) --- */
    char pre_buf[64] = "";
    if (pmsg != BSTAT_MSG_NONE) {
        switch (pmsg) {
        case BSTAT_MSG_IS_CONFUSED:
            snprintf(pre_buf, sizeof(pre_buf), "%s\nis confused!", name);
            break;
        case BSTAT_MSG_DISABLED_NO_MORE:
            snprintf(pre_buf, sizeof(pre_buf), "%s's\ndisabled no more!", name);
            break;
        case BSTAT_MSG_CONFUSED_NO_MORE:
            snprintf(pre_buf, sizeof(pre_buf), "%s's\nconfused no more!", name);
            break;
        default:
            break;
        }
    }

    /* --- Normal move text (possibly prefixed by pre-msg) --- */
    if (move_id == 0) {
        /* Bide accumulation or no move — only show pre-msg if present */
        if (pre_buf[0]) {
            snprintf(s_msg_buf, sizeof(s_msg_buf), "%s", pre_buf);
            Text_ShowASCII(s_msg_buf);
        }
        return;
    }

    const char *mn = (move_id < 166 && gMoveNames[move_id])
                     ? gMoveNames[move_id] : "?????";

    /* Build the move-use line */
    char move_buf[256];
    int pos = snprintf(move_buf, sizeof(move_buf),
                       "%s%s\nused %s!", pfx, name, mn);
    if (missed) {
        pos += snprintf(move_buf + pos, sizeof(move_buf) - pos, "\fMiss!");
    } else if (eff == 0 && move_id < 166 && gMoves[move_id].power > 0) {
        /* Only show "Doesn't affect!" for damaging moves — status moves
         * (power == 0) skip damage calc so wDamageMultipliers is stale. */
        pos += snprintf(move_buf + pos, sizeof(move_buf) - pos,
                        "\fDoesn't affect!");
    } else {
        /* Each of these matches a separate PrintText call in the original:
         * PrintCriticalOHKOText → its own box, then DisplayEffectiveness → its own box. */
        if (crit == 1)
            pos += snprintf(move_buf + pos, sizeof(move_buf) - pos,
                            "\fCritical hit!");
        else if (crit == 2)
            pos += snprintf(move_buf + pos, sizeof(move_buf) - pos,
                            "\fOne-hit KO!");
        if (eff >= 20)
            pos += snprintf(move_buf + pos, sizeof(move_buf) - pos,
                            "\fIt's super\neffective!");
        else if (eff > 0 && eff <= 5)
            pos += snprintf(move_buf + pos, sizeof(move_buf) - pos,
                            "\fIt's not very\neffective...");
    }
    /* Leech Seed landing: append "TARGET was seeded!" on a new page.
     * Mirrors LeechSeedEffect_ (leech_seed.asm:26): "PrintText WasSeededText". */
    if (!missed && move_id > 0 && move_id < 166 &&
        gMoves[move_id].effect == EFFECT_LEECH_SEED) {
        const char *tname, *tpfx;
        if (whose == 0) {
            tname = Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]);
            tpfx  = (wIsInBattle == 1) ? "Wild " : "Foe ";
        } else {
            tname = Pokemon_GetName(gSpeciesToDex[wBattleMon.species]);
            tpfx  = "";
        }
        pos += snprintf(move_buf + pos, sizeof(move_buf) - (size_t)pos,
                        "\f%s%s\nwas seeded!", tpfx, tname);
    }

    /* Drain HP: queue as a separate dialog shown after this move's text closes.
     * Mirrors drain_hp.asm: PrintText fires AFTER UpdateHPBar2 + DrawHUDs,
     * not combined with the move-use text.
     * SuckedHealthText  = "Sucked health from\nTARGET!"
     * DreamWasEatenText = "TARGET's\ndream was eaten!" */
    s_drain_text[0] = '\0';
    if (!missed && move_id > 0 && move_id < 166) {
        uint8_t eff_id = gMoves[move_id].effect;
        if (eff_id == EFFECT_DRAIN_HP || eff_id == EFFECT_DREAM_EATER) {
            const char *tname, *tpfx;
            if (whose == 0) {
                tname = Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]);
                tpfx  = (wIsInBattle == 1) ? "Wild " : "Foe ";
            } else {
                tname = Pokemon_GetName(gSpeciesToDex[wBattleMon.species]);
                tpfx  = "";
            }
            if (eff_id == EFFECT_DREAM_EATER)
                snprintf(s_drain_text, sizeof(s_drain_text),
                         "%s%s's\ndream was eaten!", tpfx, tname);
            else
                snprintf(s_drain_text, sizeof(s_drain_text),
                         "Sucked health\nfrom %s%s!", tpfx, tname);
        }
    }

    /* ---- Post-effect status / stat-change messages ----------------------
     * Compare current state to s_pre snapshot to detect what the move applied.
     * Only runs when the move landed (!missed) and isn't a "Doesn't affect!" case. */
    if (!missed && !(eff == 0 && move_id < 166 && gMoves[move_id].power > 0)) {
        static const char *kStatNames[6] = {
            "ATTACK", "DEFENSE", "SPEED", "SPECIAL", "ACCURACY", "EVASION"
        };

        /* Resolve target and attacker names for this move. */
        const char *tpfx2, *tname2;   /* target  */
        const char *apfx2, *aname2;   /* attacker (user) */
        uint8_t cur_tgt_status, cur_tgt_bstat1;
        uint8_t *cur_tgt_mods, *cur_atk_mods;
        if (whose == 0) {
            tpfx2  = (wIsInBattle == 1) ? "Wild " : "Foe ";
            tname2 = Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]);
            apfx2  = "";
            aname2 = Pokemon_GetName(gSpeciesToDex[wBattleMon.species]);
            cur_tgt_status = wEnemyMon.status;
            cur_tgt_bstat1 = wEnemyBattleStatus1;
            cur_tgt_mods   = wEnemyMonStatMods;
            cur_atk_mods   = wPlayerMonStatMods;
        } else {
            tpfx2  = "";
            tname2 = Pokemon_GetName(gSpeciesToDex[wBattleMon.species]);
            apfx2  = (wIsInBattle == 1) ? "Wild " : "Foe ";
            aname2 = Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]);
            cur_tgt_status = wBattleMon.status;
            cur_tgt_bstat1 = wPlayerBattleStatus1;
            cur_tgt_mods   = wPlayerMonStatMods;
            cur_atk_mods   = wEnemyMonStatMods;
        }

        /* Newly applied status on target */
        uint8_t sdiff = cur_tgt_status ^ s_pre.target_status;
        if ((sdiff & STATUS_SLP_MASK) && IS_ASLEEP(cur_tgt_status))
            pos += snprintf(move_buf + pos, sizeof(move_buf) - (size_t)pos,
                            "\f%s%s\nfell asleep!", tpfx2, tname2);
        else if ((sdiff & STATUS_PSN) && IS_POISONED(cur_tgt_status))
            pos += snprintf(move_buf + pos, sizeof(move_buf) - (size_t)pos,
                            "\f%s%s\nwas poisoned!", tpfx2, tname2);
        else if ((sdiff & STATUS_BRN) && IS_BURNED(cur_tgt_status))
            pos += snprintf(move_buf + pos, sizeof(move_buf) - (size_t)pos,
                            "\f%s%s\nwas burned!", tpfx2, tname2);
        else if ((sdiff & STATUS_FRZ) && IS_FROZEN(cur_tgt_status))
            pos += snprintf(move_buf + pos, sizeof(move_buf) - (size_t)pos,
                            "\f%s%s\nwas frozen solid!", tpfx2, tname2);
        else if ((sdiff & STATUS_PAR) && IS_PARALYZED(cur_tgt_status))
            pos += snprintf(move_buf + pos, sizeof(move_buf) - (size_t)pos,
                            "\f%s%s\nis paralyzed!", tpfx2, tname2);

        /* Newly confused target */
        uint8_t bdiff = cur_tgt_bstat1 ^ s_pre.target_bstat1;
        if ((bdiff & (1u << BSTAT1_CONFUSED)) && (cur_tgt_bstat1 & (1u << BSTAT1_CONFUSED)))
            pos += snprintf(move_buf + pos, sizeof(move_buf) - (size_t)pos,
                            "\f%s%s\nbecame confused!", tpfx2, tname2);

        /* Target stat fell */
        for (int si = 0; si < 6; si++) {
            if (cur_tgt_mods[si] < s_pre.target_stat_mods[si]) {
                int delta = (int)s_pre.target_stat_mods[si] - cur_tgt_mods[si];
                if (delta >= 2)
                    pos += snprintf(move_buf + pos, sizeof(move_buf) - (size_t)pos,
                                    "\f%s%s's\n%s sharply fell!", tpfx2, tname2, kStatNames[si]);
                else
                    pos += snprintf(move_buf + pos, sizeof(move_buf) - (size_t)pos,
                                    "\f%s%s's\n%s fell!", tpfx2, tname2, kStatNames[si]);
                break;
            }
        }

        /* Attacker stat rose */
        for (int si = 0; si < 6; si++) {
            if (cur_atk_mods[si] > s_pre.attacker_stat_mods[si]) {
                int delta = (int)cur_atk_mods[si] - s_pre.attacker_stat_mods[si];
                if (delta >= 2)
                    pos += snprintf(move_buf + pos, sizeof(move_buf) - (size_t)pos,
                                    "\f%s%s's\n%s rose sharply!", apfx2, aname2, kStatNames[si]);
                else
                    pos += snprintf(move_buf + pos, sizeof(move_buf) - (size_t)pos,
                                    "\f%s%s's\n%s rose!", apfx2, aname2, kStatNames[si]);
                break;
            }
        }
    }
    (void)pos;

    if (pre_buf[0]) {
        snprintf(s_msg_buf, sizeof(s_msg_buf), "%s\f%s", pre_buf, move_buf);
    } else {
        snprintf(s_msg_buf, sizeof(s_msg_buf), "%s", move_buf);
    }
    Text_ShowASCII(s_msg_buf);
}

/* Snapshot target and attacker state before executing a move. */
static void bui_snapshot_pre(int whose) {
    if (whose == 0) {
        s_pre.target_status = wEnemyMon.status;
        s_pre.target_bstat1 = wEnemyBattleStatus1;
        memcpy(s_pre.target_stat_mods,   wEnemyMonStatMods,  6);
        memcpy(s_pre.attacker_stat_mods, wPlayerMonStatMods, 6);
    } else {
        s_pre.target_status = wBattleMon.status;
        s_pre.target_bstat1 = wPlayerBattleStatus1;
        memcpy(s_pre.target_stat_mods,   wPlayerMonStatMods, 6);
        memcpy(s_pre.attacker_stat_mods, wEnemyMonStatMods,  6);
    }
}

/* ---- Move animation helpers --------------------------------------- */

/* Show or hide the enemy sprite (OAM entries 0..48) for the blink effect. */
static void bui_set_enemy_oam_visible(int visible) {
    for (int ty = 0; ty < 7; ty++)
        for (int tx = 0; tx < 7; tx++) {
            int idx = ENEMY_SPR_OAM_BASE + ty * 7 + tx;
            wShadowOAM[idx].y = visible
                ? (uint8_t)(ENEMY_SPR_PX_Y + ty * 8 + 16)
                : 0;
        }
}

/* Update enemy sprite OAM for faint slide-down animation step N (1-7).
 * Mirrors SlideDownFaintedMonPic (engine/battle/core.asm:1181).
 *
 * Original: each step shifts BG tile rows down by 1 row and blanks the top.
 * OAM equivalent: at step N, content from original row ty is displayed at
 * screen row (ty + N).  Rows ty > (6 - N) have scrolled off the sprite area
 * bottom and are hidden (y = 0). */
static void bui_enemy_faint_oam(int step) {
    for (int ty = 0; ty < 7; ty++) {
        for (int tx = 0; tx < 7; tx++) {
            int idx = ENEMY_SPR_OAM_BASE + ty * 7 + tx;
            if (ty <= 6 - step) {
                wShadowOAM[idx].y = (uint8_t)(ENEMY_SPR_PX_Y + (ty + step) * 8 + OAM_Y_OFS);
            } else {
                wShadowOAM[idx].y = 0;  /* scrolled off sprite area bottom — hide */
            }
        }
    }
}

/* Determine animation type and total frame count from the just-executed move.
 * whose: 0=player moved, 1=enemy moved.
 * Mirrors GetPlayerAnimationType / GetEnemyAnimationType from core.asm. */
static void bui_setup_anim(int whose) {
    uint8_t move_id = (whose == 0) ? wPlayerMoveNum : wEnemyMoveNum;
    s_anim_frame    = 0;

    if (wMoveMissed || move_id == 0) {
        /* Missed or no move — no applying-attack animation */
        s_anim_type  = 0;
        s_anim_total = 0;
        return;
    }

    uint8_t effect = (move_id < 166) ? gMoves[move_id].effect : 0;

    if (whose == 0) {
        /* Player's move:
         *   effect==0 → ANIMATIONTYPE_BLINK_ENEMY_MON_SPRITE (type 4)
         *   effect!=0 → ANIMATIONTYPE_SHAKE_SCREEN_HORIZONTALLY_LIGHT (type 5, b=2) */
        if (effect == 0) {
            s_anim_type  = 4;
            s_anim_total = 60;  /* 6 blinks × 10 frames */
        } else {
            s_anim_type  = 5;
            s_anim_total = 18;  /* 2 steps × 9 frames */
        }
    } else {
        /* Enemy's move:
         *   effect==0 → ANIMATIONTYPE_SHAKE_SCREEN_VERTICALLY (type 1, b=8)
         *   effect!=0 → ANIMATIONTYPE_SHAKE_SCREEN_HORIZONTALLY_HEAVY (type 2, b=8) */
        if (effect == 0) {
            s_anim_type  = 1;
            s_anim_total = 48;  /* 8 steps × 6 frames */
        } else {
            s_anim_type  = 2;
            s_anim_total = 72;  /* 8 steps × 9 frames */
        }
    }
}

/* Set up HP bar scroll animation after a move is executed.
 * whose==0 → player attacked enemy (animate enemy bar at col 4, row 2).
 * whose==1 → enemy attacked player (animate player bar at col 12, row 9).
 * s_hp_pre_hp/max must be set BEFORE the execute call. */
static void bui_setup_hp_anim(int whose) {
    s_hp_anim_who  = whose;
    s_hp_old_px    = calc_hp_pixels(s_hp_pre_hp, s_hp_pre_max);
    if (whose == 0)
        s_hp_new_px = calc_hp_pixels(wEnemyMon.hp, wEnemyMon.max_hp);
    else
        s_hp_new_px = calc_hp_pixels(wBattleMon.hp, wBattleMon.max_hp);
    s_hp_cur_px    = s_hp_old_px;
    s_hp_half_frame = 0;
}

/* Finish the first move: draw HUDs and advance state.
 * Move text was already shown in bui_exec_first_move before the animation. */
static void bui_finish_first_move(void) {
    bui_draw_enemy_hud();
    bui_draw_player_hud();
    bui_state = BUI_EXEC_MOVE_B;
}

/* Finish the second move: draw HUDs and advance state.
 * Move text was already shown in bui_exec_second_move before the animation. */
static void bui_finish_second_move(void) {
    bui_draw_enemy_hud();
    bui_draw_player_hud();
    bui_state = BUI_TURN_END;
}

/* Execute the first mover's move, update HUDs, show text.
 * Caller must have called Battle_TurnPrepare() and set wPlayerSelectedMove.
 * Sets bui_state = BUI_EXEC_MOVE_B. */
static void bui_exec_first_move(void) {
    const char *wild_pfx = (wIsInBattle == 1) ? "Wild " : "";

    if (s_player_first) {
        snprintf(s_name_a, sizeof(s_name_a), "%s",
                 Pokemon_GetName(gSpeciesToDex[wBattleMon.species]));
        s_pfx_a[0] = '\0';
        snprintf(s_name_b, sizeof(s_name_b), "%s",
                 Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]));
        snprintf(s_pfx_b, sizeof(s_pfx_b), "%s", wild_pfx);

        hWhoseTurn = 0;
        bui_snapshot_pre(0);
        s_hp_pre_hp  = wEnemyMon.hp;
        s_hp_pre_max = wEnemyMon.max_hp;
        Battle_ExecutePlayerMove();
        bui_setup_anim(0);
        bui_setup_hp_anim(0);
    } else {
        snprintf(s_name_a, sizeof(s_name_a), "%s",
                 Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]));
        snprintf(s_pfx_a, sizeof(s_pfx_a), "%s", wild_pfx);
        snprintf(s_name_b, sizeof(s_name_b), "%s",
                 Pokemon_GetName(gSpeciesToDex[wBattleMon.species]));
        s_pfx_b[0] = '\0';

        hWhoseTurn = 1;
        bui_snapshot_pre(1);
        s_hp_pre_hp  = wBattleMon.hp;
        s_hp_pre_max = wBattleMon.max_hp;
        Battle_ExecuteEnemyMove();
        bui_setup_anim(1);
        bui_setup_hp_anim(1);
    }
    s_anim_first = 1;
    /* Show "X used Y!" + effects BEFORE animation — mirrors DisplayUsedMoveText
     * in core.asm which is called before PlayMoveAnimation.
     * KeepTiles: original uses text_end (no button wait), tiles stay in VRAM
     * so the dialog box remains visible while the animation plays over it. */
    Text_KeepTilesOnClose();
    if (s_player_first)
        bui_show_after_move(0, s_pfx_a, s_name_a, wPlayerSelectedMove, wPlayerMoveNum,
                            wCriticalHitOrOHKO, wMoveMissed, wDamageMultipliers & 0x7F);
    else
        bui_show_after_move(1, s_pfx_a, s_name_a, wEnemySelectedMove, wEnemyMoveNum,
                            wCriticalHitOrOHKO, wMoveMissed, wDamageMultipliers & 0x7F);
    bui_state    = BUI_MOVE_ANIM;
}

/* Post-animation enemy faint: game state, HUD refresh, text, exp drain.
 * Called after BUI_ENEMY_FAINT_ANIM completes (or directly for residual faint).
 * Mirrors FaintEnemyPokemon + HandleEnemyMonFainted in engine/battle/core.asm. */
static void bui_handle_enemy_fainted(void) {
    /* Capture fainted mon's name before Battle_HandleEnemyMonFainted can replace it.
     * Using wEnemyMon directly is correct regardless of which s_name_a/b slot the
     * enemy occupies this turn. */
    const char *faint_pfx  = (wIsInBattle == 2) ? "" : "Wild ";
    const char *faint_name = Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]);

    Audio_PlaySFX_Faint();
    Battle_HandleEnemyMonFainted();   /* sets wBattleResult, awards exp, queues exp texts */
    /* For mid-battle trainer replacement (OUTCOME_NONE), Battle_HandleEnemyMonFainted
     * already swapped in the new mon.  Don't draw the enemy HUD yet — it will be
     * drawn in BUI_ENEMY_SEND_OUT stage 2 when the Pokémon actually materialises.
     * For wild/trainer victory the fainted mon is still in wEnemyMon, so drawing
     * here (showing 0 HP) is correct. */
    if (wBattleResult != BATTLE_OUTCOME_NONE)
        bui_draw_enemy_hud();
    bui_draw_player_hud();

    /* Play wild victory jingle now; trainer victory music fires later in
     * BUI_EXP_DRAIN just before showing "PLAYER defeated TRAINER!" —
     * matching PlayBattleVictoryMusic order in core.asm:TrainerBattleVictory. */
    if (wBattleResult == BATTLE_OUTCOME_WILD_VICTORY)
        Music_Play(MUSIC_DEFEATED_WILD_MON);

    /* Show "[Enemy] fainted!" — exp texts drained afterward in BUI_EXP_DRAIN */
    snprintf(s_msg_buf, sizeof(s_msg_buf), "%s%s\nfainted!", faint_pfx, faint_name);
    Text_ShowASCII(s_msg_buf);

    if (wBattleResult == BATTLE_OUTCOME_WILD_VICTORY) {
        bui_exp_dest    = BUI_END;
        s_exp_suffix[0] = '\0';
        s_exp_suffix2[0] = '\0';
    } else if (wBattleResult == BATTLE_OUTCOME_TRAINER_VICTORY) {
        /* Mirrors TrainerDefeatedText + MoneyForWinningText (core.asm:915-949).
         * Decode player name from GB charset (0x80='A'..0x99='Z'). */
        char player_ascii[NAME_LENGTH] = "RED";
        for (int i = 0; i < NAME_LENGTH - 1; i++) {
            uint8_t c = wPlayerName[i];
            if (c == 0x50) break;
            if (c >= 0x80 && c <= 0x99) player_ascii[i] = (char)('A' + c - 0x80);
            else if (c >= 0xA0 && c <= 0xB9) player_ascii[i] = (char)('a' + c - 0xA0);
            else { player_ascii[i] = '?'; }
            player_ascii[i + 1] = '\0';
        }
        /* Award prize money (mirrors AddBCDPredef in TrainerBattleVictory). */
        uint32_t money = (uint32_t)(
            ((wPlayerMoney[0] >> 4) & 0xF) * 100000u +
            (wPlayerMoney[0] & 0xF)        * 10000u  +
            ((wPlayerMoney[1] >> 4) & 0xF) * 1000u   +
            (wPlayerMoney[1] & 0xF)        * 100u     +
            ((wPlayerMoney[2] >> 4) & 0xF) * 10u      +
            (wPlayerMoney[2] & 0xF));
        money += wAmountMoneyWon;
        if (money > 999999u) money = 999999u;
        wPlayerMoney[0] = (uint8_t)(((money / 100000u) << 4) | ((money / 10000u) % 10u));
        wPlayerMoney[1] = (uint8_t)((((money / 1000u) % 10u) << 4) | ((money / 100u) % 10u));
        wPlayerMoney[2] = (uint8_t)((((money / 10u) % 10u) << 4) | (money % 10u));
        /* TrainerDefeatedText: "[PLAYER] defeated\n[TRAINER CLASS]!" */
        int tc2 = (int)gEngagedTrainerClass - 1;
        const char *tname = (tc2 >= 0 && tc2 < NUM_TRAINERS) ? gTrainerClassNames[tc2] : "TRAINER";
        bui_exp_dest = BUI_TRAINER_VICTORY_SLIDE;
        snprintf(s_exp_suffix, sizeof(s_exp_suffix),
                 "%s defeated\n%s!", player_ascii, tname);
        s_exp_suffix2[0] = '\0';
        /* Money text shown after trainer slide-in + defeat quote */
        snprintf(s_trainer_money_text, sizeof(s_trainer_money_text),
                 "%s got \xA5%lu\nfor winning!", player_ascii, (unsigned long)wAmountMoneyWon);
        s_slide_cx      = 48;  /* 6 tile-columns × 8px — matches scrollLoop c=1..6 */
        s_victory_timer = 0;
    } else {
        /* Trainer has more mons — enemy was already replaced by handler.
         * Show "Foe sent out X!" after exp texts are drained. */
        const char *new_name = Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]);
        s_grow_stage = 0; s_grow_frame = 0; s_mid_battle_send = 1;
        bui_exp_dest = BUI_ENEMY_SEND_OUT;
        snprintf(s_exp_suffix, sizeof(s_exp_suffix), "Foe sent out\n%s!", new_name);
    }
    bui_state = BUI_EXP_DRAIN;
}

/* Start the slide-down faint animation for the enemy sprite, then call
 * bui_handle_enemy_fainted() when it completes.
 * Mirrors SlideDownFaintedMonPic + FaintEnemyPokemon in core.asm:732. */
static void bui_start_enemy_faint_anim(void) {
    s_faint_step  = 0;    /* first tick fires step 1 immediately (timer=0) */
    s_faint_timer = 0;
    bui_state = BUI_ENEMY_FAINT_ANIM;
}

/* Handle player mon fainted: call handler, show text, set PLAYER_FAINTED. */
static void bui_handle_player_fainted(void) {
    /* PlayCry — mirrors HandlePlayerMonFainted calling PlayCry(wBattleMonSpecies)
     * before showing the "[mon] fainted!" text. */
    Audio_PlayCry(wBattleMon.species);
    Audio_PlaySFX_Faint();
    Battle_HandlePlayerMonFainted();
    bui_draw_enemy_hud();
    bui_draw_player_hud();
    snprintf(s_msg_buf, sizeof(s_msg_buf), "%s\nfainted!", s_name_b);
    Text_ShowASCII(s_msg_buf);
    bui_state = BUI_PLAYER_FAINTED;
}

/* ---- Residual message helper --------------------------------------- */
/* Formats the burn/poison/leech-seed message for the current mover.
 * Uses hWhoseTurn to determine which side's status flags to read.
 * buf[0] == '\0' means no message. */
static void bui_format_residual_msg(char *buf, int bufsz) {
    buf[0] = '\0';
    const char *mon_name, *pfx;
    uint8_t status, bstat2;

    if (hWhoseTurn == 0) {
        status   = wBattleMon.status;
        bstat2   = wPlayerBattleStatus2;
        mon_name = Pokemon_GetName(gSpeciesToDex[wBattleMon.species]);
        pfx      = "";
    } else {
        status   = wEnemyMon.status;
        bstat2   = wEnemyBattleStatus2;
        mon_name = Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]);
        pfx      = (wIsInBattle == 1) ? "Wild " : "Foe ";
    }

    if (status & STATUS_BRN)
        snprintf(buf, (size_t)bufsz, "%s%s\nwas hurt by its burn!", pfx, mon_name);
    else if (status & STATUS_PSN)
        snprintf(buf, (size_t)bufsz, "%s%s\nwas hurt by poison!", pfx, mon_name);

    /* Leech seed overwrites burn/poison line — original shows it separately,
     * but for now one message per residual tick is fine. */
    if (bstat2 & (1u << BSTAT2_SEEDED))
        snprintf(buf, (size_t)bufsz, "LEECH SEED\nsaps %s%s!", pfx, mon_name);
}

/* Execute the second mover's move and show its text.
 * Sets bui_state = BUI_TURN_END. */
static void bui_exec_second_move(void) {
    if (s_player_first) {
        hWhoseTurn = 1;
        bui_snapshot_pre(1);
        s_hp_pre_hp  = wBattleMon.hp;
        s_hp_pre_max = wBattleMon.max_hp;
        Battle_ExecuteEnemyMove();
        bui_setup_anim(1);
        bui_setup_hp_anim(1);
    } else {
        hWhoseTurn = 0;
        bui_snapshot_pre(0);
        s_hp_pre_hp  = wEnemyMon.hp;
        s_hp_pre_max = wEnemyMon.max_hp;
        Battle_ExecutePlayerMove();
        bui_setup_anim(0);
        bui_setup_hp_anim(0);
    }
    s_anim_first = 0;
    /* Show "X used Y!" + effects BEFORE animation — mirrors DisplayUsedMoveText
     * in core.asm which is called before PlayMoveAnimation. */
    Text_KeepTilesOnClose();
    if (s_player_first)
        bui_show_after_move(1, s_pfx_b, s_name_b, wEnemySelectedMove, wEnemyMoveNum,
                            wCriticalHitOrOHKO, wMoveMissed, wDamageMultipliers & 0x7F);
    else
        bui_show_after_move(0, s_pfx_b, s_name_b, wPlayerSelectedMove, wPlayerMoveNum,
                            wCriticalHitOrOHKO, wMoveMissed, wDamageMultipliers & 0x7F);
    bui_state    = BUI_MOVE_ANIM;
}

/* ---- Public API ---------------------------------------------------- */

void BattleUI_Restore(void) {
    /* Reset the UI state machine to BUI_DRAW_HUD: the normal idle state
     * after both mons are on screen.  BUI_DRAW_HUD reloads sprites and
     * redraws the HUD, then transitions to BUI_PLAYER_ACTION_SELECTION,
     * reconstructing the full battle screen from the restored WRAM state. */
    bui_state = BUI_DRAW_HUD;
}

void BattleUI_Enter(void) {
    /* SlidePlayerAndEnemySilhouettesOnScreen setup:
     * Music is already playing (started before the transition in game.c).
     * Screen is black after BattleTransition_Tick() completed.
     * Load sprite tiles, clear screen, set initial off-screen OAM positions. */
    bui_state       = BUI_SLIDE_IN;
    s_slide_cx      = 144;   /* SCX equivalent: 0x90, counts down by 2 per frame */
    bui_cursor      = 0;
    s_drain_text[0] = '\0';
    s_anim_type     = 0;
    s_anim_frame    = 0;
    s_anim_total    = 0;
    s_caught_species = 0;
    s_caught_dex = 0;
    s_caught_new_entry = 0;
    s_caught_sent_to_box = 0;
    s_caught_dex_started = 0;
    Display_SetShakeOffset(0, 0);
    gScrollPxX = 0;
    gScrollPxY = 0;
    bui_clear_rows(0, SCREEN_HEIGHT - 1);
    /* Draw empty text box border at bottom (rows 12-17) before the slide starts.
     * Matches DisplayTextBoxID(MESSAGE_BOX) called at line 13 of
     * SlidePlayerAndEnemySilhouettesOnScreen (engine/battle/core.asm). */
    Text_DrawEmptyBox();
    memset(wShadowOAM, 0, sizeof(wShadowOAM));
    Font_Load();          /* restore font tiles 128-255 (battle_transition clobbered slot $FF) */
    Font_LoadHudTiles();
    /* OBP0=0xE4: normal 4-shade sprite palette */
    Display_SetPalette(0xE4, 0xE4, 0xE4);

    uint8_t e_dex = gSpeciesToDex[wEnemyMon.species];
    uint8_t p_dex = gSpeciesToDex[wBattleMon.species];

    /* Load enemy front sprite into sprite tile slots 0-48.
     * Trainer battles: load the trainer's front sprite instead of a pokemon. */
    if (wIsInBattle == 2) {
        int tc = (int)gEngagedTrainerClass - 1;
        if (tc >= 0 && tc < NUM_TRAINERS)
            for (int i = 0; i < TRAINER_CANVAS_TILES; i++)
                Display_LoadSpriteTile((uint8_t)(ENEMY_SPR_TILE_BASE + i),
                                       gTrainerFrontSprite[tc][i]);
    } else if (e_dex > 0 && e_dex <= 151) {
        for (int i = 0; i < POKEMON_FRONT_CANVAS_TILES; i++)
            Display_LoadSpriteTile((uint8_t)(ENEMY_SPR_TILE_BASE + i),
                                   gPokemonFrontSprite[e_dex][i]);
    }

    /* Load Red's trainer back sprite into sprite tile slots 49-97 for OAM use during slide.
     * Matches LoadPlayerBackPic (core.asm:6202) which loads RedPicBack (not the pokemon).
     * The pokemon back sprite is loaded later in BUI_SEND_OUT via bui_load_sprites(). */
    for (int i = 0; i < 49; i++)
        Display_LoadSpriteTile((uint8_t)(PLAYER_SLIDE_TILE_BASE + i), kRedBackSprite[i]);

    /* Enemy: initial OAM off-screen LEFT — enters from left, slides right.
     * Mirrors pokered SCX $90→$00: BG tiles at a fixed position appear to enter
     * from the left edge as the viewport shifts right-to-left.
     * Approximated here with OAM: x starts below 0 (clamped to 0 = hidden). */
    if (e_dex > 0 && e_dex <= 151) {
        for (int ty = 0; ty < 7; ty++) {
            for (int tx = 0; tx < 7; tx++) {
                int idx = ENEMY_SPR_OAM_BASE + ty * 7 + tx;
                int raw_x = ENEMY_SPR_PX_X + tx * 8 + OAM_X_OFS - 144;
                wShadowOAM[idx].y     = (uint8_t)(ENEMY_SPR_PX_Y + ty * 8 + OAM_Y_OFS);
                wShadowOAM[idx].x     = (uint8_t)(raw_x < 0 ? 0 : raw_x);
                wShadowOAM[idx].tile  = (uint8_t)(ENEMY_SPR_TILE_BASE + ty * 7 + tx);
                wShadowOAM[idx].flags = 0;
            }
        }
    }

    /* Player back: initial OAM off-screen right (PLAYER_SPR_COL*8 + 144 offset). */
    if (p_dex > 0 && p_dex <= 151) {
        for (int ty = 0; ty < 7; ty++) {
            for (int tx = 0; tx < 7; tx++) {
                int idx = PLAYER_SLIDE_OAM_BASE + ty * 7 + tx;
                wShadowOAM[idx].y     = (uint8_t)(PLAYER_SPR_ROW * 8 + ty * 8 + 16);
                wShadowOAM[idx].x     = (uint8_t)((PLAYER_SPR_COL * 8 + 144 + tx * 8 + 8) & 0xFF);
                wShadowOAM[idx].tile  = (uint8_t)(PLAYER_SLIDE_TILE_BASE + ty * 7 + tx);
                wShadowOAM[idx].flags = 0;
            }
        }
    }
}

int BattleUI_IsActive(void) {
    return bui_state != BUI_INACTIVE;
}

int BattleUI_GetState(void) {
    return (int)bui_state;
}

void BattleUI_Tick(void) {
    switch (bui_state) {

    case BUI_INACTIVE:
        return;

    /* ---- Slide in: SlidePlayerAndEnemySilhouettesOnScreen ---------- *
     * Enemy slides LEFT→RIGHT (enters from left): x = final_x - cx.     *
     * Player slides RIGHT→LEFT (enters from right): x = final_x + cx.   *
     * Both over 72 frames (s_slide_cx: 144 → 0, step 2/frame).          *
     * Mirrors pokered: enemy via BG SCX $90→$00, player via OAM x decr. */
    case BUI_SLIDE_IN: {
        int cx = s_slide_cx;
        uint8_t e_dex2 = gSpeciesToDex[wEnemyMon.species];

        /* First frame: draw empty textbox — mirrors DisplayTextBoxID call in
         * SlidePlayerAndEnemySilhouettesOnScreen before the slide loop begins. */
        if (cx == 144)
            bui_draw_box();

        /* Enemy: slides right as cx decreases (enters from left edge).
         * Always runs for trainer battles (trainer sprite loaded in BattleUI_Enter). */
        if (wIsInBattle == 2 || (e_dex2 > 0 && e_dex2 <= 151)) {
            for (int ty = 0; ty < 7; ty++)
                for (int tx = 0; tx < 7; tx++) {
                    int idx = ENEMY_SPR_OAM_BASE + ty * 7 + tx;
                    int raw_x = ENEMY_SPR_PX_X + tx * 8 + OAM_X_OFS - cx;
                    wShadowOAM[idx].x = (uint8_t)(raw_x < 0 ? 0 : raw_x);
                }
        }
        /* Player (Red's back sprite): slides left as cx decreases (enters from right edge). */
        for (int ty = 0; ty < 7; ty++)
            for (int tx = 0; tx < 7; tx++) {
                int idx = PLAYER_SLIDE_OAM_BASE + ty * 7 + tx;
                wShadowOAM[idx].x = (uint8_t)((PLAYER_SPR_COL * 8 + cx + tx * 8 + OAM_X_OFS) & 0xFF);
            }

        if (cx > 0) {
            s_slide_cx -= 2;
        } else {
            /* Slide complete (cx == 0).
             * Pokeball tiles are loaded to sprite slots 120-123 (POKEBALL_TILE_BASE),
             * well clear of Red's slide sprite tiles 53-101 — so Red's OAM stays active
             * and he remains visible during the "appeared!" text, exactly as in the original
             * where his body stays on the BG tilemap through PrintBeginningBattleText. */
            bui_draw_pokeballs();
            /* "X wants to fight!" / "Wild X appeared!" — mirrors PrintBeginningBattleText. */
            if (wIsInBattle == 2) {
                int tc = (int)gEngagedTrainerClass - 1;
                const char *t_name = (tc >= 0 && tc < NUM_TRAINERS)
                                     ? gTrainerClassNames[tc] : "TRAINER";
                snprintf(s_msg_buf, sizeof(s_msg_buf), "%s\nwants to fight!", t_name);
                Text_ShowASCII(s_msg_buf);
                bui_state = BUI_SEND_OUT;
            } else {
                Audio_PlayCry(wEnemyMon.species);
                const char *e_name2 = Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]);
                snprintf(s_wait_cry_text, sizeof(s_wait_cry_text), "Wild %s\nappeared!", e_name2);
                s_wait_cry_next_state = BUI_SEND_OUT;
                bui_state = BUI_WAIT_CRY;
            }
        }
        break;
    }

    /* ---- "Wild X appeared!" text is up; wait for A then send out --- */
    case BUI_APPEARED:
        /* Unused: BUI_SLIDE_IN transitions directly to BUI_SEND_OUT via Text_ShowASCII. */
        bui_state = BUI_SEND_OUT;
        break;

    /* ---- Send out player's mon: reload Red + begin slide-out ------- *
     * Mirrors core.asm StartBattle: SlideTrainerPicOffScreen then       *
     * SendOutMon (PrintSendOutMonMessage + pokeball grow animation).    */
    case BUI_SEND_OUT: {
        /* Mirrors core.asm LoadScreenTilesFromBuffer1 before SlideTrainerPicOffScreen:
         * that call restores the screen to the post-slide state (no HUD border tiles,
         * text box border present).  We replicate that here by:
         *   1. Hiding pokeballs (OAM off-screen).
         *   2. Clearing the player HUD border tiles added by bui_draw_pokeballs()
         *      so they don't appear during Red's slide-out.
         *   3. Clearing enemy HUD border tiles (trainer battles).
         *   4. Redrawing the empty text box border so it stays visible during the
         *      slide (matches the MESSAGE_BOX that was in the saved screen buffer). */
        bui_hide_pokeballs();
        bui_clear_rect(9, 7, 19, 11);   /* remove player HUD border tiles */
        if (wIsInBattle == 2)
            bui_clear_rect(0, 0, 11, 3); /* remove enemy HUD border tiles (trainer) */
        bui_draw_box();                  /* keep text box border visible during slide */
        s_slide_cx = 0;
        /* Trainer battle: slide trainer pic off right first (EnemySendOutFirstMon).
         * Wild battle: skip straight to sliding Red off. */
        bui_state = (wIsInBattle == 2) ? BUI_ENEMY_SLIDE_OUT : BUI_TRAINER_SLIDE_OUT;
        break;
    }

    /* ---- Enemy slide-out: trainer sprite exits right ------------- *
     * Mirrors SlideTrainerPicOffScreen(hlcoord(18,0), a=8) called    *
     * at the top of EnemySendOutFirstMon (core.asm:1312).            *
     * 4px/tick, ~80px travel until all 7 cols are off-screen.        */
    case BUI_ENEMY_SLIDE_OUT: {
        s_slide_cx += 4;
        for (int ty = 0; ty < 7; ty++)
            for (int tx = 0; tx < 7; tx++) {
                int idx = ENEMY_SPR_OAM_BASE + ty * 7 + tx;
                int nx = ENEMY_SPR_PX_X + tx * 8 + OAM_X_OFS + s_slide_cx;
                wShadowOAM[idx].x = (uint8_t)(nx > 255 ? 0 : nx);
            }
        if (s_slide_cx >= 64) {
            /* Trainer fully off-screen: hide enemy OAM, show "sent out" text */
            bui_set_enemy_oam_visible(0);
            const char *e_name3 = Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]);
            snprintf(s_msg_buf, sizeof(s_msg_buf), "Foe sent out\n%s!", e_name3);
            Text_ShowASCII(s_msg_buf);
            s_grow_stage = 0;
            s_grow_frame = 0;
            s_mid_battle_send = 0;  /* intro path — not a mid-battle replacement */
            bui_state = BUI_ENEMY_SEND_OUT;
        }
        break;
    }

    /* ---- Trainer slide-out: Red exits left ----------------------- *
     * Mirrors SlideTrainerPicOffScreen(hlcoord(1,5), a=9).           *
     * Original: 9 tile-steps × 2 delay frames = ~18 frames total.   *
     * Here: 4px/tick, 64px travel = 16 ticks (≈18 frames at 60fps). */
    case BUI_TRAINER_SLIDE_OUT: {
        s_slide_cx += 4;
        for (int ty = 0; ty < 7; ty++)
            for (int tx = 0; tx < 7; tx++) {
                int idx = PLAYER_SLIDE_OAM_BASE + ty * 7 + tx;
                int nx = PLAYER_SPR_COL * 8 + tx * 8 + OAM_X_OFS - s_slide_cx;
                wShadowOAM[idx].x = (uint8_t)(nx < 0 ? 0 : nx);
            }
        /* tx=6 final_x=64; all sprites off-screen once s_slide_cx >= 64. */
        if (s_slide_cx >= 72) {
            for (int i = 0; i < 49; i++)
                wShadowOAM[PLAYER_SLIDE_OAM_BASE + i].y = 0;
            s_grow_stage = 0;
            s_grow_frame = 0;
            if (wIsInBattle != 2) {
                /* Wild battle: load back sprite and show "Go! X!" now. */
                uint8_t p_dex2 = gSpeciesToDex[wBattleMon.species];
                if (p_dex2 > 0 && p_dex2 <= 151) {
                    for (int i = 0; i < POKEMON_BACK_TILES; i++)
                        Display_LoadTile((uint8_t)(PLAYER_SPR_BG_BASE + i),
                                         gPokemonBackSprite[p_dex2][i]);
                }
                const char *p_name2 = Pokemon_GetName(gSpeciesToDex[wBattleMon.species]);
                snprintf(s_msg_buf, sizeof(s_msg_buf), "Go! %s!", p_name2);
                Text_ShowASCII(s_msg_buf);
            }
            /* Trainer battle: "Go! X!" already shown in BUI_ENEMY_SEND_OUT stage 3. */
            bui_state = BUI_POKEMON_APPEAR;
        }
        break;
    }

    /* ---- Enemy send-out: trainer throws pokemon from pokeball --------- *
     * Trainer battle only. Mirrors EnemySendOutFirstMon (core.asm:1413): *
     * AnimateSendingOutMon at enemy sprite position.                      *
     * Stage 0 ( 8f): single pokeball OAM at enemy center.               *
     * Stage 1 ( 4f): 3×3 center-crop of front sprite via OAM.           *
     * Stage 2 ( 4f): 5×5 center-crop via OAM.                           *
     * Stage 3:       full 7×7 → cry → enemy HUD → queue player send-out */
    case BUI_ENEMY_SEND_OUT: {
        uint8_t e_dex4 = gSpeciesToDex[wEnemyMon.species];
        /* One-time setup on first frame after "sent out" text dismissed. */
        if (s_grow_stage == 0 && s_grow_frame == 0) {
            /* Load enemy pokemon front sprite into tile slots (replaces trainer sprite). */
            if (e_dex4 > 0 && e_dex4 <= 151) {
                for (int i = 0; i < POKEMON_FRONT_CANVAS_TILES; i++)
                    Display_LoadSpriteTile((uint8_t)(ENEMY_SPR_TILE_BASE + i),
                                           gPokemonFrontSprite[e_dex4][i]);
            }
            bui_set_enemy_oam_visible(0);  /* hide full sprite; crops shown below */
            wShadowOAM[POKEBALL_OAM_BASE].y    = (uint8_t)(ENEMY_SPR_PX_Y + 6 * 8 + OAM_Y_OFS);
            wShadowOAM[POKEBALL_OAM_BASE].x    = (uint8_t)(ENEMY_SPR_PX_X + 3 * 8 + OAM_X_OFS);
            wShadowOAM[POKEBALL_OAM_BASE].tile = POKEBALL_TILE_BASE;
            wShadowOAM[POKEBALL_OAM_BASE].flags = 0;
        }
        s_grow_frame++;
        if (s_grow_stage == 0 && s_grow_frame >= 8) {
            Audio_PlaySFX_BallPoof();
            wShadowOAM[POKEBALL_OAM_BASE].y = 0;  /* hide pokeball */
            /* 3×3 center-crop: show tiles (2..4, 2..4) of the 7×7 sprite */
            if (e_dex4 > 0 && e_dex4 <= 151) {
                for (int dty = 0; dty < 3; dty++)
                    for (int dtx = 0; dtx < 3; dtx++) {
                        int oidx = ENEMY_SPR_OAM_BASE + (2 + dty) * 7 + (2 + dtx);
                        wShadowOAM[oidx].y    = (uint8_t)(ENEMY_SPR_PX_Y + (2+dty)*8 + OAM_Y_OFS);
                        wShadowOAM[oidx].x    = (uint8_t)(ENEMY_SPR_PX_X + (2+dtx)*8 + OAM_X_OFS);
                        wShadowOAM[oidx].tile = (uint8_t)(ENEMY_SPR_TILE_BASE + (2+dty)*7 + (2+dtx));
                    }
            }
            s_grow_stage = 1;  s_grow_frame = 0;
        } else if (s_grow_stage == 1 && s_grow_frame >= 4) {
            /* 5×5 center-crop: show tiles (1..5, 1..5) */
            bui_set_enemy_oam_visible(0);
            if (e_dex4 > 0 && e_dex4 <= 151) {
                for (int dty = 0; dty < 5; dty++)
                    for (int dtx = 0; dtx < 5; dtx++) {
                        int oidx = ENEMY_SPR_OAM_BASE + (1+dty)*7 + (1+dtx);
                        wShadowOAM[oidx].y    = (uint8_t)(ENEMY_SPR_PX_Y + (1+dty)*8 + OAM_Y_OFS);
                        wShadowOAM[oidx].x    = (uint8_t)(ENEMY_SPR_PX_X + (1+dtx)*8 + OAM_X_OFS);
                        wShadowOAM[oidx].tile = (uint8_t)(ENEMY_SPR_TILE_BASE + (1+dty)*7 + (1+dtx));
                    }
            }
            s_grow_stage = 2;  s_grow_frame = 0;
        } else if (s_grow_stage == 2 && s_grow_frame >= 4) {
            /* Full 7×7: restore all OAM, play cry, draw enemy HUD */
            bui_set_enemy_oam_visible(1);
            Audio_PlayCry(wEnemyMon.species);
            bui_draw_enemy_hud();
            s_grow_stage = 0;  s_grow_frame = 0;
            if (s_mid_battle_send) {
                /* Mid-battle trainer replacement: no player send-out needed */
                s_mid_battle_send = 0;
                s_wait_cry_next_state = BUI_DRAW_HUD;
            } else {
                /* Battle intro: load player back sprite, show "Go! X!", slide Red out */
                uint8_t p_dex2 = gSpeciesToDex[wBattleMon.species];
                if (p_dex2 > 0 && p_dex2 <= 151) {
                    for (int i = 0; i < POKEMON_BACK_TILES; i++)
                        Display_LoadTile((uint8_t)(PLAYER_SPR_BG_BASE + i),
                                         gPokemonBackSprite[p_dex2][i]);
                }
                const char *p_name2 = Pokemon_GetName(gSpeciesToDex[wBattleMon.species]);
                snprintf(s_wait_cry_text, sizeof(s_wait_cry_text), "Go! %s!", p_name2);
                s_slide_cx = 0;
                s_wait_cry_next_state = BUI_TRAINER_SLIDE_OUT;
            }
            bui_state = BUI_WAIT_CRY;
        }
        break;
    }

    /* ---- Pokemon appear: AnimateSendingOutMon approximation -------- *
     * Runs after "Go! X!" text dismissed (Text_IsOpen() == 0).        *
     * Stage 0 ( 8f): single pokeball OAM at hlcoord(4,11).            *
     * Stage 1 ( 4f): 3×3 center-crop of back sprite at hlcoord(3,9).  *
     * Stage 2 ( 4f): 5×5 center-crop at hlcoord(2,7).                 *
     * Stage 3:       full 7×7 placed → BUI_DRAW_HUD.                  */
    case BUI_POKEMON_APPEAR: {
        uint8_t p_dex3 = gSpeciesToDex[wBattleMon.species];
        /* One-time setup on entry (first frame after "Go! X!" dismissed):
         * Draw HUDs now — mirrors SendOutMon calling Draw*HUDAndHPBar after
         * PrintSendOutMonMessage returns. Then clear the player sprite area
         * and place the pokeball OAM to start the grow animation. */
        if (s_grow_stage == 0 && s_grow_frame == 0) {
            bui_draw_enemy_hud();
            bui_draw_player_hud();
            /* Redraw empty text box border — mirrors SendOutMon which keeps the
             * text box visible through AnimateSendingOutMon and only clears it
             * via PrintEmptyString AFTER the animation completes. In our port
             * Text_Close() already erased the box when "Go! X!" was dismissed. */
            bui_draw_box();
            bui_clear_rect(1, 5, 7, 11);
            wShadowOAM[POKEBALL_OAM_BASE].y    = (uint8_t)(11 * 8 + OAM_Y_OFS);
            wShadowOAM[POKEBALL_OAM_BASE].x    = (uint8_t)( 4 * 8 + OAM_X_OFS);
            wShadowOAM[POKEBALL_OAM_BASE].tile = POKEBALL_TILE_BASE;
            wShadowOAM[POKEBALL_OAM_BASE].flags = 0;
        }
        s_grow_frame++;
        if (s_grow_stage == 0 && s_grow_frame >= 8) {
            /* Pokeball vanishes — set up player-side poof (Subanim_0BallPoof, 3 entries).
             * Stage 10 handles drawing; SFX fires at counter=5 (mid-poof, per ASM). */
            wShadowOAM[POKEBALL_OAM_BASE].y = 0;
            bui_clear_rect(1, 5, 7, 11);
            bui_ball_load_poof_tiles();
            for (int i = 0; i < POOF_OAM_COUNT; i++)
                wShadowOAM[POOF_OAM_BASE + i].y = 0;
            s_grow_stage = 10;  s_grow_frame = 0;
        } else if (s_grow_stage == 10) {
            /* Player poof: 3 entries × 6 frames each = 18 frames (FB06→FB07→FB08).
             * SFX_BALL_POOF mirrors DoPoofSpecialEffects: fires at wSubAnimCounter==5
             * (frame 5 of poof, mid-way through entry 0). */
            int entry    = (s_grow_frame - 1) / 6;
            int subframe = (s_grow_frame - 1) % 6;
            if (s_grow_frame == 5)
                Audio_PlaySFX_BallPoof();
            if (entry >= 3) {
                /* Poof complete — clear OAM, begin 3×3 crop grow. */
                for (int i = 0; i < POOF_OAM_COUNT; i++)
                    wShadowOAM[POOF_OAM_BASE + i].y = 0;
                if (p_dex3 > 0 && p_dex3 <= 151)
                    for (int dty = 0; dty < 3; dty++)
                        for (int dtx = 0; dtx < 3; dtx++)
                            bui_set_tile(3 + dtx, 9 + dty,
                                (uint8_t)(PLAYER_SPR_BG_BASE + (2+dty)*7 + (2+dtx)));
                s_grow_stage = 1;  s_grow_frame = 0;
            } else if (subframe == 0) {
                for (int i = 0; i < POOF_OAM_COUNT; i++)
                    wShadowOAM[POOF_OAM_BASE + i].y = 0;
                bui_draw_player_poof_frame(entry);
            }
        } else if (s_grow_stage == 1 && s_grow_frame >= 4) {
            /* 5×5 center-crop: tiles (tx=1..5, ty=1..5) at hlcoord(2,7). */
            bui_clear_rect(1, 5, 7, 11);
            if (p_dex3 > 0 && p_dex3 <= 151)
                for (int dty = 0; dty < 5; dty++)
                    for (int dtx = 0; dtx < 5; dtx++)
                        bui_set_tile(2 + dtx, 7 + dty,
                            (uint8_t)(PLAYER_SPR_BG_BASE + (1+dty)*7 + (1+dtx)));
            s_grow_stage = 2;  s_grow_frame = 0;
        } else if (s_grow_stage == 2 && s_grow_frame >= 5) {
            /* Full 7×7 at hlcoord(1,5) → done.
             * PlayCry — mirrors SendOutMon calling PlayCry(wCurPartySpecies)
             * after AnimateSendingOutMon. */
            Audio_PlayCry(wBattleMon.species);
            bui_place_player_sprite();
            s_grow_stage = 0;  s_grow_frame = 0;
            if (s_grow_after_switch) {
                /* Voluntary switch: enemy still needs to take their turn */
                s_grow_after_switch = 0;
                s_wait_cry_next_state = BUI_SWITCH_ENEMY_TURN;
            } else {
                s_wait_cry_next_state = BUI_DRAW_HUD;
            }
            bui_state = BUI_WAIT_CRY;
        }
        break;
    }

    /* ---- Intro: (legacy — no longer used as entry point) ----------- */
    case BUI_INTRO: {
        bui_draw_enemy_hud();
        bui_draw_player_hud();
        bui_load_sprites();
        const char *e_name = Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]);
        const char *p_name = Pokemon_GetName(gSpeciesToDex[wBattleMon.species]);
        if (wIsInBattle == 2)
            snprintf(s_msg_buf, sizeof(s_msg_buf),
                     "Trainer sent out\n%s!\fGo! %s!", e_name, p_name);
        else
            snprintf(s_msg_buf, sizeof(s_msg_buf),
                     "Wild %s\nappeared!\fGo! %s!", e_name, p_name);
        Text_ShowASCII(s_msg_buf);
        bui_state = BUI_DRAW_HUD;
        break;
    }

    /* ---- Draw HUD + open main menu -------------------------------- */
    case BUI_DRAW_HUD:
        bui_clear_rows(0, SCREEN_HEIGHT - 1);  /* erase party menu / stale tiles */
        bui_draw_enemy_hud();
        bui_draw_player_hud();
        bui_load_sprites();
        bui_cursor = 0;
        bui_draw_main_menu(0);
        hWY = SCREEN_HEIGHT_PX;  /* disable window — battle UI is in gScrollTileMap, not gWindowTileMap */
        bui_state = BUI_MENU;
        break;

    /* ---- Main battle menu: FIGHT / PKMN / ITEM / RUN -------------- *
     * cursor: 0=FIGHT(top-left) 1=PKMN(top-right)                   *
     *         2=ITEM(bot-left)  3=RUN(bot-right)                     *
     * LEFT/RIGHT toggle bit 0; UP/DOWN toggle bit 1.                 */
    case BUI_MENU:
        if (hJoyPressed & PAD_LEFT)  { bui_cursor &= ~1; bui_draw_main_menu(bui_cursor); }
        if (hJoyPressed & PAD_RIGHT) { bui_cursor |=  1; bui_draw_main_menu(bui_cursor); }
        if (hJoyPressed & PAD_UP)    { bui_cursor &= ~2; bui_draw_main_menu(bui_cursor); }
        if (hJoyPressed & PAD_DOWN)  { bui_cursor |=  2; bui_draw_main_menu(bui_cursor); }
        if (hJoyPressed & PAD_A) {
            switch (bui_cursor) {
            case 0: /* FIGHT */
                /* AnyMoveToSelect: if all moves have 0 PP, force Struggle */
                {
                    int any_pp = 0;
                    for (int i = 0; i < 4 && !any_pp; i++)
                        if (wBattleMon.moves[i] && (wBattleMon.pp[i] & 0x3F) > 0)
                            any_pp = 1;
                    if (!any_pp) {
                        wPlayerSelectedMove  = MOVE_STRUGGLE;
                        wPlayerMoveListIndex = 0;
                        s_struggle_pending   = 1;
                        Text_ShowASCII("No moves left!");
                    } else {
                        bui_cursor = 0;
                        bui_draw_move_menu(0);
                        bui_state = BUI_MOVE_SELECT;
                    }
                }
                break;
            case 1: /* PKMN — voluntary switch */
                bui_set_enemy_oam_visible(0);  /* hide enemy sprite over party menu */
                PartyMenu_Open(2 /* battle voluntary: SWITCH/STATS/CANCEL submenu */);
                bui_state = BUI_SWITCH_SELECT;
                break;
            case 2: /* ITEM — not yet implemented */
                bui_set_enemy_oam_visible(0);  /* hide OAM while bag is open */
                BagMenu_OpenBattle();
                bui_state = BUI_BAG_BATTLE;
                break;
            case 3: /* RUN */
                if (wIsInBattle == 2) {
                    Text_ShowASCII("Can't escape from\na trainer battle!");
                    bui_state = BUI_DRAW_HUD;
                } else if (Battle_TryRunningFromBattle()) {
                    Audio_PlaySFX_Run();
                    Text_ShowASCII("Got away safely!");
                    bui_state = BUI_END;
                } else {
                    Text_ShowASCII("Couldn't get\naway!");
                    bui_state = BUI_DRAW_HUD;
                }
                break;
            }
        }
        break;

    /* ---- Move selection ------------------------------------------- */
    case BUI_MOVE_SELECT:
        /* s_struggle_pending: "No moves left!" text was shown, now execute Struggle */
        if (s_struggle_pending) {
            s_struggle_pending = 0;
            battle_result_t prep = Battle_TurnPrepare();
            if (prep == BATTLE_RESULT_PLAYER_FAINTED) { bui_state = BUI_PLAYER_FAINTED; break; }
            if (prep == BATTLE_RESULT_ENEMY_FAINTED)  { Battle_HandleEnemyMonFainted(); bui_state = BUI_END; break; }
            s_player_first = Battle_TurnPlayerFirst();
            bui_exec_first_move();
            break;
        }

        /* Single-column list: UP/DOWN navigate, LEFT/RIGHT unused */
        if (hJoyPressed & PAD_UP)   { if (bui_cursor > 0) { bui_cursor--; bui_draw_move_menu(bui_cursor); } }
        if (hJoyPressed & PAD_DOWN) { if (bui_cursor < 3) { bui_cursor++; bui_draw_move_menu(bui_cursor); } }

        if (hJoyPressed & PAD_B) {
            bui_cursor = 0;
            bui_clear_rect(0, 8, 10, 11);  /* erase PP info box (rows 8-11, cols 0-10) */
            bui_place_player_sprite();      /* restore sprite tiles cleared by PP box */
            bui_draw_player_hud();          /* col 10 (HP label) was inside clear rect */
            bui_draw_main_menu(0);
            bui_state = BUI_MENU;
        }

        if (hJoyPressed & PAD_A) {
            uint8_t move = wBattleMon.moves[bui_cursor];
            if (!move) break;   /* empty slot */

            /* MoveNoPPText: can't select a move with 0 PP — return to menu */
            if ((wBattleMon.pp[bui_cursor] & 0x3F) == 0) {
                Text_ShowASCII("No PP left for\nthis move!");
                break;
            }

            bui_clear_rect(0, 8, 10, 11);  /* erase PP info box before executing move */
            bui_place_player_sprite();      /* restore sprite tiles cleared by PP box */
            bui_draw_player_hud();          /* col 10 (HP label) was inside clear rect */

            wPlayerSelectedMove  = move;
            wPlayerMoveListIndex = (uint8_t)bui_cursor;

            /* Prepare turn: flinch clear, enemy move select, order */
            battle_result_t prep = Battle_TurnPrepare();
            if (prep == BATTLE_RESULT_PLAYER_FAINTED) {
                bui_state = BUI_PLAYER_FAINTED;
                break;
            }
            if (prep == BATTLE_RESULT_ENEMY_FAINTED) {
                /* Enemy was at 0 HP at loop entry — shouldn't normally happen */
                Battle_HandleEnemyMonFainted();
                bui_state = BUI_END;
                break;
            }
            s_player_first = Battle_TurnPlayerFirst();
            bui_exec_first_move();
        }
        break;

    /* ---- Second move: after first move's text is dismissed --------- */
    case BUI_EXEC_MOVE_B: {
        /* Drain HP text (mirrors drain_hp.asm PrintText after UpdateHPBar2).
         * Shown as a separate dialog, then re-enter this state to continue. */
        if (s_drain_text[0]) {
            Text_ShowASCII(s_drain_text);
            s_drain_text[0] = '\0';
            return;
        }

        /* Check escape (Teleport / Roar / Whirlwind from first move) */
        if (wEscapedFromBattle) {
            wEscapedFromBattle = 0;
            Text_ShowASCII("Got away safely!");
            bui_state = BUI_END;
            return;
        }

        /* Check if first move's TARGET fainted */
        if (s_player_first) {
            /* Player went first — did enemy faint? */
            if (wEnemyMon.hp == 0) {
                bui_start_enemy_faint_anim();
                return;
            }
        } else {
            /* Enemy went first — did player faint? */
            if (wBattleMon.hp == 0) {
                /* Use player name as "second mover" name was stored in s_name_b */
                bui_handle_player_fainted();
                return;
            }
        }

        /* First mover's end-of-move residual (poison/burn/leech seed).
         * Applied silently; HP bars updated below. */
        if (!Battle_HandlePoisonBurnLeechSeed()) {
            /* First mover fainted from residual */
            bui_draw_enemy_hud();
            bui_draw_player_hud();
            if (s_player_first) {
                /* Player (first mover) fainted from their own residual */
                Battle_HandlePlayerMonFainted();
                snprintf(s_msg_buf, sizeof(s_msg_buf), "%s\nfainted!", s_name_a);
                Text_ShowASCII(s_msg_buf);
                bui_state = BUI_PLAYER_FAINTED;
            } else {
                /* Enemy (first mover) fainted from their own residual */
                Battle_HandleEnemyMonFainted();
                bui_draw_enemy_hud();
                bui_draw_player_hud();
                snprintf(s_msg_buf, sizeof(s_msg_buf), "%s%s\nfainted!", s_pfx_a, s_name_a);
                Text_ShowASCII(s_msg_buf);
                if (wBattleResult == BATTLE_OUTCOME_WILD_VICTORY ||
                    wBattleResult == BATTLE_OUTCOME_TRAINER_VICTORY) {
                    bui_exp_dest    = BUI_END;
                    s_exp_suffix[0] = '\0';
                    s_exp_suffix2[0] = '\0';
                } else {
                    const char *nn = Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]);
                    s_grow_stage = 0; s_grow_frame = 0; s_mid_battle_send = 1;
                    bui_exp_dest = BUI_ENEMY_SEND_OUT;
                    snprintf(s_exp_suffix, sizeof(s_exp_suffix), "Foe sent out\n%s!", nn);
                    s_exp_suffix2[0] = '\0';
                }
                bui_state = BUI_EXP_DRAIN;
            }
            return;
        }
        bui_draw_enemy_hud();
        bui_draw_player_hud();
        {
            char rmsg[80];
            bui_format_residual_msg(rmsg, sizeof(rmsg));
            if (rmsg[0]) {
                Text_ShowASCII(rmsg);
                bui_state = BUI_EXEC_SECOND;
                break;
            }
        }
        bui_exec_second_move();
        break;
    }

    /* ---- Move animation: PlayApplyingAttackAnimation equivalent ------- *
     * Runs for s_anim_total frames after the move is executed but before  *
     * HUDs are updated and the move-use text is shown.  Matches the       *
     * original's: execute → PlayMoveAnimation → DrawHPBars → PrintText.  */
    case BUI_MOVE_ANIM: {
        if (s_anim_total == 0) {
            /* No animation (missed or no move) — proceed immediately to HP anim */
            Display_SetShakeOffset(0, 0);
            if (!wMoveMissed)
                Audio_PlaySFX_BattleHit(wDamageMultipliers & 0x7Fu);
            bui_state = BUI_HP_ANIM;
            break;
        }

        int ox = 0, oy = 0;
        switch (s_anim_type) {
        case 1: {
            /* ShakeScreenVertically b=8: 8 steps × 6 frames = 48 total.
             * Step 0: oy=+8 for 3f, oy=0 for 3f.
             * Steps 1-7: oy=0 for 3f, oy=+(8-step) for 3f.
             * Mirrors PredefShakeScreenVertically XOR pattern. */
            int step  = s_anim_frame / 6;
            int phase = s_anim_frame % 6;
            if (step == 0)
                oy = (phase < 3) ? 8 : 0;
            else
                oy = (phase < 3) ? 0 : (8 - step);
            break;
        }
        case 2:
            /* ShakeScreenHorizontallyHeavy b=8: 8 steps × 9 frames = 72 total.
             * Each step: 5 frames shifted right by 8px, 4 frames normal.
             * Mirrors PredefShakeScreenHorizontally MutateWX delay pattern. */
            ox = ((s_anim_frame % 9) < 5) ? 8 : 0;
            break;
        case 4: {
            /* BlinkEnemyMonSprite: 6 blinks × 10 frames = 60 total.
             * Mirrors AnimationBlinkMon: 5f hidden + 5f shown per blink. */
            int hidden = (s_anim_frame % 10) < 5;
            bui_set_enemy_oam_visible(!hidden);
            break;
        }
        case 5:
            /* ShakeScreenHorizontallyLight b=2: 2 steps × 9 frames = 18 total. */
            ox = ((s_anim_frame % 9) < 5) ? 2 : 0;
            break;
        }

        Display_SetShakeOffset(ox, oy);
        s_anim_frame++;

        if (s_anim_frame >= s_anim_total) {
            Display_SetShakeOffset(0, 0);
            bui_set_enemy_oam_visible(1);  /* restore if blink left it hidden */
            if (!wMoveMissed)
                Audio_PlaySFX_BattleHit(wDamageMultipliers & 0x7Fu);
            bui_state = BUI_HP_ANIM;
        }
        break;
    }

    /* ---- HP bar scroll: UpdateHPBar2 equivalent -------------------- *
     * Animates the target's HP bar from old pixel count to new at      *
     * 1 pixel per 2 frames, matching UpdateHPBar_AnimateHPBar's        *
     * DrawHPBar → DelayFrames(2) loop in the original.                 */
    case BUI_HP_ANIM: {
        if (s_hp_cur_px == s_hp_new_px) {
            /* Scroll complete — hand off to finish (which redraws full HUD) */
            if (s_anim_first) bui_finish_first_move();
            else              bui_finish_second_move();
            break;
        }
        /* Draw bar at current intermediate pixel count */
        if (s_hp_anim_who == 0)
            bui_draw_hp_bar_px(4,  2,  s_hp_cur_px);   /* enemy bar */
        else
            bui_draw_hp_bar_px(12, 9,  s_hp_cur_px);   /* player bar */
        /* Advance 1 pixel every 2 frames */
        s_hp_half_frame ^= 1;
        if (s_hp_half_frame == 0) {
            if (s_hp_cur_px > s_hp_new_px) s_hp_cur_px--;
            else                            s_hp_cur_px++;
        }
        break;
    }

    /* ---- Second move (after residual-A message dismissed) ---------- */
    case BUI_EXEC_SECOND:
        bui_exec_second_move();
        break;

    /* ---- Turn end: after second move's text is dismissed ----------- */
    case BUI_TURN_END: {
        /* Drain HP text — same pattern as BUI_EXEC_MOVE_B above */
        if (s_drain_text[0]) {
            Text_ShowASCII(s_drain_text);
            s_drain_text[0] = '\0';
            return;
        }

        /* Check escape */
        if (wEscapedFromBattle) {
            wEscapedFromBattle = 0;
            Text_ShowASCII("Got away safely!");
            bui_state = BUI_END;
            return;
        }

        /* Check if second move's TARGET fainted.
         * Second mover attacked the first mover's side:
         *   player_first=1 → enemy went second → target = player
         *   player_first=0 → player went second → target = enemy */
        if (s_player_first) {
            if (wBattleMon.hp == 0) {
                /* Player fainted from enemy's second move.
                 * s_name_a = player's mon name (player was first mover). */
                Battle_HandlePlayerMonFainted();
                bui_draw_enemy_hud();
                bui_draw_player_hud();
                snprintf(s_msg_buf, sizeof(s_msg_buf), "%s\nfainted!", s_name_a);
                Text_ShowASCII(s_msg_buf);
                bui_state = BUI_PLAYER_FAINTED;
                return;
            }
        } else {
            if (wEnemyMon.hp == 0) {
                /* Enemy fainted from player's second move — start slide animation. */
                bui_start_enemy_faint_anim();
                return;
            }
        }

        /* Second mover's end-of-move residual */
        if (!Battle_HandlePoisonBurnLeechSeed()) {
            bui_draw_enemy_hud();
            bui_draw_player_hud();
            if (s_player_first) {
                /* Enemy (second mover) fainted from residual */
                Battle_HandleEnemyMonFainted();
                bui_draw_enemy_hud();
                bui_draw_player_hud();
                snprintf(s_msg_buf, sizeof(s_msg_buf),
                         "%s%s\nfainted!", s_pfx_b, s_name_b);
                Text_ShowASCII(s_msg_buf);
                if (wBattleResult == BATTLE_OUTCOME_WILD_VICTORY ||
                    wBattleResult == BATTLE_OUTCOME_TRAINER_VICTORY) {
                    bui_exp_dest    = BUI_END;
                    s_exp_suffix[0] = '\0';
                    s_exp_suffix2[0] = '\0';
                } else {
                    const char *nn = Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]);
                    s_grow_stage = 0; s_grow_frame = 0; s_mid_battle_send = 1;
                    bui_exp_dest = BUI_ENEMY_SEND_OUT;
                    snprintf(s_exp_suffix, sizeof(s_exp_suffix), "Foe sent out\n%s!", nn);
                    s_exp_suffix2[0] = '\0';
                }
                bui_state = BUI_EXP_DRAIN;
            } else {
                /* Player (second mover) fainted from residual */
                Battle_HandlePlayerMonFainted();
                snprintf(s_msg_buf, sizeof(s_msg_buf), "%s\nfainted!", s_name_b);
                Text_ShowASCII(s_msg_buf);
                bui_state = BUI_PLAYER_FAINTED;
            }
            return;
        }

        bui_draw_enemy_hud();
        bui_draw_player_hud();
        {
            char rmsg[80];
            bui_format_residual_msg(rmsg, sizeof(rmsg));
            if (rmsg[0]) {
                Text_ShowASCII(rmsg);
                bui_state = BUI_TURN_FINISH;
                break;
            }
        }
        Battle_CheckNumAttacksLeft();
        bui_draw_enemy_hud();
        bui_draw_player_hud();
        /* Draw the main menu box immediately instead of deferring to BUI_DRAW_HUD.
         * HUDs are already drawn above; skipping BUI_DRAW_HUD avoids a 1-frame
         * blank in rows 12-17 between text close and the next menu draw. */
        bui_cursor = 0;
        bui_draw_main_menu(0);
        hWY = SCREEN_HEIGHT_PX;
        bui_state = BUI_MENU;
        break;
    }

    /* ---- Turn finish (after residual-B message dismissed) ---------- */
    case BUI_TURN_FINISH:
        Battle_CheckNumAttacksLeft();
        bui_draw_enemy_hud();
        bui_draw_player_hud();
        bui_state = BUI_DRAW_HUD;
        break;

    /* ---- Drain exp/level-up texts queued by Battle_GainExperience --- */
    case BUI_EXP_DRAIN: {
        /* If the previous text was a level-up, show the stats box first.
         * Mirrors experience.asm: PrintStatsBox(LEVEL_UP_STATS_BOX) after GrewLevelText. */
        if (s_pending_lvl_stats.valid) {
            /* Clear move menu artifacts (PP box rows 8-11, move list rows 12-17)
             * then restore the normal battle view before overlaying the stat box.
             * Mirrors: DrawPlayerHUDAndHPBar + PrintEmptyString + SaveScreenTiles
             * + PrintStatsBox in experience.asm. */
            bui_clear_rect(0, 8, 19, 17);
            bui_place_player_sprite();
            bui_draw_player_hud();
            bui_draw_box();   /* PrintEmptyString: empty text box at bottom */
            bui_draw_levelup_stats(&s_pending_lvl_stats);
            s_pending_lvl_stats.valid = 0;
            bui_state = BUI_LEVELUP_STATS;
            break;
        }
        levelup_stats_t lvl_stats;
        const char *txt = BattleExp_TakeNextText(&lvl_stats);
        if (txt) {
            bui_draw_player_hud();
            /* Level-up text: play jingle (mirrors sound_level_up in GrewLevelText) */
            if (lvl_stats.valid) Audio_PlaySFX_LevelUp();
            snprintf(s_msg_buf, sizeof(s_msg_buf), "%s", txt);
            Text_ShowASCII(s_msg_buf);
            /* Store stats so next tick draws the stats box */
            s_pending_lvl_stats = lvl_stats;
            /* stay in BUI_EXP_DRAIN for next text */
        } else if (s_exp_suffix[0] != '\0') {
            /* Trainer victory: music fires here, just before "PLAYER defeated TRAINER!" —
             * mirrors PlayBattleVictoryMusic order in core.asm:TrainerBattleVictory. */
            if (bui_exp_dest == BUI_TRAINER_VICTORY_SLIDE) {
                extern uint8_t wGymLeaderNo;
                Music_Play(wGymLeaderNo ? MUSIC_DEFEATED_GYM_LEADER : MUSIC_DEFEATED_TRAINER);
            }
            snprintf(s_msg_buf, sizeof(s_msg_buf), "%s", s_exp_suffix);
            s_exp_suffix[0] = '\0';
            Text_ShowASCII(s_msg_buf);
            /* If there's a second page (e.g. money text after defeated text),
             * stay in BUI_EXP_DRAIN; s_exp_suffix2 will be shown on next tick. */
            if (s_exp_suffix2[0] == '\0')
                bui_state = bui_exp_dest;
        } else if (s_exp_suffix2[0] != '\0') {
            snprintf(s_msg_buf, sizeof(s_msg_buf), "%s", s_exp_suffix2);
            s_exp_suffix2[0] = '\0';
            Text_ShowASCII(s_msg_buf);
            bui_state = bui_exp_dest;
        } else {
            bui_state = bui_exp_dest;
        }
        break;
    }

    /* ---- Level-up stats box: wait for A then clear and resume drain --
     * Mirrors experience.asm: WaitForTextScrollButtonPress after PrintStatsBox,
     * then LoadScreenTilesFromBuffer1 clears it. */
    case BUI_LEVELUP_STATS:
        if (hJoyPressed & PAD_A) {
            /* Clear the stats box area and return to exp drain */
            bui_clear_rect(9, 2, 19, 11);
            bui_state = BUI_EXP_DRAIN;
        }
        break;

    /* ---- Enemy sprite slide-down faint animation ------------------- *
     * Mirrors SlideDownFaintedMonPic (engine/battle/core.asm:1181).    *
     * 7 steps × 2 frames/step = 14 frames total.  Each step shifts all *
     * OAM rows down by 8px, hiding the bottom row that scrolled out.   */
    case BUI_ENEMY_FAINT_ANIM: {
        if (--s_faint_timer <= 0) {
            s_faint_step++;
            if (s_faint_step > 7) {
                /* Animation complete — proceed to game-state / text phase. */
                s_faint_step = 0;
                bui_handle_enemy_fainted();
            } else {
                bui_enemy_faint_oam(s_faint_step);
                s_faint_timer = 2;
            }
        }
        break;
    }

    /* ---- Player mon fainted: checks then open party menu ---------- */
    case BUI_PLAYER_FAINTED: {
        /* Simultaneous faint: enemy also at 0 HP */
        if (wEnemyMon.hp == 0) {
            wBoostExpByExpAll = 0;
            Battle_GainExperience();
            if (wIsInBattle != 2) {
                bui_exp_dest    = BUI_END;
                s_exp_suffix[0] = '\0';
                s_exp_suffix2[0] = '\0';
            } else if (!Battle_AnyEnemyPokemonAliveCheck()) {
                Battle_TrainerBattleVictory();
                bui_exp_dest = BUI_END;
                snprintf(s_exp_suffix, sizeof(s_exp_suffix),
                         "Trainer has no more\nusable Pokemon!");
                s_exp_suffix2[0] = '\0';
            } else {
                Battle_ReplaceFaintedEnemyMon();
                const char *nn = Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]);
                bui_exp_dest = BUI_PLAYER_FAINTED;
                snprintf(s_exp_suffix, sizeof(s_exp_suffix), "Foe sent out\n%s!", nn);
                s_exp_suffix2[0] = '\0';
            }
            bui_state = BUI_EXP_DRAIN;
            return;
        }

        /* Blackout check */
        if (!Battle_AnyPartyAlive()) {
            Battle_HandlePlayerBlackOut();
            Text_ShowASCII("All Pokemon fainted!\nBlacked out!");
            bui_state = BUI_END;
            return;
        }

        /* Wild battle: ask "Use next Pokemon?" before party select.
         * Trainer battle: go straight to party select (core.asm:1058 ret nz). */
        if (wIsInBattle != 2) {
            bui_cursor = 0;
            Text_ShowASCII("Use next\nPokemon?");
            bui_state = BUI_USE_NEXT_MON;
        } else {
            bui_open_party_select();
        }
        break;
    }

    /* ---- "Use next Pokemon?" YES/NO (wild battles only) ----------- */
    case BUI_USE_NEXT_MON: {
        /* Redraw YES/NO in text box area each tick */
        bui_draw_box();
        bui_put_str(1, 13, bui_cursor == 0 ? ">YES" : " YES");
        bui_put_str(1, 14, bui_cursor == 1 ? ">NO " : " NO ");

        if (hJoyPressed & (PAD_UP | PAD_DOWN))
            bui_cursor ^= 1;

        if ((hJoyPressed & PAD_A) || (hJoyPressed & PAD_B)) {
            int chose_no = (bui_cursor == 1) || (hJoyPressed & PAD_B);
            if (!chose_no) {
                /* YES: open party menu */
                bui_open_party_select();
            } else {
                /* NO: try to run (core.asm DoUseNextMonDialogue .tryRunning).
                 * On failure the original falls through to ChooseNextMon —
                 * the player cannot stay in battle with no mon alive. */
                if (Battle_TryRunningFromBattle()) {
                    Text_ShowASCII("Got away safely!");
                    bui_state = BUI_END;
                } else {
                    bui_open_party_select();
                }
            }
        }
        break;
    }

    /* ---- Party selection ------------------------------------------ */
    case BUI_PARTY_SELECT: {
        if (!PartyMenu_IsOpen()) {
            /* Initial entry or re-entry after an error dialog closed:
             * (re)open the full party screen.  BUI_DRAW_HUD will redraw
             * the battle screen once a valid selection is confirmed. */
            PartyMenu_Open(1 /* force */);
            break;
        }
        PartyMenu_Tick();
        if (PartyMenu_IsOpen()) break;  /* still navigating */

        /* A was pressed — validate the selection */
        int slot = PartyMenu_GetSelected();
        if (wPartyMons[slot].base.hp == 0) {
            Text_ShowASCII("No energy left\nto battle!");
            /* state stays BUI_PARTY_SELECT; menu reopens after text closes */
            break;
        }
        if (slot == (int)wPlayerMonNumber) {
            Text_ShowASCII("Already in battle!");
            break;
        }
        Battle_ChooseNextMon((uint8_t)slot);
        snprintf(s_msg_buf, sizeof(s_msg_buf), "Go! %s!",
                 Pokemon_GetName(gSpeciesToDex[wBattleMon.species]));
        Text_ShowASCII(s_msg_buf);
        bui_state = BUI_DRAW_HUD;
        break;
    }

    /* ---- Voluntary switch: party select --------------------------- *
     * Player pressed PKMN from the main menu.  PartyMenu_Open(0) was  *
     * already called so the menu is open on first entry.               */
    case BUI_SWITCH_SELECT: {
        if (!PartyMenu_IsOpen()) {
            /* Re-open after an error dialog closed */
            PartyMenu_Open(2);
            break;
        }
        PartyMenu_Tick();
        if (PartyMenu_IsOpen()) break;  /* still navigating */

        int slot = PartyMenu_GetSelected();
        if (slot < 0) {
            /* Player cancelled with B — redraw battle screen and back to menu */
            bui_state = BUI_DRAW_HUD;
            break;
        }
        if (wPartyMons[slot].base.hp == 0) {
            Text_ShowASCII("No energy left\nto battle!");
            /* state stays BUI_SWITCH_SELECT; menu re-opens after text */
            break;
        }
        if (slot == (int)wPlayerMonNumber) {
            Text_ShowASCII("Already in battle!");
            break;
        }

        /* Valid selection: save old mon info before the switch data loads */
        s_retreat_species = wBattleMon.species;
        s_switch_slot     = (uint8_t)slot;

        /* Enemy picks their move now (player's switch action consumed the turn) */
        Battle_SelectEnemyMove();

        /* Restore the battle screen — party menu tiles are still on screen */
        bui_clear_rows(0, SCREEN_HEIGHT - 1);
        bui_draw_enemy_hud();
        bui_draw_player_hud();
        bui_load_sprites();

        /* "Come back, [name]!" — mirrors RetreatMon PrintText */
        const char *old_name = Pokemon_GetName(gSpeciesToDex[s_retreat_species]);
        snprintf(s_msg_buf, sizeof(s_msg_buf), "Come back,\n%s!", old_name);
        Text_ShowASCII(s_msg_buf);

        s_retreat_stage = 0;
        s_retreat_frame = 0;
        bui_state = BUI_RETREAT_ANIM;
        break;
    }

    /* ---- Voluntary switch: player sprite shrink ------------------- *
     * Mirrors AnimateRetreatingPlayerMon (engine/battle/core.asm).    *
     * 3 stages × 4 frames each = 12 frames total.                     *
     * Stage 0: full 7×7 (already drawn) → wait 4f → draw 5×5.        *
     * Stage 1: 5×5 center-crop at hlcoord(2,7) → wait 4f → draw 3×3. *
     * Stage 2: 3×3 center-crop at hlcoord(3,9) → wait 4f → blank.    */
    case BUI_RETREAT_ANIM: {
        s_retreat_frame++;
        uint8_t rdex = gSpeciesToDex[s_retreat_species];

        if (s_retreat_stage == 0 && s_retreat_frame >= 4) {
            /* Shrink from 7×7 to 5×5 center-crop */
            bui_clear_rect(1, 5, 7, 11);
            if (rdex > 0 && rdex <= 151)
                for (int dty = 0; dty < 5; dty++)
                    for (int dtx = 0; dtx < 5; dtx++)
                        bui_set_tile(2 + dtx, 7 + dty,
                            (uint8_t)(PLAYER_SPR_BG_BASE + (1+dty)*7 + (1+dtx)));
            s_retreat_stage = 1;  s_retreat_frame = 0;
        } else if (s_retreat_stage == 1 && s_retreat_frame >= 3) {
            /* Shrink from 5×5 to 3×3 center-crop */
            bui_clear_rect(1, 5, 7, 11);
            if (rdex > 0 && rdex <= 151)
                for (int dty = 0; dty < 3; dty++)
                    for (int dtx = 0; dtx < 3; dtx++)
                        bui_set_tile(3 + dtx, 9 + dty,
                            (uint8_t)(PLAYER_SPR_BG_BASE + (2+dty)*7 + (2+dtx)));
            s_retreat_stage = 2;  s_retreat_frame = 0;
        } else if (s_retreat_stage == 2 && s_retreat_frame >= 1) {
            /* Mon gone — play poof SFX, clear sprite area (ASM: no delay after 3×3) */
            Audio_PlaySFX_BallPoof();
            bui_clear_rect(1, 5, 7, 11);
            s_retreat_stage = 0;  s_retreat_frame = 0;

            /* Perform the data switch — loads new mon into wBattleMon */
            Battle_SwitchPlayerMon(s_switch_slot);

            /* Load new mon's back sprite tiles */
            uint8_t p_dex_new = gSpeciesToDex[wBattleMon.species];
            if (p_dex_new > 0 && p_dex_new <= 151) {
                for (int i = 0; i < POKEMON_BACK_TILES; i++)
                    Display_LoadTile((uint8_t)(PLAYER_SPR_BG_BASE + i),
                                     gPokemonBackSprite[p_dex_new][i]);
            }

            /* "Go!/Do it!/Get'm!" text — mirrors PrintSendOutMonMessage.
             * Variant based on enemy current HP percentage. */
            const char *new_name = Pokemon_GetName(gSpeciesToDex[wBattleMon.species]);
            const char *go_prefix;
            if (wEnemyMon.max_hp == 0 || wEnemyMon.hp == 0) {
                go_prefix = "Go";
            } else {
                int pct = (int)wEnemyMon.hp * 100 / (int)wEnemyMon.max_hp;
                if (pct >= 70)      go_prefix = "Go";
                else if (pct >= 40) go_prefix = "Do it";
                else                go_prefix = "Get'm";
            }
            snprintf(s_msg_buf, sizeof(s_msg_buf), "%s! %s!", go_prefix, new_name);
            Text_ShowASCII(s_msg_buf);

            /* Set up grow animation (BUI_POKEMON_APPEAR will handle it) */
            s_grow_stage       = 0;
            s_grow_frame       = 0;
            s_grow_after_switch = 1;  /* → BUI_SWITCH_ENEMY_TURN after grow */
            bui_state          = BUI_POKEMON_APPEAR;
        }
        break;
    }

    /* ---- Bag menu open during battle --------------------------------- *
     * Mirrors ItemMenuLoop while in battle.  Ticks the bag each frame; *
     * on close processes the selected item.                             *
     *                                                                   *
     * Text sequence (item_effects.asm:146): show "[PLAYER] used X!"    *
     * BEFORE the throw animation, then outcome text AFTER.             */
    case BUI_BAG_BATTLE: {
        BagMenu_Tick();
        if (BagMenu_IsOpen()) break;  /* still open */

        /* Bag closed — restore battle screen.
         * Matches core.asm UseBagItem: ClearSprites → LoadScreenTilesFromBuffer1
         * → DrawHUDsAndHPBars.  Pokeballs are NOT redrawn after item use. */
        bui_clear_rows(0, SCREEN_HEIGHT - 1);
        bui_draw_enemy_hud();
        bui_draw_player_hud();
        bui_load_sprites();
        bui_hide_pokeballs();

        uint8_t item = BagMenu_GetSelected();

        if (item == 0) {
            /* Cancelled — back to main menu, turn not consumed */
            bui_set_enemy_oam_visible(1);
            bui_cursor = 0;
            bui_draw_main_menu(0);
            bui_state = BUI_MENU;
            break;
        }

        if (bui_is_ball(item) &&
            wIsInBattle == 1 &&
            wBattleType != 1 &&
            wPartyCount >= PARTY_LENGTH &&
            bui_current_box_is_full()) {
            /* ASM ItemUseBall: reject before the turn is consumed if both the
             * party and the current box are already full. */
            Text_ShowASCII("The #MON BOX\nis full! Can't\nuse that item!");
            bui_set_enemy_oam_visible(1);
            bui_cursor = 0;
            bui_draw_main_menu(0);
            bui_state = BUI_MENU;
            break;
        }

        /* Item selected: enemy picks their move now (turn is consumed) */
        Battle_SelectEnemyMove();

        if (bui_is_ball(item)) {
            /* ---- Pokéball throw ---- */
            if (wIsInBattle == 2) {
                /* Trainer battle: ball blocked (ThrowBallAtTrainerMon, item_effects.asm:2292) */
                Inventory_Remove(item, 1);
                /* Place thrown ball at start, play arc, then show blocked texts */
                s_ball_item   = item;
                s_throw_frame = 0;
                s_catch_result = CATCH_RESULT_CANNOT_CATCH; /* signal trainer-block */
                bui_ball_load_tiles();
                bui_ball_hide();
                bui_state = BUI_BALL_THROW;
                break;
            }
            /* Wild battle — remove ball, run catch formula (ASM: formula runs BEFORE
             * throw animation, item_effects.asm:422 → wPokeBallAnimData → MoveAnimation). */
            Inventory_Remove(item, 1);
            s_ball_item    = item;
            s_throw_frame  = 0;
            s_catch_result = Battle_CatchAttempt(s_ball_item);
            /* "PLAYER used BALL!" — mirrors ItemUseText00 (item_effects.asm:146) */
            char pname[NAME_LENGTH + 1];
            char iname[20];
            bui_decode_poke_str(wPlayerName, pname, sizeof(pname));
            Inventory_DecodeASCII(item, iname, sizeof(iname));
            snprintf(s_msg_buf, sizeof(s_msg_buf), "%s used\n%s!", pname, iname);
            Text_KeepTilesOnClose();
            Text_ShowASCII(s_msg_buf);
            /* After text dismissed: start throw animation */
            bui_ball_load_tiles();
            bui_ball_hide();  /* start hidden; BUI_BALL_THROW places on first frame */
            bui_state = BUI_BALL_THROW;
        } else if (item_needs_target(item)) {
            /* Medicine/revive: open party menu to pick a target slot first.
             * Enemy move was already selected above — turn is committed. */
            s_pending_item = item;
            bui_state = BUI_ITEM_TARGET;
        } else {
            /* Non-target item: dispatch immediately, then enemy attacks. */
            char pname[NAME_LENGTH + 1];
            char iname[20];
            bui_decode_poke_str(wPlayerName, pname, sizeof(pname));
            Inventory_DecodeASCII(item, iname, sizeof(iname));

            item_use_result_t use_result = Battle_UseItem(item, (uint8_t)wPlayerMonNumber);

            if (item == ITEM_POKE_FLUTE) {
                /* Mirrors ItemUsePokeFlute in-battle text (item_effects.asm:1671+) */
                if (use_result == ITEM_USE_OK) {
                    snprintf(s_msg_buf, sizeof(s_msg_buf),
                        "%s played the\nPOKE FLUTE.\fAll sleeping\nPOKEMON woke up.", pname);
                } else {
                    snprintf(s_msg_buf, sizeof(s_msg_buf),
                        "Played the\nPOKE FLUTE.\n\nNow, that's a\ncatchy tune!");
                }
                Text_ShowASCII(s_msg_buf);
            } else if (use_result == ITEM_USE_CANNOT_USE) {
                snprintf(s_msg_buf, sizeof(s_msg_buf), "Can't use that\nin battle!");
                Text_ShowASCII(s_msg_buf);
            } else {
                snprintf(s_msg_buf, sizeof(s_msg_buf), "%s used\n%s!", pname, iname);
                Text_ShowASCII(s_msg_buf);
                if (use_result == ITEM_USE_OK && !Inventory_IsKeyItem(item))
                    Inventory_Remove(item, 1);
            }

            bui_set_enemy_oam_visible(1);
            bui_state = BUI_SWITCH_ENEMY_TURN;
        }
        break;
    }

    /* ---- Item targeting: choose party slot for medicine / revive -------- *
     * Entered from BUI_BAG_BATTLE after a targetable item is confirmed.    *
     * Enemy has already selected their move; turn is committed.            *
     * Cancel (slot < 0) still advances to enemy turn.                      */
    case BUI_ITEM_TARGET: {
        if (!PartyMenu_IsOpen()) {
            PartyMenu_Open(PARTY_MENU_TMHM);  /* A=select, B=cancel */
            break;
        }
        PartyMenu_Tick();
        if (PartyMenu_IsOpen()) break;

        int slot = PartyMenu_GetSelected();

        /* Restore battle screen before showing any result text.
         * Matches UseBagItem: ClearSprites → LoadScreenTilesFromBuffer1
         * → DrawHUDsAndHPBars.  Pokeballs are NOT redrawn after item use. */
        bui_clear_rows(0, SCREEN_HEIGHT - 1);
        bui_draw_enemy_hud();
        bui_draw_player_hud();
        bui_load_sprites();
        bui_hide_pokeballs();
        bui_set_enemy_oam_visible(1);

        if (slot < 0) {
            /* Cancelled — enemy still takes their turn (turn was committed) */
            bui_state = BUI_SWITCH_ENEMY_TURN;
            break;
        }

        char pname[NAME_LENGTH + 1];
        char iname[20];
        bui_decode_poke_str(wPlayerName, pname, sizeof(pname));
        Inventory_DecodeASCII(s_pending_item, iname, sizeof(iname));

        item_use_result_t use_result = Battle_UseItem(s_pending_item, (uint8_t)slot);

        if (use_result == ITEM_USE_FAILED || use_result == ITEM_USE_CANNOT_USE) {
            Text_ShowASCII("It won't have\nany effect.");
        } else {
            snprintf(s_msg_buf, sizeof(s_msg_buf), "%s used\n%s!", pname, iname);
            Text_ShowASCII(s_msg_buf);
            if (use_result == ITEM_USE_OK && !Inventory_IsKeyItem(s_pending_item))
                Inventory_Remove(s_pending_item, 1);
            /* Refresh HUD — HP or status may have changed */
            bui_draw_player_hud();
        }

        bui_state = BUI_SWITCH_ENEMY_TURN;
        break;
    }

    /* ---- Pokéball arc animation ---------------------------------------- *
     * Subanim_0BallTossHigh (subanimations.asm:139).                       *
     * 11 waypoints, each displayed for THROW_FPW=3 frames                  *
     * (BallTossAnim: battle_anim NO_MOVE, SUBANIM_0_BALL_TOSS_HIGH, 0, 3). *
     * Waypoints from BASECOORD_30,A2,93,61,73,A7,33,A8,0E,A9,34           *
     * (base_coords.asm): raw OAM coordinates.                               *
     * FrameBlock03: 2×2 tile (16×16px) ball, neutral orientation.          *
     * SFX_BALL_TOSS fires at the very beginning (DoBallTossSpecialEffects, *
     * animations.asm:698: fires when wSubAnimCounter == 11 = first entry). */
    case BUI_BALL_THROW: {
        /* OAM Y, X for each of the 11 waypoints — BASECOORD values from base_coords.asm */
        static const uint8_t kArcY[11] = {88,76,64,56,48,40,32,30,32,41,50};
        static const uint8_t kArcX[11] = {40,48,56,64,72,80,88,96,104,112,120};

        if (s_throw_frame == 0)
            Audio_PlaySFX_BallToss();  /* SFX_BALL_TOSS at start of first waypoint */

        int waypoint = s_throw_frame / THROW_FPW;  /* which of the 11 waypoints */

        if (waypoint < 11) {
            bui_ball_set_oam(kArcY[waypoint], kArcX[waypoint], 0);
            s_throw_frame++;
            break;
        }

        /* Arc complete: ball at BASECOORD_34 = final waypoint (Y=50, X=120).
         * Trainer-block path */
        if (wIsInBattle == 2) {
            bui_ball_hide();
            bui_set_enemy_oam_visible(1);
            Text_ShowASCII("The trainer\nblocked the BALL!\n\nDon't be a thief!");
            bui_state = BUI_SWITCH_ENEMY_TURN;
            break;
        }

        /* Determine shake count from s_catch_result (set in BUI_BAG_BATTLE) */
        switch (s_catch_result) {
        case CATCH_RESULT_SUCCESS:  s_shake_total = 3; break;
        case CATCH_RESULT_3_SHAKES: s_shake_total = 3; break;
        case CATCH_RESULT_2_SHAKES: s_shake_total = 2; break;
        case CATCH_RESULT_1_SHAKE:  s_shake_total = 1; break;
        default:                    s_shake_total = 0; break;
        }
        s_shake_frame = 0;
        s_poof_frame  = 0;

        /* The original sequence transitions into the poof/open burst and the
         * ball itself drops out briefly before the closed ball reappears for
         * the shake. Keep the cloud OAM clear here; BUI_BALL_POOF owns the
         * visible effect. */
        for (int i = 0; i < POOF_OAM_COUNT; i++)
            wShadowOAM[POOF_OAM_BASE + i].y = 0;

        bui_state = BUI_BALL_POOF;
        break;
    }

    /* ---- Pokéball poof cloud (Subanim_0BallPoofEnemy) ------------------ *
     * subanimations.asm:160: SUBANIMTYPE_HFLIP, 6 entries × 4 frames = 24f.*
     * GetSubanimationTransform1 returns SUBANIMTYPE_NORMAL for player turn  *
     * (non-ENEMY type + hWhoseTurn=0), so coords from base_coords.asm are  *
     * used directly without transformation.                                 *
     *   Entry 0: FB06 (12 sprites) at BASECOORD_1B (Y=48, X=112)          *
     *   Entry 1: FB07 (16 sprites) at BASECOORD_1B                        *
     *   Entry 2: FB08 (16 sprites) at BASECOORD_36 (Y=44, X=108)          *
     *   Entry 3: FB09 (12 sprites) at BASECOORD_36                        *
     *   Entry 4: FB0A (12 sprites) at BASECOORD_15 (Y=40, X=104)          *
     *   Entry 5: FB0A (12 sprites) at BASECOORD_15                        *
     * Enemy is hidden at the start so the poof cloud is visible over it.   *
     * (Original: enemy was on BG tiles and stayed visible behind OAM poof; *
     * we pre-hide because poof OAM priority is below enemy OAM.)           *
     * Ball stays at BASECOORD_34 (arc endpoint) during poof, then moves   *
     * to BASECOORD_21 for shake when poof completes.                       */
    case BUI_BALL_POOF: {
        /* First frame: hide both the enemy and the ball so the poof/open
         * effect stands alone before the closed ball returns for shaking. */
        if (s_poof_frame == 0) {
            /* For trainer-block this state is never entered. For all wild
             * outcomes (including 0-shakes) we hide the enemy during poof
             * and restore below when needed. */
            bui_set_enemy_oam_visible(0);
            bui_ball_hide();
        }

        /* DoPoofSpecialEffects fires at wSubAnimCounter==5, not immediately
         * on impact, so the ball/open beat is visible before the burst SFX. */
        if (s_poof_frame == 4)
            Audio_PlaySFX_BallPoof();

        int entry    = s_poof_frame / 4;
        int subframe = s_poof_frame % 4;

        if (subframe == 0) {
            /* FRAMEBLOCKMODE_00: clear poof OAM, then draw new sprites */
            for (int i = 0; i < POOF_OAM_COUNT; i++)
                wShadowOAM[POOF_OAM_BASE + i].y = 0;
            bui_draw_poof_frame(entry);
        }

        s_poof_frame++;
        if (s_poof_frame >= 24) {
            /* Poof complete: clear poof cloud OAM */
            for (int i = 0; i < POOF_OAM_COUNT; i++)
                wShadowOAM[POOF_OAM_BASE + i].y = 0;

            /* HIDEPIC_ANIM: enemy stays hidden for 1-3 shakes / success.
             * For 0-shakes and cannot-catch: restore enemy visibility now. */
            if (s_catch_result == CATCH_RESULT_0_SHAKES ||
                s_catch_result == CATCH_RESULT_CANNOT_CATCH) {
                bui_set_enemy_oam_visible(1);
            }

            /* Ball moves from arc endpoint to BASECOORD_21 for shake */
            bui_ball_set_oam(BALL_SHAKE_OAM_Y, BALL_SHAKE_OAM_X, 0);
            bui_state = BUI_BALL_SHAKE;
        }
        break;
    }

    /* ---- Pokéball shake animation (0–3 rocks) ------------------------- *
     * Mirrors Subanim_0BallShakeEnemy + DoBallShakeSpecialEffects.        *
     * Subanim_0BallShakeEnemy (subanimations.asm:153):                    *
     *   4 entries × FRAMEBLOCKMODE_04 × 4 frames = 16 visual frames:      *
     *   FB03(neutral), FB04(tilt-right), FB03(neutral), FB05(tilt-left).  *
     * DoBallShakeSpecialEffects (animations.asm:739):                     *
     *   fires at start of each shake (wSubAnimCounter==4):                 *
     *   plays SFX_TINK then DelayFrames 40.                               *
     * Per-shake total: 40 pause + 4×4 visual = SHAKE_CYCLE(56) frames.   *
     *                                                                     *
     * 0-shakes: s_shake_total==0, bail immediately (enemy never hidden).  *
     * Fail 1-3: SHOWPIC (restore enemy) after N shakes → fail text.      *
     * Success: after 3 shakes → BUI_CAUGHT.                              */
    case BUI_BALL_SHAKE: {
        if (s_shake_total == 0 || s_shake_frame >= s_shake_total * SHAKE_CYCLE) {
            if (s_catch_result == CATCH_RESULT_SUCCESS) {
                /* ItemUseBallText05 "All right! X was caught!" (item_effects.asm:609) */
                const char *ename = Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]);
                /* The original keeps the closed ball on-screen in a slight
                 * right-lean pose while the caught text appears. */
                bui_ball_set_oam(BALL_SHAKE_OAM_Y, BALL_SHAKE_OAM_X, 2);
                Audio_PlaySFX_CaughtMon();
                snprintf(s_msg_buf, sizeof(s_msg_buf), "All right!\n%s was caught!", ename);
                Text_ShowASCII(s_msg_buf);
                bui_state = BUI_CAUGHT;
            } else {
                bui_ball_hide();
                /* SHOWPIC: restore enemy if hidden (1–3 shake fail) */
                if (s_shake_total > 0)
                    bui_set_enemy_oam_visible(1);
                /* Fail texts: ItemUseBallText00-04 (data/text/text_6.asm:1-27) */
                const char *fail;
                switch (s_catch_result) {
                case CATCH_RESULT_CANNOT_CATCH:
                    fail = "It dodged the\nthrown BALL!\n\nThis POKeMON\ncan't be caught!";
                    break;
                case CATCH_RESULT_0_SHAKES:
                    fail = "You missed the\nPOKeMON!";
                    break;
                case CATCH_RESULT_1_SHAKE:
                    fail = "Darn! The\nPOKeMON broke\nfree!";
                    break;
                case CATCH_RESULT_2_SHAKES:
                    fail = "Aww! It appeared\nto be caught!";
                    break;
                default: /* CATCH_RESULT_3_SHAKES */
                    fail = "Shoot! It was so\nclose too!";
                    break;
                }
                Text_ShowASCII(fail);
                bui_state = BUI_SWITCH_ENEMY_TURN;
            }
            break;
        }

        int phase = s_shake_frame % SHAKE_CYCLE;

        /* DoBallShakeSpecialEffects: at start of each shake play SFX_TINK + 40f pause.
         * In our non-blocking model the pause is 40 frames of stationary ball. */
        if (phase == 0)
            Audio_PlaySFX_Tink();

        if (phase < SHAKE_DELAY) {
            /* 40-frame pause: ball neutral at BASECOORD_21 */
            bui_ball_set_oam(BALL_SHAKE_OAM_Y, BALL_SHAKE_OAM_X, 0);
        } else {
            /* 16-frame visual rock:
             *   frames  0- 3 (vp 0-3):  FrameBlock03 neutral
             *   frames  4- 7 (vp 4-7):  FrameBlock04 tilt-right
             *   frames  8-11 (vp 8-11): FrameBlock03 neutral
             *   frames 12-15 (vp 12-15):FrameBlock05 tilt-left */
            int vp = phase - SHAKE_DELAY;   /* 0–15 */
            int fb = (vp < 4) ? 0 : (vp < 8) ? 1 : (vp < 12) ? 0 : 2;
            bui_ball_set_oam(BALL_SHAKE_OAM_Y, BALL_SHAKE_OAM_X, fb);
        }

        s_shake_frame++;
        break;
    }

    /* ---- Add caught mon to party, then mirror item_effects.asm ------- *
     * Success flow in the original:
     *   caught text -> set owned bit -> if new, show "New #DEX data..."
     *   -> ShowPokedexData -> finish add/send -> end battle. */
    case BUI_CAUGHT: {
        const char *ename;

        s_caught_species = wEnemyMon.species;
        s_caught_dex = gSpeciesToDex[s_caught_species];
        s_caught_new_entry = !bui_pokedex_owned_num(s_caught_dex);
        s_caught_sent_to_box = 0;
        s_caught_dex_started = 0;
        ename = Pokemon_GetName(s_caught_dex);

        /* The battle UI handles wild ball catches directly, bypassing
         * Battle_UseItem's Pokédex update path. Mark the species owned here
         * once the catch is confirmed, whether it goes to party or PC. */
        Pokedex_SetOwned(s_caught_species);

        if (wPartyCount < PARTY_LENGTH) {
            /* AddPartyMon — copy wEnemyMon into wPartyMons[wPartyCount] */
            uint8_t party_slot = wPartyCount;
            party_mon_t *p = &wPartyMons[party_slot];
            memset(p, 0, sizeof(*p));
            p->base.species    = s_caught_species;
            p->base.hp         = wEnemyMon.hp;    /* keep current (wounded) HP */
            p->base.box_level  = wEnemyMon.level; /* critical for PC storage integrity */
            p->base.status     = wEnemyMon.status; /* keep status */
            p->base.type1      = wEnemyMon.type1;
            p->base.type2      = wEnemyMon.type2;
            p->base.catch_rate = wEnemyMon.catch_rate;
            memcpy(p->base.moves, wEnemyMon.moves, 4);
            memcpy(p->base.pp,    wEnemyMon.pp,    4);
            p->base.dvs        = wEnemyMon.dvs;
            p->base.ot_id      = wPlayerID;
            /* Experience: base exp for caught level */
            uint8_t caught_dex = gSpeciesToDex[s_caught_species];
            uint8_t growth = (caught_dex > 0 && caught_dex <= NUM_POKEMON)
                           ? gBaseStats[caught_dex].growth_rate
                           : GROWTH_MEDIUM_FAST;
            uint32_t xp = CalcExpForLevel(growth, wEnemyMon.level);
            p->base.exp[0] = (uint8_t)((xp >> 16) & 0xFF);
            p->base.exp[1] = (uint8_t)((xp >>  8) & 0xFF);
            p->base.exp[2] = (uint8_t)( xp         & 0xFF);
            p->level   = wEnemyMon.level;
            p->max_hp  = wEnemyMon.max_hp;
            p->atk     = wEnemyMon.atk;
            p->def     = wEnemyMon.def;
            p->spd     = wEnemyMon.spd;
            p->spc     = wEnemyMon.spc;
            memcpy(wPartyMonOT[party_slot], wPlayerName, NAME_LENGTH);
            Pokemon_EncodeNameString(ename ? ename : "", wPartyMonNicks[party_slot]);
            /* Update party species list (0xFF-terminated) */
            wPartySpecies[party_slot]     = s_caught_species;
            wPartySpecies[party_slot + 1] = 0xFF;
            wPartyCount++;
        } else {
            s_caught_sent_to_box = 1;
            if (!Pokemon_SendBattleMonToBox(&wEnemyMon)) {
                /* Should be unreachable because ItemUseBall rejects a full
                 * current box before the throw, but fail closed if state drifted. */
                bui_state = BUI_END;
                break;
            }
        }

        wBattleResult = BATTLE_OUTCOME_CAUGHT;
        if (s_caught_new_entry) {
            snprintf(s_msg_buf, sizeof(s_msg_buf),
                     "New #DEX data\nwill be added\nfor %s!", ename);
            Text_ShowASCII(s_msg_buf);
            bui_state = BUI_CAUGHT_DEX_WAIT;
        } else if (s_caught_sent_to_box) {
            snprintf(s_msg_buf, sizeof(s_msg_buf),
                     "%s was\ntransferred to\nsomeone's PC!", ename);
            Text_ShowASCII(s_msg_buf);
            bui_state = BUI_CAUGHT_BOX_TEXT;
        } else {
            bui_state = BUI_END;
        }
        break;
    }

    case BUI_CAUGHT_DEX_WAIT:
        if (!s_caught_dex_started) {
            Pokedex_ShowData(s_caught_dex);
            s_caught_dex_started = 1;
            break;
        }
        if (Pokedex_IsShowingData()) {
            Pokedex_ShowDataTick();
            break;
        }
        /* Catch flow returns here outside the normal start-menu Pokedex path,
         * so explicitly reset any lingering text/window state before ending
         * the battle or showing the PC transfer text. */
        if (Text_IsOpen())
            Text_Close();
        hWY = 144;
        bui_restore_catch_screen_after_dex();
        if (s_caught_sent_to_box) {
            const char *ename = Pokemon_GetName(s_caught_dex);
            snprintf(s_msg_buf, sizeof(s_msg_buf),
                     CheckEvent(EVENT_MET_BILL)
                        ? "%s was\ntransferred to\nBILL's PC!"
                        : "%s was\ntransferred to\nsomeone's PC!",
                     ename);
            Text_ShowASCII(s_msg_buf);
            bui_state = BUI_CAUGHT_BOX_TEXT;
        } else {
            bui_state = BUI_END;
        }
        break;

    case BUI_CAUGHT_BOX_TEXT:
        if (Text_IsOpen()) break;
        bui_state = BUI_END;
        break;

    /* ---- Voluntary switch: enemy executes their pre-selected move -- *
     * Mirrors the second-move execution (player switched = "first     *
     * mover", enemy attacks as "second mover").                        */
    case BUI_SWITCH_ENEMY_TURN: {
        const char *wild_pfx = (wIsInBattle == 1) ? "Wild " : "";
        /* Set up name buffers for faint-check messages in BUI_TURN_END */
        snprintf(s_name_a, sizeof(s_name_a), "%s",
                 Pokemon_GetName(gSpeciesToDex[wBattleMon.species]));
        s_pfx_a[0] = '\0';
        snprintf(s_name_b, sizeof(s_name_b), "%s",
                 Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]));
        snprintf(s_pfx_b, sizeof(s_pfx_b), "%s", wild_pfx);
        s_player_first = 1;  /* for BUI_TURN_END faint-check logic */

        hWhoseTurn = 1;
        bui_snapshot_pre(1);
        s_hp_pre_hp  = wBattleMon.hp;
        s_hp_pre_max = wBattleMon.max_hp;
        Battle_ExecuteEnemyMove();
        bui_setup_anim(1);
        bui_setup_hp_anim(1);
        s_anim_first = 0;   /* finishing "second" move → bui_finish_second_move */

        Text_KeepTilesOnClose();
        bui_show_after_move(1, s_pfx_b, s_name_b, wEnemySelectedMove, wEnemyMoveNum,
                            wCriticalHitOrOHKO, wMoveMissed, wDamageMultipliers & 0x7F);
        bui_state = BUI_MOVE_ANIM;
        break;
    }

    /* ---- Trainer victory: sprite slides in from right 56px ---------- *
     * Mirrors ScrollTrainerPicAfterBattle (engine/battle/core.asm).    *
     * s_slide_cx starts at 56 (set in bui_handle_enemy_fainted),       *
     * decrements 4px/tick until 0 = final position.                    */
    case BUI_TRAINER_VICTORY_SLIDE: {
        if (s_victory_timer == 0) {
            /* First tick: wipe enemy HUD + battle menu, keep player sprite/HUD.
             * Enemy sprite is OAM so no BG tiles to clear in rows 4-6.
             * Rows 5-11 hold the player back sprite + HUD — don't touch them. */
            bui_clear_rows(0, 4);                    /* enemy HUD (rows 0-3) + buffer */
            bui_clear_rows(12, SCREEN_HEIGHT - 1);   /* battle menu / move list */
            bui_place_player_sprite();               /* re-stamp in case anything cleared it */
            bui_draw_box();
            /* First tick: reload trainer sprite tiles and position OAM off-screen right. */
            int tc = (int)gEngagedTrainerClass - 1;
            if (tc >= 0 && tc < NUM_TRAINERS)
                for (int i = 0; i < TRAINER_CANVAS_TILES; i++)
                    Display_LoadSpriteTile((uint8_t)(ENEMY_SPR_TILE_BASE + i),
                                           gTrainerFrontSprite[tc][i]);
            for (int ty = 0; ty < 7; ty++)
                for (int tx = 0; tx < 7; tx++) {
                    int idx = ENEMY_SPR_OAM_BASE + ty * 7 + tx;
                    wShadowOAM[idx].y     = (uint8_t)(ENEMY_SPR_PX_Y + ty * 8 + OAM_Y_OFS);
                    wShadowOAM[idx].x     = (uint8_t)(VICTORY_TRAINER_PX_X + tx * 8 + OAM_X_OFS + s_slide_cx);
                    wShadowOAM[idx].tile  = (uint8_t)(ENEMY_SPR_TILE_BASE + ty * 7 + tx);
                    wShadowOAM[idx].flags = 0;
                }
            s_victory_timer = 1;  /* mark initialized */
            break;
        }
        if (s_slide_cx > 0) {
            s_slide_cx -= 2;  /* 8px per 4 frames = 2px/frame, matching DelayFrames 4 */
            if (s_slide_cx < 0) s_slide_cx = 0;
        }
        for (int ty = 0; ty < 7; ty++)
            for (int tx = 0; tx < 7; tx++) {
                int idx = ENEMY_SPR_OAM_BASE + ty * 7 + tx;
                wShadowOAM[idx].x = (uint8_t)(VICTORY_TRAINER_PX_X + tx * 8 + OAM_X_OFS + s_slide_cx);
            }
        if (s_slide_cx == 0) {
            s_victory_timer = 40;
            bui_state = BUI_TRAINER_VICTORY_PAUSE;
        }
        break;
    }

    /* ---- Trainer victory pause: 40 frames then defeat quote ---------- *
     * Mirrors the DelayFrames 40 after ScrollTrainerPicAfterBattle.     */
    case BUI_TRAINER_VICTORY_PAUSE: {
        if (--s_victory_timer <= 0) {
            if (gTrainerAfterText && gTrainerAfterText[0]) {
                Text_ShowASCII(gTrainerAfterText);
                bui_state = BUI_TRAINER_VICTORY_TEXT;
            } else {
                /* No defeat quote — show money text directly then fade. */
                if (s_trainer_money_text[0])
                    Text_ShowASCII(s_trainer_money_text);
                s_victory_timer = 0;
                bui_state = BUI_FADE_WHITE;
            }
        }
        break;
    }

    /* ---- Defeat quote (pages 1-2) dismissed ----------------------------
     * If badge recv text is queued: play key-item jingle AND show page 3
     * simultaneously — mirrors sound_level_up firing right as text_end
     * closes _PewterGymBrockReceivedBoulderBadgeText in PewterGym.asm. */
    case BUI_TRAINER_VICTORY_TEXT:
        if (s_badge_recv_text) {
            Audio_PlaySFX_GetKeyItem();
            Text_ShowASCII(s_badge_recv_text);
            s_badge_recv_text = NULL;
            bui_state = BUI_GYM_BADGE_JINGLE;
        } else {
            bui_state = BUI_WAIT_SOUND;
        }
        break;

    /* ---- "[PLAYER] received BADGE" text dismissed — show badge info --- */
    case BUI_GYM_BADGE_JINGLE:
        if (s_badge_info_text) {
            Text_ShowASCII(s_badge_info_text);
            s_badge_info_text = NULL;
        }
        bui_state = BUI_WAIT_SOUND;
        break;

    /* ---- Wait for cry to finish before continuing -------------------- *
     * Mirrors PlayCry in the original battle flow, which blocks in
     * WaitForSoundToFinish before returning to the caller. */
    case BUI_WAIT_CRY:
        if (!Audio_IsCryPlaying()) {
            if (s_wait_cry_text[0]) {
                /* Text_ShowASCII stores a pointer; copy into shared message
                 * buffer before clearing s_wait_cry_text. */
                snprintf(s_msg_buf, sizeof(s_msg_buf), "%s", s_wait_cry_text);
                Text_ShowASCII(s_msg_buf);
                s_wait_cry_text[0] = '\0';
            }
            bui_state = s_wait_cry_next_state;
        }
        break;

    /* ---- WaitForSoundToFinish: poll SFX channels, then money text ---- *
     * Mirrors home/delay.asm WaitForSoundToFinish: spins until Ch5/6/8   *
     * are all silent, then continues.  Usually completes in 1 tick.      */
    case BUI_WAIT_SOUND:
        if (!Audio_IsSFXPlaying()) {
            if (s_trainer_money_text[0])
                Text_ShowASCII(s_trainer_money_text);
            s_victory_timer = 0;
            /* ASM EvolutionAfterBattle path does not insert an extra whiteout
             * before "What? X is evolving!". */
            {
                uint8_t evo_slot, evo_new;
                if (Battle_CheckNextEvolution(&evo_slot, &evo_new))
                    bui_state = BUI_END;
                else
                    bui_state = BUI_FADE_WHITE;
            }
        }
        break;

    /* ---- Fade from white: hold ~30f then return to overworld --------- *
     * Mirrors GBPalWhiteOut + return to overworld (core.asm).           *
     * s_victory_timer == 0 on first tick: set palette white + start.   */
    case BUI_FADE_WHITE: {
        if (s_victory_timer == 0) {
            Display_SetPalette(0x00, 0x00, 0x00);  /* all white */
            s_victory_timer = 62;
        }
        if (--s_victory_timer <= 0) {
            /* Clear battle tilemap while palette is still white so no battle UI
             * bleeds through.  Leave palette white — game.c fades in from white. */
            bui_clear_rows(0, SCREEN_HEIGHT - 1);
            s_evo_screen_white = 1;  /* signal to BUI_EVOLUTION: already white+clear */
            bui_state = BUI_END;
        }
        break;
    }

    /* ---- Post-battle evolution screen --------------------------------- *
     * Mirrors evos_moves.asm + engine/movie/evolution.asm (EvolveMon).    *
     * Phase 0: print "What? X is evolving!" and hold                       *
     * Phase 1: clear scene + sprites (textbox remains)                    *
     * Phase 2: hold on cleared scene, then show old sprite + old cry      *
     * Phase 3: old sprite + old cry; wait cry then start Safari music     *
     * Phase 4: 80-frame countdown before flicker                          *
     * Phase 5: exact ASM anim loop (b=1..8, c=16..2)                      *
     * Phase 6: show final sprite + cry + result text                      *
     * Phase 7: apply/cancel, chain next evo or return                     */
    case BUI_EVOLUTION: {
/* Helper macro: load species front sprite into BG tilemap at hlcoord(7,2),
 * matching Evolution_LoadPic in ASM. */
#define EVO_SPR_COL 7  /* ASM: Evolution_LoadPic uses hlcoord 7,2 */
#define EVO_SPR_ROW 2
#define EVO_PIC_TILE_BASE PLAYER_SPR_BG_BASE
#define EVO_LOAD_SPRITE(species) do { \
    uint8_t _dex = gSpeciesToDex[(species)]; \
    if (_dex > 0 && _dex <= 151) { \
        for (int _ty = 0; _ty < 7; _ty++) \
            for (int _tx = 0; _tx < 7; _tx++) { \
                int _src = _ty * 7 + (6 - _tx); \
                uint8_t _tile[16]; \
                for (int _row = 0; _row < 8; _row++) { \
                    _tile[_row * 2 + 0] = bui_reverse_bits(gPokemonFrontSprite[_dex][_src][_row * 2 + 0]); \
                    _tile[_row * 2 + 1] = bui_reverse_bits(gPokemonFrontSprite[_dex][_src][_row * 2 + 1]); \
                } \
                uint8_t _tid = (uint8_t)(EVO_PIC_TILE_BASE + _ty * 7 + _tx); \
                Display_LoadTile(_tid, _tile); \
                bui_set_tile(EVO_SPR_COL + _tx, EVO_SPR_ROW + _ty, _tid); \
            } \
    } \
} while(0)

        switch (s_evo_phase) {

        case 0: {
            /* Entry text first; once text flow is done, transition immediately
             * into the clear->sprite sequence. */
            wDoNotWaitForButtonPress = 1;
            Text_InstantNext();
            snprintf(s_msg_buf, sizeof(s_msg_buf),
                     "What? %s is\nevolving!",
                     Pokemon_GetName(gSpeciesToDex[s_evo_old_species]));
            Text_ShowASCII(s_msg_buf);
            /* Let the opening text breathe for ~1 second. */
            s_evo_timer = 60;
            s_evo_phase = 1;
            break;
        }

        case 1:
            if (--s_evo_timer > 0) break;
            /* ClearSceneArea 12x20 + ClearSprites before EvolveMon. */
            bui_clear_rows(0, 11);
            /* Keep textbox rows intact; wipe only the battle scene area to white. */
            {
                static const uint8_t kWhiteTile[16] = {0};
                Display_LoadTile(0xFF, kWhiteTile);
                bui_fill_rows_tile_both(0, 11, 0xFF);
            }
            for (int i = 0; i < MAX_SPRITES; i++) wShadowOAM[i].y = 0; /* ClearSprites */
            Display_SetPalette(0xE4, 0xD0, 0xE0);
            /* EvolveMon prelude: stop music, play SFX_TINK, then Delay3. */
            Music_Stop();
            Audio_PlaySFX_Tink();
            /* Hold the cleared scene for ~1 second before sprite appears. */
            s_evo_timer = 60;
            s_evo_phase = 2;
            break;

        case 2:
            if (--s_evo_timer > 0) break;
            EVO_LOAD_SPRITE(s_evo_old_species);
            Audio_PlayCry(s_evo_old_species);
            s_evo_timer = 120;  /* fallback: max wait 2 sec for cry */
            s_evo_phase = 3;
            break;

        case 3:
            /* Wait for old cry to finish, then start evolution music and
             * apply ASM's ~80-frame lead-in before flicker starts. */
            if (Audio_IsSFXPlaying() && --s_evo_timer > 0) break;
            Music_Play(MUSIC_SAFARI_ZONE);
            s_evo_timer = 80;
            s_evo_phase = 4;
            break;

        case 4:
            /* 80-frame countdown, then start flicker. */
            if (--s_evo_timer > 0) break;
            /* ASM calls EvolutionSetWholeScreenPalette(c=1) here, but that routes
             * through RunPaletteCommand (SGB-only). On DMG/C this is effectively
             * a no-op, so do not force a full black palette in the port. */
            s_evo_blink = 0;
            s_evo_wave  = 1;
            s_evo_wave_units = 0;  /* 0 = pre-wave cancel-check window */
            s_evo_timer = 16;      /* ASM c starts at $10 (16) */
            s_evo_phase = 5;
            break;

        case 5: {
            /* Exact EvolveMon .animLoop cadence:
             *   1) Evolution_CheckForCancel(c): DelayFrame+poll B for c frames.
             *   2) Evolution_BackAndForthAnim(b): b units, each unit is
             *      new(Delay3) then old(Delay3).
             *   3) b++, c-=2, repeat until c==0 (8 waves total). */
            if (s_evo_wave_units == 0) {
                /* Cancel-check window (only here, like ASM). */
                if (hJoyPressed & PAD_B) {
                    s_evo_cancelled = 1;
                    s_evo_phase     = 6;
                    break;
                }
                if (--s_evo_timer > 0) break;
                s_evo_wave_units = s_evo_wave * 2; /* b units -> 2 toggles each */
                s_evo_timer = 3;                   /* Evolution_ChangeMonPic Delay3 */
                s_evo_blink = 0;                   /* start from old pic */
                break;
            }

            if (--s_evo_timer > 0) break;
            s_evo_timer = 3;
            s_evo_blink ^= 1;
            EVO_LOAD_SPRITE(s_evo_blink ? s_evo_new_species : s_evo_old_species);
            if (--s_evo_wave_units <= 0) {
                s_evo_wave++;
                if (s_evo_wave > 8) {
                    s_evo_phase = 6;
                    break;
                }
                s_evo_timer = 18 - (2 * s_evo_wave); /* ASM c progression: 16..2 */
                if (s_evo_timer < 1) s_evo_timer = 1;
                s_evo_wave_units = 0; /* enter next cancel-check window */
            }
            break;
        }

        case 6: {
            /* Show final sprite, play cry, show result text.
             * Mirrors EvolveMon lines 63-74: Evolution_ChangeMonPic + PlayCry. */
            uint8_t final_species = s_evo_cancelled ? s_evo_old_species : s_evo_new_species;
            EVO_LOAD_SPRITE(final_species);
            Music_Stop(); /* SFX_STOP_ALL_MUSIC before final cry */
            Audio_PlayCry(final_species);
            /* ASM immediately restores whole-screen palette (c=0) after final cry call. */
            Display_SetPalette(0xE4, 0xD0, 0xE0);
            if (!s_evo_cancelled) {
                snprintf(s_msg_buf, sizeof(s_msg_buf),
                         "%s evolved\ninto %s!",
                         Pokemon_GetName(gSpeciesToDex[s_evo_old_species]),
                         Pokemon_GetName(gSpeciesToDex[s_evo_new_species]));
            } else {
                snprintf(s_msg_buf, sizeof(s_msg_buf),
                         "%s\nstopped evolving.",
                         Pokemon_GetName(gSpeciesToDex[s_evo_old_species]));
            }
            Text_ShowASCII(s_msg_buf);
            s_evo_phase = 7;
            break;
        }

        case 7: {
            /* Tick after result text dismissed: commit or cancel, then chain. */
            if (!s_evo_cancelled)
                Battle_ApplyEvolution(s_evo_slot, s_evo_new_species);
            else
                Battle_CancelEvolution(s_evo_slot);

            uint8_t next_slot, next_species;
            if (Battle_CheckNextEvolution(&next_slot, &next_species)) {
                s_evo_slot         = next_slot;
                s_evo_old_species  = wPartyMons[next_slot].base.species;
                s_evo_new_species  = next_species;
                s_evo_phase        = 0;
                s_evo_cancelled    = 0;
                s_evo_timer        = 0;
                s_evo_screen_white = 0;
            } else {
                Music_Play(Music_GetMapID(wCurMap)); /* EvolutionAfterBattle -> PlayDefaultMusic */
                bui_state = BUI_INACTIVE;
            }
            break;
        }

        } /* switch s_evo_phase */
#undef EVO_LOAD_SPRITE
        break;
    }

    /* ---- Battle over: sync HP, check evolutions, mark inactive ------- */
    case BUI_END: {
        Display_SetShakeOffset(0, 0);
        /* Don't restore enemy sprite if the mon was caught (it stays hidden) */
        if (wBattleResult != BATTLE_OUTCOME_CAUGHT)
            bui_set_enemy_oam_visible(1);
        /* Sync active mon HP back to party */
        wPartyMons[wPlayerMonNumber].base.hp = wBattleMon.hp;
        wIsInBattle = 0;
        /* Clear cross-battle state to prevent carry-over */
        wBattleResult        = 0;
        wEscapedFromBattle   = 0;
        wPlayerSelectedMove  = 0;
        wEnemySelectedMove   = 0;
        /* Check for pending evolutions — if any, drive the animated screen;
         * otherwise fall through to BUI_INACTIVE immediately. */
        {
            uint8_t evo_slot, evo_new;
            if (Battle_CheckNextEvolution(&evo_slot, &evo_new)) {
                s_evo_slot         = evo_slot;
                s_evo_old_species  = wPartyMons[evo_slot].base.species;
                s_evo_new_species  = evo_new;
                s_evo_phase        = 0;
                s_evo_cancelled    = 0;
                s_evo_timer        = 0;
                s_evo_screen_white = 0;
                bui_state = BUI_EVOLUTION;
            } else {
                bui_state = BUI_INACTIVE;
            }
        }
        break;
    }
    }
}
