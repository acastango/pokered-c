/* summary_screen.c — Two-page Gen 1 Pokémon summary screen.
 * Ports StatusScreen / StatusScreen2 (engine/pokemon/status_screen.asm).
 *
 * Page 1 layout (matching StatusScreen exactly):
 *   Sprite:      OAM at pixel (8,0), 7×7 tiles.
 *   Name:        (9,1)
 *   Level:       (14,2)  <LV> + digits
 *   HP bar:      (11,3)  label+caps+6 tiles; fraction at (12,4)
 *   Status/OK:   (16,6) / (9,6) "STATUS/"
 *   Right border: │ col 19 rows 1–6, ┘(19,7), ─ (9–18,7), ←(8,7)
 *   №.:          (1,7),(2,7); dex# at (3,7)
 *   Stats box:   TextBoxBorder (0,8) b=8 c=8, labels col 1 rows 9/11/13/15,
 *                values col 6 rows 10/12/14/16
 *   Types:       "TYPE1/"(10,9) "TYPE2/"(10,11); values (11,10),(11,12)
 *   ID/OT:       "<ID>№/"(10,13) "OT/"(10,15); values (12,14),(12,16)
 *   Right border2: │ col 19 rows 9–16, ┘(19,17), ─(13–18,17), ←(12,17)
 *
 * Page 2 layout (matching StatusScreen2):
 *   Name:        keep at (9,1)
 *   EXP POINTS:  (9,3); value (12,4) 7 digits
 *   LEVEL UP:    (9,5); value (7,6) 7 digits; next-lv indicator (14,6)
 *   Moves box:   TextBoxBorder (0,8) b=8 c=18
 *   Move names:  (2,9),(2,11),(2,13),(2,15) — max 9 chars each
 *   PP labels:   "PP" at (11,10),(11,12),(11,14),(11,16)
 *   PP values:   cur/max at (14,10),(14,12),(14,14),(14,16)
 */
#include "summary_screen.h"
#include "../platform/hardware.h"
#include "../platform/display.h"
#include "../platform/audio.h"
#include "../data/font_data.h"
#include "../data/base_stats.h"
#include "../data/moves_data.h"
#include "../data/pokemon_sprites.h"
#include "constants.h"
#include "overworld.h"   /* gScrollTileMap, SCROLL_MAP_W */
#include "pokemon.h"
#include <stdio.h>
#include <string.h>

/* ---- WRAM externs ---- */
extern int         gScrollPxX, gScrollPxY;  /* player.c */
extern uint8_t     wPartyCount;
extern party_mon_t wPartyMons[PARTY_LENGTH];
extern uint8_t     wPartyMonNicks[PARTY_LENGTH][NAME_LENGTH];
extern uint8_t     wPartyMonOT[PARTY_LENGTH][NAME_LENGTH];

/* ---- State ---- */
typedef enum { SS_CLOSED = 0, SS_PAGE1, SS_PAGE2 } ss_state_t;
static ss_state_t s_state = SS_CLOSED;
static int        s_slot  = 0;

/* OAM/tile slots for the Pokémon front sprite (49 tiles, 7×7 OAM entries).
 * Reuses the same range as bui_load_sprites enemy slot — safe because
 * battles are not active when viewing the summary screen. */
#define SS_SPR_TILE_BASE  0
#define SS_SPR_OAM_BASE   0
#define SS_SPR_PX_X       8   /* col 1 × 8 px — matches hlcoord(1,0) */
#define SS_SPR_PX_Y       8   /* row 1 × 8 px — matches visual position in reference */

/* ---- Tile helpers (same pattern as party_menu.c) ---- */

static void ss_put(int col, int row, int tile_idx) {
    if ((unsigned)col >= SCREEN_WIDTH || (unsigned)row >= SCREEN_HEIGHT) return;
    gScrollTileMap[(row + 2) * SCROLL_MAP_W + (col + 2)] = (uint8_t)tile_idx;
}

static void ss_gb(int col, int row, uint8_t gb_char) {
    ss_put(col, row, Font_CharToTile(gb_char));
}

static int ss_ascii_tile(unsigned char c) {
    if (c >= 'A' && c <= 'Z') return Font_CharToTile(0x80 + (c - 'A'));
    if (c >= 'a' && c <= 'z') return Font_CharToTile(0xA0 + (c - 'a'));
    if (c >= '0' && c <= '9') return Font_CharToTile(0xF6 + (c - '0'));
    if (c == ' ')  return BLANK_TILE_SLOT;
    if (c == '.')  return Font_CharToTile(0xE8);
    if (c == '!')  return Font_CharToTile(0xE7);
    if (c == '?')  return Font_CharToTile(0xE6);
    if (c == ',')  return Font_CharToTile(0xF4);
    if (c == '-')  return Font_CharToTile(0xE3);
    if (c == '\'') return Font_CharToTile(0xE0);
    if (c == '/')  return Font_CharToTile(0xF3);
    return BLANK_TILE_SLOT;
}

static void ss_ascii(int col, int row, const char *s) {
    for (; *s; s++, col++)
        ss_put(col, row, ss_ascii_tile((unsigned char)*s));
}

/* Right-aligned decimal, `width` digits (1-8), space-padded on left */
static void ss_num(int col, int row, uint32_t val, int width) {
    char buf[9];
    snprintf(buf, sizeof(buf), "%*u", width, (unsigned)val);
    for (int i = 0; i < width; i++) {
        char c = buf[i];
        ss_put(col + i, row,
               c == ' ' ? BLANK_TILE_SLOT
                        : Font_CharToTile(0xF6 + (c - '0')));
    }
}

/* GB-encoded nickname → tiles (mirrors pm_draw_nick, with species-name fallback) */
static void ss_nick(int col, int row, int slot) {
    const uint8_t *nick = wPartyMonNicks[slot];
    if (nick[0] != 0x00) {
        for (int i = 0; i < 10; i++) {
            uint8_t ch = nick[i];
            if (ch == 0x50) {
                for (int j = i; j < 10; j++) ss_put(col + j, row, BLANK_TILE_SLOT);
                return;
            }
            ss_put(col + i, row, Font_CharToTile(ch));
        }
    } else {
        /* Nickname unset — fall back to ASCII species name */
        uint8_t dex = gSpeciesToDex[wPartyMons[slot].base.species];
        const char *name = Pokemon_GetName(dex);
        int len = (int)strlen(name);
        for (int i = 0; i < 10; i++) {
            ss_put(col + i, row,
                   i < len ? ss_ascii_tile((unsigned char)name[i]) : BLANK_TILE_SLOT);
        }
    }
}

/* GB-encoded OT name → tiles (NAME_LENGTH buffer, terminated with $50) */
static void ss_ot_name(int col, int row, int slot) {
    const uint8_t *ot = wPartyMonOT[slot];
    for (int i = 0; i < 7; i++) {
        uint8_t ch = ot[i];
        if (ch == 0x50 || ch == 0x00) {
            for (int j = i; j < 7; j++) ss_put(col + j, row, BLANK_TILE_SLOT);
            return;
        }
        ss_put(col + i, row, Font_CharToTile(ch));
    }
}

/* Level display — mirrors PrintLevel / pm_draw_level */
static void ss_level(int col, int row, uint8_t level) {
    if (level >= 100) {
        ss_put(col,   row, Font_CharToTile(0xF6 + (level / 100)));
        ss_put(col+1, row, Font_CharToTile(0xF6 + ((level / 10) % 10)));
        ss_put(col+2, row, Font_CharToTile(0xF6 + (level % 10)));
    } else {
        ss_gb(col,    row, 0x6E);  /* <LV> tile */
        ss_put(col+1, row, Font_CharToTile(0xF6 + (level / 10)));
        ss_put(col+2, row, Font_CharToTile(0xF6 + (level % 10)));
    }
}

/* HP bar at (col,row) — mirrors DrawHP (wHPBarType=1, fraction below).
 * Layout: [HP-label][left-cap][×6 bar tiles][right-cap]
 *         [###/###] on the row below at col+1. */
static void ss_hp_bar(int col, int row, uint16_t hp, uint16_t max_hp) {
    int pixels = max_hp ? ((int)hp * 48 / (int)max_hp) : 0;
    if (hp > 0 && pixels == 0) pixels = 1;
    if (pixels > 48) pixels = 48;

    ss_gb(col,   row, 0x71);   /* HP label */
    ss_gb(col+1, row, 0x62);   /* left cap */
    for (int i = 0; i < 6; i++) {
        int seg = pixels - i * 8;
        uint8_t tile = (seg <= 0) ? 0x63u
                     : (seg >= 8) ? 0x6Bu
                     :              (uint8_t)(0x63u + (unsigned)seg);
        ss_gb(col+2+i, row, tile);
    }
    ss_gb(col+8, row, 0x6D);   /* right cap — $6D (wHPBarType=1, status screen).
                                 * $6D has the same center stripe as border $78,
                                 * so it continues the vertical line visually. */

    /* Fraction below bar: matches "printFractionBelowBar" bc=SCREEN_WIDTH+1 */
    ss_num(col+1, row+1, hp,     3);
    ss_gb( col+4, row+1, 0xF3);    /* '/' */
    ss_num(col+5, row+1, max_hp, 3);
}

/* Status condition (mirrors PrintStatusCondition + pm_draw_status) */
static const uint8_t k_fnt[3] = {0x85, 0x8D, 0x93};
static const uint8_t k_slp[3] = {0x92, 0x8B, 0x8F};
static const uint8_t k_psn[3] = {0x8F, 0x92, 0x8D};
static const uint8_t k_brn[3] = {0x81, 0x91, 0x8D};
static const uint8_t k_frz[3] = {0x85, 0x91, 0x99};
static const uint8_t k_par[3] = {0x8F, 0x80, 0x91};

static void ss_status(int col, int row, uint8_t status, uint16_t hp) {
    const uint8_t *s = NULL;
    if (!hp)                  s = k_fnt;
    else if (status & 0x07)   s = k_slp;
    else if (status & (1<<3)) s = k_psn;
    else if (status & (1<<4)) s = k_brn;
    else if (status & (1<<5)) s = k_frz;
    else if (status & (1<<6)) s = k_par;
    for (int i = 0; i < 3; i++) {
        if (s) ss_gb(col + i, row, s[i]);
        else   ss_put(col + i, row, BLANK_TILE_SLOT);
    }
}

/* Pokémon type name (ASCII) — covers all Gen 1 types */
static const char *ss_type_name(uint8_t type_id) {
    static const char *names[27] = {
        "NORMAL","FIGHTING","FLYING","POISON","GROUND","ROCK","BIRD","BUG","GHOST",
        "","","","","","","","","","","",   /* 9-19 unused */
        "FIRE","WATER","GRASS","ELECTRIC","PSYCHIC","ICE","DRAGON"
    };
    if (type_id < 27) return names[type_id];
    return "";
}

/* TextBoxBorder: ┌─…─┐ / │…│ / └─…─┘  (mirrors TextBoxBorder predef) */
static void ss_box(int x, int y, int b, int c) {
    ss_gb(x,     y,     0x79);
    for (int i = 1; i <= c; i++) ss_gb(x+i, y,     0x7A);
    ss_gb(x+c+1, y,     0x7B);
    for (int r = 1; r <= b; r++) {
        ss_gb(x,     y+r, 0x7C);
        for (int i = 1; i <= c; i++) ss_put(x+i, y+r, BLANK_TILE_SLOT);
        ss_gb(x+c+1, y+r, 0x7C);
    }
    ss_gb(x,     y+b+1, 0x7D);
    for (int i = 1; i <= c; i++) ss_gb(x+i, y+b+1, 0x7A);
    ss_gb(x+c+1, y+b+1, 0x7E);
}

/* DrawLineBox: right │ from (col,row) for b_tiles rows, then ┘,
 * then ─ going left for c_tiles, then ←.
 * Mirrors DrawLineBox (engine/pokemon/status_screen.asm).
 * Uses BattleHudTiles: $78=│  $77=┘  $76=─  $6F=← */
static void ss_line_box(int col, int row, int b_tiles, int c_tiles) {
    for (int i = 0; i < b_tiles; i++)
        ss_gb(col, row + i, 0x78);               /* │  solid vertical (BattleHudTiles2) */
    ss_gb(col,           row + b_tiles, 0x77);   /* ┘  corner       (BattleHudTiles3) */
    for (int i = 1; i <= c_tiles; i++)
        ss_gb(col - i,   row + b_tiles, 0x76);   /* ─  horizontal   (BattleHudTiles3) */
    ss_gb(col - c_tiles - 1, row + b_tiles, 0x6F); /* ← halfarrow end */
}

/* Load front sprite tiles into OBJ tile cache and set up 49 OAM entries. */
static void ss_load_sprite(uint8_t dex) {
    if (dex == 0 || dex > 151) return;
    for (int i = 0; i < POKEMON_FRONT_CANVAS_TILES; i++)
        Display_LoadSpriteTile((uint8_t)(SS_SPR_TILE_BASE + i),
                               gPokemonFrontSprite[dex][i]);
    /* Horizontal flip mirrors LoadFlippedFrontSpriteByMonIndex:
     * reverse tile column order within each row, set OAM_FLAG_FLIP_X so
     * each tile's pixel data is also mirrored. */
    for (int ty = 0; ty < 7; ty++) {
        for (int tx = 0; tx < 7; tx++) {
            int idx = SS_SPR_OAM_BASE + ty * 7 + tx;
            wShadowOAM[idx].y    = (uint8_t)(SS_SPR_PX_Y + ty * 8 + OAM_Y_OFS);
            wShadowOAM[idx].x    = (uint8_t)(SS_SPR_PX_X + tx * 8 + OAM_X_OFS);
            wShadowOAM[idx].tile = (uint8_t)(SS_SPR_TILE_BASE + ty * 7 + (6 - tx));
            wShadowOAM[idx].flags = OAM_FLAG_FLIP_X;
        }
    }
}

static void ss_clear_sprite_oam(void) {
    for (int i = SS_SPR_OAM_BASE; i < SS_SPR_OAM_BASE + 49; i++) {
        wShadowOAM[i].y    = 0;
        wShadowOAM[i].x    = 0;
        wShadowOAM[i].tile = 0;
        wShadowOAM[i].flags = 0;
    }
}

/* ---- Page 1: stats screen ---- */
static void ss_draw_page1(int slot) {
    const party_mon_t *mon = &wPartyMons[slot];
    uint8_t dex = gSpeciesToDex[mon->base.species];

    /* Clear full screen */
    for (int r = 0; r < SCREEN_HEIGHT; r++)
        for (int c = 0; c < SCREEN_WIDTH; c++)
            ss_put(c, r, BLANK_TILE_SLOT);

    /* DrawLineBox 1: right border (col 19 rows 1–6) + bottom bar (row 7)
     * hlcoord(19,1) b=6 c=10 → │ rows 1-6, ┘(19,7), ─(9-18,7), ←(8,7) */
    ss_line_box(19, 1, 6, 10);

    /* '№' + '<DOT>' at (1,7),(2,7) — mirrors status_screen.asm which places
     * '№'($74) then '<DOT>'($F2) manually after DrawLineBox1. */
    ss_gb(1, 7, 0x74);   /* '№' compact glyph (font_battle_extra tile) */
    ss_gb(2, 7, 0xF2);   /* '<DOT>' decimal point (charmap $F2, main font) */

    /* Pokédex number at (3,7): 3 digits with leading zeros */
    if (dex > 0 && dex <= 151) {
        ss_put(3, 7, Font_CharToTile(0xF6 + (dex / 100)));
        ss_put(4, 7, Font_CharToTile(0xF6 + ((dex / 10) % 10)));
        ss_put(5, 7, Font_CharToTile(0xF6 + (dex % 10)));
    }

    /* Pokémon name at (9,1) */
    ss_nick(9, 1, slot);

    /* Level at (14,2): <LV> + 2 digits */
    ss_level(14, 2, mon->level);

    /* HP bar at (11,3), fraction at (12,4) */
    ss_hp_bar(11, 3, mon->base.hp, mon->max_hp);

    /* STATUS/ label at (9,6); status condition at (16,6) */
    ss_ascii(9, 6, "STATUS/");
    ss_status(16, 6, mon->base.status, mon->base.hp);
    /* "OK" when no status condition and not fainted */
    if (mon->base.hp > 0 && (mon->base.status & 0x7F) == 0) {
        ss_gb(16, 6, 0x8E);   /* 'O' */
        ss_gb(17, 6, 0x8A);   /* 'K' */
    }

    /* Stats box: TextBoxBorder at (0,8) b=8 c=8
     * Inner area cols 1-8, rows 9-16 */
    ss_box(0, 8, 8, 8);

    /* Stat labels at (1,9),(1,11),(1,13),(1,15) — mirrors .StatsText */
    ss_ascii(1, 9,  "ATTACK");
    ss_ascii(1, 11, "DEFENSE");
    ss_ascii(1, 13, "SPEED");
    ss_ascii(1, 15, "SPECIAL");

    /* Stat values at (6,10),(6,12),(6,14),(6,16) — right-aligned 3 digits */
    ss_num(6, 10, mon->atk, 3);
    ss_num(6, 12, mon->def, 3);
    ss_num(6, 14, mon->spd, 3);
    ss_num(6, 16, mon->spc, 3);

    /* DrawLineBox 2: right border (col 19 rows 9–16) + bottom bar (row 17)
     * hlcoord(19,9) b=8 c=6 → │ rows 9-16, ┘(19,17), ─(13-18,17), ←(12,17) */
    ss_line_box(19, 9, 8, 6);

    /* Type/ID/OT labels at (10, 9/11/13/15) — mirrors TypesIDNoOTText */
    ss_ascii(10, 9,  "TYPE1/");
    ss_ascii(10, 11, "TYPE2/");
    /* "<ID>№/" — use the compact glyphs: $73=<ID>, $74=№, $F3=/ */
    ss_gb(10, 13, 0x73);   /* '<ID>' compact glyph */
    ss_gb(11, 13, 0x74);   /* '№'   compact glyph */
    ss_gb(12, 13, 0xF3);   /* '/'   separator */
    ss_ascii(10, 15, "OT/");

    /* Type values at (11,10) and (11,12) */
    ss_ascii(11, 10, ss_type_name(mon->base.type1));
    if (mon->base.type2 != mon->base.type1)
        ss_ascii(11, 12, ss_type_name(mon->base.type2));

    /* OT ID at (12,14): 5 digits */
    ss_num(12, 14, mon->base.ot_id, 5);

    /* OT name at (12,16) */
    ss_ot_name(12, 16, slot);

    /* Load front sprite + OAM (mirrors LoadFlippedFrontSpriteByMonIndex) */
    ss_load_sprite(dex);
}

/* ---- Page 2: moves / EXP screen ---- */
static void ss_draw_page2(int slot) {
    const party_mon_t *mon = &wPartyMons[slot];
    uint8_t dex = gSpeciesToDex[mon->base.species];

    /* Decode 24-bit big-endian EXP */
    uint32_t cur_exp = ((uint32_t)mon->base.exp[0] << 16)
                     | ((uint32_t)mon->base.exp[1] <<  8)
                     |  (uint32_t)mon->base.exp[2];

    /* EXP to next level (0 at Lv100) */
    uint32_t exp_to_next = 0;
    if (mon->level < 100 && dex > 0 && dex <= 151) {
        uint32_t next = CalcExpForLevel(gBaseStats[dex].growth_rate,
                                        (uint8_t)(mon->level + 1));
        exp_to_next = (next > cur_exp) ? (next - cur_exp) : 0;
    }

    /* ClearScreenArea hlcoord(9,2) b=5 c=10: rows 2-6, cols 9-18 only.
     * Col 19 (DrawLineBox1 border │) and row 7 (bottom ┘─←) are preserved. */
    for (int r = 2; r <= 6; r++)
        for (int c = 9; c <= 18; c++)
            ss_put(c, r, BLANK_TILE_SLOT);

    /* Redraw name at (9,1) — may have been cleared by full clear */
    ss_nick(9, 1, slot);

    /* DrawLineBox right border fragment: │ at (19,3) only
     * (keeps the right-side feel on page 2) */
    ss_gb(19, 3, 0x78);   /* │ */

    /* EXP labels and values */
    ss_ascii(9, 3, "EXP POINTS");
    ss_ascii(9, 5, "LEVEL UP");
    ss_num(12, 4, cur_exp,    7);
    ss_num(7,  6, exp_to_next, 7);

    /* Next-level indicator: <to> at (14,6), next level at (16,6)
     * mirrors `ld [hl], '<to>'; inc hl; inc hl; call PrintLevel` */
    ss_gb(14, 6, 0x70);   /* <to> narrow arrow */
    if (mon->level < 100)
        ss_level(16, 6, (uint8_t)(mon->level + 1));
    else
        ss_level(16, 6, 100);

    /* Move container: TextBoxBorder (0,8) b=8 c=18
     * Inner area cols 1-18, rows 9-16 */
    ss_box(0, 8, 8, 18);

    /* Moves: names at (2, 9/11/13/15), PP at (11, 10/12/14/16),
     * values at (14, 10/12/14/16).
     * Mirrors move-name placement via wMovesString + StatusScreen_PrintPP. */
    for (int i = 0; i < NUM_MOVES; i++) {
        int name_row = 9  + i * 2;
        int pp_row   = 10 + i * 2;
        uint8_t move_id = mon->base.moves[i];

        if (move_id > 0 && move_id < NUM_MOVE_DEFS) {
            /* Move name — truncate to 9 chars (cols 2-10) */
            const char *name = gMoveNames[move_id];
            int len = (int)strlen(name);
            if (len > 9) len = 9;
            for (int j = 0; j < 9; j++) {
                if (j < len)
                    ss_put(2 + j, name_row, ss_ascii_tile((unsigned char)name[j]));
                else
                    ss_put(2 + j, name_row, BLANK_TILE_SLOT);
            }

            /* PP label: "PP" using <BOLD_P> tile ($72) × 2 */
            ss_gb(11, pp_row, 0x72);
            ss_gb(12, pp_row, 0x72);

            /* PP value: current / max at (14, pp_row) */
            uint8_t cur_pp  = mon->base.pp[i] & PP_MASK;
            uint8_t pp_ups  = mon->base.pp[i] >> 6;
            uint8_t max_pp  = (uint8_t)(gMoves[move_id].pp * (5 + pp_ups) / 5);
            ss_num(14, pp_row, cur_pp, 2);
            ss_gb( 16, pp_row, 0xF3);   /* '/' */
            ss_num(17, pp_row, max_pp, 2);
        } else {
            /* No move: "------" name, "--/--" PP */
            ss_ascii(2, name_row, "------");
            ss_gb(11, pp_row, 0xE3);   /* '-' */
            ss_gb(12, pp_row, 0xE3);
            ss_gb(14, pp_row, 0xE3);
            ss_gb(15, pp_row, 0xE3);
            ss_gb(16, pp_row, 0xF3);   /* '/' */
            ss_gb(17, pp_row, 0xE3);
            ss_gb(18, pp_row, 0xE3);
        }
    }
}

/* ---- Public API ---- */

void SummaryScreen_Open(int slot) {
    if (slot < 0 || slot >= (int)wPartyCount) return;
    Font_LoadHudTiles();
    /* Override 3 HUD tile slots with status-screen-specific glyphs.
     * pokered status_screen.asm loads font_battle_extra at $62+ (which has
     * the compact '<ID>'/$73 and '№'/$74 glyphs), then BattleHudTiles2 at
     * $78 (solid │) and BattleHudTiles3 at $76-$77 (─ ┘).
     * Our gHudTiles has battle HUD data at $73/$74/$78; override here.
     * Font_LoadHudTiles() at next battle start restores battle data. */
    static const uint8_t kIdGlyph[16]  = {
        0x00,0x00, 0x00,0x00, 0x5C,0x5C, 0x52,0x52,
        0x52,0x52, 0x52,0x52, 0x5C,0x5C, 0x00,0x00 };  /* <ID> $73 */
    static const uint8_t kNoGlyph[16]  = {
        0x00,0x00, 0x00,0x00, 0x90,0x90, 0xD7,0xD7,
        0xF5,0xF5, 0xB5,0xB5, 0x97,0x97, 0x00,0x00 };  /* '№' $74 */
    static const uint8_t kVBarGlyph[16] = {
        0x18,0x18, 0x18,0x18, 0x18,0x18, 0x18,0x18,
        0x18,0x18, 0x18,0x18, 0x18,0x18, 0x18,0x18 };  /* │  $78 */
    static const uint8_t kPTile[16] = {
        0x00,0x00, 0xFC,0xFC, 0xC6,0xC6, 0xC6,0xC6,
        0xC6,0xC6, 0xFC,0xFC, 0xC0,0xC0, 0xC0,0xC0 };  /* bold 'P' $72 */
    Display_LoadTile((uint8_t)Font_CharToTile(0x72), kPTile);
    Display_LoadTile((uint8_t)Font_CharToTile(0x73), kIdGlyph);
    Display_LoadTile((uint8_t)Font_CharToTile(0x74), kNoGlyph);
    Display_LoadTile((uint8_t)Font_CharToTile(0x78), kVBarGlyph);
    /* Reset scroll — ensures OAM sprite aligns with BG tile grid */
    gScrollPxX = 0;
    gScrollPxY = 0;
    s_slot  = slot;
    s_state = SS_PAGE1;
    ss_draw_page1(slot);
}

int SummaryScreen_IsOpen(void) {
    return s_state != SS_CLOSED;
}

void SummaryScreen_Tick(void) {
    if (s_state == SS_CLOSED) return;

    if (s_state == SS_PAGE1) {
        if (hJoyPressed & PAD_A) {
            Audio_PlaySFX_PressAB();
            s_state = SS_PAGE2;
            ss_draw_page2(s_slot);
        } else if (hJoyPressed & PAD_B) {
            Audio_PlaySFX_PressAB();
            ss_clear_sprite_oam();
            s_state = SS_CLOSED;
        }
    } else { /* SS_PAGE2 */
        if ((hJoyPressed & PAD_A) || (hJoyPressed & PAD_B)) {
            Audio_PlaySFX_PressAB();
            ss_clear_sprite_oam();
            s_state = SS_CLOSED;
        }
    }
}
