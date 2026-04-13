/* menu.c — START menu (overworld pause menu).
 *
 * Mirrors home/start_menu.asm + engine/menus/draw_start_menu.asm.
 *
 * Box geometry (no Pokédex — 6 items):
 *   hlcoord 10, 0 / b=$0c / c=$08  →  cols 10-18, rows 0-13
 *   Items at col 12, rows 2,4,6,8,10,12.  Arrow cursor at col 11.
 *
 * Rendering:
 *   Tiles are written directly into gScrollTileMap at the matching buffer
 *   offset (screen col C → buffer col C+2, row R → buffer row R+2).
 *   This works because the menu only opens while the player is idle
 *   (gScrollPxX = gScrollPxY = 0), so the 2-tile margin maps 1:1 to screen.
 *   On close, Map_BuildScrollView() + NPC_BuildView() rebuild the buffer.
 *
 * Items (no Pokédex):
 *   0  POKéMON  → party_menu
 *   1  ITEM     → stub
 *   2  [player] → stub (trainer info)
 *   3  SAVE     → Save_Write()
 *   4  OPTION   → stub
 *   5  EXIT     → close
 */
#include "menu.h"
#include "bag_menu.h"
#include "party_menu.h"
#include "pokedex.h"
#include "overworld.h"       /* gScrollTileMap, SCROLL_MAP_W, Map_BuildScrollView */
#include "npc.h"             /* NPC_BuildView */
#include "../data/font_data.h"
#include "../data/event_constants.h"
#include "../platform/hardware.h"
#include "../platform/audio.h"
#include "../platform/save.h"
#include <string.h>
#include <stdio.h>

/* ---- Box / item geometry (mirrors draw_start_menu.asm) ----------- */
#define BOX_COL_L      10   /* left border column */
#define BOX_COL_R      19   /* right border column */
#define BOX_ROW_T       0   /* top border row */
/* BOX_ROW_B is 13 without Pokédex (6 items), 15 with (7 items) */
#define ITEM_COL       12   /* text start column */
#define ARROW_COL      11   /* ▶ cursor column */
#define ITEM_ROW_FIRST  2   /* row of first item */
#define ITEM_ROW_STEP   2   /* rows between items */
#define NUM_ITEMS_NO_DEX  6
#define NUM_ITEMS_DEX     7

/* Pokéred char codes */
#define CHAR_TERM  0x50   /* '@' string terminator */
#define CHAR_SPACE 0x7F   /* space */
#define CHAR_ARROW 0xEC   /* ▶ menu cursor */

/* ---- Menu item strings (pokéred-encoded) -------------------------- */
/* Character encoding: uppercase A=$80…Z=$99, é=$E9, digits $F6… */
static const uint8_t kStrPokedex[] = {0x8F,0x8E,0x8A,0xBA,0x83,0x84,0x97,CHAR_TERM}; /* POKéDEX */
static const uint8_t kStrPokemon[] = {0x8F,0x8E,0x8A,0xBA,0x8C,0x8E,0x8D,CHAR_TERM};
static const uint8_t kStrItem[]    = {0x88,0x93,0x84,0x8C,CHAR_TERM};
static const uint8_t kStrSave[]    = {0x92,0x80,0x95,0x84,CHAR_TERM};
static const uint8_t kStrOption[]  = {0x8E,0x8F,0x93,0x88,0x8E,0x8D,CHAR_TERM};
static const uint8_t kStrExit[]    = {0x84,0x97,0x88,0x93,CHAR_TERM};

/* ---- State -------------------------------------------------------- */
static int gMenuOpen     = 0;
static int gMenuCursor   = 0;  /* 0-based index */
static int gMenuNumItems = NUM_ITEMS_NO_DEX;
static int gMenuHasDex   = 0;

/* ---- Scroll-buffer tile writer ----------------------------------- */
/* Screen position (col, row) → gScrollTileMap buffer at +2 offset.
 * Valid only when gScrollPxX = gScrollPxY = 0 (player idle). */
static void smset(int col, int row, uint8_t tile) {
    gScrollTileMap[(row + 2) * SCROLL_MAP_W + (col + 2)] = tile;
}

/* ---- Drawing helpers --------------------------------------------- */
static void draw_box(void) {
    int L = BOX_COL_L, R = BOX_COL_R;
    int T = BOX_ROW_T;
    int B = ITEM_ROW_FIRST + (gMenuNumItems - 1) * ITEM_ROW_STEP + 1;

    /* top border */
    smset(L, T, (uint8_t)Font_CharToTile(0x79));   /* ┌ */
    for (int c = L + 1; c < R; c++)
        smset(c, T, (uint8_t)Font_CharToTile(0x7A)); /* ─ */
    smset(R, T, (uint8_t)Font_CharToTile(0x7B));   /* ┐ */

    /* interior */
    for (int r = T + 1; r < B; r++) {
        smset(L, r, (uint8_t)Font_CharToTile(0x7C));  /* │ */
        for (int c = L + 1; c < R; c++)
            smset(c, r, (uint8_t)Font_CharToTile(CHAR_SPACE));
        smset(R, r, (uint8_t)Font_CharToTile(0x7C));  /* │ */
    }

    /* bottom border */
    smset(L, B, (uint8_t)Font_CharToTile(0x7D));   /* └ */
    for (int c = L + 1; c < R; c++)
        smset(c, B, (uint8_t)Font_CharToTile(0x7A)); /* ─ */
    smset(R, B, (uint8_t)Font_CharToTile(0x7E));   /* ┘ */
}

static void print_str(int col, int row, const uint8_t *s) {
    for (; *s != CHAR_TERM; s++, col++)
        smset(col, row, (uint8_t)Font_CharToTile(*s));
}

/* Print pokéred-encoded wPlayerName (terminated by CHAR_TERM or NUL). */
static void print_player_name(int col, int row) {
    for (int i = 0; i < NAME_LENGTH; i++) {
        uint8_t b = wPlayerName[i];
        if (b == 0x00 || b == CHAR_TERM) break;
        smset(col++, row, (uint8_t)Font_CharToTile(b));
    }
}

static void draw_items(void) {
    int row = ITEM_ROW_FIRST;
    if (gMenuHasDex) {
        print_str(ITEM_COL, row, kStrPokedex); row += ITEM_ROW_STEP;
    }
    print_str(ITEM_COL, row, kStrPokemon); row += ITEM_ROW_STEP;
    print_str(ITEM_COL, row, kStrItem);    row += ITEM_ROW_STEP;
    print_player_name(ITEM_COL, row);      row += ITEM_ROW_STEP;
    print_str(ITEM_COL, row, kStrSave);    row += ITEM_ROW_STEP;
    print_str(ITEM_COL, row, kStrOption);  row += ITEM_ROW_STEP;
    print_str(ITEM_COL, row, kStrExit);
}

static void set_cursor(int item, uint8_t tile) {
    smset(ARROW_COL, ITEM_ROW_FIRST + item * ITEM_ROW_STEP, tile);
}

/* ---- Public API -------------------------------------------------- */
void Menu_Open(void) {
    gMenuHasDex  = CheckEvent(EVENT_GOT_POKEDEX) ? 1 : 0;
    gMenuNumItems = gMenuHasDex ? NUM_ITEMS_DEX : NUM_ITEMS_NO_DEX;
    gMenuOpen    = 1;
    gMenuCursor  = 0;
    /* Hide all sprites by zeroing Y only — tile/flags must survive intact
     * so NPC_BuildView() can restore them without needing to re-set tiles. */
    for (int i = 0; i < MAX_SPRITES; i++) wShadowOAM[i].y = 0;
    draw_box();
    draw_items();
    set_cursor(0, (uint8_t)Font_CharToTile(CHAR_ARROW));
}

int Menu_IsOpen(void) {
    return gMenuOpen;
}

static void menu_close(void) {
    gMenuOpen = 0;
    Map_BuildScrollView();
    NPC_BuildView(0, 0);
}

void Menu_Tick(void) {
    /* UP / DOWN: move cursor, wraps */
    if (hJoyPressed & PAD_UP) {
        set_cursor(gMenuCursor, (uint8_t)Font_CharToTile(CHAR_SPACE));
        gMenuCursor = (gMenuCursor == 0) ? gMenuNumItems - 1 : gMenuCursor - 1;
        set_cursor(gMenuCursor, (uint8_t)Font_CharToTile(CHAR_ARROW));
        return;
    }
    if (hJoyPressed & PAD_DOWN) {
        set_cursor(gMenuCursor, (uint8_t)Font_CharToTile(CHAR_SPACE));
        gMenuCursor = (gMenuCursor == gMenuNumItems - 1) ? 0 : gMenuCursor + 1;
        set_cursor(gMenuCursor, (uint8_t)Font_CharToTile(CHAR_ARROW));
        return;
    }

    /* B or START: close */
    if (hJoyPressed & (PAD_B | PAD_START)) {
        Audio_PlaySFX_PressAB();
        menu_close();
        return;
    }

    /* A: dispatch — item indices shift by 1 when Pokédex is present */
    if (hJoyPressed & PAD_A) {
        Audio_PlaySFX_PressAB();
        int idx = gMenuCursor;
        /* Normalize: if dex present, item 0 = POKÉDEX; shift rest down */
        if (gMenuHasDex) {
            if (idx == 0) {
                menu_close();
                Pokedex_Open();
                return;
            }
            idx--; /* now idx 0=POKéMON,1=ITEM,2=player,3=SAVE,4=OPTION,5=EXIT */
        }
        switch (idx) {
            case 0:  /* POKéMON → party menu */
                menu_close();
                NPC_HideOAM();  /* NPC OAM points at slots PartyIcons_LoadTiles() will overwrite */
                PartyMenu_Open(0 /* cancelable */);
                return;
            case 1:  /* ITEM → open bag menu */
                menu_close();
                BagMenu_Open();
                return;
            case 2:  /* [player] — stub */
            case 4:  /* OPTION   — stub */
                break;
            case 3:  /* SAVE */
                if (Save_Write() == 0)
                    printf("[menu] Game saved.\n");
                menu_close();
                break;
            case 5:  /* EXIT */
                menu_close();
                break;
        }
    }
}
