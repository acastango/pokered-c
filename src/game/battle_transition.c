/* battle_transition.c
 *
 * Ports engine/battle/battle_transitions.asm to a non-blocking C state machine.
 *
 * Transition type is a 3-bit index (matching the original):
 *   bit 0 : trainer battle
 *   bit 1 : enemy >= player level + 3 (stronger enemy)
 *   bit 2 : dungeon map
 *
 * Type dispatch (BattleTransitions table):
 *   0 = DoubleCircle      (wild,  weak)
 *   1 = Spiral-inward     (trainer, weak)
 *   2 = Circle            (wild,  strong)
 *   3 = Spiral-outward    (trainer, strong)
 *   4 = HorizontalStripes (dungeon wild,  weak)
 *   5 = Shrink            (dungeon trainer, weak)
 *   6 = VerticalStripes   (dungeon wild,  strong)
 *   7 = Split             (dungeon trainer, strong)
 *
 * Frame budget (60 Hz):
 *   Flash:          12 entries × 2 frames × 3 cycles = 72 frames (types 0,2 only)
 *   DoubleCircle:   10 steps × 3 frames               = 30 frames
 *   Circle:         20 steps × 3 frames               = 60 frames
 *   HStripes:       20 steps × 3 frames               = 60 frames
 *   VStripes:       18 steps × 3 frames               = 54 frames
 *   Spiral(out):    360 substeps / 3 per frame         = 120 frames
 *   Spiral(in):     ~324 tiles / 7 per frame           = ~46 frames
 *   Shrink:          9 steps × 6 frames               = 54 frames
 *   Split:           9 steps × 6 frames               = 54 frames
 */
#include "battle_transition.h"
#include "constants.h"
#include "../platform/display.h"
#include "../platform/hardware.h"
#include "overworld.h"
#include "player.h"

#include <string.h>
#include <stdint.h>

/* ------------------------------------------------------------------ */
/*  Dungeon map detection — mirrors DungeonMaps1/DungeonMaps2 tables  */
/*  from data/maps/dungeon_maps.asm + constants/map_constants.asm     */
/* ------------------------------------------------------------------ */
#define MAP_VIRIDIAN_FOREST       0x33
#define MAP_ROCK_TUNNEL_1F        0x52
#define MAP_ROCK_TUNNEL_B1F       0xE8
#define MAP_SEAFOAM_ISLANDS_1F    0xC0
#define MAP_MT_MOON_1F            0x3B
#define MAP_MT_MOON_B2F           0x3D
#define MAP_SS_ANNE_1F            0x5F
#define MAP_HALL_OF_FAME          0x76
#define MAP_LAVENDER_POKECENTER   0x8D
#define MAP_LAVENDER_CUBONE_HOUSE 0x97
#define MAP_SILPH_CO_2F           0xCF
#define MAP_CERULEAN_CAVE_1F      0xE4

static int is_dungeon_map(void) {
    uint8_t m = wCurMap;
    /* DungeonMaps1: individual entries */
    if (m == MAP_VIRIDIAN_FOREST)    return 1;
    if (m == MAP_ROCK_TUNNEL_1F)     return 1;
    if (m == MAP_SEAFOAM_ISLANDS_1F) return 1;
    if (m == MAP_ROCK_TUNNEL_B1F)    return 1;
    /* DungeonMaps2: inclusive ranges */
    if (m >= MAP_MT_MOON_1F          && m <= MAP_MT_MOON_B2F)           return 1;
    if (m >= MAP_SS_ANNE_1F          && m <= MAP_HALL_OF_FAME)           return 1;
    if (m >= MAP_LAVENDER_POKECENTER && m <= MAP_LAVENDER_CUBONE_HOUSE)  return 1;
    if (m >= MAP_SILPH_CO_2F         && m <= MAP_CERULEAN_CAVE_1F)       return 1;
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Internal state machine phases                                      */
/* ------------------------------------------------------------------ */
typedef enum {
    BPHASE_IDLE = 0,
    BPHASE_FLASH,
    BPHASE_ANIM,
    BPHASE_DONE,
} BPhase;

static BPhase g_phase       = BPHASE_IDLE;
static int    g_type        = 0;
static int    g_step        = 0;
static int    g_frame       = 0;

/* Flash sub-state */
static int    g_flash_idx   = 0;
static int    g_flash_cycle = 0;

/* Outward spiral sub-state */
static int    g_sp_tx, g_sp_ty;
static int    g_sp_dir;     /* 0=up 1=left 2=down 3=right */
static int    g_sp_total;   /* substeps executed */

/* Inward spiral — pre-computed sequence */
#define ISP_MAX 400
static int8_t g_isp_x[ISP_MAX];
static int8_t g_isp_y[ISP_MAX];
static int    g_isp_len;
static int    g_isp_idx;

/* ------------------------------------------------------------------ */
/*  Constants from pokered                                             */
/* ------------------------------------------------------------------ */

/* All-black 8×8 tile (2bpp — all pixels color 3, the darkest shade).
 * Loaded at BG tile slot 0xFF, matching vChars1 tile $7F in the original. */
static const uint8_t kBlackTile[16] = {
    0xFF,0xFF, 0xFF,0xFF, 0xFF,0xFF, 0xFF,0xFF,
    0xFF,0xFF, 0xFF,0xFF, 0xFF,0xFF, 0xFF,0xFF,
};

/* BattleTransition_FlashScreenPalettes — 12 BGP bytes (terminal skipped).
 * dc a,b,c,d packs as (d<<6)|(c<<4)|(b<<2)|a where a..d are shade indices 0-3. */
static const uint8_t kFlashPals[12] = {
    0x6F,  /* dc 3,3,2,1 */
    0xBF,  /* dc 3,3,3,2 */
    0xFF,  /* dc 3,3,3,3 */
    0xBF,  /* dc 3,3,3,2 */
    0x6F,  /* dc 3,3,2,1 */
    0x1B,  /* dc 3,2,1,0 */
    0x06,  /* dc 2,1,0,0 */
    0x01,  /* dc 1,0,0,0 */
    0x00,  /* dc 0,0,0,0 */
    0x01,  /* dc 1,0,0,0 */
    0x06,  /* dc 2,1,0,0 */
    0x1B,  /* dc 3,2,1,0 */
};

/* BattleTransition_CircleData1..5 */
static const int8_t kCData1[] = { 2,  3,  5,  4,  9, -1 };
static const int8_t kCData2[] = { 1,  1,  2,  2,  4,  2,  4,  2,  3, -1 };
static const int8_t kCData3[] = { 2,  1,  3,  1,  4,  1,  4,  1,  4,  1,
                                   3,  1,  2,  1,  1,  1,  1, -1 };
static const int8_t kCData4[] = { 4,  1,  4,  0,  3,  1,  3,  0,  2,  1,
                                   2,  0,  1, -1 };
static const int8_t kCData5[] = { 4,  0,  3,  0,  3,  0,  2,  0,  2,  0,
                                   1,  0,  1,  0,  1, -1 };

/* Half-circle table entry */
typedef struct { int qx; const int8_t *data; int tx, ty; } HC;

/* BattleTransition_HalfCircle1 */
static const HC kHC1[10] = {
    { 1, kCData1, 18,  6 },
    { 1, kCData2, 19,  3 },
    { 1, kCData3, 18,  0 },
    { 1, kCData4, 14,  0 },
    { 1, kCData5, 10,  0 },
    { 0, kCData5,  9,  0 },
    { 0, kCData4,  5,  0 },
    { 0, kCData3,  1,  0 },
    { 0, kCData2,  0,  3 },
    { 0, kCData1,  1,  6 },
};

/* BattleTransition_HalfCircle2 */
static const HC kHC2[10] = {
    { 0, kCData1,  1, 11 },
    { 0, kCData2,  0, 14 },
    { 0, kCData3,  1, 17 },
    { 0, kCData4,  5, 17 },
    { 0, kCData5,  9, 17 },
    { 1, kCData5, 10, 17 },
    { 1, kCData4, 14, 17 },
    { 1, kCData3, 18, 17 },
    { 1, kCData2, 19, 14 },
    { 1, kCData1, 18, 11 },
};

/* ------------------------------------------------------------------ */
/*  Tile helpers                                                        */
/* ------------------------------------------------------------------ */

/* Write tile to the visible portion of gScrollTileMap.
 * Screen tile (tx,ty) maps to buffer offset (ty+2)*SCROLL_MAP_W + (tx+2). */
static void set_stile(int tx, int ty, uint8_t tile) {
    if ((unsigned)tx >= SCREEN_WIDTH || (unsigned)ty >= SCREEN_HEIGHT) return;
    gScrollTileMap[(ty + 2) * SCROLL_MAP_W + (tx + 2)] = tile;
}

static uint8_t get_stile(int tx, int ty) {
    if ((unsigned)tx >= SCREEN_WIDTH || (unsigned)ty >= SCREEN_HEIGHT) return 0xFF;
    return gScrollTileMap[(ty + 2) * SCROLL_MAP_W + (tx + 2)];
}

/* ------------------------------------------------------------------ */
/*  Circle sub-routine — ports BattleTransition_Circle_Sub3           */
/*                                                                     */
/*  quad_x : 0=CIRCLE_LEFT(go left), 1=CIRCLE_RIGHT(go right)         */
/*  quad_y : 0=go down each row,     1=go up each row                  */
/* ------------------------------------------------------------------ */
static void circle_sub3(const int8_t *data, int tx, int ty, int qx, int qy) {
    while (1) {
        /* Read run length (always ≥ 0) */
        int8_t raw = *data++;
        if (raw == -1) break;
        int count = (uint8_t)raw;

        /* Write count tiles horizontally (tx stays fixed per iteration). */
        int cx = tx;
        for (int i = 0; i < count; i++) {
            set_stile(cx, ty, 0xFF);
            if (qx) cx++; else cx--;   /* right or left */
        }
        /* tx is restored (push/pop in original) — only ty advances. */
        if (qy) ty--; else ty++;       /* up or down */

        /* Read horizontal offset for the next row's start column. */
        int8_t off = *data++;
        if (off == -1) break;
        if (off == 0)  continue;       /* same column start */
        /* Adjust tx in the OPPOSITE direction of writing. */
        if (qx) tx -= off; else tx += off;
    }
}

static void apply_hc(const HC *e, int qy) {
    circle_sub3(e->data, e->tx, e->ty, e->qx, qy);
}

/* ------------------------------------------------------------------ */
/*  Outward spiral helpers                                             */
/*  Ports BattleTransition_OutwardSpiral_ (one substep per call).     */
/*                                                                     */
/*  Direction encoding (matches the ASM $0/$1/$2/$3 constants):        */
/*    0=up  1=left  2=down  3=right                                    */
/*                                                                     */
/*  For each direction, check the "turn" neighbour:                    */
/*    right → check up    (row-1); if empty: turn up,    write there   */
/*    up    → check left  (col-1); if empty: turn left,  write there   */
/*    left  → check down  (row+1); if empty: turn down,  write there   */
/*    down  → check right (col+1); if empty: turn right, write there   */
/*  If neighbour is $FF (already filled): continue straight.           */
/* ------------------------------------------------------------------ */
static void outward_substep(void) {
    static const int8_t check_dx[4] = { -1, 0, 1, 0 }; /* up/left/down/right */
    static const int8_t check_dy[4] = {  0, 1, 0,-1 };
    static const int8_t move_dx [4] = {  0,-1, 0, 1 };
    static const int8_t move_dy [4] = { -1, 0, 1, 0 };

    int cx = g_sp_tx + check_dx[g_sp_dir];
    int cy = g_sp_ty + check_dy[g_sp_dir];

    /* Turn if check cell is empty (not yet filled). */
    int empty = (get_stile(cx, cy) != 0xFF);
    if (empty) {
        /* Write at check position and move there. */
        set_stile(cx, cy, 0xFF);
        g_sp_tx = cx;
        g_sp_ty = cy;
        g_sp_dir = (g_sp_dir + 1) & 3;
    } else {
        /* Continue straight. */
        int nx = g_sp_tx + move_dx[g_sp_dir];
        int ny = g_sp_ty + move_dy[g_sp_dir];
        set_stile(nx, ny, 0xFF);
        g_sp_tx = nx;
        g_sp_ty = ny;
    }
}

/* ------------------------------------------------------------------ */
/*  Inward spiral — pre-compute full path                             */
/*  Mirrors BattleTransition_InwardSpiral: traces rectangles from the  */
/*  outside in, writing every tile position into g_isp_x/y.           */
/* ------------------------------------------------------------------ */
static void isp_precompute(void) {
    int tx = 0, ty = 0;
    int n = 0;
    int c = SCREEN_HEIGHT - 1;  /* 17 */

    /* Pre-loop: DOWN c=17, then jump to .skip. */
    for (int i = 0; i < c && n < ISP_MAX; i++) {
        g_isp_x[n] = (int8_t)tx; g_isp_y[n++] = (int8_t)ty; ty++;
    }
    c += 2;  /* c=18 after pre-call + c=19 at .skip label (two inc c in original ASM) */

    while (c > 0 && n < ISP_MAX) {
        /* RIGHT c */
        for (int i = 0; i < c && n < ISP_MAX; i++) {
            g_isp_x[n] = (int8_t)tx; g_isp_y[n++] = (int8_t)ty; tx++;
        }
        c -= 2;
        if (c <= 0) break;

        /* UP c */
        for (int i = 0; i < c && n < ISP_MAX; i++) {
            g_isp_x[n] = (int8_t)tx; g_isp_y[n++] = (int8_t)ty; ty--;
        }
        c++;  /* inc c */

        /* LEFT c */
        for (int i = 0; i < c && n < ISP_MAX; i++) {
            g_isp_x[n] = (int8_t)tx; g_isp_y[n++] = (int8_t)ty; tx--;
        }
        c -= 2;
        if (c <= 0) break;

        /* DOWN c */
        for (int i = 0; i < c && n < ISP_MAX; i++) {
            g_isp_x[n] = (int8_t)tx; g_isp_y[n++] = (int8_t)ty; ty++;
        }
        c++;  /* inc c, back to .skip */
    }

    g_isp_len = n;
    g_isp_idx = 0;
}

/* ------------------------------------------------------------------ */
/*  Per-frame animation tickers (return 1 when animation is done)     */
/* ------------------------------------------------------------------ */

/* --- Flash (types 0 and 2 only) --- */
static int tick_flash(void) {
    /* Each palette entry lasts 2 frames; 12 entries per cycle; 3 cycles. */
    Display_SetBGP(kFlashPals[g_flash_idx]);
    if (++g_frame >= 2) {
        g_frame = 0;
        if (++g_flash_idx >= 12) {
            g_flash_idx = 0;
            if (++g_flash_cycle >= 3) {
                /* Flash done → enter animation phase. */
                g_phase = BPHASE_ANIM;
                g_step  = 0;
                g_frame = 0;
                /* Restore normal palette before animation begins. */
                Display_SetPalette(0xE4, 0xD0, 0xE0);
            }
        }
    }
    return 0;
}

/* --- DoubleCircle (type 0) --- */
static int tick_double_circle(void) {
    if (g_frame == 0) {
        apply_hc(&kHC1[g_step], 0);  /* quad_y=0 (down) */
        apply_hc(&kHC2[g_step], 1);  /* quad_y=1 (up)   */
    }
    if (++g_frame >= 3) { g_frame = 0; g_step++; }
    return (g_step >= 10);
}

/* --- Circle (type 2) --- */
static int tick_circle(void) {
    if (g_frame == 0) {
        if (g_step < 10)
            apply_hc(&kHC1[g_step],     0);
        else
            apply_hc(&kHC2[g_step - 10], 1);
    }
    if (++g_frame >= 3) { g_frame = 0; g_step++; }
    return (g_step >= 20);
}

/* --- HorizontalStripes (type 4) --- */
static int tick_hstripes(void) {
    if (g_frame == 0) {
        int s = g_step;
        for (int row = 0; row < SCREEN_HEIGHT; row += 2)
            set_stile(s, row, 0xFF);                         /* left half: even rows */
        for (int row = 1; row < SCREEN_HEIGHT; row += 2)
            set_stile(SCREEN_WIDTH - 1 - s, row, 0xFF);     /* right half: odd rows */
    }
    if (++g_frame >= 3) { g_frame = 0; g_step++; }
    return (g_step >= SCREEN_WIDTH);
}

/* --- VerticalStripes (type 6) --- */
static int tick_vstripes(void) {
    if (g_frame == 0) {
        int s = g_step;
        for (int col = 0; col < SCREEN_WIDTH; col += 2)
            set_stile(col, s, 0xFF);                          /* top half: even cols */
        for (int col = 1; col < SCREEN_WIDTH; col += 2)
            set_stile(col, SCREEN_HEIGHT - 1 - s, 0xFF);     /* bot half: odd cols  */
    }
    if (++g_frame >= 3) { g_frame = 0; g_step++; }
    return (g_step >= SCREEN_HEIGHT);
}

/* --- Spiral outward (type 3) --- */
static int tick_spiral_out(void) {
    /* 3 substeps per frame, 120 frames total = 360 substeps. */
    for (int i = 0; i < 3; i++) {
        outward_substep();
        if (++g_sp_total >= 360) return 1;
    }
    return 0;
}

/* --- Spiral inward (type 1) --- */
static int tick_spiral_in(void) {
    /* 7 tiles per frame (TransferDelay3 every 7 tiles in original). */
    for (int i = 0; i < 7; i++) {
        if (g_isp_idx >= g_isp_len) return 1;
        set_stile(g_isp_x[g_isp_idx], g_isp_y[g_isp_idx], 0xFF);
        g_isp_idx++;
    }
    return (g_isp_idx >= g_isp_len);
}

/* --- Shrink (type 5): fill rings inward from edges, 9 steps × 6 frames. */
static int tick_shrink(void) {
    if (g_frame == 0) {
        int s   = g_step;
        int top = s,                  bot = SCREEN_HEIGHT - 1 - s;
        int lft = s,                  rgt = SCREEN_WIDTH  - 1 - s;
        for (int x = lft; x <= rgt; x++) { set_stile(x, top, 0xFF); set_stile(x, bot, 0xFF); }
        for (int y = top + 1; y < bot; y++) { set_stile(lft, y, 0xFF); set_stile(rgt, y, 0xFF); }
    }
    if (++g_frame >= 6) { g_frame = 0; g_step++; }
    return (g_step >= SCREEN_HEIGHT / 2);
}

/* --- Split (type 7): fill rings outward from center, 9 steps × 6 frames. */
static int tick_split(void) {
    if (g_frame == 0) {
        int s    = g_step;
        int tmid = SCREEN_HEIGHT / 2;
        int lmid = SCREEN_WIDTH  / 2;
        int top  = tmid - 1 - s,  bot = tmid + s;
        int lft  = lmid - 1 - s,  rgt = lmid + s;
        for (int x = 0; x < SCREEN_WIDTH;  x++) { set_stile(x, top, 0xFF); set_stile(x, bot, 0xFF); }
        for (int y = 0; y < SCREEN_HEIGHT; y++) { set_stile(lft, y, 0xFF); set_stile(rgt, y, 0xFF); }
    }
    if (++g_frame >= 6) { g_frame = 0; g_step++; }
    return (g_step >= SCREEN_HEIGHT / 2);
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

void BattleTransition_Start(int is_trainer, int enemy_level, int player_level) {
    int stronger = (enemy_level >= player_level + 3);
    int dungeon  = is_dungeon_map();
    int type     = (is_trainer ? 1 : 0)
                 | (stronger   ? 2 : 0)
                 | (dungeon    ? 4 : 0);

    g_type        = type;
    g_step        = 0;
    g_frame       = 0;
    g_flash_idx   = 0;
    g_flash_cycle = 0;

    /* Load the all-black tile into BG slot 0xFF (vChars1 tile $7F). */
    Display_LoadTile(0xFF, kBlackTile);

    /* Reset scroll offset so OAM / tile grid align correctly. */
    gScrollPxX = 0;
    gScrollPxY = 0;

    /* Type-specific init */
    switch (type) {
        case 1:  /* spiral inward */
            isp_precompute();
            g_phase = BPHASE_ANIM;
            break;
        case 3:  /* spiral outward */
            g_sp_tx    = 10;
            g_sp_ty    = 10;  /* hlcoord 10, 10 */
            g_sp_dir   = 3;   /* initial direction = right */
            g_sp_total = 0;
            g_phase = BPHASE_ANIM;
            break;
        case 0:
        case 2:
            /* DoubleCircle and Circle both start with the flash phase. */
            g_phase = BPHASE_FLASH;
            break;
        default:
            /* Types 4,5,6,7: no flash, go straight to animation. */
            g_phase = BPHASE_ANIM;
            break;
    }
}

int BattleTransition_Tick(void) {
    if (g_phase == BPHASE_IDLE || g_phase == BPHASE_DONE) return 1;

    if (g_phase == BPHASE_FLASH) {
        tick_flash();
        return 0;
    }

    /* BPHASE_ANIM */
    int done = 0;
    switch (g_type) {
        case 0: done = tick_double_circle(); break;
        case 1: done = tick_spiral_in();     break;
        case 2: done = tick_circle();        break;
        case 3: done = tick_spiral_out();    break;
        case 4: done = tick_hstripes();      break;
        case 5: done = tick_shrink();        break;
        case 6: done = tick_vstripes();      break;
        case 7: done = tick_split();         break;
        default: done = 1; break;
    }

    if (done) {
        /* BattleTransition_BlackScreen: set all palettes to $FF (all black). */
        Display_SetPalette(0xFF, 0xFF, 0xFF);
        g_phase = BPHASE_DONE;
        return 1;
    }
    return 0;
}

int BattleTransition_IsActive(void) {
    return (g_phase != BPHASE_IDLE && g_phase != BPHASE_DONE);
}
