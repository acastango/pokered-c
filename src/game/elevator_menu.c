#include "elevator_menu.h"
#include "overworld.h"
#include "npc.h"
#include "player.h"
#include "text.h"
#include "warp.h"
#include "../data/font_data.h"
#include "../platform/hardware.h"
#include "../platform/audio.h"

/* ASM list menu box (LIST_MENU_BOX): (4,2) to (19,12) */
#define BOX_COL_L  4
#define BOX_COL_R 19
#define BOX_ROW_T  2
#define BOX_ROW_B 12
/* ASM list cursor/name start */
#define ARROW_COL  5
#define ITEM_COL   6
#define ITEM_ROW0  4
#define ITEM_STEP  2

#define CHAR_TERM  0x50
#define CHAR_SPACE 0x7F
#define CHAR_ARROW 0xEC

#define RH_B1F_MAP 0xC7
#define RH_B2F_MAP 0xC8
#define RH_B4F_MAP 0xCA

typedef struct {
    uint8_t map_id;
    int x;
    int y;
    const uint8_t *label;
} floor_choice_t;

static const uint8_t kLblB1F[] = { 0x81, 0xF7, 0x85, CHAR_TERM }; /* B1F */
static const uint8_t kLblB2F[] = { 0x81, 0xF8, 0x85, CHAR_TERM }; /* B2F */
static const uint8_t kLblB4F[] = { 0x81, 0xFA, 0x85, CHAR_TERM }; /* B4F */
static const uint8_t kLblCancel[] = { 0x82, 0x80, 0x8D, 0x82, 0x84, 0x8B, CHAR_TERM }; /* CANCEL */

static const floor_choice_t kRocketChoices[3] = {
    { RH_B1F_MAP, 24, 19, kLblB1F },
    { RH_B2F_MAP, 24, 19, kLblB2F },
    { RH_B4F_MAP, 24, 15, kLblB4F },
};

static int g_open = 0;
static int g_cursor = 0;
static int g_wait_a_release = 0;
static int g_wait_neutral = 0;
static uint8_t g_prev_held = 0;
static int g_open_queued = 0;
static int g_pending_tp = 0;
static uint8_t g_tp_map = 0;
static int g_tp_x = 0;
static int g_tp_y = 0;
static int g_shake_phase = 0;
static int g_shake_delay = 0;
static int g_shake_timer = 0;
static int g_shake_loops = 0;
static int g_shake_off = 1;
static uint8_t g_shake_base_scy = 0;
static int g_shake_base_scroll_y = 0;

enum {
    ELEV_SHAKE_IDLE = 0,
    ELEV_SHAKE_PRE_DELAY,
    ELEV_SHAKE_RUN,
    ELEV_SHAKE_DING_WAIT,
};

static void smset(int col, int row, uint8_t tile) {
    gScrollTileMap[(row + 2) * SCROLL_MAP_W + (col + 2)] = tile;
}

static void draw_box(void) {
    smset(BOX_COL_L, BOX_ROW_T, (uint8_t)Font_CharToTile(0x79));
    for (int c = BOX_COL_L + 1; c < BOX_COL_R; c++)
        smset(c, BOX_ROW_T, (uint8_t)Font_CharToTile(0x7A));
    smset(BOX_COL_R, BOX_ROW_T, (uint8_t)Font_CharToTile(0x7B));

    for (int r = BOX_ROW_T + 1; r < BOX_ROW_B; r++) {
        smset(BOX_COL_L, r, (uint8_t)Font_CharToTile(0x7C));
        for (int c = BOX_COL_L + 1; c < BOX_COL_R; c++)
            smset(c, r, (uint8_t)Font_CharToTile(CHAR_SPACE));
        smset(BOX_COL_R, r, (uint8_t)Font_CharToTile(0x7C));
    }

    smset(BOX_COL_L, BOX_ROW_B, (uint8_t)Font_CharToTile(0x7D));
    for (int c = BOX_COL_L + 1; c < BOX_COL_R; c++)
        smset(c, BOX_ROW_B, (uint8_t)Font_CharToTile(0x7A));
    smset(BOX_COL_R, BOX_ROW_B, (uint8_t)Font_CharToTile(0x7E));
}

static void print_str(int col, int row, const uint8_t *s) {
    for (; *s != CHAR_TERM; s++, col++)
        smset(col, row, (uint8_t)Font_CharToTile(*s));
}

static void draw_items(void) {
    for (int i = 0; i < 3; i++)
        print_str(ITEM_COL, ITEM_ROW0 + i * ITEM_STEP, kRocketChoices[i].label);
    print_str(ITEM_COL, ITEM_ROW0 + 3 * ITEM_STEP, kLblCancel);
}

static void set_cursor(int idx, uint8_t tile) {
    smset(ARROW_COL, ITEM_ROW0 + idx * ITEM_STEP, tile);
}

void ElevatorMenu_OpenRocketHideout(void) {
    g_shake_phase = ELEV_SHAKE_IDLE;
    g_open = 1;
    g_cursor = 0;
    g_wait_a_release = 1;
    g_wait_neutral = 1;
    g_prev_held = 0xFF; /* force neutral pass before any edge can be seen */
    g_open_queued = 0;
    g_pending_tp = 0;
    /* Mirror ASM behavior: clear NPC sprites behind menu, keep player OAM. */
    for (int i = 4; i < MAX_SPRITES; i++) wShadowOAM[i].y = 0;
    /* Keep player behind menu text flow: hide player while floor menu is open. */
    for (int i = 0; i < 4; i++) wShadowOAM[i].y = 0;
    draw_box();
    draw_items();
    set_cursor(0, (uint8_t)Font_CharToTile(CHAR_ARROW));
    /* Consume the panel interaction press so floor A-select requires
     * a fresh button press after the menu is visible. */
    hJoyPressed = 0;
}

void ElevatorMenu_QueueOpenRocketHideout(void) {
    g_shake_phase = ELEV_SHAKE_IDLE;
    g_open_queued = 1;
}

void ElevatorMenu_TryOpenQueued(void) {
    if (!g_open_queued) return;
    if (g_open || Text_IsOpen()) return;
    Text_BlitBoxToBGAndHideWindow();
    ElevatorMenu_OpenRocketHideout();
}

void ElevatorMenu_Tick(void) {
    uint8_t held = (uint8_t)(hJoyHeld & (PAD_A | PAD_B | PAD_UP | PAD_DOWN | PAD_LEFT | PAD_RIGHT));
    uint8_t pressed_local = (uint8_t)(held & (uint8_t)~g_prev_held);
    g_prev_held = held;

    if (g_shake_phase != ELEV_SHAKE_IDLE) {
        if (g_shake_phase == ELEV_SHAKE_PRE_DELAY) {
            if (--g_shake_delay <= 0) {
                g_shake_phase = ELEV_SHAKE_RUN;
                g_shake_loops = 100;
                g_shake_timer = 2;
            }
            return;
        }
        if (g_shake_phase == ELEV_SHAKE_RUN) {
            if (--g_shake_timer <= 0) {
                g_shake_timer = 2;
                g_shake_off = -g_shake_off;
                hSCY = (uint8_t)(g_shake_base_scy + g_shake_off);
                gScrollPxY = g_shake_base_scroll_y + g_shake_off;
                NPC_BuildView(gScrollPxX, gScrollPxY);
                Player_SyncOAM();
                Audio_PlaySFX_CollisionRetrigger();
                if (--g_shake_loops <= 0) {
                    hSCY = g_shake_base_scy;
                    gScrollPxY = g_shake_base_scroll_y;
                    Player_SyncOAM();
                    Audio_PlaySFX_SafariZonePA();
                    g_shake_phase = ELEV_SHAKE_DING_WAIT;
                }
            }
            return;
        }
        if (g_shake_phase == ELEV_SHAKE_DING_WAIT) {
            if (!Audio_IsSFXPlaying()) {
                g_shake_phase = ELEV_SHAKE_IDLE;
                Map_BuildScrollView();
                NPC_BuildView(0, 0);
            }
            return;
        }
    }

    if (!g_open) return;
    if (g_wait_a_release) {
        if (held & PAD_A)
            return;
        g_wait_a_release = 0;
        return;
    }
    /* ASM-like low-sensitivity neutral gate:
     * after opening, require a full neutral frame before accepting any input,
     * so the interaction press cannot auto-confirm a floor choice. */
    if (g_wait_neutral) {
        if (held != 0)
            return;
        g_wait_neutral = 0;
        g_prev_held = 0;
        return;
    }

    if (pressed_local & PAD_UP) {
        set_cursor(g_cursor, (uint8_t)Font_CharToTile(CHAR_SPACE));
        g_cursor = (g_cursor == 0) ? 3 : g_cursor - 1;
        set_cursor(g_cursor, (uint8_t)Font_CharToTile(CHAR_ARROW));
        return;
    }
    if (pressed_local & PAD_DOWN) {
        set_cursor(g_cursor, (uint8_t)Font_CharToTile(CHAR_SPACE));
        g_cursor = (g_cursor == 3) ? 0 : g_cursor + 1;
        set_cursor(g_cursor, (uint8_t)Font_CharToTile(CHAR_ARROW));
        return;
    }
    if (pressed_local & PAD_B) {
        Audio_PlaySFX_PressAB();
        Text_Close();
        g_open = 0;
        Player_SyncOAM();
        g_pending_tp = 0;
        Map_BuildScrollView();
        NPC_BuildView(0, 0);
        return;
    }
    if (pressed_local & PAD_A) {
        if (g_cursor == 3) {
            Audio_PlaySFX_PressAB();
            Text_Close();
            g_open = 0;
            Player_SyncOAM();
            g_pending_tp = 0;
            Map_BuildScrollView();
            NPC_BuildView(0, 0);
            return;
        }
        const floor_choice_t *f = &kRocketChoices[g_cursor];
        Audio_PlaySFX_PressAB();
        Text_Close();
        Warp_SetRocketElevatorDestination(f->map_id, f->x, f->y);
        g_shake_base_scy = hSCY;
        g_shake_base_scroll_y = gScrollPxY;
        g_shake_off = 1;
        g_shake_delay = 3;
        g_shake_phase = ELEV_SHAKE_PRE_DELAY;
        g_open = 0;
        Player_SyncOAM();
        /* Clear both prompt/menu boxes before shake animation starts. */
        Map_BuildScrollView();
        NPC_BuildView(gScrollPxX, gScrollPxY);
    }
}

int ElevatorMenu_IsOpen(void) {
    return g_open;
}

int ElevatorMenu_IsBusy(void) {
    return g_shake_phase != ELEV_SHAKE_IDLE;
}

int ElevatorMenu_ConsumeTeleport(uint8_t *map_out, int *x_out, int *y_out) {
    if (!g_pending_tp) return 0;
    g_pending_tp = 0;
    *map_out = g_tp_map;
    *x_out = g_tp_x;
    *y_out = g_tp_y;
    return 1;
}
