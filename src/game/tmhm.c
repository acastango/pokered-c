/* tmhm.c — TM/HM teaching state machine.
 *
 * Mirrors ItemUseTMHM (engine/items/item_effects.asm:2154),
 * CanLearnTM (engine/items/tmhm.asm), and
 * LearnMove (engine/pokemon/learn_move.asm).
 */
#include "tmhm.h"
#include "yesno.h"
#include "text.h"
#include "party_menu.h"
#include "overworld.h"          /* gScrollTileMap, Map_BuildScrollView, Map_ReloadGfx */
#include "npc.h"                /* NPC_BuildView */
#include "constants.h"          /* PAD_*, MOVE_FLY, etc. */
#include "../data/tmhm_data.h"
#include "../data/base_stats.h" /* gBaseStats, gSpeciesToDex */
#include "../data/moves_data.h" /* gMoves, gMoveNames, NUM_MOVE_DEFS */
#include "../data/font_data.h"
#include "../platform/hardware.h"
#include "pokemon.h"            /* Pokemon_GetName */
#include "inventory.h"          /* Inventory_Remove, HM01, TM01 */
#include "player.h"             /* gScrollPxX, gScrollPxY */
#include "../platform/display.h"/* Display_SetPalette */
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* ---- HM move list (mirrors home/names.asm:IsMoveHM) ------------------- */
static const uint8_t kHMMoves[5] = {
    MOVE_CUT, MOVE_FLY, MOVE_SURF, MOVE_STRENGTH, MOVE_FLASH
};

static int is_hm_move(uint8_t move_id) {
    for (int i = 0; i < 5; i++)
        if (kHMMoves[i] == move_id) return 1;
    return 0;
}

/* ---- State machine ----------------------------------------------------- */
typedef enum {
    TS_IDLE = 0,
    TS_CONFIRM_WAIT,      /* YesNo: "Teach to a Pokemon?" */
    TS_PICK_MON,          /* party menu open */
    TS_CANT_LEARN,        /* text: can't learn */
    TS_ALREADY_KNOWS,     /* text: already knows */
    TS_TRY_FORGET_WAIT,   /* YesNo: "Forget a move?" */
    TS_PICK_MOVE,         /* move-forget cursor menu */
    TS_CANT_FORGET,       /* text: HM can't be forgotten */
    TS_SUCCESS,           /* text: "[Mon] learned [Move]!" */
    TS_DECLINED,          /* text: "Did not learn [Move]." */
} ts_t;

static ts_t   s_state       = TS_IDLE;
static uint8_t s_item_id    = 0;
static int     s_tmhm_idx   = 0;
static uint8_t s_move_id    = 0;
static int     s_party_slot = 0;
static int     s_move_cursor= 0;
static char    s_str[128];          /* scratch for snprintf */

/* ---- Compatibility / move helpers ------------------------------------- */

static int can_learn(int party_slot) {
    uint8_t species = wPartyMons[party_slot].base.species;
    uint8_t dex     = gSpeciesToDex[species];
    const uint8_t *bits = gBaseStats[dex].tmhm;
    return (bits[s_tmhm_idx >> 3] >> (s_tmhm_idx & 7)) & 1;
}

static int already_knows(int party_slot) {
    for (int i = 0; i < 4; i++)
        if (wPartyMons[party_slot].base.moves[i] == s_move_id) return 1;
    return 0;
}

static int find_empty_slot(int party_slot) {
    for (int i = 0; i < 4; i++)
        if (wPartyMons[party_slot].base.moves[i] == 0) return i;
    return -1;
}

static void install_move(int party_slot, int move_slot) {
    wPartyMons[party_slot].base.moves[move_slot] = s_move_id;
    wPartyMons[party_slot].base.pp[move_slot]    = gMoves[s_move_id].pp;
}

static const char *mon_name(int party_slot) {
    uint8_t species = wPartyMons[party_slot].base.species;
    return Pokemon_GetName(gSpeciesToDex[species]);
}

static const char *move_name(void) {
    if (s_move_id < NUM_MOVE_DEFS && gMoveNames[s_move_id])
        return gMoveNames[s_move_id];
    return "???";
}

/* ---- Restore overworld after party menu ------------------------------- */
static void restore_overworld(void) {
    Display_SetPalette(0xE4, 0xD0, 0xE0);
    Map_ReloadGfx();
    Font_Load();
    Map_BuildScrollView();
    NPC_BuildView(gScrollPxX, gScrollPxY);
}

/* ---- Move menu (TS_PICK_MOVE) ----------------------------------------- */
#define MOVE_MENU_ROW   12   /* first row of text box area */
#define MOVE_MENU_COLS  20

/* Pokemon encoding helpers */
static int poke_char(unsigned char c) {
    if (c >= 'A' && c <= 'Z') return Font_CharToTile((unsigned char)(0x80 + (c - 'A')));
    if (c >= 'a' && c <= 'z') return Font_CharToTile((unsigned char)(0xA0 + (c - 'a')));
    if (c >= '0' && c <= '9') return Font_CharToTile((unsigned char)(0xF6 + (c - '0')));
    if (c == '-') return Font_CharToTile(0xE3);
    if (c == '.') return Font_CharToTile(0xE8);
    if (c == '!') return Font_CharToTile(0xE7);
    if (c == '?') return Font_CharToTile(0xE6);
    if (c == ' ') return Font_CharToTile(0x7F);
    return Font_CharToTile(0x7F);
}

#define SCROLL_MAP_STRIDE  SCROLL_MAP_W
#define TMIDX(r, c)  (((r) + 2) * SCROLL_MAP_STRIDE + ((c) + 2))

static void set_tile(int row, int col, int tile) {
    gScrollTileMap[TMIDX(row, col)] = (uint8_t)tile;
}

static void write_str(int row, int col, const char *s) {
    for (int i = 0; s[i] && col + i < MOVE_MENU_COLS; i++)
        set_tile(row, col + i, poke_char((unsigned char)s[i]));
}

static void draw_move_menu(void) {
    /* Clear rows 12-17 */
    for (int r = MOVE_MENU_ROW; r < MOVE_MENU_ROW + 6; r++)
        for (int c = 0; c < MOVE_MENU_COLS; c++)
            set_tile(r, c, Font_CharToTile(0x7F));

    /* Header */
    write_str(MOVE_MENU_ROW, 0, "FORGET WHICH MOVE?");

    /* Four moves */
    for (int i = 0; i < 4; i++) {
        int row = MOVE_MENU_ROW + 1 + i;
        uint8_t mid = wPartyMons[s_party_slot].base.moves[i];
        const char *mname = (mid > 0 && mid < NUM_MOVE_DEFS && gMoveNames[mid])
                            ? gMoveNames[mid] : "---";

        /* Cursor */
        set_tile(row, 0,
                 (i == s_move_cursor) ? Font_CharToTile(0xED)  /* ► */
                                      : Font_CharToTile(0x7F));
        /* Move name */
        write_str(row, 1, mname);

        /* PP: right-aligned at col 15 */
        if (mid > 0 && mid < NUM_MOVE_DEFS) {
            uint8_t pp = wPartyMons[s_party_slot].base.pp[i] & 0x3Fu;
            char pp_str[8];
            snprintf(pp_str, sizeof(pp_str), "%2d", (int)pp);
            write_str(row, 16, pp_str);
        }
    }
}

/* ---- Transition helpers ----------------------------------------------- */

static void open_confirm(void) {
    /* Format: "TM24\nTHUNDERBOLT!\nTeach to a\nPOKEMON?" */
    char item_name[6];
    if (s_item_id >= TM01)
        snprintf(item_name, sizeof(item_name), "TM%02d", s_item_id - TM01 + 1);
    else
        snprintf(item_name, sizeof(item_name), "HM%02d", s_item_id - HM01 + 1);
    snprintf(s_str, sizeof(s_str), "%s\n%s!\nTeach to a\nPOKEMON?", item_name, move_name());
    YesNo_Show(s_str);
    s_state = TS_CONFIRM_WAIT;
}

static void validate_mon(int slot);  /* forward decl */

static void reopen_party(void) {
    PartyMenu_Open(PARTY_MENU_TMHM);
    s_state = TS_PICK_MON;
}

static void validate_mon(int slot) {
    s_party_slot = slot;
    if (!can_learn(slot)) {
        snprintf(s_str, sizeof(s_str), "%s can't\nlearn %s!", mon_name(slot), move_name());
        Text_ShowASCII(s_str);
        s_state = TS_CANT_LEARN;
        return;
    }
    if (already_knows(slot)) {
        snprintf(s_str, sizeof(s_str), "%s already\nknows %s!", mon_name(slot), move_name());
        Text_ShowASCII(s_str);
        s_state = TS_ALREADY_KNOWS;
        return;
    }
    int empty = find_empty_slot(slot);
    if (empty >= 0) {
        install_move(slot, empty);
        if (s_item_id >= TM01) Inventory_Remove(s_item_id, 1);
        snprintf(s_str, sizeof(s_str), "%s learned\n%s!", mon_name(slot), move_name());
        Text_ShowASCII(s_str);
        s_state = TS_SUCCESS;
        return;
    }
    /* 4 moves full — ask to forget */
    snprintf(s_str, sizeof(s_str),
             "%s tried to\nlearn %s.\f"
             "But %s\ncan't learn\nmore moves!\f"
             "Forget a\nmove?",
             mon_name(slot), move_name(), mon_name(slot));
    YesNo_Show(s_str);
    s_state = TS_TRY_FORGET_WAIT;
}

/* ---- Public API ------------------------------------------------------- */

void TMHM_Use(uint8_t item_id) {
    s_item_id   = item_id;
    s_tmhm_idx  = (item_id >= TM01) ? (item_id - TM01) : (item_id - HM01 + 50);
    s_move_id   = gTMHMMoves[s_tmhm_idx];
    s_move_cursor = 0;
    open_confirm();
}

int TMHM_IsActive(void) { return s_state != TS_IDLE; }

void TMHM_PostRender(void) {
    switch (s_state) {
        case TS_CONFIRM_WAIT:
        case TS_TRY_FORGET_WAIT:
            YesNo_PostRender();
            break;
        case TS_PICK_MOVE:
        case TS_CANT_FORGET:
            draw_move_menu();
            break;
        default:
            break;
    }
}

void TMHM_Tick(void) {
    switch (s_state) {
        case TS_IDLE:
            break;

        case TS_CONFIRM_WAIT:
            YesNo_Tick();
            if (!YesNo_IsOpen()) {
                if (YesNo_GetResult()) {
                    reopen_party();
                } else {
                    snprintf(s_str, sizeof(s_str), "Did not learn\n%s.", move_name());
                    Text_ShowASCII(s_str);
                    s_state = TS_DECLINED;
                }
            }
            break;

        case TS_PICK_MON:
            PartyMenu_Tick();
            if (!PartyMenu_IsOpen()) {
                restore_overworld();
                int slot = PartyMenu_GetSelected();
                if (slot < 0) {
                    s_state = TS_IDLE;
                } else {
                    validate_mon(slot);
                }
            }
            break;

        case TS_CANT_LEARN:
        case TS_ALREADY_KNOWS:
            /* Text handled by game.c (lines 446-456); by the time TMHM_Tick
             * runs, text has closed.  Reopen the party menu. */
            reopen_party();
            break;

        case TS_TRY_FORGET_WAIT:
            YesNo_Tick();
            if (!YesNo_IsOpen()) {
                if (YesNo_GetResult()) {
                    s_move_cursor = 0;
                    s_state = TS_PICK_MOVE;
                } else {
                    snprintf(s_str, sizeof(s_str), "Did not learn\n%s.", move_name());
                    Text_ShowASCII(s_str);
                    s_state = TS_DECLINED;
                }
            }
            break;

        case TS_PICK_MOVE:
            if (hJoyPressed & PAD_UP) {
                if (s_move_cursor > 0) s_move_cursor--;
            } else if (hJoyPressed & PAD_DOWN) {
                /* Skip empty slots */
                if (s_move_cursor < 3 &&
                    wPartyMons[s_party_slot].base.moves[s_move_cursor + 1] != 0)
                    s_move_cursor++;
            } else if (hJoyPressed & PAD_B) {
                /* Cancel — go back to forget question */
                snprintf(s_str, sizeof(s_str), "Did not learn\n%s.", move_name());
                Text_ShowASCII(s_str);
                s_state = TS_DECLINED;
            } else if (hJoyPressed & PAD_A) {
                uint8_t old_move = wPartyMons[s_party_slot].base.moves[s_move_cursor];
                if (is_hm_move(old_move)) {
                    Text_ShowASCII("HM moves can't\nbe forgotten!");
                    s_state = TS_CANT_FORGET;
                } else {
                    install_move(s_party_slot, s_move_cursor);
                    if (s_item_id >= TM01) Inventory_Remove(s_item_id, 1);
                    snprintf(s_str, sizeof(s_str), "%s learned\n%s!", mon_name(s_party_slot), move_name());
                    Text_ShowASCII(s_str);
                    s_state = TS_SUCCESS;
                }
            }
            break;

        case TS_CANT_FORGET:
            /* Text closed — return to move menu */
            s_state = TS_PICK_MOVE;
            break;

        case TS_SUCCESS:
        case TS_DECLINED:
            /* Text closed — teaching done */
            s_state = TS_IDLE;
            break;
    }
}
