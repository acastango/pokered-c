/* pokemart.c — Pokémart buy/sell sequence.
 *
 * Source: pokered-master/engine/events/pokemart.asm
 *         pokered-master/data/items/marts.asm
 *         pokered-master/data/items/prices.asm
 *         pokered-master/data/text_boxes.asm  (box coordinates)
 *
 * Panel layout (Gen 1 faithful):
 *   BUY/SELL/QUIT box : cols  0-10, rows 0-6  (BUY_SELL_QUIT_MENU_TEMPLATE)
 *   Money box         : cols 11-19, rows 0-2  (MONEY_BOX_TEMPLATE)
 *   Item list box     : cols  4-19, rows 2-12 (LIST_MENU_BOX)
 *   Quantity box      : cols  7-19, rows 9-11 (PRICEDITEMLISTMENU picker)
 *   YES/NO box        : cols 14-19, rows 7-11 (TWO_OPTION_MENU at hlcoord 14,7)
 */

#include "pokemart.h"
#include "text.h"
#include "inventory.h"
#include "overworld.h"
#include "npc.h"
#include "player.h"
#include "../platform/hardware.h"
#include "../platform/audio.h"
#include "../data/font_data.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* ---- Pokered char codes ------------------------------------------- */
#define CHAR_TERM   0x50u
#define CHAR_SPACE  0x7Fu
#define CHAR_ARROW  0xECu   /* ▶ cursor */
#define CHAR_YEN    0xF0u   /* ¥ */
#define CHAR_TIMES  0xF1u   /* × */

/* Box-drawing chars */
#define BC_TL   0x79u   /* ┌ */
#define BC_H    0x7Au   /* ─ */
#define BC_TR   0x7Bu   /* ┐ */
#define BC_V    0x7Cu   /* │ */
#define BC_BL   0x7Du   /* └ */
#define BC_BR   0x7Eu   /* ┘ */

/* ---- Panel geometry (Gen 1 source coordinates) -------------------- */

/* BUY/SELL/QUIT menu box: cols 0-10, rows 0-6
 * Source: BUY_SELL_QUIT_MENU_TEMPLATE = 0,0,10,6 text at col 2 row 1 */
#define MENU_L          0
#define MENU_R         10
#define MENU_T          0
#define MENU_B          6
#define MENU_CURSOR_COL 1
#define MENU_TEXT_COL   2
#define MENU_ENTRY_ROW  1   /* first entry row (BUY at 1, SELL at 2, QUIT at 3) */

/* Money box: cols 11-19, rows 0-2
 * Source: MONEY_BOX_TEMPLATE = 11,0,19,2 text at col 13 row 0
 * "MONEY" sits on the top border; ¥+digits in content row 1. */
#define MONEY_L         11
#define MONEY_R         19
#define MONEY_T          0
#define MONEY_B          2
#define MONEY_LABEL_COL 13   /* "MONEY" written on top-border row */
#define MONEY_YEN_COL   12   /* ¥ in content row */
#define MONEY_DIGIT_COL 13   /* 6-digit amount starts here */
#define MONEY_ROW        1   /* content row */

/* Item list box: cols 4-19, rows 2-12
 * Source: LIST_MENU_BOX = 4,2,19,12  (inner 14 wide, 9 tall)
 * Gen 1 PrintListMenuEntries: hlcoord 6,4 for first entry, 2 rows per entry,
 * 4 entries visible.  Price offset = SCREEN_WIDTH+5 = 1 row down + 5 cols right. */
#define LIST_L              4
#define LIST_R             19
#define LIST_T              2
#define LIST_B             12
#define LIST_ITEM_TOP       4   /* first item name row (Gen 1: wTopMenuItemY=4) */
#define LIST_CURSOR_COL     5   /* ▶ cursor col (Gen 1: wTopMenuItemX=5) */
#define LIST_NAME_COL       6   /* item name col (Gen 1: hlcoord 6,4) */
#define LIST_NAME_W        12   /* max name chars (cols 6-17) */
#define LIST_PRICE_COL     11   /* ¥ col on price row (name_col+5, Gen 1 SCREEN_WIDTH+5) */
#define MAX_LIST_VISIBLE    4   /* 4 items per screen (2 rows×4 = rows 4-11) */
#define LIST_SCROLL_COL    18   /* ▼ scroll indicator col (Gen 1: hl-8 after 4 entries) */
#define LIST_SCROLL_ROW    11   /* ▼ scroll indicator row */

/* Quantity box: cols 7-19, rows 9-11
 * Source: PRICEDITEMLISTMENU picker hlcoord(7,9) b=1 c=11 */
#define QTY_L           7
#define QTY_R          19
#define QTY_T           9
#define QTY_B          11
#define QTY_ROW        10   /* inner content row */
#define QTY_TIMES_COL   8
#define QTY_YEN_COL    13
#define QTY_PRICE_COL  14   /* 5-digit total at cols 14-18 */

/* YES/NO box: cols 14-19, rows 7-11
 * Source: TWO_OPTION_MENU at hlcoord(14,7) width=4 height=3 → total 6×5 */
#define YESNO_L        14
#define YESNO_R        19
#define YESNO_T         7
#define YESNO_B        11
#define YESNO_YES_ROW   8
#define YESNO_NO_ROW    9
#define YESNO_CURSOR_COL 15
#define YESNO_TEXT_COL  16

/* ---- Tile writer -------------------------------------------------- */
/* smset(col, row, tile): writes into gScrollTileMap with +2 border offset */
#define smset(c, r, t) \
    gScrollTileMap[((r) + 2) * SCROLL_MAP_W + ((c) + 2)] = (t)

static void sm_tile(int col, int row, uint8_t ch) {
    smset(col, row, (uint8_t)Font_CharToTile(ch));
}

/* ---- State machine ------------------------------------------------ */
typedef enum {
    MART_IDLE = 0,
    MART_MAIN,
    MART_BUY_LIST,
    MART_BUY_QTY,
    MART_BUY_CONFIRM,
    MART_BUY_YESNO,
    MART_BUY_AFTER,
    MART_SELL_LIST,
    MART_SELL_QTY,
    MART_SELL_CONFIRM,
    MART_SELL_YESNO,
    MART_SELL_AFTER,
    MART_LOOP,
    MART_DONE,
} mart_state_t;

/* ---- Module state ------------------------------------------------- */
static mart_state_t g_state       = MART_IDLE;
static const uint8_t *g_inv       = NULL;
static int  g_inv_count           = 0;
static int  g_list_cursor         = 0;
static int  g_list_scroll         = 0;
static int  g_main_cursor         = 0;
static int  g_yesno_cursor        = 0;
static int  g_qty                 = 1;
static uint8_t g_selected_item    = 0;
static int  g_needs_redraw        = 0;
static int  g_panel_drawn         = 0;
static int  g_sell_mode           = 0;
/* Persistent text buffer — must outlive Pokemart_Tick() since Text_ShowASCII
 * only stores the pointer; Text_Update() reads it on subsequent frames. */
static char g_text_buf[80];

/* ---- Item price table (indexed by item ID, 0-based) --------------- */
/* Source: pokered-master/data/items/prices.asm */
static const uint16_t kItemPrices[86] = {
    /*  0 */ 0,
    /*  1 MASTER BALL  */ 0,
    /*  2 ULTRA BALL   */ 1200,
    /*  3 GREAT BALL   */ 600,
    /*  4 POKE BALL    */ 200,
    /*  5 TOWN MAP     */ 0,
    /*  6 BICYCLE      */ 0,
    /*  7 (unused)     */ 0,
    /*  8 SAFARI BALL  */ 1000,
    /*  9 (unused)     */ 0,
    /* 10 (unused)     */ 0,
    /* 11 ANTIDOTE     */ 100,
    /* 12 BURN HEAL    */ 250,
    /* 13 ICE HEAL     */ 250,
    /* 14 AWAKENING    */ 200,
    /* 15 PARLYZ HEAL  */ 200,
    /* 16 FULL RESTORE */ 3000,
    /* 17 MAX POTION   */ 2500,
    /* 18 HYPER POTION */ 1500,
    /* 19 SUPER POTION */ 700,
    /* 20 POTION       */ 300,
    /* 21-28 badges    */ 0, 0, 0, 0, 0, 0, 0, 0,
    /* 29 ESCAPE ROPE  */ 550,
    /* 30 REPEL        */ 350,
    /* 31 (unused)     */ 0,
    /* 32 FIRE STONE   */ 2100,
    /* 33 THUNDER STONE*/ 2100,
    /* 34 WATER STONE  */ 2100,
    /* 35 HP UP        */ 9800,
    /* 36 PROTEIN      */ 9800,
    /* 37 IRON         */ 9800,
    /* 38 CARBOS        */ 9800,
    /* 39 CALCIUM      */ 9800,
    /* 40 RARE CANDY   */ 4800,
    /* 41-45 (unused)  */ 0, 0, 0, 0, 0,
    /* 46 X ACCURACY   */ 950,
    /* 47 LEAF STONE   */ 2100,
    /* 48 (unused)     */ 0,
    /* 49 NUGGET       */ 10000,
    /* 50 (unused)     */ 9800,
    /* 51 POKE DOLL    */ 1000,
    /* 52 FULL HEAL    */ 600,
    /* 53 REVIVE       */ 1500,
    /* 54 MAX REVIVE   */ 4000,
    /* 55 GUARD SPEC.  */ 700,
    /* 56 SUPER REPEL  */ 500,
    /* 57 MAX REPEL    */ 700,
    /* 58 DIRE HIT     */ 650,
    /* 59 COIN         */ 10,
    /* 60 FRESH WATER  */ 200,
    /* 61 SODA POP     */ 300,
    /* 62 LEMONADE     */ 350,
    /* 63-64 (unused)  */ 0, 0,
    /* 65 X ATTACK     */ 500,
    /* 66 X DEFEND     */ 550,
    /* 67 X SPEED      */ 350,
    /* 68 X SPECIAL    */ 350,
    /* 69-85 (unused)  */ 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* ---- Mart inventories --------------------------------------------- */
/* Source: pokered-master/data/items/marts.asm */
static const uint8_t kInv_Viridian[]  = {0x04,0x0B,0x0F,0x0C,0};
static const uint8_t kInv_Pewter[]    = {0x04,0x14,0x1D,0x0B,0x0C,0x0E,0x0F,0};
static const uint8_t kInv_Cerulean[]  = {0x04,0x14,0x1E,0x0B,0x0C,0x0E,0x0F,0};
static const uint8_t kInv_Vermilion[] = {0x04,0x13,0x0D,0x0E,0x0F,0x1E,0};
static const uint8_t kInv_Lavender[]  = {0x03,0x13,0x35,0x1D,0x38,0x0B,0x0C,0x0D,0x0F,0};
static const uint8_t kInv_Cel2F1[]    = {0x03,0x13,0x35,0x38,0x0B,0x0C,0x0D,0x0E,0x0F,0};
static const uint8_t kInv_Cel2F2[]    = {0xE8,0xE9,0xCA,0xCF,0xED,0xC9,0xCD,0xDC,0xE2,0};
static const uint8_t kInv_Cel4F[]     = {0x33,0x20,0x21,0x22,0x2F,0};
static const uint8_t kInv_Cel5F1[]    = {0x2E,0x37,0x3A,0x41,0x42,0x43,0x44,0};
static const uint8_t kInv_Cel5F2[]    = {0x23,0x24,0x25,0x26,0x27,0};
static const uint8_t kInv_Fuchsia[]   = {0x02,0x03,0x13,0x35,0x34,0x38,0};
static const uint8_t kInv_Cinnabar[]  = {0x02,0x03,0x12,0x39,0x1D,0x34,0x35,0};
static const uint8_t kInv_Saffron[]   = {0x03,0x12,0x39,0x1D,0x34,0x35,0};
static const uint8_t kInv_Indigo[]    = {0x02,0x03,0x10,0x11,0x34,0x35,0x39,0};

/* ---- Pokered string constants ------------------------------------- */
static const uint8_t kStrCancel[] = {0x82,0x80,0x8D,0x82,0x84,0x8B,CHAR_TERM};
static const uint8_t kStrBuy[]    = {0x81,0x94,0x98,CHAR_TERM};
static const uint8_t kStrSell[]   = {0x92,0x84,0x8B,0x8B,CHAR_TERM};
static const uint8_t kStrQuit[]   = {0x90,0x94,0x88,0x93,CHAR_TERM};
static const uint8_t kStrMoney[]  = {0x8C,0x8E,0x8D,0x84,0x98,CHAR_TERM};

/* ---- Money helpers (wPlayerMoney = 3-byte packed BCD, big-endian) -- */
static uint32_t money_get(void) {
    return (uint32_t)(
        ((wPlayerMoney[0] >> 4) & 0xF) * 100000u +
        (wPlayerMoney[0] & 0xF)        * 10000u  +
        ((wPlayerMoney[1] >> 4) & 0xF) * 1000u   +
        (wPlayerMoney[1] & 0xF)        * 100u     +
        ((wPlayerMoney[2] >> 4) & 0xF) * 10u      +
        (wPlayerMoney[2] & 0xF)
    );
}

static void money_set(uint32_t v) {
    if (v > 999999u) v = 999999u;
    wPlayerMoney[0] = (uint8_t)(((v / 100000u) << 4) | ((v / 10000u) % 10u));
    wPlayerMoney[1] = (uint8_t)((((v / 1000u) % 10u) << 4) | ((v / 100u) % 10u));
    wPlayerMoney[2] = (uint8_t)((((v / 10u) % 10u) << 4) | (v % 10u));
}

/* ---- Helper: item price lookup ------------------------------------ */
static uint16_t item_price(uint8_t id) {
    if (id == 0 || id >= 86) return 0;
    return kItemPrices[id];
}

/* ---- Helper: can sell item? --------------------------------------- */
static int can_sell(uint8_t id) {
    if (id >= 0xC4) return 0;
    if (Inventory_IsKeyItem(id)) return 0;
    if (item_price(id) == 0) return 0;
    return 1;
}

/* ---- Pokered name → ASCII string ---------------------------------- */
/* Converts pokered-encoded item name to a NUL-terminated ASCII string.
 * Handles uppercase/lowercase letters, digits, space, and common punctuation. */
static char pokered_char_to_ascii(uint8_t b) {
    if (b >= 0x80 && b <= 0x99) return (char)('A' + (b - 0x80));
    if (b >= 0xA0 && b <= 0xB9) return (char)('a' + (b - 0xA0));
    if (b >= 0xF6)               return (char)('0' + (b - 0xF6));
    if (b == 0x7F) return ' ';
    if (b == 0xE3) return '-';
    if (b == 0xE0) return '\'';
    if (b == 0xE8) return '.';
    return ' ';
}

static void get_item_name_ascii(uint8_t item_id, char *out, int max_len) {
    const uint8_t *name = Inventory_GetName(item_id);
    int i = 0;
    for (; i < max_len - 1 && name[i] != CHAR_TERM; i++)
        out[i] = pokered_char_to_ascii(name[i]);
    out[i] = '\0';
}

/* ---- ASCII char → pokered font tile ------------------------------ */
static uint8_t ascii_tile(char c) {
    if (c >= 'A' && c <= 'Z') return (uint8_t)Font_CharToTile((unsigned char)(0x80u + (c - 'A')));
    if (c >= 'a' && c <= 'z') return (uint8_t)Font_CharToTile((unsigned char)(0xA0u + (c - 'a')));
    if (c >= '0' && c <= '9') return (uint8_t)Font_CharToTile((unsigned char)(0xF6u + (c - '0')));
    return (uint8_t)Font_CharToTile(CHAR_SPACE);
}

/* ---- Draw helpers ------------------------------------------------- */

/* Draw a pokered-encoded string at (col, row). */
static void draw_pstr(int col, int row, const uint8_t *s) {
    for (; *s != CHAR_TERM; s++, col++)
        smset(col, row, (uint8_t)Font_CharToTile(*s));
}

/* Draw a box border from (box_l, box_t) to (box_r, box_b). */
static void draw_box(int box_l, int box_t, int box_r, int box_b) {
    sm_tile(box_l, box_t, BC_TL);
    for (int c = box_l + 1; c < box_r; c++) sm_tile(c, box_t, BC_H);
    sm_tile(box_r, box_t, BC_TR);
    for (int r = box_t + 1; r < box_b; r++) {
        sm_tile(box_l, r, BC_V);
        sm_tile(box_r, r, BC_V);
    }
    sm_tile(box_l, box_b, BC_BL);
    for (int c = box_l + 1; c < box_r; c++) sm_tile(c, box_b, BC_H);
    sm_tile(box_r, box_b, BC_BR);
}

/* Fill interior of a box with spaces. */
static void clear_box_interior(int box_l, int box_t, int box_r, int box_b) {
    for (int r = box_t + 1; r < box_b; r++)
        for (int c = box_l + 1; c < box_r; c++)
            sm_tile(c, r, CHAR_SPACE);
}

/* Draw a right-aligned N-digit number starting at (col, row). */
static void draw_digits(int col, int row, uint32_t v, int width) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%*u", width, (unsigned)v);
    for (int i = 0; i < width; i++) {
        char ch = buf[i];
        uint8_t tile = (ch >= '0' && ch <= '9')
            ? (uint8_t)Font_CharToTile((unsigned char)(0xF6u + (ch - '0')))
            : (uint8_t)Font_CharToTile(CHAR_SPACE);
        smset(col + i, row, tile);
    }
}

/* Draw pokered-encoded item name at (col, row), padded/truncated to `width`. */
static void draw_item_name(int col, int row, uint8_t item_id, int width) {
    const uint8_t *name = Inventory_GetName(item_id);
    int drawn = 0;
    for (; drawn < width && name[drawn] != CHAR_TERM; drawn++)
        smset(col + drawn, row, (uint8_t)Font_CharToTile(name[drawn]));
    for (; drawn < width; drawn++)
        sm_tile(col + drawn, row, CHAR_SPACE);
}

/* ---- Money box ---------------------------------------------------- */
/* Draws the MONEY box at cols 11-19, rows 0-2.
 * "MONEY" is placed on the top-border row (col 13) as in Gen 1. */
static void draw_money_box(void) {
    draw_box(MONEY_L, MONEY_T, MONEY_R, MONEY_B);
    clear_box_interior(MONEY_L, MONEY_T, MONEY_R, MONEY_B);
    /* "MONEY" label on the top border row (overwrites ─ chars) */
    draw_pstr(MONEY_LABEL_COL, MONEY_T, kStrMoney);
    /* ¥ + 6-digit amount on content row */
    sm_tile(MONEY_YEN_COL, MONEY_ROW, CHAR_YEN);
    draw_digits(MONEY_DIGIT_COL, MONEY_ROW, money_get(), 6);
}

/* ---- Main menu (BUY/SELL/QUIT) ------------------------------------ */
static void draw_main_menu(void) {
    draw_box(MENU_L, MENU_T, MENU_R, MENU_B);
    for (int r = MENU_T + 1; r < MENU_B; r++)
        for (int c = MENU_L + 1; c < MENU_R; c++)
            sm_tile(c, r, CHAR_SPACE);

    const uint8_t *labels[3] = { kStrBuy, kStrSell, kStrQuit };
    for (int i = 0; i < 3; i++) {
        int row = MENU_ENTRY_ROW + i;
        sm_tile(MENU_CURSOR_COL, row, (i == g_main_cursor) ? CHAR_ARROW : CHAR_SPACE);
        draw_pstr(MENU_TEXT_COL, row, labels[i]);
    }
}

/* ---- Item list ---------------------------------------------------- */
/* One 2-row entry: name row + price row.
 * Mirrors Gen 1 PrintListMenuEntries (home/list_menu.asm):
 *   name  at (LIST_NAME_COL, name_row)
 *   price at (LIST_PRICE_COL, name_row+1)  [SCREEN_WIDTH+5 offset = 1 row + 5 cols]
 *   2*SCREEN_WIDTH advance between entries → 2 rows per entry */
static void draw_list_row(int name_row, int entry_idx, int is_cursor, int show_price) {
    int price_row = name_row + 1;
    /* Clear both rows */
    for (int c = LIST_L + 1; c < LIST_R; c++) {
        sm_tile(c, name_row,  CHAR_SPACE);
        sm_tile(c, price_row, CHAR_SPACE);
    }

    sm_tile(LIST_CURSOR_COL, name_row, is_cursor ? CHAR_ARROW : CHAR_SPACE);

    int total_items = g_sell_mode ? (int)wNumBagItems : g_inv_count;
    if (entry_idx == total_items) {
        draw_pstr(LIST_NAME_COL, name_row, kStrCancel);
        return;
    }

    uint8_t item_id = g_sell_mode ? wBagItems[entry_idx * 2] : g_inv[entry_idx];
    draw_item_name(LIST_NAME_COL, name_row, item_id, LIST_NAME_W);

    if (show_price) {
        uint16_t price = item_price(item_id);
        if (g_sell_mode) price = (uint16_t)(price / 2u);
        sm_tile(LIST_PRICE_COL, price_row, CHAR_YEN);
        draw_digits(LIST_PRICE_COL + 1, price_row, price, 5);
    }
}

static void draw_buy_list(void) {
    g_sell_mode = 0;
    draw_box(LIST_L, LIST_T, LIST_R, LIST_B);
    clear_box_interior(LIST_L, LIST_T, LIST_R, LIST_B);
    int total = g_inv_count + 1; /* items + CANCEL */
    for (int s = 0; s < MAX_LIST_VISIBLE; s++) {
        int idx = g_list_scroll + s;
        if (idx < total)
            draw_list_row(LIST_ITEM_TOP + s * 2, idx, (g_list_cursor == idx), 1);
    }
    /* ▼ scroll indicator when more entries exist below (Gen 1: hl-8 after 4 entries) */
    if (g_list_scroll + MAX_LIST_VISIBLE < total)
        sm_tile(LIST_SCROLL_COL, LIST_SCROLL_ROW, CHAR_ARROW); /* best available glyph */
}

static void draw_sell_list(void) {
    g_sell_mode = 1;
    draw_box(LIST_L, LIST_T, LIST_R, LIST_B);
    clear_box_interior(LIST_L, LIST_T, LIST_R, LIST_B);
    int total = (int)wNumBagItems + 1;
    for (int s = 0; s < MAX_LIST_VISIBLE; s++) {
        int idx = g_list_scroll + s;
        if (idx < total)
            draw_list_row(LIST_ITEM_TOP + s * 2, idx, (g_list_cursor == idx), 0);
    }
    if (g_list_scroll + MAX_LIST_VISIBLE < total)
        sm_tile(LIST_SCROLL_COL, LIST_SCROLL_ROW, CHAR_ARROW);
}

/* ---- Quantity picker box ------------------------------------------ */
/* Box at cols 7-19, rows 9-11.  Content row 10:
 *   col  8 : ×
 *   cols 9-10: 2-digit qty
 *   col 13 : ¥
 *   cols 14-18: 5-digit total price */
static void draw_qty_box(void) {
    draw_box(QTY_L, QTY_T, QTY_R, QTY_B);
    clear_box_interior(QTY_L, QTY_T, QTY_R, QTY_B);

    sm_tile(QTY_TIMES_COL, QTY_ROW, CHAR_TIMES);
    draw_digits(QTY_TIMES_COL + 1, QTY_ROW, (uint32_t)g_qty, 2);

    uint32_t unit_price = g_sell_mode
        ? (uint32_t)(item_price(g_selected_item) / 2u)
        : (uint32_t)item_price(g_selected_item);
    uint32_t total = (uint32_t)g_qty * unit_price;

    sm_tile(QTY_YEN_COL, QTY_ROW, CHAR_YEN);
    draw_digits(QTY_PRICE_COL, QTY_ROW, total, 5);
}

static void clear_qty_box(void) {
    for (int r = QTY_T; r <= QTY_B; r++)
        for (int c = QTY_L; c <= QTY_R; c++)
            sm_tile(c, r, CHAR_SPACE);
}

/* ---- YES/NO box --------------------------------------------------- */
/* Box at cols 14-19, rows 7-11.
 * Source: TWO_OPTION_MENU hlcoord(14,7) width=4 height=3 → total 6×5 */
static void draw_yesno(void) {
    draw_box(YESNO_L, YESNO_T, YESNO_R, YESNO_B);
    clear_box_interior(YESNO_L, YESNO_T, YESNO_R, YESNO_B);

    /* YES row */
    sm_tile(YESNO_CURSOR_COL, YESNO_YES_ROW,
            (g_yesno_cursor == 0) ? CHAR_ARROW : CHAR_SPACE);
    smset(YESNO_TEXT_COL,     YESNO_YES_ROW, ascii_tile('Y'));
    smset(YESNO_TEXT_COL + 1, YESNO_YES_ROW, ascii_tile('E'));
    smset(YESNO_TEXT_COL + 2, YESNO_YES_ROW, ascii_tile('S'));

    /* NO row */
    sm_tile(YESNO_CURSOR_COL, YESNO_NO_ROW,
            (g_yesno_cursor == 1) ? CHAR_ARROW : CHAR_SPACE);
    smset(YESNO_TEXT_COL,     YESNO_NO_ROW, ascii_tile('N'));
    smset(YESNO_TEXT_COL + 1, YESNO_NO_ROW, ascii_tile('O'));
    smset(YESNO_TEXT_COL + 2, YESNO_NO_ROW, ascii_tile(' '));
}

static void clear_yesno(void) {
    for (int r = YESNO_T; r <= YESNO_B; r++)
        for (int c = YESNO_L; c <= YESNO_R; c++)
            sm_tile(c, r, CHAR_SPACE);
}

/* ---- Full panel clear --------------------------------------------- */
static void clear_panel(void) {
    /* Clear BUY/SELL/QUIT box area */
    for (int r = MENU_T; r <= MENU_B; r++)
        for (int c = MENU_L; c <= MENU_R; c++)
            sm_tile(c, r, CHAR_SPACE);
    /* Clear MONEY box area */
    for (int r = MONEY_T; r <= MONEY_B; r++)
        for (int c = MONEY_L; c <= MONEY_R; c++)
            sm_tile(c, r, CHAR_SPACE);
    /* Clear LIST box area */
    for (int r = LIST_T; r <= LIST_B; r++)
        for (int c = LIST_L; c <= LIST_R; c++)
            sm_tile(c, r, CHAR_SPACE);
}

/* Rebuild map tiles and restore all sprite OAM.
 * Called whenever a UI panel is cleared mid-mart so sprites aren't left
 * invisible.  The end-of-tick NPC_HideOverUITiles / Player_HideIfOverUI
 * will re-hide any sprite that lands under a newly drawn box. */
static void restore_map_and_sprites(void) {
    Map_BuildScrollView();
    NPC_BuildView(0, 0);
    Player_SyncOAM();
}

/* ---- Scrolling helpers -------------------------------------------- */
static void list_scroll_clamp(int total_entries) {
    int max_scroll = total_entries - MAX_LIST_VISIBLE;
    if (max_scroll < 0) max_scroll = 0;
    if (g_list_scroll > max_scroll) g_list_scroll = max_scroll;
    if (g_list_scroll < 0) g_list_scroll = 0;
}

static int list_cursor_up(int total_entries) {
    if (g_list_cursor <= 0) return 0;
    g_list_cursor--;
    if (g_list_cursor < g_list_scroll) g_list_scroll--;
    list_scroll_clamp(total_entries);
    return 1;
}

static int list_cursor_down(int total_entries) {
    if (g_list_cursor >= total_entries - 1) return 0;
    g_list_cursor++;
    if (g_list_cursor > g_list_scroll + MAX_LIST_VISIBLE - 1)
        g_list_scroll++;
    list_scroll_clamp(total_entries);
    return 1;
}

/* ---- Public API --------------------------------------------------- */

void Pokemart_SetInventory(const uint8_t *inv) {
    g_inv = inv;
    g_inv_count = 0;
    if (inv) {
        while (inv[g_inv_count] != 0) g_inv_count++;
    }
}

void Pokemart_Start(void) {
    g_state        = MART_MAIN;
    g_main_cursor  = 0;
    g_list_cursor  = 0;
    g_list_scroll  = 0;
    g_qty          = 1;
    g_panel_drawn  = 0;
    g_needs_redraw = 0;
    g_yesno_cursor = 0;
    /* Flush NPC_FacePlayer's tile/flag changes before drawing any UI.
     * game.c returns early after activating the mart so NPC_BuildView
     * won't run again this frame — we must call it here first. */
    NPC_BuildView(0, 0);
    /* Draw the persistent money box, then run the tile-based NPC hide.
     * Mirrors Gen 1 CheckSpriteAvailability: any NPC whose footprint
     * overlaps a UI tile (slot >= 96) is hidden; others stay visible. */
    draw_money_box();
    NPC_HideOverUITiles();
    Player_HideIfOverUI();
}

int Pokemart_IsActive(void) {
    return g_state != MART_IDLE;
}

void Pokemart_Tick(void) {
    switch (g_state) {

    /* ---------------------------------------------------------------- */
    case MART_MAIN:
        if (!g_panel_drawn) {
            draw_money_box();
            draw_main_menu();
            g_panel_drawn = 1;
        }
        if (hJoyPressed & PAD_UP) {
            if (g_main_cursor > 0) {
                sm_tile(MENU_CURSOR_COL, MENU_ENTRY_ROW + g_main_cursor, CHAR_SPACE);
                g_main_cursor--;
                sm_tile(MENU_CURSOR_COL, MENU_ENTRY_ROW + g_main_cursor, CHAR_ARROW);
            }
        }
        if (hJoyPressed & PAD_DOWN) {
            if (g_main_cursor < 2) {
                sm_tile(MENU_CURSOR_COL, MENU_ENTRY_ROW + g_main_cursor, CHAR_SPACE);
                g_main_cursor++;
                sm_tile(MENU_CURSOR_COL, MENU_ENTRY_ROW + g_main_cursor, CHAR_ARROW);
            }
        }
        if (hJoyPressed & PAD_A) {
            if (g_main_cursor == 0) {
                Text_ShowASCII("Take your time.");
                g_state        = MART_BUY_LIST;
                g_list_cursor  = 0;
                g_list_scroll  = 0;
                g_needs_redraw = 1;
            } else if (g_main_cursor == 1) {
                if (wNumBagItems == 0) {
                    Text_ShowASCII("You don't have\nanything to sell.");
                    g_state = MART_LOOP;
                } else {
                    Text_ShowASCII("What would you\nlike to sell?");
                    g_state        = MART_SELL_LIST;
                    g_list_cursor  = 0;
                    g_list_scroll  = 0;
                    g_needs_redraw = 1;
                }
            } else {
                Text_ShowASCII("Thank you!");
                g_state = MART_DONE;
            }
        }
        if (hJoyPressed & PAD_B) {
            Text_ShowASCII("Thank you!");
            g_state = MART_DONE;
        }
        break;

    /* ---------------------------------------------------------------- */
    case MART_BUY_LIST:
        if (g_needs_redraw) {
            draw_money_box();
            draw_buy_list();
            g_needs_redraw = 0;
        }
        {
            int total = g_inv_count + 1;
            if (hJoyPressed & PAD_UP) {
                if (list_cursor_up(total)) g_needs_redraw = 1;
            }
            if (hJoyPressed & PAD_DOWN) {
                if (list_cursor_down(total)) g_needs_redraw = 1;
            }
            if (g_needs_redraw) {
                draw_buy_list();
                g_needs_redraw = 0;
            }
            if (hJoyPressed & PAD_A) {
                if (g_list_cursor == g_inv_count) {
                    restore_map_and_sprites();
                    Text_ShowASCII("Is there anything\nelse I can do?");
                    g_state = MART_LOOP;
                } else {
                    g_selected_item = g_inv[g_list_cursor];
                    g_qty           = 1;
                    g_sell_mode     = 0;
                    draw_qty_box();
                    g_state = MART_BUY_QTY;
                }
            }
            if (hJoyPressed & PAD_B) {
                restore_map_and_sprites();
                Text_ShowASCII("Is there anything\nelse I can do?");
                g_state = MART_LOOP;
            }
        }
        break;

    /* ---------------------------------------------------------------- */
    case MART_BUY_QTY:
        if (hJoyPressed & PAD_UP) {
            if (g_qty < 99) { g_qty++; draw_qty_box(); }
        }
        if (hJoyPressed & PAD_DOWN) {
            if (g_qty > 1) { g_qty--; draw_qty_box(); }
        }
        if (hJoyPressed & PAD_A) {
            uint32_t total = (uint32_t)g_qty * item_price(g_selected_item);
            char item_name[16];
            get_item_name_ascii(g_selected_item, item_name, sizeof(item_name));
            /* Gen 1: _PokemartTellBuyPriceText = "[name]?\nThat will be\n¥[price]. OK?"
             * \xa5 = ISO-8859-1 ¥, mapped to CHAR_YEN tile in Text_ShowASCII. */
            snprintf(g_text_buf, sizeof(g_text_buf), "%s?\nThat will be\n\xa5%u. OK?",
                     item_name, (unsigned)total);
            clear_qty_box();
            Text_ShowASCII(g_text_buf);
            g_state = MART_BUY_CONFIRM;
        }
        if (hJoyPressed & PAD_B) {
            clear_qty_box();
            g_qty          = 1;
            g_needs_redraw = 1;
            g_state        = MART_BUY_LIST;
        }
        break;

    /* ---------------------------------------------------------------- */
    case MART_BUY_CONFIRM:
        g_yesno_cursor = 0;
        draw_yesno();
        g_state = MART_BUY_YESNO;
        break;

    /* ---------------------------------------------------------------- */
    case MART_BUY_YESNO:
        if (hJoyPressed & PAD_UP) {
            if (g_yesno_cursor > 0) { g_yesno_cursor = 0; draw_yesno(); }
        }
        if (hJoyPressed & PAD_DOWN) {
            if (g_yesno_cursor < 1) { g_yesno_cursor = 1; draw_yesno(); }
        }
        if (hJoyPressed & PAD_A) {
            if (g_yesno_cursor == 0) {
                clear_yesno();
                uint32_t total = (uint32_t)g_qty * item_price(g_selected_item);
                uint32_t money = money_get();
                if (money < total) {
                    Text_ShowASCII("You don't have\nenough money.");
                    g_state = MART_BUY_AFTER;
                } else if (Inventory_Add(g_selected_item, (uint8_t)g_qty) != 0) {
                    Text_ShowASCII("You can't carry\nany more items.");
                    g_state = MART_BUY_AFTER;
                } else {
                    money_set(money - total);
                    Audio_PlaySFX_Purchase();
                    Text_ShowASCII("Here you are!\nThank you!");
                    g_state = MART_BUY_AFTER;
                }
            } else {
                clear_yesno();
                g_needs_redraw = 1;
                g_state        = MART_BUY_LIST;
            }
        }
        if (hJoyPressed & PAD_B) {
            clear_yesno();
            g_needs_redraw = 1;
            g_state        = MART_BUY_LIST;
        }
        break;

    /* ---------------------------------------------------------------- */
    case MART_BUY_AFTER:
        g_qty          = 1;
        g_needs_redraw = 1;
        draw_money_box();
        g_state = MART_BUY_LIST;
        break;

    /* ---------------------------------------------------------------- */
    case MART_SELL_LIST:
        if (g_needs_redraw) {
            draw_money_box();
            draw_sell_list();
            g_needs_redraw = 0;
        }
        {
            int total = (int)wNumBagItems + 1;
            if (hJoyPressed & PAD_UP) {
                if (list_cursor_up(total)) g_needs_redraw = 1;
            }
            if (hJoyPressed & PAD_DOWN) {
                if (list_cursor_down(total)) g_needs_redraw = 1;
            }
            if (g_needs_redraw) {
                draw_sell_list();
                g_needs_redraw = 0;
            }
            if (hJoyPressed & PAD_A) {
                if (g_list_cursor == (int)wNumBagItems) {
                    Map_BuildScrollView();
                    Text_ShowASCII("Is there anything\nelse I can do?");
                    g_state = MART_LOOP;
                } else {
                    g_selected_item = wBagItems[g_list_cursor * 2];
                    g_qty           = 1;
                    g_sell_mode     = 1;
                    draw_qty_box();
                    g_state = MART_SELL_QTY;
                }
            }
            if (hJoyPressed & PAD_B) {
                restore_map_and_sprites();
                Text_ShowASCII("Is there anything\nelse I can do?");
                g_state = MART_LOOP;
            }
        }
        break;

    /* ---------------------------------------------------------------- */
    case MART_SELL_QTY:
        if (hJoyPressed & PAD_UP) {
            if (g_qty < 99) { g_qty++; draw_qty_box(); }
        }
        if (hJoyPressed & PAD_DOWN) {
            if (g_qty > 1) { g_qty--; draw_qty_box(); }
        }
        if (hJoyPressed & PAD_A) {
            uint32_t sell_per = item_price(g_selected_item) / 2;
            uint32_t sell_total = (uint32_t)g_qty * sell_per;
            snprintf(g_text_buf, sizeof(g_text_buf), "I can pay you\n\xa5%u for that.", (unsigned)sell_total);
            clear_qty_box();
            Text_ShowASCII(g_text_buf);
            g_state = MART_SELL_CONFIRM;
        }
        if (hJoyPressed & PAD_B) {
            clear_qty_box();
            g_qty          = 1;
            g_needs_redraw = 1;
            g_state        = MART_SELL_LIST;
        }
        break;

    /* ---------------------------------------------------------------- */
    case MART_SELL_CONFIRM:
        if (!can_sell(g_selected_item)) {
            Text_ShowASCII("I can't put a\nprice on that.");
            g_state = MART_SELL_AFTER;
        } else {
            g_yesno_cursor = 0;
            draw_yesno();
            g_state = MART_SELL_YESNO;
        }
        break;

    /* ---------------------------------------------------------------- */
    case MART_SELL_YESNO:
        if (hJoyPressed & PAD_UP) {
            if (g_yesno_cursor > 0) { g_yesno_cursor = 0; draw_yesno(); }
        }
        if (hJoyPressed & PAD_DOWN) {
            if (g_yesno_cursor < 1) { g_yesno_cursor = 1; draw_yesno(); }
        }
        if (hJoyPressed & PAD_A) {
            if (g_yesno_cursor == 0) {
                clear_yesno();
                uint32_t sell_per   = item_price(g_selected_item) / 2;
                uint32_t sell_total = (uint32_t)g_qty * sell_per;
                Inventory_Remove(g_selected_item, (uint8_t)g_qty);
                money_set(money_get() + sell_total);
                Audio_PlaySFX_PressAB();
                Text_ShowASCII("Here you are!\nThank you!");
                g_state = MART_SELL_AFTER;
            } else {
                clear_yesno();
                g_needs_redraw = 1;
                g_state        = MART_SELL_LIST;
            }
        }
        if (hJoyPressed & PAD_B) {
            clear_yesno();
            g_needs_redraw = 1;
            g_state        = MART_SELL_LIST;
        }
        break;

    /* ---------------------------------------------------------------- */
    case MART_SELL_AFTER:
        g_qty          = 1;
        g_needs_redraw = 1;
        draw_money_box();
        if (g_list_cursor > (int)wNumBagItems)
            g_list_cursor = (int)wNumBagItems;
        list_scroll_clamp((int)wNumBagItems + 1);
        g_state = MART_SELL_LIST;
        break;

    /* ---------------------------------------------------------------- */
    case MART_LOOP:
        g_main_cursor = 0;
        g_panel_drawn = 0;
        g_state       = MART_MAIN;
        break;

    /* ---------------------------------------------------------------- */
    case MART_DONE:
        clear_panel();
        restore_map_and_sprites();
        g_state = MART_IDLE;
        break;

    /* ---------------------------------------------------------------- */
    case MART_IDLE:
    default:
        break;
    }

    /* Re-run tile-based hide every tick (mirrors Gen 1 per-frame
     * CheckSpriteAvailability / UpdatePlayerSprite tile check) so sprites
     * are hidden as new boxes appear. */
    if (g_state != MART_IDLE) {
        NPC_HideOverUITiles();
        Player_HideIfOverUI();
    }
}

/* ---- Per-mart entry points ---------------------------------------- */
void ViridianMart_Start(void)   { Pokemart_SetInventory(kInv_Viridian);  Pokemart_Start(); }
void PewterMart_Start(void)     { Pokemart_SetInventory(kInv_Pewter);    Pokemart_Start(); }
void CeruleanMart_Start(void)   { Pokemart_SetInventory(kInv_Cerulean);  Pokemart_Start(); }
void VermilionMart_Start(void)  { Pokemart_SetInventory(kInv_Vermilion); Pokemart_Start(); }
void LavenderMart_Start(void)   { Pokemart_SetInventory(kInv_Lavender);  Pokemart_Start(); }
void Celadon2F1Mart_Start(void) { Pokemart_SetInventory(kInv_Cel2F1);    Pokemart_Start(); }
void Celadon2F2Mart_Start(void) { Pokemart_SetInventory(kInv_Cel2F2);    Pokemart_Start(); }
void Celadon4FMart_Start(void)  { Pokemart_SetInventory(kInv_Cel4F);     Pokemart_Start(); }
void Celadon5F1Mart_Start(void) { Pokemart_SetInventory(kInv_Cel5F1);    Pokemart_Start(); }
void Celadon5F2Mart_Start(void) { Pokemart_SetInventory(kInv_Cel5F2);    Pokemart_Start(); }
void FuchsiaMart_Start(void)    { Pokemart_SetInventory(kInv_Fuchsia);   Pokemart_Start(); }
void CinnabarMart_Start(void)   { Pokemart_SetInventory(kInv_Cinnabar);  Pokemart_Start(); }
void SaffronMart_Start(void)    { Pokemart_SetInventory(kInv_Saffron);   Pokemart_Start(); }
void IndigoMart_Start(void)     { Pokemart_SetInventory(kInv_Indigo);    Pokemart_Start(); }
