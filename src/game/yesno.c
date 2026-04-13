/* yesno.c — YES/NO prompt overlay.
 * Shows a text prompt then renders a YES/NO selection box at the
 * bottom-right corner of the screen. */
#include "yesno.h"
#include "text.h"
#include "overworld.h"       /* gScrollTileMap, SCROLL_MAP_W */
#include "constants.h"       /* PAD_* */
#include "../data/font_data.h"
#include "../platform/hardware.h"

/* Box position on screen (0-indexed, rows 0-17, cols 0-19) */
#define BOX_ROW  14
#define BOX_COL  13
#define BOX_W     7
#define BOX_H     4

/* Pokemon font encoding helpers */
#define POKE_SPACE  0x7F
#define POKE_CORNER_TL 0x79
#define POKE_HORIZ     0x7A
#define POKE_CORNER_TR 0x7B
#define POKE_VERT      0x7C
#define POKE_CORNER_BL 0x7D
#define POKE_CORNER_BR 0x7E
#define POKE_CURSOR    0xED  /* ► arrow in Pokemon Red font */

typedef enum {
    YN_IDLE = 0,
    YN_TEXT,   /* text showing; wait for it to close */
    YN_DRAW,   /* first frame after text: draw box */
    YN_INPUT,  /* waiting for A/B/UP/DOWN */
    YN_DONE,   /* result ready; returns to IDLE next tick */
} yn_state_t;

static yn_state_t s_state  = YN_IDLE;
static int        s_cursor = 0;  /* 0=YES, 1=NO */
static int        s_result = 0;

static uint8_t s_saved[BOX_H * BOX_W];

static int poke_char(unsigned char c) {
    if (c >= 'A' && c <= 'Z') return Font_CharToTile((unsigned char)(0x80 + (c - 'A')));
    if (c >= 'a' && c <= 'z') return Font_CharToTile((unsigned char)(0xA0 + (c - 'a')));
    return Font_CharToTile(POKE_SPACE);
}

#define TMIDX(r, c) (((r) + 2) * SCROLL_MAP_W + ((c) + 2))

static void set_tile(int row, int col, int tile) {
    gScrollTileMap[TMIDX(row, col)] = (uint8_t)tile;
}

static void save_tiles(void) {
    for (int r = 0; r < BOX_H; r++)
        for (int c = 0; c < BOX_W; c++)
            s_saved[r * BOX_W + c] = gScrollTileMap[TMIDX(BOX_ROW + r, BOX_COL + c)];
}

static void restore_tiles(void) {
    for (int r = 0; r < BOX_H; r++)
        for (int c = 0; c < BOX_W; c++)
            gScrollTileMap[TMIDX(BOX_ROW + r, BOX_COL + c)] = s_saved[r * BOX_W + c];
}

static void draw_box(void) {
    /* Top border: ┌─────┐ */
    set_tile(BOX_ROW, BOX_COL,             Font_CharToTile(POKE_CORNER_TL));
    for (int c = 1; c < BOX_W - 1; c++)
        set_tile(BOX_ROW, BOX_COL + c,     Font_CharToTile(POKE_HORIZ));
    set_tile(BOX_ROW, BOX_COL + BOX_W - 1, Font_CharToTile(POKE_CORNER_TR));

    /* Bottom border: └─────┘ */
    set_tile(BOX_ROW + 3, BOX_COL,              Font_CharToTile(POKE_CORNER_BL));
    for (int c = 1; c < BOX_W - 1; c++)
        set_tile(BOX_ROW + 3, BOX_COL + c,      Font_CharToTile(POKE_HORIZ));
    set_tile(BOX_ROW + 3, BOX_COL + BOX_W - 1,  Font_CharToTile(POKE_CORNER_BR));

    /* Row 1: │ ►YES │ or │  YES │ */
    /* Row 2: │  NO  │ or │ ►NO  │ */
    static const char * const kLabels[2] = { "YES", "NO " };
    for (int r = 0; r < 2; r++) {
        set_tile(BOX_ROW + 1 + r, BOX_COL,             Font_CharToTile(POKE_VERT));
        set_tile(BOX_ROW + 1 + r, BOX_COL + BOX_W - 1, Font_CharToTile(POKE_VERT));
        /* cursor */
        int is_selected = (r == s_cursor);
        set_tile(BOX_ROW + 1 + r, BOX_COL + 1,
                 is_selected ? Font_CharToTile(POKE_CURSOR)
                             : Font_CharToTile(POKE_SPACE));
        /* label (3 chars) */
        const char *label = kLabels[r];
        for (int i = 0; i < 3; i++)
            set_tile(BOX_ROW + 1 + r, BOX_COL + 2 + i, poke_char(label[i]));
        /* trailing space */
        set_tile(BOX_ROW + 1 + r, BOX_COL + 5, Font_CharToTile(POKE_SPACE));
    }
}

/* ---- Public API ------------------------------------------------------- */

void YesNo_Show(const char *prompt) {
    s_state  = YN_TEXT;
    s_cursor = 0;
    s_result = 0;
    wDoNotWaitForButtonPress = 0;
    Text_ShowASCII(prompt);
}

int  YesNo_IsOpen(void)      { return s_state != YN_IDLE; }
int  YesNo_GetResult(void)   { return s_result; }
void YesNo_PostRender(void)  { if (s_state == YN_INPUT) draw_box(); }

void YesNo_Tick(void) {
    switch (s_state) {
        case YN_IDLE:
            break;

        case YN_TEXT:
            /* Text is ticked by game.c before TMHM_Tick runs; by the time
             * YesNo_Tick is called, text has already been closed. */
            if (!Text_IsOpen()) {
                save_tiles();
                s_state = YN_DRAW;
            }
            break;

        case YN_DRAW:
            draw_box();
            s_state = YN_INPUT;
            break;

        case YN_INPUT:
            if (hJoyPressed & PAD_UP)   s_cursor = 0;
            if (hJoyPressed & PAD_DOWN) s_cursor = 1;
            draw_box();  /* refresh cursor each frame */
            if (hJoyPressed & PAD_A) {
                s_result = (s_cursor == 0) ? 1 : 0;
                restore_tiles();
                s_state = YN_DONE;
            } else if (hJoyPressed & PAD_B) {
                s_result = 0;
                restore_tiles();
                s_state = YN_DONE;
            }
            break;

        case YN_DONE:
            s_state = YN_IDLE;
            break;
    }
}
