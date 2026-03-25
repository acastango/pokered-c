/* party_menu.c — Gen 1 party menu.
 *
 * Ports DisplayPartyMenu / DrawPartyMenu_ / HandlePartyMenuInput
 * (home/pokemon.asm, engine/menus/party_menu.asm).
 *
 * Tile layout per slot N (matching DrawHPBar + PrintLevel + PrintStatusCondition):
 *   Name row (row N*2):
 *     col  3-12: GB-encoded nickname (10 chars)
 *     col 13-15: level (<LV>=$6E tile + 2 digits, or 3 digits at Lv100)
 *     col 17-19: status condition ("FNT","SLP","PSN","BRN","FRZ","PAR") or blank
 *   HP row (row N*2+1):
 *     col  0:    cursor (▶=$ED) or blank
 *     col  4:    HP label ($71)
 *     col  5:    left bar cap ($62)
 *     col  6-11: 6 HP bar tiles ($63=empty .. $6B=full)
 *     col 12:    right bar cap ($6C, party-menu type: $6D-1)
 *     col 13-15: current HP (right-aligned 3 digits)
 *     col 16:    '/' ($F3)
 *     col 17-19: max HP (right-aligned 3 digits)
 *   Message box: rows 12-17 (box border + "Choose a / POKeMON.")
 */
#include "party_menu.h"
#include "summary_screen.h"
#include "../platform/hardware.h"
#include "../platform/display.h"
#include "../platform/audio.h"
#include "../data/font_data.h"
#include "../data/base_stats.h"
#include "../data/party_icon_data.h"
#include "constants.h"
#include "overworld.h"   /* gScrollTileMap, SCROLL_MAP_W */
#include "pokemon.h"
#include <stdio.h>
#include <string.h>

/* ---- WRAM externs ----------------------------------------------------- */
extern uint8_t     wPartyCount;
extern party_mon_t wPartyMons[PARTY_LENGTH];
extern uint8_t     wPartyMonNicks[PARTY_LENGTH][NAME_LENGTH];
extern uint8_t     wWhichPokemon;

/* ---- State ------------------------------------------------------------ */
static int g_open        = 0;
static int g_force       = 0;
static int g_cursor      = 0;
static int g_selected    = -1;
static int g_anim_tick   = 0;
static int g_anim_frame  = 0;
static int g_submenu     = 0;   /* 1 while STATS/CANCEL popup is showing */
static int g_sub_cursor  = 0;   /* 0=STATS  1=CANCEL */
static int g_in_summary  = 0;   /* 1 while SummaryScreen is showing */

/* OAM slots 0..(PARTY_LENGTH*4-1) are owned by the party menu while open.
 * Each slot uses 4 OAM entries: top-left, top-right, bottom-left, bottom-right.
 * Matches WriteMonPartySpriteOAM (engine/items/town_map.asm). */
#define PM_OAM_BASE  0                /* first OAM entry used by party icons */
#define PM_ANIM_RATE 16               /* frames per animation step (normal HP) */

/* ---- Icon OAM helpers ------------------------------------------------- */

/* Get the ICON_* type for a party slot (via dex lookup).
 * Returns ICON_MON (0) as safe default for unknown/invalid species. */
static uint8_t pm_icon_type(int slot) {
    if (slot >= (int)wPartyCount) return ICON_MON;
    uint8_t species = wPartyMons[slot].base.species;
    uint8_t dex = gSpeciesToDex[species];
    if (dex == 0 || dex > 151) return ICON_MON;
    return gMonPartyIconType[dex];
}

/* Write 4 OAM entries for one party slot.
 * frame=0: base tiles (ICON_TYPE<<2); frame=1: +ICON_ICONOFFSET.
 * Matches WriteSymmetricMonPartySpriteOAM / WriteAsymmetricMonPartySpriteOAM. */
static void pm_write_slot_oam(int slot, int frame) {
    int base_oam = PM_OAM_BASE + slot * 4;
    uint8_t icon = pm_icon_type(slot);
    uint8_t tile_base = (uint8_t)(icon << 2);
    if (frame) tile_base = (uint8_t)(tile_base + ICON_ICONOFFSET);

    /* GB OAM coordinates: Y = screen_y + OAM_Y_OFS, X = screen_x + OAM_X_OFS.
     * Icon column 1 (8px from left), row starts at slot*16. */
    uint8_t oam_y = (uint8_t)(slot * 16 + OAM_Y_OFS);      /* top row Y */
    uint8_t oam_x = (uint8_t)(1 * 8 + OAM_X_OFS);          /* col 1 = 8px */

    if (slot >= (int)wPartyCount) {
        /* Empty slot — hide all 4 sprites */
        for (int i = 0; i < 4; i++) {
            wShadowOAM[base_oam + i].y = 0;
            wShadowOAM[base_oam + i].x = 0;
            wShadowOAM[base_oam + i].tile = 0;
            wShadowOAM[base_oam + i].flags = 0;
        }
        return;
    }

    if (icon == ICON_HELIX) {
        /* Asymmetric: 4 unique tiles, no X-flip (WriteAsymmetricMonPartySpriteOAM) */
        wShadowOAM[base_oam+0] = (oam_entry_t){ oam_y,   oam_x,   tile_base+0, 0 };
        wShadowOAM[base_oam+1] = (oam_entry_t){ oam_y,   oam_x+8, tile_base+1, 0 };
        wShadowOAM[base_oam+2] = (oam_entry_t){ oam_y+8, oam_x,   tile_base+2, 0 };
        wShadowOAM[base_oam+3] = (oam_entry_t){ oam_y+8, oam_x+8, tile_base+3, 0 };
    } else {
        /* Symmetric: 2 unique tiles, right column X-flipped (WriteSymmetricMonPartySpriteOAM) */
        wShadowOAM[base_oam+0] = (oam_entry_t){ oam_y,   oam_x,   tile_base+0, 0 };
        wShadowOAM[base_oam+1] = (oam_entry_t){ oam_y,   oam_x+8, tile_base+0, OAM_FLAG_FLIP_X };
        wShadowOAM[base_oam+2] = (oam_entry_t){ oam_y+8, oam_x,   tile_base+2, 0 };
        wShadowOAM[base_oam+3] = (oam_entry_t){ oam_y+8, oam_x+8, tile_base+2, OAM_FLAG_FLIP_X };
    }
}

static void pm_write_all_oam(int frame) {
    for (int i = 0; i < PARTY_LENGTH; i++)
        pm_write_slot_oam(i, frame);
}

static void pm_clear_icon_oam(void) {
    for (int i = PM_OAM_BASE; i < PM_OAM_BASE + PARTY_LENGTH * 4; i++) {
        wShadowOAM[i].y = 0;
        wShadowOAM[i].x = 0;
        wShadowOAM[i].tile = 0;
        wShadowOAM[i].flags = 0;
    }
}

/* ---- Tile helpers ----------------------------------------------------- */

/* Screen tile (col, row) → gScrollTileMap with the +2 border offset.
 * Matches bui_set_tile / menu.c approach: the scroll map is 24×22 with a
 * 2-tile border; at scroll offset (0,0) position (2+col, 2+row) maps 1:1
 * to screen tile (col, row). */
static void pm_put(int col, int row, int tile_idx) {
    if ((unsigned)col >= SCREEN_WIDTH || (unsigned)row >= SCREEN_HEIGHT) return;
    gScrollTileMap[(row + 2) * SCROLL_MAP_W + (col + 2)] = (uint8_t)tile_idx;
}

static void pm_gb(int col, int row, uint8_t gb_char) {
    pm_put(col, row, Font_CharToTile(gb_char));
}

/* ASCII → tile (uppercase/lowercase/digits/punctuation) */
static int pm_ascii_tile(unsigned char c) {
    if (c >= 'A' && c <= 'Z') return Font_CharToTile(0x80 + (c - 'A'));
    if (c >= 'a' && c <= 'z') return Font_CharToTile(0xA0 + (c - 'a'));
    if (c >= '0' && c <= '9') return Font_CharToTile(0xF6 + (c - '0'));
    if (c == ' ')              return BLANK_TILE_SLOT;
    if (c == '.')              return Font_CharToTile(0xE8);
    if (c == '!')              return Font_CharToTile(0xE7);
    if (c == '?')              return Font_CharToTile(0xE6);
    if (c == ',')              return Font_CharToTile(0xF4);
    if (c == '-')              return Font_CharToTile(0xE3);
    if (c == '\'')             return Font_CharToTile(0xE0);
    if (c == '/')              return Font_CharToTile(0xF3);
    if (c == '>')              return Font_CharToTile(0xED);  /* ▶ cursor */
    return BLANK_TILE_SLOT;
}

static void pm_ascii(int col, int row, const char *s) {
    for (; *s; s++, col++)
        pm_put(col, row, pm_ascii_tile((unsigned char)*s));
}

/* Right-aligned 3-digit decimal number (0-999) */
static void pm_num3(int col, int row, int val) {
    char buf[4];
    if (val < 0)   val = 0;
    if (val > 999) val = 999;
    snprintf(buf, sizeof(buf), "%3d", val);
    for (int i = 0; i < 3; i++) {
        char c = buf[i];
        pm_put(col + i, row,
               c == ' ' ? BLANK_TILE_SLOT
                        : Font_CharToTile(0xF6 + (c - '0')));
    }
}

/* ---- HP bar ----------------------------------------------------------- */

/* Pixel fill count (0-48) from current / max HP.
 * Matches HPBarLength predef: hp*48/max_hp, minimum 1 if alive (c!=0). */
static int pm_hp_pixels(uint16_t hp, uint16_t max_hp) {
    if (!max_hp || !hp) return 0;
    int px = (int)hp * 48 / (int)max_hp;
    return px < 1 ? 1 : (px > 48 ? 48 : px);
}

/* 6 bar tiles at (col..col+5, row).  Matches DrawHPBar inner loop:
 *   seg>=8  → $6B (full tile)
 *   0<seg<8 → $63+seg (partial)
 *   seg<=0  → $63 (empty) */
static void pm_draw_bar_tiles(int col, int row, int pixels) {
    for (int i = 0; i < 6; i++) {
        int     seg  = pixels - i * 8;
        uint8_t tile = (seg <= 0) ? 0x63u
                     : (seg >= 8) ? 0x6Bu
                     :              (uint8_t)(0x63u + seg);
        pm_gb(col + i, row, tile);
    }
}

/* Full HP bar row starting at col 4 of hp_row.
 * Matches DrawHPBar (wHPBarType=2) + DrawHP2 fraction placement. */
static void pm_draw_hp_bar(int hp_row, uint16_t hp, uint16_t max_hp) {
    pm_gb(4,  hp_row, 0x71);  /* HP label                        */
    pm_gb(5,  hp_row, 0x62);  /* left bar cap                    */
    pm_draw_bar_tiles(6, hp_row, pm_hp_pixels(hp, max_hp));
    pm_gb(12, hp_row, 0x6C);  /* right cap (party menu type = $6D-1) */
    /* HP fraction at col 13: DrawHP2 adds $9 to bar start (col 4+9=13) */
    pm_num3(13, hp_row, (int)hp);
    pm_gb(16, hp_row, 0xF3);  /* '/' */
    pm_num3(17, hp_row, (int)max_hp);
}

/* ---- Status condition ------------------------------------------------- */

/* GB-encoded 3-letter status strings (PrintStatusCondition / PrintStatusAilment) */
static const uint8_t k_fnt[3] = {0x85, 0x8D, 0x93};  /* FNT */
static const uint8_t k_slp[3] = {0x92, 0x8B, 0x8F};  /* SLP */
static const uint8_t k_psn[3] = {0x8F, 0x92, 0x8D};  /* PSN */
static const uint8_t k_brn[3] = {0x81, 0x91, 0x8D};  /* BRN */
static const uint8_t k_frz[3] = {0x85, 0x91, 0x99};  /* FRZ */
static const uint8_t k_par[3] = {0x8F, 0x80, 0x91};  /* PAR */

static void pm_draw_status(int col, int row, uint8_t status, uint16_t hp) {
    const uint8_t *s = NULL;
    if (!hp)               s = k_fnt;           /* fainted             */
    else if (status & 0x07) s = k_slp;           /* sleep (bits 0-2)    */
    else if (status & (1<<3)) s = k_psn;         /* PSN                 */
    else if (status & (1<<4)) s = k_brn;         /* BRN                 */
    else if (status & (1<<5)) s = k_frz;         /* FRZ                 */
    else if (status & (1<<6)) s = k_par;         /* PAR                 */

    for (int i = 0; i < 3; i++) {
        if (s) pm_gb(col + i, row, s[i]);
        else   pm_put(col + i, row, BLANK_TILE_SLOT);
    }
}

/* ---- Level display ---------------------------------------------------- */

/* Matches PrintLevel: <LV> tile ($6E) + 2 left-aligned digits;
 * at Lv100+ overwrites the <LV> tile with 3 digits. */
static void pm_draw_level(int col, int row, uint8_t level) {
    if (level >= 100) {
        pm_put(col,   row, Font_CharToTile(0xF6 + (level / 100)));
        pm_put(col+1, row, Font_CharToTile(0xF6 + ((level / 10) % 10)));
        pm_put(col+2, row, Font_CharToTile(0xF6 + (level % 10)));
    } else if (level >= 10) {
        pm_gb(col,    row, 0x6E);                                    /* <LV> */
        pm_put(col+1, row, Font_CharToTile(0xF6 + (level / 10)));   /* tens  */
        pm_put(col+2, row, Font_CharToTile(0xF6 + (level % 10)));   /* units */
    } else {
        pm_gb(col,    row, 0x6E);                                    /* <LV> */
        pm_put(col+1, row, Font_CharToTile(0xF6 + level));           /* digit */
        pm_put(col+2, row, BLANK_TILE_SLOT);                         /* LEFT_ALIGN pad */
    }
}

/* ---- Nickname display ------------------------------------------------- */

/* Render GB-encoded nickname (10 chars, $50=terminator).
 * Falls back to ASCII species name if nickname is unset (first byte == 0). */
static void pm_draw_nick(int col, int row, int slot) {
    const uint8_t *nick = wPartyMonNicks[slot];
    if (nick[0] != 0x00) {
        /* GB-encoded nickname */
        for (int i = 0; i < 10; i++) {
            uint8_t ch = nick[i];
            if (ch == 0x50) {
                for (int j = i; j < 10; j++) pm_put(col + j, row, BLANK_TILE_SLOT);
                return;
            }
            pm_put(col + i, row, Font_CharToTile(ch));
        }
    } else {
        /* Nickname not set — display species name in ASCII */
        uint8_t     dex  = gSpeciesToDex[wPartyMons[slot].base.species];
        const char *name = Pokemon_GetName(dex);
        int         len  = (int)strlen(name);
        for (int i = 0; i < 10; i++) {
            if (i < len) pm_put(col + i, row, pm_ascii_tile((unsigned char)name[i]));
            else         pm_put(col + i, row, BLANK_TILE_SLOT);
        }
    }
}

/* ---- Message box ------------------------------------------------------ */

/* Draw the standard dialog box at rows 12-17 with a two-line message.
 * Matches DrawTextBox / TextBoxBorder tile IDs ($79-$7E). */
static void pm_draw_msg_box(const char *line1, const char *line2) {
    pm_gb(0,              12, 0x79);   /* ┌ top-left  */
    pm_gb(SCREEN_WIDTH-1, 12, 0x7B);  /* ┐ top-right */
    pm_gb(0,              17, 0x7D);  /* └ bot-left  */
    pm_gb(SCREEN_WIDTH-1, 17, 0x7E);  /* ┘ bot-right */
    for (int c = 1; c < SCREEN_WIDTH-1; c++) {
        pm_gb(c, 12, 0x7A);           /* ─ top edge  */
        pm_gb(c, 17, 0x7A);           /* ─ bot edge  */
    }
    for (int r = 13; r <= 16; r++) {
        pm_gb(0,              r, 0x7C);  /* │ left  */
        pm_gb(SCREEN_WIDTH-1, r, 0x7C);  /* │ right */
        for (int c = 1; c < SCREEN_WIDTH-1; c++)
            pm_put(c, r, BLANK_TILE_SLOT);
    }
    if (line1) pm_ascii(1, 13, line1);
    if (line2) pm_ascii(1, 15, line2);
}

/* ---- Sub-menu --------------------------------------------------------- */

/* Draw or redraw the small popup over the message box area.
 *
 * force=0 (overworld): 2 items — STATS / CANCEL
 *   Box: cols 12-19, rows 12-15
 *
 * force=2 (battle voluntary switch): 3 items — SWITCH / STATS / CANCEL
 *   Box: cols 12-19, rows 12-16
 */
static void pm_draw_submenu(int sub_cursor) {
    if (g_force == 2) {
        /* 3-item box: rows 12-16 */
        pm_gb(12, 12, 0x79);
        for (int c = 13; c <= 18; c++) pm_gb(c, 12, 0x7A);
        pm_gb(19, 12, 0x7B);
        for (int r = 13; r <= 15; r++) {
            pm_gb(12, r, 0x7C);
            for (int c = 13; c <= 18; c++) pm_put(c, r, BLANK_TILE_SLOT);
            pm_gb(19, r, 0x7C);
        }
        pm_gb(12, 16, 0x7D);
        for (int c = 13; c <= 18; c++) pm_gb(c, 16, 0x7A);
        pm_gb(19, 16, 0x7E);

        pm_put(13, 13, sub_cursor == 0 ? Font_CharToTile(0xED) : BLANK_TILE_SLOT);
        pm_ascii(14, 13, "SWITCH");
        pm_put(13, 14, sub_cursor == 1 ? Font_CharToTile(0xED) : BLANK_TILE_SLOT);
        pm_ascii(14, 14, "STATS");
        pm_put(13, 15, sub_cursor == 2 ? Font_CharToTile(0xED) : BLANK_TILE_SLOT);
        pm_ascii(14, 15, "CANCEL");
    } else {
        /* 2-item box: rows 12-15 */
        pm_gb(12, 12, 0x79);
        for (int c = 13; c <= 18; c++) pm_gb(c, 12, 0x7A);
        pm_gb(19, 12, 0x7B);
        pm_gb(12, 13, 0x7C);  pm_gb(12, 14, 0x7C);
        pm_gb(19, 13, 0x7C);  pm_gb(19, 14, 0x7C);
        pm_gb(12, 15, 0x7D);
        for (int c = 13; c <= 18; c++) pm_gb(c, 15, 0x7A);
        pm_gb(19, 15, 0x7E);

        pm_put(13, 13, sub_cursor == 0 ? Font_CharToTile(0xED) : BLANK_TILE_SLOT);
        pm_ascii(14, 13, "STATS");
        pm_put(13, 14, sub_cursor == 1 ? Font_CharToTile(0xED) : BLANK_TILE_SLOT);
        pm_ascii(14, 14, "CANCEL");
    }
}

/* ---- Cursor ----------------------------------------------------------- */

/* Erase all 6 cursor positions (col 0 of each HP row). */
static void pm_erase_cursors(void) {
    for (int i = 0; i < PARTY_LENGTH; i++)
        pm_put(0, i * 2 + 1, BLANK_TILE_SLOT);
}

/* Place ▶ ($ED) at col 0 of the selected slot's HP row. */
static void pm_draw_cursor(int cursor) {
    pm_erase_cursors();
    if (cursor >= 0 && cursor < (int)wPartyCount)
        pm_put(0, cursor * 2 + 1, Font_CharToTile(0xED));
}

/* ---- Slot drawing ----------------------------------------------------- */

static void pm_draw_slot(int slot) {
    int name_row = slot * 2;
    int hp_row   = slot * 2 + 1;

    /* Clear both rows */
    for (int c = 0; c < SCREEN_WIDTH; c++) {
        pm_put(c, name_row, BLANK_TILE_SLOT);
        pm_put(c, hp_row,   BLANK_TILE_SLOT);
    }
    if (slot >= (int)wPartyCount) return;

    const party_mon_t *mon    = &wPartyMons[slot];
    uint16_t           hp     = mon->base.hp;
    uint16_t           max_hp = mon->max_hp;

    /* Name row: nickname | level | status */
    pm_draw_nick(3,  name_row, slot);
    pm_draw_level(13, name_row, mon->level);
    pm_draw_status(17, name_row, mon->base.status, hp);

    /* HP row: HP bar + fraction */
    pm_draw_hp_bar(hp_row, hp, max_hp);
}

/* ---- Full redraw ------------------------------------------------------ */

static void pm_draw_all(void) {
    /* Clear rows 0-11 (slot area) */
    for (int r = 0; r < 12; r++)
        for (int c = 0; c < SCREEN_WIDTH; c++)
            pm_put(c, r, BLANK_TILE_SLOT);

    for (int i = 0; i < PARTY_LENGTH; i++)
        pm_draw_slot(i);

    pm_draw_cursor(g_cursor);
    pm_draw_msg_box("Choose a", "POKeMON.");
}

/* ---- Public API ------------------------------------------------------- */

void PartyMenu_Open(int force) {
    Font_LoadHudTiles();  /* HP bar tiles ($62-$78) required for bar drawing */
    PartyIcons_LoadTiles();

    g_force      = force;
    g_open       = 1;
    g_selected   = -1;
    g_anim_tick  = 0;
    g_anim_frame = 0;
    g_submenu    = 0;
    g_sub_cursor = 0;
    g_in_summary = 0;

    /* Default to first non-fainted slot */
    g_cursor = 0;
    for (int i = 0; i < (int)wPartyCount; i++) {
        if (wPartyMons[i].base.hp > 0) {
            g_cursor = i;
            break;
        }
    }

    pm_draw_all();
    pm_write_all_oam(0);
}

int PartyMenu_IsOpen(void) {
    return g_open;
}

int PartyMenu_GetSelected(void) {
    return g_selected;
}

void PartyMenu_Tick(void) {
    if (!g_open) return;

    /* ---- Summary screen is showing: tick it and wait for it to close ---- */
    if (g_in_summary) {
        SummaryScreen_Tick();
        if (!SummaryScreen_IsOpen()) {
            g_in_summary = 0;
            /* Redraw the party menu underneath */
            Font_LoadHudTiles();
            PartyIcons_LoadTiles();
            pm_draw_all();
            pm_write_all_oam(g_anim_frame);
        }
        return;
    }

    /* ---- Action sub-menu is showing ---- */
    if (g_submenu) {
        int num_items = (g_force == 2) ? 3 : 2;
        if (hJoyPressed & PAD_UP) {
            g_sub_cursor = (g_sub_cursor - 1 + num_items) % num_items;
            pm_draw_submenu(g_sub_cursor);
            return;
        }
        if (hJoyPressed & PAD_DOWN) {
            g_sub_cursor = (g_sub_cursor + 1) % num_items;
            pm_draw_submenu(g_sub_cursor);
            return;
        }
        if (hJoyPressed & PAD_A) {
            Audio_PlaySFX_PressAB();
            g_submenu = 0;
            if (g_force == 2) {
                /* force=2 (battle voluntary): SWITCH(0) / STATS(1) / CANCEL(2) */
                if (g_sub_cursor == 0) {
                    /* SWITCH — confirm selection */
                    wWhichPokemon = (uint8_t)g_cursor;
                    g_selected    = g_cursor;
                    g_open        = 0;
                    pm_clear_icon_oam();
                } else if (g_sub_cursor == 1) {
                    /* STATS — open summary screen */
                    g_in_summary = 1;
                    SummaryScreen_Open(g_cursor);
                } else {
                    /* CANCEL — back to party list */
                    pm_draw_all();
                    pm_write_all_oam(g_anim_frame);
                }
            } else {
                /* force=0 (overworld): STATS(0) / CANCEL(1) */
                if (g_sub_cursor == 0) {
                    g_in_summary = 1;
                    SummaryScreen_Open(g_cursor);
                } else {
                    pm_draw_all();
                    pm_write_all_oam(g_anim_frame);
                }
            }
            return;
        }
        if (hJoyPressed & PAD_B) {
            Audio_PlaySFX_PressAB();
            g_submenu = 0;
            pm_draw_all();
            pm_write_all_oam(g_anim_frame);
            return;
        }
        return;
    }

    /* ---- Normal party menu ---- */

    /* Icon animation */
    if (++g_anim_tick >= PM_ANIM_RATE) {
        g_anim_tick  = 0;
        g_anim_frame ^= 1;
        pm_write_all_oam(g_anim_frame);
    }

    if (hJoyPressed & PAD_UP) {
        g_cursor = (g_cursor - 1 + (int)wPartyCount) % (int)wPartyCount;
        pm_draw_cursor(g_cursor);
        return;
    }
    if (hJoyPressed & PAD_DOWN) {
        g_cursor = (g_cursor + 1) % (int)wPartyCount;
        pm_draw_cursor(g_cursor);
        return;
    }
    if (hJoyPressed & PAD_A) {
        Audio_PlaySFX_PressAB();
        if (g_force == 1) {
            /* Battle forced-faint: select directly (no submenu) */
            wWhichPokemon = (uint8_t)g_cursor;
            g_selected    = g_cursor;
            g_open        = 0;
            pm_clear_icon_oam();
        } else {
            /* Overworld (force=0) or battle voluntary (force=2): show action submenu */
            g_submenu    = 1;
            g_sub_cursor = 0;
            pm_draw_submenu(g_sub_cursor);
        }
        return;
    }
    if (g_force != 1 && (hJoyPressed & PAD_B)) {
        Audio_PlaySFX_PressAB();
        g_selected = -1;
        g_open     = 0;
        pm_clear_icon_oam();
        return;
    }
}
