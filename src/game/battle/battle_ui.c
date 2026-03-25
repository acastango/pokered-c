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
#include "../party_menu.h"
#include "../../platform/hardware.h"
#include "../../platform/display.h"
#include "../../platform/audio.h"
#include "../music.h"
#include "../../data/font_data.h"
#include "../../data/moves_data.h"
#include "../../data/base_stats.h"
#include "../../data/pokemon_sprites.h"
#include "../constants.h"
#include "../text.h"
#include "../pokemon.h"
#include "../overworld.h"   /* gScrollTileMap, SCROLL_MAP_W */
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
    BUI_SEND_OUT,       /* trigger: reload Red + start trainer slide-out    */
    BUI_TRAINER_SLIDE_OUT, /* Red's OAM slides left off screen (4px/tick)    */
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
    BUI_LEVELUP_STATS,  /* wait for A on the level-up stats box     */
    BUI_ENEMY_FAINT_ANIM, /* enemy sprite slides down off-screen (14f) */
    BUI_PLAYER_FAINTED, /* player mon fainted: checks + open menu   */
    BUI_USE_NEXT_MON,   /* wild only: "Use next Pokemon?" YES/NO    */
    BUI_PARTY_SELECT,   /* party menu: choose next mon              */
    BUI_SWITCH_SELECT,  /* voluntary: party menu for mid-turn switch */
    BUI_RETREAT_ANIM,   /* voluntary: player sprite shrink (7×7→3×3) */
    BUI_SWITCH_ENEMY_TURN, /* voluntary: enemy executes their pre-selected move */
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

/* BUI_EXP_DRAIN destination and optional suffix message */
static bui_state_t   bui_exp_dest   = BUI_END;
static char          s_exp_suffix[64];

/* BUI_ENEMY_FAINT_ANIM state
 * Mirrors SlideDownFaintedMonPic (engine/battle/core.asm:1181):
 *   7 steps × 2 frames = 14 frames.  Each step shifts the sprite's OAM
 *   rows down by 8px and hides the bottommost row (it scrolled off the
 *   sprite area in the original BG-tile implementation). */
static int s_faint_step  = 0;  /* 0=inactive, 1-7=animation progress */
static int s_faint_timer = 0;
/* Pending level-up stats box — drawn after the "grew to level N!" text closes */
static levelup_stats_t s_pending_lvl_stats;

/* Pokemon appear (grow) animation state — AnimateSendingOutMon equivalent.
 * stage 0 = pokeball OAM at center; 1 = 3×3 crop; 2 = 5×5 crop; 3 → BUI_DRAW_HUD */
static int s_grow_stage;
static int s_grow_frame;
/* 1 = grow animation was triggered by a voluntary mid-turn switch (→ BUI_SWITCH_ENEMY_TURN) */
static int s_grow_after_switch;

/* Retreat animation state (AnimateRetreatingPlayerMon equivalent).
 * stage 0 = full 7×7 (4f); 1 = 5×5 crop (4f); 2 = 3×3 crop (4f); 3 = done */
static int     s_retreat_stage;
static int     s_retreat_frame;
static uint8_t s_retreat_species;  /* old mon species for sprite tile lookup */
static uint8_t s_switch_slot;      /* party slot chosen for voluntary switch */

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

static void bui_clear_rows(int r0, int r1) {
    for (int r = r0; r <= r1; r++)
        for (int c = 0; c < SCREEN_WIDTH; c++)
            bui_set_tile(c, r, BLANK_TILE_SLOT);
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
 *   49-97 : player back sprite (7×7 canvas, 49 tiles)                  *
 *                                                                       *
 * OAM entries:                                                          *
 *   0-48  : enemy sprite tiles (7×7 grid, screen pixel 88,0)           */
#define ENEMY_SPR_TILE_BASE   0
#define PLAYER_SPR_BG_BASE    49   /* BG tile slots for back sprite */
#define ENEMY_SPR_OAM_BASE    0
/* Enemy: top-left at tile col 11, row 0 = pixel (88, 0) */
#define ENEMY_SPR_PX_X        88
#define ENEMY_SPR_PX_Y        0
/* Player back: hlcoord(1,5) = tile col 1, row 5 = pixel (8, 40) */
#define PLAYER_SPR_COL        1
#define PLAYER_SPR_ROW        5

/* Slide-in intro: player back sprite as OAM during the 72-frame slide animation.
 * Uses sprite tile slots 49-97 (separate from BG tile slots 49-97 for the same gfx). */
#define PLAYER_SLIDE_OAM_BASE 49    /* OAM entries 49-97, sprite tiles 49-97 */
/* Party pokeball OAM (DrawAllPokeballs): 6 entries after the two sprite blocks. */
#define POKEBALL_OAM_BASE          98   /* player party balls: OAM 98-103 */
#define ENEMY_POKEBALL_OAM_BASE   104   /* enemy party balls (trainer only): OAM 104-109 */
#define POKEBALL_TILE_BASE    110   /* sprite tile 110=normal,111=status,112=fainted,113=empty
                                    * Must not overlap player slide tiles 49-97 or enemy tiles 0-48. */
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
     * Tiles go into tile_gfx slots 49-97, written to gScrollTileMap at (col 1-7, row 5-11).
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
    bui_draw_enemy_hud();
    bui_draw_player_hud();

    /* Play victory jingle — mirrors PlayBattleVictoryMusic (core.asm:959).
     * Fires on wild victory and trainer victory; silence on mid-trainer faint. */
    if (wBattleResult == BATTLE_OUTCOME_WILD_VICTORY)
        Music_Play(MUSIC_DEFEATED_WILD_MON);
    else if (wBattleResult == BATTLE_OUTCOME_TRAINER_VICTORY)
        Music_Play(MUSIC_DEFEATED_TRAINER);

    /* Show "[Enemy] fainted!" — exp texts drained afterward in BUI_EXP_DRAIN */
    snprintf(s_msg_buf, sizeof(s_msg_buf), "%s%s\nfainted!", faint_pfx, faint_name);
    Text_ShowASCII(s_msg_buf);

    if (wBattleResult == BATTLE_OUTCOME_WILD_VICTORY) {
        bui_exp_dest    = BUI_END;
        s_exp_suffix[0] = '\0';
    } else if (wBattleResult == BATTLE_OUTCOME_TRAINER_VICTORY) {
        /* Mirrors TrainerDefeatedText: "[PLAYER] defeated [TRAINER]!"
         * We decode wPlayerName from GB charset (0x80='A'..0x99='Z'). */
        char player_ascii[NAME_LENGTH] = "RED";
        for (int i = 0; i < NAME_LENGTH - 1; i++) {
            uint8_t c = wPlayerName[i];
            if (c == 0x50) break;
            if (c >= 0x80 && c <= 0x99) player_ascii[i] = (char)('A' + c - 0x80);
            else if (c >= 0xA0 && c <= 0xB9) player_ascii[i] = (char)('a' + c - 0xA0);
            else { player_ascii[i] = '?'; }
            player_ascii[i + 1] = '\0';
        }
        bui_exp_dest = BUI_END;
        snprintf(s_exp_suffix, sizeof(s_exp_suffix),
                 "%s\ndefeated the trainer!", player_ascii);
    } else {
        /* Trainer has more mons — enemy was already replaced by handler.
         * Show "Foe sent out X!" after exp texts are drained. */
        const char *new_name = Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]);
        bui_exp_dest = BUI_DRAW_HUD;
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

    /* Load enemy front sprite into sprite tile slots 0-48. */
    if (e_dex > 0 && e_dex <= 151) {
        for (int i = 0; i < POKEMON_FRONT_CANVAS_TILES; i++)
            Display_LoadSpriteTile((uint8_t)(ENEMY_SPR_TILE_BASE + i),
                                   gPokemonFrontSprite[e_dex][i]);
    }

    /* Load Red's trainer back sprite into sprite tile slots 49-97 for OAM use during slide.
     * Matches LoadPlayerBackPic (core.asm:6202) which loads RedPicBack (not the pokemon).
     * The pokemon back sprite is loaded later in BUI_SEND_OUT via bui_load_sprites(). */
    for (int i = 0; i < 49; i++)
        Display_LoadSpriteTile((uint8_t)(PLAYER_SLIDE_OAM_BASE + i), kRedBackSprite[i]);

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
                wShadowOAM[idx].tile  = (uint8_t)(PLAYER_SLIDE_OAM_BASE + ty * 7 + tx);
                wShadowOAM[idx].flags = 0;
            }
        }
    }
}

int BattleUI_IsActive(void) {
    return bui_state != BUI_INACTIVE;
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

        /* Enemy: slides right as cx decreases (enters from left edge). */
        if (e_dex2 > 0 && e_dex2 <= 151) {
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
             * Pokeball tiles are loaded to sprite slots 110-113 (POKEBALL_TILE_BASE),
             * well clear of Red's slide sprite tiles 49-97 — so Red's OAM stays active
             * and he remains visible during the "appeared!" text, exactly as in the original
             * where his body stays on the BG tilemap through PrintBeginningBattleText. */
            bui_draw_pokeballs();
            /* "Wild X appeared!" — mirrors PrintBeginningBattleText. */
            const char *e_name2 = Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]);
            /* PlayCry — mirrors PrintBeginningBattleText calling PlayCry(wEnemyMonSpecies2)
             * for wild battles.  Trainer battles play no cry here (they play a SFX instead). */
            if (wIsInBattle != 2)
                Audio_PlayCry(wEnemyMon.species);
            if (wIsInBattle == 2)
                snprintf(s_msg_buf, sizeof(s_msg_buf), "Foe %s\nwants to fight!", e_name2);
            else
                snprintf(s_msg_buf, sizeof(s_msg_buf), "Wild %s\nappeared!", e_name2);
            Text_ShowASCII(s_msg_buf);
            bui_state = BUI_SEND_OUT;
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
        s_slide_cx = 0;   /* slide-out offset counter (counts up, 4px/tick) */
        bui_state = BUI_TRAINER_SLIDE_OUT;
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
        if (s_slide_cx >= 64) {
            for (int i = 0; i < 49; i++)
                wShadowOAM[PLAYER_SLIDE_OAM_BASE + i].y = 0;
            /* Load pokemon back sprite BG tiles now (needed for grow animation). */
            uint8_t p_dex2 = gSpeciesToDex[wBattleMon.species];
            if (p_dex2 > 0 && p_dex2 <= 151) {
                for (int i = 0; i < POKEMON_BACK_TILES; i++)
                    Display_LoadTile((uint8_t)(PLAYER_SPR_BG_BASE + i),
                                     gPokemonBackSprite[p_dex2][i]);
            }
            /* "Go! [name]!" — mirrors PrintSendOutMonMessage.
             * In SendOutMon the text fires BEFORE Draw*HUDAndHPBar, so HUDs
             * are drawn only after the player dismisses this box (in
             * BUI_POKEMON_APPEAR's first frame). */
            const char *p_name2 = Pokemon_GetName(gSpeciesToDex[wBattleMon.species]);
            snprintf(s_msg_buf, sizeof(s_msg_buf), "Go! %s!", p_name2);
            Text_ShowASCII(s_msg_buf);
            s_grow_stage = 0;
            s_grow_frame = 0;
            bui_state = BUI_POKEMON_APPEAR;
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
            /* Pokeball opens: fire SFX_BALL_POOF (DoPoofSpecialEffects fires at frame 5
             * of the subanimation — we fire at the pokeball→pokemon transition). */
            Audio_PlaySFX_BallPoof();
            /* 3×3 center-crop: tiles (tx=2..4, ty=2..4) at hlcoord(3,9). */
            wShadowOAM[POKEBALL_OAM_BASE].y = 0;
            bui_clear_rect(1, 5, 7, 11);
            if (p_dex3 > 0 && p_dex3 <= 151)
                for (int dty = 0; dty < 3; dty++)
                    for (int dtx = 0; dtx < 3; dtx++)
                        bui_set_tile(3 + dtx, 9 + dty,
                            (uint8_t)(PLAYER_SPR_BG_BASE + (2+dty)*7 + (2+dtx)));
            s_grow_stage = 1;  s_grow_frame = 0;
        } else if (s_grow_stage == 1 && s_grow_frame >= 4) {
            /* 5×5 center-crop: tiles (tx=1..5, ty=1..5) at hlcoord(2,7). */
            bui_clear_rect(1, 5, 7, 11);
            if (p_dex3 > 0 && p_dex3 <= 151)
                for (int dty = 0; dty < 5; dty++)
                    for (int dtx = 0; dtx < 5; dtx++)
                        bui_set_tile(2 + dtx, 7 + dty,
                            (uint8_t)(PLAYER_SPR_BG_BASE + (1+dty)*7 + (1+dtx)));
            s_grow_stage = 2;  s_grow_frame = 0;
        } else if (s_grow_stage == 2 && s_grow_frame >= 4) {
            /* Full 7×7 at hlcoord(1,5) → done.
             * PlayCry — mirrors SendOutMon calling PlayCry(wCurPartySpecies)
             * after AnimateSendingOutMon. */
            Audio_PlayCry(wBattleMon.species);
            bui_place_player_sprite();
            s_grow_stage = 0;  s_grow_frame = 0;
            if (s_grow_after_switch) {
                /* Voluntary switch: enemy still needs to take their turn */
                s_grow_after_switch = 0;
                bui_state = BUI_SWITCH_ENEMY_TURN;
            } else {
                bui_state = BUI_DRAW_HUD;
            }
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
                bui_cursor = 0;
                bui_draw_move_menu(0);
                bui_state = BUI_MOVE_SELECT;
                break;
            case 1: /* PKMN — voluntary switch */
                bui_set_enemy_oam_visible(0);  /* hide enemy sprite over party menu */
                PartyMenu_Open(2 /* battle voluntary: SWITCH/STATS/CANCEL submenu */);
                bui_state = BUI_SWITCH_SELECT;
                break;
            case 2: /* ITEM — not yet implemented */
                Text_ShowASCII("Can't use items\nin battle yet!");
                bui_state = BUI_DRAW_HUD;
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
            bui_clear_rect(0, 8, 10, 11);  /* erase PP info box before executing move */
            bui_place_player_sprite();      /* restore sprite tiles cleared by PP box */
            bui_draw_player_hud();          /* col 10 (HP label) was inside clear rect */

            /* Set selected move (no PP → Struggle) */
            if ((wBattleMon.pp[bui_cursor] & 0x3F) == 0) {
                wPlayerSelectedMove  = MOVE_STRUGGLE;
                wPlayerMoveListIndex = 0;
            } else {
                wPlayerSelectedMove  = move;
                wPlayerMoveListIndex = (uint8_t)bui_cursor;
            }

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
                } else {
                    const char *nn = Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]);
                    bui_exp_dest = BUI_DRAW_HUD;
                    snprintf(s_exp_suffix, sizeof(s_exp_suffix), "Foe sent out\n%s!", nn);
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
                } else {
                    const char *nn = Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]);
                    bui_exp_dest = BUI_DRAW_HUD;
                    snprintf(s_exp_suffix, sizeof(s_exp_suffix), "Foe sent out\n%s!", nn);
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
        bui_state = BUI_DRAW_HUD;
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
            snprintf(s_msg_buf, sizeof(s_msg_buf), "%s", s_exp_suffix);
            s_exp_suffix[0] = '\0';
            /* PlayCry — mirrors trainer battle calling PlayCry(wEnemyMonSpecies2)
             * after AnimateSendingOutMon when the trainer sends out their next mon. */
            if (bui_exp_dest == BUI_DRAW_HUD)
                Audio_PlayCry(wEnemyMon.species);
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
            } else if (!Battle_AnyEnemyPokemonAliveCheck()) {
                Battle_TrainerBattleVictory();
                bui_exp_dest = BUI_END;
                snprintf(s_exp_suffix, sizeof(s_exp_suffix),
                         "Trainer has no more\nusable Pokemon!");
            } else {
                Battle_ReplaceFaintedEnemyMon();
                const char *nn = Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]);
                bui_exp_dest = BUI_PLAYER_FAINTED;
                snprintf(s_exp_suffix, sizeof(s_exp_suffix), "Foe sent out\n%s!", nn);
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
        } else if (s_retreat_stage == 1 && s_retreat_frame >= 4) {
            /* Shrink from 5×5 to 3×3 center-crop */
            bui_clear_rect(1, 5, 7, 11);
            if (rdex > 0 && rdex <= 151)
                for (int dty = 0; dty < 3; dty++)
                    for (int dtx = 0; dtx < 3; dtx++)
                        bui_set_tile(3 + dtx, 9 + dty,
                            (uint8_t)(PLAYER_SPR_BG_BASE + (2+dty)*7 + (2+dtx)));
            s_retreat_stage = 2;  s_retreat_frame = 0;
        } else if (s_retreat_stage == 2 && s_retreat_frame >= 4) {
            /* Mon gone — play poof SFX, clear sprite area */
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

    /* ---- Battle over: sync HP, mark inactive ---------------------- */
    case BUI_END:
        Display_SetShakeOffset(0, 0);
        bui_set_enemy_oam_visible(1);
        /* Sync active mon HP back to party */
        wPartyMons[wPlayerMonNumber].base.hp = wBattleMon.hp;
        wIsInBattle = 0;
        /* Clear cross-battle state to prevent carry-over */
        wBattleResult        = 0;
        wEscapedFromBattle   = 0;
        wPlayerSelectedMove  = 0;
        wEnemySelectedMove   = 0;
        /* HUD tiles ($62–$78) use dedicated slots — no restore needed */
        Battle_EvolutionAfterBattle();
        bui_state = BUI_INACTIVE;
        break;
    }
}
