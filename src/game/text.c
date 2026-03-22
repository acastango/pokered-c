/* text.c — Text box rendering and dialog state machine.
 *
 * Dialog box layout (bottom 4 tile rows of the 20×18 screen):
 *   Row 14:  ┌──────────────────┐   (top border)
 *   Row 15:  │ text line 1      │
 *   Row 16:  │ text line 2      │
 *   Row 17:  └──────────────────┘   (bottom border)
 *
 * Text is pokered-encoded (see constants/charmap.asm).
 * Supported control codes (pokered path):
 *   $50 '@'  — string terminator
 *   $4E <LINE> — next line; if already on line 2, scroll
 *   $4F <PARA> — wait for A then clear and continue
 *   $51 <PAGE> — same as <PARA>
 *   $55 <CONT> — scroll
 *
 * ASCII path (\n, \f, @, {PLAYER}, {RIVAL}):
 *   \n — next line; if already on line 2, scroll
 *   \f — wait for A then clear (paragraph break)
 *   @  — end of string
 *   {PLAYER} / {RIVAL} — substituted with player/rival name
 *
 * wait_input values:
 *   0 = not waiting
 *   1 = paragraph break (clear box on A, restart from line 1)
 *   2 = end of string   (close box on A)
 *   3 = scroll          (scroll line2→line1 on A, continue on line2)
 *
 * After each pause, the ▼ cursor appears in the bottom-right corner.
 */
#include "text.h"
#include "../platform/hardware.h"
#include "../platform/audio.h"
#include "../data/font_data.h"
#include "../game/constants.h"
#include <string.h>

/* Control character codes from pokered charmap.asm */
#define CHAR_TERM   0x50   /* '@' — end of string */
#define CHAR_LINE   0x4E   /* <LINE> — next line / scroll */
#define CHAR_PARA   0x4F   /* <PARA> — wait then clear */
#define CHAR_PAGE   0x51   /* <PAGE> — same as PARA */
#define CHAR_CONT   0x55   /* <CONT> — scroll */

/* Screen layout constants — mirrors pokered's overworld text box.
 *
 * Original: TextBoxBorder called with hlcoord 0,12 / b=4 / c=18.
 *   Row 12: ┌──────────────────┐  top border
 *   Row 13: │                  │  blank padding (scroll buffer)
 *   Row 14: │ text line 1      │  first printed line  ← TEXT_ROW1
 *   Row 15: │                  │  blank gap (double-spacing)
 *   Row 16: │ text line 2      │  second printed line ← TEXT_ROW2
 *   Row 17: └──────────────────┘  bottom border
 *
 * <LINE> always jumps to row 16. <NEXT> advances 2 rows.
 * ▼ cursor always at (18, 16).
 */
#define BOX_ROW     12     /* top border */
#define BOX_ROWS     6     /* total rows occupied (12-17) */
#define TEXT_ROW1   14     /* first text line */
#define TEXT_ROW2   16     /* second text line / LINE target */
#define TEXT_COL0    1     /* first text column (after │) */
#define TEXT_COLS   18     /* usable chars per line */

#define CURSOR_CHAR 0xEE   /* pokered char code for ▼ */
#define BLANK_CHAR  0x7F   /* pokered space */

/* ---- State -------------------------------------------------- */
/* VBlanks between each printed character — matches original MED text speed.
 * Original: wOptions bits 0-3 → hFrameCounter.  MED default = 3 frames/char.
 * A or B held skips this delay (prints one char per frame = ~60 chars/sec). */
#define TEXT_LETTER_DELAY  3

static const uint8_t *text_ptr   = NULL;
static const char    *ascii_ptr  = NULL;
static int            ascii_mode = 0;
static int            text_open  = 0;
static int            wait_input = 0;
static int            letter_timer = 0;   /* frames remaining before next char */
/* cursor position saved so scroll can clear it before copying */
static int            cur_col    = TEXT_COL0;
static int            cur_row    = TEXT_ROW1;

/* ---- Internal helpers --------------------------------------- */

static int ascii_to_tile(unsigned char c) {
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
    if (c == ';')              return Font_CharToTile(0x9D);
    if (c == '(')              return Font_CharToTile(0x9A);
    if (c == ')')              return Font_CharToTile(0x9B);
    if (c == '/')              return Font_CharToTile(0xF3);
    if (c == '!')              return Font_CharToTile(0xE7);
    return BLANK_TILE_SLOT;
}

static void set_tile(int col, int row, uint8_t tile_id) {
    wTileMap[row * SCREEN_WIDTH + col] = tile_id;
}

static uint8_t get_tile(int col, int row) {
    return wTileMap[row * SCREEN_WIDTH + col];
}

static void show_cursor(void) {
    set_tile(SCREEN_WIDTH - 2, TEXT_ROW2, (uint8_t)Font_CharToTile(CURSOR_CHAR));
}

static void hide_cursor(void) {
    set_tile(SCREEN_WIDTH - 2, TEXT_ROW2, (uint8_t)Font_CharToTile(BLANK_CHAR));
}

static void draw_box_border(void) {
    /* Top border (row 12) */
    set_tile(0,              BOX_ROW, (uint8_t)Font_CharToTile(0x79)); /* ┌ */
    for (int c = 1; c < SCREEN_WIDTH - 1; c++)
        set_tile(c,          BOX_ROW, (uint8_t)Font_CharToTile(0x7A)); /* ─ */
    set_tile(SCREEN_WIDTH-1, BOX_ROW, (uint8_t)Font_CharToTile(0x7B)); /* ┐ */

    /* Interior rows 13-16: │ blank │ */
    for (int r = BOX_ROW + 1; r <= BOX_ROW + BOX_ROWS - 2; r++) {
        set_tile(0,               r, (uint8_t)Font_CharToTile(0x7C)); /* │ */
        for (int c = 1; c < SCREEN_WIDTH - 1; c++)
            set_tile(c,           r, (uint8_t)Font_CharToTile(BLANK_CHAR));
        set_tile(SCREEN_WIDTH-1,  r, (uint8_t)Font_CharToTile(0x7C)); /* │ */
    }

    /* Bottom border (row 17) */
    int bot = BOX_ROW + BOX_ROWS - 1;
    set_tile(0,              bot, (uint8_t)Font_CharToTile(0x7D)); /* └ */
    for (int c = 1; c < SCREEN_WIDTH - 1; c++)
        set_tile(c,          bot, (uint8_t)Font_CharToTile(0x7A)); /* ─ */
    set_tile(SCREEN_WIDTH-1, bot, (uint8_t)Font_CharToTile(0x7E)); /* ┘ */
}

static void clear_text_rows(void) {
    /* Only clear the two actual text rows (14 and 16); rows 13 and 15 stay blank. */
    uint8_t blank = (uint8_t)Font_CharToTile(BLANK_CHAR);
    for (int c = TEXT_COL0; c < TEXT_COL0 + TEXT_COLS; c++) {
        set_tile(c, TEXT_ROW1, blank);
        set_tile(c, TEXT_ROW2, blank);
    }
}

/* Scroll line 2 up into line 1, clear line 2.
 * Mirrors _ContText / ScrollTextUpOneLine from home/text.asm.
 * Original does two ScrollTextUpOneLine calls (each copies 3 rows up 1),
 * net effect: old text-row2 → text-row1, text-row2 clears. */
static void scroll_up(void) {
    hide_cursor();
    /* Copy text row 2 content into text row 1 */
    for (int c = 0; c < SCREEN_WIDTH; c++)
        set_tile(c, TEXT_ROW1, get_tile(c, TEXT_ROW2));
    /* Clear text row 2 for next line */
    uint8_t blank = (uint8_t)Font_CharToTile(BLANK_CHAR);
    for (int c = TEXT_COL0; c < TEXT_COL0 + TEXT_COLS; c++)
        set_tile(c, TEXT_ROW2, blank);
}

/* Print the pokered-encoded name (wPlayerName / wRivalName) into the box.
 * Name bytes use pokered charmap; terminates at CHAR_TERM (0x50) or NUL. */
static int print_poke_name(const uint8_t *name, int col, int row) {
    for (int i = 0; i < NAME_LENGTH; i++) {
        uint8_t b = name[i];
        if (b == 0x00 || b == CHAR_TERM) break;
        if (col < TEXT_COL0 + TEXT_COLS) {
            set_tile(col++, row, (uint8_t)Font_CharToTile(b));
        }
    }
    return col;
}

/* Print a NUL-terminated ASCII literal into the box (for name defaults). */
static int print_ascii_literal(const char *s, int col, int row) {
    for (; *s; s++) {
        if (col < TEXT_COL0 + TEXT_COLS)
            set_tile(col++, row, (uint8_t)ascii_to_tile((unsigned char)*s));
    }
    return col;
}

/* Print exactly ONE visible character from the current text stream.
 *
 * Mirrors the original's per-character PrintLetterDelay loop:
 *   TextCommandProcessor prints one char, calls PrintLetterDelay, loops.
 *
 * Returns:
 *   1  — a visible character was written; caller should (re)start letter_timer
 *   0  — a control code was handled; wait_input is set (or stays 0 for
 *         LINE on row 1, which just moves the cursor — no pause needed)
 *
 * Control-code moves that don't pause (LINE on row 1) return 0 with
 * wait_input == 0 so the caller skips the timer reset and tries again on the
 * very next frame — one blank frame between lines, matching the original. */
static int print_one_char(void) {
    if (ascii_mode) {
        if (!ascii_ptr || !*ascii_ptr) {
            show_cursor(); wait_input = 2; return 0;
        }
        char ac = *ascii_ptr;

        if (ac == '@') { show_cursor(); wait_input = 2; return 0; }

        if (ac == '\n') {
            ascii_ptr++;
            if (cur_row == TEXT_ROW2) { show_cursor(); wait_input = 3; return 0; }
            cur_row = TEXT_ROW2; cur_col = TEXT_COL0;
            return 0;   /* cursor moved, no visible char — no timer reset */
        }
        if (ac == '\f') { ascii_ptr++; show_cursor(); wait_input = 1; return 0; }

        /* {PLAYER} / {RIVAL} — expand inline (all name chars printed at once,
         * same frame; matches PlaceString which loops without per-char delay
         * in the name substitution path). */
        if (ac == '{') {
            if (strncmp(ascii_ptr, "{PLAYER}", 8) == 0) {
                ascii_ptr += 8;
                if (wPlayerName[0] && wPlayerName[0] != CHAR_TERM)
                    cur_col = print_poke_name(wPlayerName, cur_col, cur_row);
                else
                    cur_col = print_ascii_literal("RED", cur_col, cur_row);
                return 1;
            }
            if (strncmp(ascii_ptr, "{RIVAL}", 7) == 0) {
                ascii_ptr += 7;
                if (wRivalName[0] && wRivalName[0] != CHAR_TERM)
                    cur_col = print_poke_name(wRivalName, cur_col, cur_row);
                else
                    cur_col = print_ascii_literal("GARY", cur_col, cur_row);
                return 1;
            }
            /* Unknown token — skip to closing brace */
            while (*ascii_ptr && *ascii_ptr != '}') ascii_ptr++;
            if (*ascii_ptr == '}') ascii_ptr++;
            return 0;
        }

        if (cur_col < TEXT_COL0 + TEXT_COLS)
            set_tile(cur_col++, cur_row, (uint8_t)ascii_to_tile((unsigned char)ac));
        ascii_ptr++;
        return 1;
    }

    /* Pokered-encoded path */
    if (!text_ptr || !*text_ptr) {
        show_cursor(); wait_input = 2; return 0;
    }
    uint8_t c = *text_ptr;

    if (c == CHAR_TERM) { show_cursor(); wait_input = 2; return 0; }

    if (c == CHAR_LINE) {
        text_ptr++;
        if (cur_row == TEXT_ROW2) { show_cursor(); wait_input = 3; return 0; }
        cur_row = TEXT_ROW2; cur_col = TEXT_COL0;
        return 0;   /* cursor moved, no visible char — no timer reset */
    }
    if (c == CHAR_PARA || c == CHAR_PAGE) {
        text_ptr++; show_cursor(); wait_input = 1; return 0;
    }
    if (c == CHAR_CONT) {
        text_ptr++; show_cursor(); wait_input = 3; return 0;
    }

    if (cur_col < TEXT_COL0 + TEXT_COLS)
        set_tile(cur_col++, cur_row, (uint8_t)Font_CharToTile(c));
    text_ptr++;
    return 1;
}

/* ---- Public API -------------------------------------------- */

void Text_ShowBox(const uint8_t *str) {
    text_ptr     = str;
    ascii_ptr    = NULL;
    ascii_mode   = 0;
    text_open    = 1;
    wait_input   = 0;
    letter_timer = 0;   /* first char prints on the very next Text_Update call */

    draw_box_border();
    clear_text_rows();

    cur_col = TEXT_COL0; cur_row = TEXT_ROW1;
}

void Text_ShowASCII(const char *str) {
    ascii_ptr    = str;
    text_ptr     = NULL;
    ascii_mode   = 1;
    text_open    = 1;
    wait_input   = 0;
    letter_timer = 0;

    draw_box_border();
    clear_text_rows();

    cur_col = TEXT_COL0; cur_row = TEXT_ROW1;
}

int Text_IsOpen(void) {
    return text_open;
}

void Text_Update(void) {
    if (!text_open) return;

    /* ---- Waiting for A press (pause / scroll / end) ---- */
    if (wait_input != 0) {
        if (!(hJoyPressed & PAD_A)) return;
        Audio_PlaySFX_PressAB();

        if (wait_input == 2) { Text_Close(); return; }

        if (wait_input == 3) {
            /* Scroll: line 2 → line 1, continue printing on line 2 */
            scroll_up();
            wait_input   = 0;
            letter_timer = 0;
            cur_col = TEXT_COL0; cur_row = TEXT_ROW2;
            return;
        }

        /* wait_input == 1: paragraph break — clear both lines and restart */
        wait_input   = 0;
        letter_timer = 0;
        clear_text_rows();
        cur_col = TEXT_COL0; cur_row = TEXT_ROW1;
        return;
    }

    /* ---- Printing mode: per-character delay ---- *
     * Mirrors PrintLetterDelay: wait N VBlanks between chars.         *
     * A or B held → skip the delay (prints one char per frame).       *
     * Original: hJoyHeld check; held skips the counter, waits 1 more  *
     * DelayFrame then prints.  We simplify to: held → timer = 0.      */
    if (letter_timer > 0) {
        if (hJoyHeld & (PAD_A | PAD_B))
            letter_timer = 0;
        else
            { letter_timer--; return; }
    }

    /* Print one character; if it returns 1, start the inter-char timer. */
    if (print_one_char())
        letter_timer = TEXT_LETTER_DELAY;
    /* if 0: control code or cursor-move — wait_input is set (or stays 0
     * for LINE on row 1), and letter_timer stays 0 so we retry next frame */
}

void Text_Close(void) {
    text_open    = 0;
    wait_input   = 0;
    letter_timer = 0;
    text_ptr     = NULL;
    ascii_ptr    = NULL;
    ascii_mode   = 0;

    for (int r = BOX_ROW; r < BOX_ROW + BOX_ROWS; r++)
        for (int c = 0; c < SCREEN_WIDTH; c++)
            set_tile(c, r, 0);
}
