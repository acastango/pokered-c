/* bag_menu.c -- Overworld ITEM bag menu.
 *
 * Mirrors engine/menus/start_sub_menus.asm (ItemMenuLoop) +
 *         home/list_menu.asm (PrintListMenuEntries).
 *
 * Box geometry (full-width, 14 rows):
 *   cols 0-19, rows 0-13.
 *   Items at col 2, rows 2,4,6,8 (up to 4 visible).
 *   Cursor ▶ at col 1.  Quantity ×NN at col 14-16.
 *   CANCEL at row 10 (always visible).
 *
 * Action submenu (USE/TOSS/CANCEL) overlaid at cols 11-18, rows 7-13.
 */
#include "bag_menu.h"
#include "inventory.h"
#include "overworld.h"
#include "npc.h"
#include "tmhm.h"
#include "pokeflute.h"
#include "../platform/hardware.h"
#include "../data/font_data.h"
#include <string.h>

/* ---- Char codes ------------------------------------------------------ */
#define CHAR_TERM   0x50
#define CHAR_SPACE  0x7F
#define CHAR_ARROW  0xEC   /* ▶ cursor (matches menu.c) */
#define CHAR_TIMES  0xF1   /* × */

/* ---- Box geometry ---------------------------------------------------- */
#define BOX_L           0
#define BOX_R          19
#define BOX_T           0
#define BOX_B          13
#define ITEM_COL        2   /* item name start column */
#define CURSOR_COL      1
#define QTY_COL        14   /* × at 14, tens at 15, ones at 16 */
#define ROW_FIRST       2   /* first item row */
#define ROW_STEP        2   /* rows between items */
#define VISIBLE         4   /* max visible items at once */
#define CANCEL_ROW     10   /* ROW_FIRST + VISIBLE*ROW_STEP */

/* ---- Action submenu geometry ----------------------------------------- */
#define ASUB_L         11
#define ASUB_R         18
#define ASUB_T          7
#define ASUB_B         13
#define ASUB_CURSOR    12
#define ASUB_ITEM      13
#define ASUB_ROW_FIRST  8
#define ASUB_ROW_STEP   2
#define NUM_ACTIONS     3   /* USE, TOSS, CANCEL */

/* ---- State ----------------------------------------------------------- */
typedef enum { BAG_LIST = 0, BAG_ACTION } BagState;

static int      gBagOpen         = 0;
static int      gBagCursor       = 0;   /* 0..wNumBagItems; wNumBagItems = CANCEL */
static int      gBagScrollTop    = 0;
static BagState gBagState        = BAG_LIST;
static int      gActionCursor    = 0;   /* 0=USE 1=TOSS 2=CANCEL */
static int      gBagBattleMode   = 0;   /* 1 = opened from battle (skip map rebuild on close) */
static uint8_t  gBagSelectedItem = 0;   /* item ID of last battle-mode USE; 0 = none/cancel */

/* ---- Tile writer ----------------------------------------------------- */
static void smset(int col, int row, uint8_t tile) {
    gScrollTileMap[(row + 2) * SCROLL_MAP_W + (col + 2)] = tile;
}

/* ---- String printers ------------------------------------------------- */
static void pstr(int col, int row, const uint8_t *s) {
    for (; *s != CHAR_TERM; s++, col++)
        smset(col, row, (uint8_t)Font_CharToTile(*s));
}

static void pqty(int col, int row, uint8_t qty) {
    smset(col,   row, (uint8_t)Font_CharToTile(CHAR_TIMES));
    smset(col+1, row, (uint8_t)Font_CharToTile(qty >= 10 ? 0xF6 + qty/10 : CHAR_SPACE));
    smset(col+2, row, (uint8_t)Font_CharToTile(0xF6 + qty % 10));
}

/* ---- Pokéred-encoded static strings ---------------------------------- */
/* ITEM */
static const uint8_t kStrItem[]   = {0x88,0x93,0x84,0x8C,CHAR_TERM};
/* CANCEL */
static const uint8_t kStrCancel[] = {0x82,0x80,0x8D,0x82,0x84,0x8B,CHAR_TERM};
/* NOTHING */
static const uint8_t kStrNothing[]= {0x8D,0x8E,0x93,0x87,0x88,0x8D,0x86,CHAR_TERM};
/* USE */
static const uint8_t kStrUse[]    = {0x94,0x92,0x84,CHAR_TERM};
/* TOSS */
static const uint8_t kStrToss[]   = {0x93,0x8E,0x92,0x92,CHAR_TERM};

/* ---- Draw main box --------------------------------------------------- */
static void draw_box(void) {
    smset(BOX_L, BOX_T, (uint8_t)Font_CharToTile(0x79));
    for (int c = BOX_L+1; c < BOX_R; c++) smset(c, BOX_T, (uint8_t)Font_CharToTile(0x7A));
    smset(BOX_R, BOX_T, (uint8_t)Font_CharToTile(0x7B));
    for (int r = BOX_T+1; r < BOX_B; r++) {
        smset(BOX_L, r, (uint8_t)Font_CharToTile(0x7C));
        for (int c = BOX_L+1; c < BOX_R; c++) smset(c, r, (uint8_t)Font_CharToTile(CHAR_SPACE));
        smset(BOX_R, r, (uint8_t)Font_CharToTile(0x7C));
    }
    smset(BOX_L, BOX_B, (uint8_t)Font_CharToTile(0x7D));
    for (int c = BOX_L+1; c < BOX_R; c++) smset(c, BOX_B, (uint8_t)Font_CharToTile(0x7A));
    smset(BOX_R, BOX_B, (uint8_t)Font_CharToTile(0x7E));
}

/* ---- Draw item list (4 visible rows + CANCEL) ------------------------ */
static void draw_items(void) {
    /* Header */
    pstr(ITEM_COL, 1, kStrItem);

    if (wNumBagItems == 0) {
        for (int s = 0; s < VISIBLE; s++) {
            int row = ROW_FIRST + s * ROW_STEP;
            for (int c = BOX_L+1; c < BOX_R; c++) smset(c, row, (uint8_t)Font_CharToTile(CHAR_SPACE));
        }
        pstr(ITEM_COL, ROW_FIRST, kStrNothing);
    } else {
        for (int s = 0; s < VISIBLE; s++) {
            int idx = gBagScrollTop + s;
            int row = ROW_FIRST + s * ROW_STEP;
            /* Clear row */
            for (int c = BOX_L+1; c < BOX_R; c++) smset(c, row, (uint8_t)Font_CharToTile(CHAR_SPACE));
            if (idx < (int)wNumBagItems) {
                uint8_t id  = wBagItems[idx * 2];
                uint8_t qty = wBagItems[idx * 2 + 1];
                pstr(ITEM_COL, row, Inventory_GetName(id));
                if (!Inventory_IsKeyItem(id))
                    pqty(QTY_COL, row, qty);
            }
        }
    }

    /* CANCEL row */
    for (int c = BOX_L+1; c < BOX_R; c++) smset(c, CANCEL_ROW, (uint8_t)Font_CharToTile(CHAR_SPACE));
    pstr(ITEM_COL, CANCEL_ROW, kStrCancel);
}

/* ---- Cursor row for current gBagCursor ------------------------------- */
static int cursor_screen_row(void) {
    if (gBagCursor == (int)wNumBagItems) return CANCEL_ROW;
    return ROW_FIRST + (gBagCursor - gBagScrollTop) * ROW_STEP;
}

static void draw_cursor(void) {
    /* Erase all candidate rows */
    for (int s = 0; s < VISIBLE; s++)
        smset(CURSOR_COL, ROW_FIRST + s * ROW_STEP, (uint8_t)Font_CharToTile(CHAR_SPACE));
    smset(CURSOR_COL, CANCEL_ROW, (uint8_t)Font_CharToTile(CHAR_SPACE));
    /* Place at current position */
    smset(CURSOR_COL, cursor_screen_row(), (uint8_t)Font_CharToTile(CHAR_ARROW));
}

/* ---- Draw action submenu --------------------------------------------- */
static void draw_action_submenu(void) {
    smset(ASUB_L, ASUB_T, (uint8_t)Font_CharToTile(0x79));
    for (int c = ASUB_L+1; c < ASUB_R; c++) smset(c, ASUB_T, (uint8_t)Font_CharToTile(0x7A));
    smset(ASUB_R, ASUB_T, (uint8_t)Font_CharToTile(0x7B));
    for (int r = ASUB_T+1; r < ASUB_B; r++) {
        smset(ASUB_L, r, (uint8_t)Font_CharToTile(0x7C));
        for (int c = ASUB_L+1; c < ASUB_R; c++) smset(c, r, (uint8_t)Font_CharToTile(CHAR_SPACE));
        smset(ASUB_R, r, (uint8_t)Font_CharToTile(0x7C));
    }
    smset(ASUB_L, ASUB_B, (uint8_t)Font_CharToTile(0x7D));
    for (int c = ASUB_L+1; c < ASUB_R; c++) smset(c, ASUB_B, (uint8_t)Font_CharToTile(0x7A));
    smset(ASUB_R, ASUB_B, (uint8_t)Font_CharToTile(0x7E));

    pstr(ASUB_ITEM, ASUB_ROW_FIRST + 0 * ASUB_ROW_STEP, kStrUse);
    pstr(ASUB_ITEM, ASUB_ROW_FIRST + 1 * ASUB_ROW_STEP, kStrToss);
    pstr(ASUB_ITEM, ASUB_ROW_FIRST + 2 * ASUB_ROW_STEP, kStrCancel);
    smset(ASUB_CURSOR, ASUB_ROW_FIRST + gActionCursor * ASUB_ROW_STEP,
          (uint8_t)Font_CharToTile(CHAR_ARROW));
}

static void erase_action_submenu(void) {
    /* Restore the section of the bag list that the submenu overlaid */
    for (int r = ASUB_T; r <= ASUB_B; r++) {
        for (int c = ASUB_L; c <= ASUB_R; c++) {
            int row = r;
            /* Check if this row is a list row */
            int is_item_row = 0;
            for (int s = 0; s < VISIBLE; s++)
                if (row == ROW_FIRST + s * ROW_STEP) { is_item_row = 1; break; }
            if (row == CANCEL_ROW) is_item_row = 1;
            if (is_item_row) {
                /* Redraw from list data */
                smset(c, row, (uint8_t)Font_CharToTile(CHAR_SPACE));
            } else {
                /* Interior of box */
                if (c == BOX_L || c == BOX_R)
                    smset(c, row, (uint8_t)Font_CharToTile(0x7C));
                else
                    smset(c, row, (uint8_t)Font_CharToTile(CHAR_SPACE));
            }
        }
    }
    draw_items();
    draw_cursor();
}

/* ---- Scroll helpers -------------------------------------------------- */
static void scroll_clamp(void) {
    int max_top = (int)wNumBagItems - VISIBLE;
    if (max_top < 0) max_top = 0;
    if (gBagScrollTop > max_top) gBagScrollTop = max_top;
    if (gBagScrollTop < 0)       gBagScrollTop = 0;
}

/* ---- Close ----------------------------------------------------------- */
static void bag_close(void) {
    gBagOpen = 0;
    if (!gBagBattleMode) {
        Map_BuildScrollView();
        NPC_BuildView(0, 0);
    }
    gBagBattleMode = 0;
}

/* ---- Public API ------------------------------------------------------ */
void BagMenu_Open(void) {
    gBagOpen      = 1;
    gBagCursor    = 0;
    gBagScrollTop = 0;
    gBagState     = BAG_LIST;
    gActionCursor = 0;
    /* Hide sprites (same pattern as menu.c) */
    for (int i = 0; i < MAX_SPRITES; i++) wShadowOAM[i].y = 0;
    draw_box();
    draw_items();
    draw_cursor();
}

int BagMenu_IsOpen(void) { return gBagOpen; }

void BagMenu_OpenBattle(void) {
    gBagBattleMode   = 1;
    gBagSelectedItem = 0;
    gBagOpen         = 1;
    gBagCursor       = 0;
    gBagScrollTop    = 0;
    gBagState        = BAG_LIST;
    gActionCursor    = 0;
    /* In battle the screen is managed by battle_ui — just draw the bag overlay */
    draw_box();
    draw_items();
    draw_cursor();
}

uint8_t BagMenu_GetSelected(void) { return gBagSelectedItem; }

void BagMenu_Tick(void) {
    if (gBagState == BAG_ACTION) {
        /* ---- Action submenu input ------------------------------------ */
        if (hJoyPressed & PAD_UP) {
            smset(ASUB_CURSOR, ASUB_ROW_FIRST + gActionCursor * ASUB_ROW_STEP,
                  (uint8_t)Font_CharToTile(CHAR_SPACE));
            gActionCursor = (gActionCursor == 0) ? NUM_ACTIONS - 1 : gActionCursor - 1;
            smset(ASUB_CURSOR, ASUB_ROW_FIRST + gActionCursor * ASUB_ROW_STEP,
                  (uint8_t)Font_CharToTile(CHAR_ARROW));
            return;
        }
        if (hJoyPressed & PAD_DOWN) {
            smset(ASUB_CURSOR, ASUB_ROW_FIRST + gActionCursor * ASUB_ROW_STEP,
                  (uint8_t)Font_CharToTile(CHAR_SPACE));
            gActionCursor = (gActionCursor == NUM_ACTIONS - 1) ? 0 : gActionCursor + 1;
            smset(ASUB_CURSOR, ASUB_ROW_FIRST + gActionCursor * ASUB_ROW_STEP,
                  (uint8_t)Font_CharToTile(CHAR_ARROW));
            return;
        }
        if (hJoyPressed & (PAD_B | PAD_START)) {
            gBagState = BAG_LIST;
            erase_action_submenu();
            return;
        }
        if (hJoyPressed & PAD_A) {
            uint8_t id = wBagItems[gBagCursor * 2];
            switch (gActionCursor) {
                case 0: /* USE */
                    if (gBagBattleMode) {
                        /* In battle: record selection and close immediately */
                        gBagSelectedItem = id;
                        bag_close();
                    } else if (id >= HM01) {
                        /* Overworld TM/HM: start teaching flow */
                        TMHM_Use(id);
                        bag_close();
                    } else if (id == ITEM_POKE_FLUTE) {
                        PokeFlute_Use();
                        bag_close();
                    } else {
                        /* Overworld: no effect for other items yet */
                        gBagState = BAG_LIST;
                        erase_action_submenu();
                    }
                    break;
                case 1: /* TOSS */
                    if (Inventory_IsKeyItem(id)) {
                        /* Can't toss key items — just dismiss submenu */
                        gBagState = BAG_LIST;
                        erase_action_submenu();
                    } else {
                        Inventory_Remove(id, 1);
                        /* Clamp cursor in case item was removed */
                        if (gBagCursor > (int)wNumBagItems)
                            gBagCursor = (int)wNumBagItems;
                        scroll_clamp();
                        gBagState = BAG_LIST;
                        erase_action_submenu();
                    }
                    break;
                case 2: /* CANCEL */
                    gBagState = BAG_LIST;
                    erase_action_submenu();
                    break;
            }
            return;
        }
        return;
    }

    /* ---- List input -------------------------------------------------- */
    if (hJoyPressed & PAD_UP) {
        if (gBagCursor == (int)wNumBagItems) {
            /* At CANCEL → jump to last item */
            if (wNumBagItems > 0) {
                gBagCursor = (int)wNumBagItems - 1;
                gBagScrollTop = (int)wNumBagItems - VISIBLE;
                scroll_clamp();
            }
        } else if (gBagCursor == 0) {
            /* At top item → wrap to CANCEL */
            gBagCursor = (int)wNumBagItems;
        } else {
            if (gBagCursor == gBagScrollTop) gBagScrollTop--;
            gBagCursor--;
        }
        draw_items();
        draw_cursor();
        return;
    }
    if (hJoyPressed & PAD_DOWN) {
        if (gBagCursor == (int)wNumBagItems) {
            /* At CANCEL → wrap to first item */
            gBagCursor    = 0;
            gBagScrollTop = 0;
        } else if (gBagCursor == (int)wNumBagItems - 1) {
            /* At last item → move to CANCEL */
            gBagCursor = (int)wNumBagItems;
        } else {
            gBagCursor++;
            if (gBagCursor >= gBagScrollTop + VISIBLE) gBagScrollTop++;
        }
        draw_items();
        draw_cursor();
        return;
    }

    if (hJoyPressed & (PAD_B | PAD_START)) {
        bag_close();
        return;
    }

    if (hJoyPressed & PAD_A) {
        if (gBagCursor == (int)wNumBagItems) {
            /* CANCEL selected */
            bag_close();
        } else {
            /* Item selected — open action submenu */
            gBagState     = BAG_ACTION;
            gActionCursor = 0;
            draw_action_submenu();
        }
        return;
    }
}
