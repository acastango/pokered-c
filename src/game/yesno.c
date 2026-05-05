/* yesno.c — YES/NO prompt overlay.
 * Shows a text prompt then renders a YES/NO selection box at the
 * bottom-right corner of the screen. */
#include "yesno.h"
#include "text.h"
#include "overworld.h"       /* gScrollTileMap, SCROLL_MAP_W */
#include "constants.h"       /* PAD_* */
#include "../data/font_data.h"
#include "../platform/hardware.h"

/* ASM menu geometry (YesNoChoice): cols 14-19, rows 8-11 */
#define BOX_ROW   8
#define BOX_COL  14
#define BOX_W     6
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
    YN_TEXT,   /* text showing; may draw box now or wait for final page close */
    YN_DRAW,   /* first frame after text: draw box */
    YN_INPUT,  /* waiting for A/B/UP/DOWN */
    YN_DONE,   /* result ready; returns to IDLE next tick */
} yn_state_t;

static yn_state_t s_state  = YN_IDLE;
static int        s_cursor = 0;  /* 0=YES, 1=NO */
static int        s_result = 0;
static int        s_saved_valid = 0;
static int        s_wait_for_text_close = 0;

static uint8_t s_saved[BOX_H * BOX_W];

/* If prompt requires multiple pages/scrolling, defer YES/NO until the final
 * page is reached (after text closes). Otherwise show the box immediately. */
static int prompt_requires_scroll(const char *p) {
    int line = 1;
    if (!p) return 0;
    while (*p) {
        if (*p == '\f') return 1;
        if (*p == '\n') {
            line++;
            if (line > 2) return 1;
        }
        p++;
    }
    return 0;
}

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
    /* Top border: ┌────┐ */
    set_tile(BOX_ROW, BOX_COL,             Font_CharToTile(POKE_CORNER_TL));
    for (int c = 1; c < BOX_W - 1; c++)
        set_tile(BOX_ROW, BOX_COL + c,     Font_CharToTile(POKE_HORIZ));
    set_tile(BOX_ROW, BOX_COL + BOX_W - 1, Font_CharToTile(POKE_CORNER_TR));

    /* Bottom border: └────┘ */
    set_tile(BOX_ROW + 3, BOX_COL,              Font_CharToTile(POKE_CORNER_BL));
    for (int c = 1; c < BOX_W - 1; c++)
        set_tile(BOX_ROW + 3, BOX_COL + c,      Font_CharToTile(POKE_HORIZ));
    set_tile(BOX_ROW + 3, BOX_COL + BOX_W - 1,  Font_CharToTile(POKE_CORNER_BR));

    /* YES row */
    set_tile(BOX_ROW + 1, BOX_COL,             Font_CharToTile(POKE_VERT));
    set_tile(BOX_ROW + 1, BOX_COL + 1, s_cursor == 0 ? Font_CharToTile(POKE_CURSOR)
                                                      : Font_CharToTile(POKE_SPACE));
    set_tile(BOX_ROW + 1, BOX_COL + 2, poke_char('Y'));
    set_tile(BOX_ROW + 1, BOX_COL + 3, poke_char('E'));
    set_tile(BOX_ROW + 1, BOX_COL + 4, poke_char('S'));
    set_tile(BOX_ROW + 1, BOX_COL + BOX_W - 1, Font_CharToTile(POKE_VERT));

    /* NO row */
    set_tile(BOX_ROW + 2, BOX_COL,             Font_CharToTile(POKE_VERT));
    set_tile(BOX_ROW + 2, BOX_COL + 1, s_cursor == 1 ? Font_CharToTile(POKE_CURSOR)
                                                      : Font_CharToTile(POKE_SPACE));
    set_tile(BOX_ROW + 2, BOX_COL + 2, poke_char('N'));
    set_tile(BOX_ROW + 2, BOX_COL + 3, poke_char('O'));
    set_tile(BOX_ROW + 2, BOX_COL + 4, Font_CharToTile(POKE_SPACE));
    set_tile(BOX_ROW + 2, BOX_COL + BOX_W - 1, Font_CharToTile(POKE_VERT));
}

/* ---- Public API ------------------------------------------------------- */

void YesNo_Show(const char *prompt) {
    s_state  = YN_TEXT;
    s_cursor = 0;
    s_result = 0;
    s_saved_valid = 0;
    s_wait_for_text_close = prompt_requires_scroll(prompt);
    /* Keep the question box visible after text completes so YES/NO overlays
     * on top of it (Gen 1 behavior). */
    Text_KeepTilesOnClose();
    Text_ShowASCII(prompt);
}

int  YesNo_IsOpen(void)      { return s_state != YN_IDLE; }
int  YesNo_GetResult(void)   { return s_result; }
void YesNo_PostRender(void)  {
    int show_box = (s_state == YN_DRAW || s_state == YN_INPUT);
    if (s_state == YN_TEXT && !s_wait_for_text_close) show_box = 1;
    if (show_box) {
        if (!s_saved_valid) {
            save_tiles();
            s_saved_valid = 1;
        }
        draw_box();
    }
}

void YesNo_Tick(void) {
    switch (s_state) {
        case YN_IDLE:
            break;

        case YN_TEXT:
            if (!s_wait_for_text_close || !Text_IsOpen()) {
                if (!s_saved_valid) {
                    save_tiles();
                    s_saved_valid = 1;
                }
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
                if (s_saved_valid) restore_tiles();
                s_state = YN_DONE;
            } else if (hJoyPressed & PAD_B) {
                s_result = 0;
                if (s_saved_valid) restore_tiles();
                s_state = YN_DONE;
            }
            break;

        case YN_DONE:
            s_saved_valid = 0;
            s_state = YN_IDLE;
            break;
    }
}
