/* pokedex.c — Pokédex list + detail screen.
 *
 * Screen layout uses the same gScrollTileMap tile-grid as menu.c /
 * party_menu.c: screen (col, row) → buffer [(row+2)*SCROLL_MAP_W+(col+2)].
 * Screen is 20 cols × 18 rows.
 *
 * LIST SCREEN
 *   ┌──────────────────┐
 *   │> 001 BULBASAUR   │  ← 7 entries (rows 1,3,5,7,9,11,13)
 *   │  SEEN:012 OWN:03 │  row 15
 *   └──────────────────┘
 *
 * DETAIL SCREEN
 *   ┌──────────────────┐
 *   │ №001 BULBASAUR   │
 *   │ SEED POKEMON     │
 *   │ TYPE: GRASS/PSNS │
 *   │ HT 2'04"  WT 1.5 │
 *   │ (description)    │
 *   └──────────────────┘
 */
#include "pokedex.h"
#include "overworld.h"
#include "npc.h"
#include "text.h"
#include "pokemon.h"
#include "constants.h"
#include "../data/font_data.h"
#include "../data/base_stats.h"
#include "../data/dex_data.h"
#include "../data/pokemon_sprites.h"
#include "../platform/hardware.h"
#include "../platform/display.h"
#include "../platform/audio.h"
#include <string.h>
#include <stdio.h>

/* ---- Tile rendering helpers (mirrors party_menu.c / menu.c) ---------- */

static void dex_put(int col, int row, uint8_t tile) {
    extern uint8_t gScrollTileMap[];
    gScrollTileMap[(row + 2) * SCROLL_MAP_W + (col + 2)] = tile;
}

static int ascii_tile(unsigned char c) {
    if (c >= 'A' && c <= 'Z') return Font_CharToTile((uint8_t)(0x80 + (c - 'A')));
    if (c >= 'a' && c <= 'z') return Font_CharToTile((uint8_t)(0xA0 + (c - 'a')));
    if (c >= '0' && c <= '9') return Font_CharToTile((uint8_t)(0xF6 + (c - '0')));
    if (c == ' ')  return BLANK_TILE_SLOT;
    if (c == '.')  return Font_CharToTile(0xE8);
    if (c == '!')  return Font_CharToTile(0xE7);
    if (c == '\'') return Font_CharToTile(0xE0);
    if (c == '-')  return Font_CharToTile(0xE3);
    if (c == ',')  return Font_CharToTile(0xF4);
    if (c == '/')  return Font_CharToTile(0xF3);
    if (c == ':')  return Font_CharToTile(0x9C);
    if (c == '"')  return Font_CharToTile(0xE0); /* approximate */
    if (c == '>')  return Font_CharToTile(0xEC); /* ▶ arrow */
    if (c == '#')  return Font_CharToTile(0xEB); /* № */
    return BLANK_TILE_SLOT;
}

static void dex_str(int col, int row, const char *s) {
    for (; *s; s++, col++)
        dex_put(col, row, (uint8_t)ascii_tile((unsigned char)*s));
}

static void dex_fill(int col, int row, int len, uint8_t tile) {
    for (int i = 0; i < len; i++)
        dex_put(col + i, row, tile);
}

static void dex_num3(int col, int row, int n) {
    char buf[4];
    snprintf(buf, sizeof(buf), "%03d", n);
    dex_str(col, row, buf);
}

static void dex_box(int L, int T, int R, int B) {
    dex_put(L, T, (uint8_t)Font_CharToTile(0x79)); /* ┌ */
    dex_put(R, T, (uint8_t)Font_CharToTile(0x7B)); /* ┐ */
    dex_put(L, B, (uint8_t)Font_CharToTile(0x7D)); /* └ */
    dex_put(R, B, (uint8_t)Font_CharToTile(0x7E)); /* ┘ */
    for (int c = L+1; c < R; c++) {
        dex_put(c, T, (uint8_t)Font_CharToTile(0x7A)); /* ─ */
        dex_put(c, B, (uint8_t)Font_CharToTile(0x7A));
    }
    for (int r = T+1; r < B; r++) {
        dex_put(L, r, (uint8_t)Font_CharToTile(0x7C)); /* │ */
        dex_put(R, r, (uint8_t)Font_CharToTile(0x7C));
        dex_fill(L+1, r, R-L-1, (uint8_t)BLANK_TILE_SLOT);
    }
}

/* ---- Type name lookup ------------------------------------------------ */
static const char *type_name(uint8_t t) {
    switch (t) {
        case 0x00: return "NORMAL";
        case 0x01: return "FIGHTNG";
        case 0x02: return "FLYING";
        case 0x03: return "POISON";
        case 0x04: return "GROUND";
        case 0x05: return "ROCK";
        case 0x07: return "BUG";
        case 0x08: return "GHOST";
        case 0x14: return "FIRE";
        case 0x15: return "WATER";
        case 0x16: return "GRASS";
        case 0x17: return "ELECTRC";
        case 0x18: return "PSYCHIC";
        case 0x19: return "ICE";
        case 0x1A: return "DRAGON";
        default:   return "?????";
    }
}

/* ---- Pokédex seen/owned bit helpers ---------------------------------- */
static int dex_owned(int dex_num) {
    int bit = dex_num - 1;
    return (wPokedexOwned[bit >> 3] >> (bit & 7)) & 1;
}
static int dex_seen(int dex_num) {
    int bit = dex_num - 1;
    return (wPokedexSeen[bit >> 3] >> (bit & 7)) & 1;
}
static int count_bits(const uint8_t *arr, int len) {
    int n = 0;
    for (int i = 0; i < len; i++) {
        uint8_t b = arr[i];
        while (b) { n += b & 1; b >>= 1; }
    }
    return n;
}

/* ---- State ----------------------------------------------------------- */
#define DEX_SCREEN_LIST   0
#define DEX_SCREEN_DETAIL 1
#define DEX_VISIBLE       7     /* entries shown at once */
#define DEX_MAX          151

static int gDexOpen   = 0;
static int gDexScreen = DEX_SCREEN_LIST;
static int gDexCursor = 1;  /* current dex number (1-151) */
static int gDexScroll = 1;  /* top of visible window (1-151) */

/* ---- List screen drawing -------------------------------------------- */
static void draw_list(void) {
    dex_box(0, 0, 19, 17);

    /* 7 visible entries */
    for (int i = 0; i < DEX_VISIBLE; i++) {
        int dex_num = gDexScroll + i;
        int row     = 1 + i * 2;
        if (dex_num > DEX_MAX) {
            dex_fill(1, row, 18, (uint8_t)BLANK_TILE_SLOT);
            continue;
        }

        /* cursor arrow */
        dex_put(1, row, (uint8_t)ascii_tile(dex_num == gDexCursor ? '>' : ' '));

        /* pokéball indicator: ● = owned, · = seen, space = unseen */
        if (dex_owned(dex_num))
            dex_put(2, row, (uint8_t)Font_CharToTile(0xA5)); /* filled circle */
        else if (dex_seen(dex_num))
            dex_put(2, row, (uint8_t)Font_CharToTile(0xAC)); /* open circle */
        else
            dex_put(2, row, (uint8_t)BLANK_TILE_SLOT);

        /* dex number: №001 */
        dex_put(3, row, (uint8_t)ascii_tile('#'));  /* № */
        dex_num3(4, row, dex_num);

        /* name or dashes */
        if (dex_seen(dex_num) || dex_owned(dex_num)) {
            const char *name = Pokemon_GetName(dex_num);
            dex_fill(8, row, 11, (uint8_t)BLANK_TILE_SLOT);
            dex_str(8, row, name ? name : "?");
        } else {
            for (int j = 8; j < 18; j++)
                dex_put(j, row, (uint8_t)Font_CharToTile(0xE3)); /* --- */
        }
    }

    /* SEEN / OWN counts */
    int seen_cnt = count_bits(wPokedexSeen, 19);
    int own_cnt  = count_bits(wPokedexOwned, 19);
    char buf[20];
    dex_fill(1, 15, 18, (uint8_t)BLANK_TILE_SLOT);
    snprintf(buf, sizeof(buf), "SEEN:%3d OWN:%3d", seen_cnt, own_cnt);
    dex_str(1, 15, buf);
}

/* ---- Detail screen drawing ------------------------------------------ */
static void draw_detail(int dex_num) {
    dex_box(0, 0, 19, 17);

    const dex_entry_t *e = (dex_num >= 1 && dex_num <= 151) ? &gDexEntries[dex_num] : NULL;
    const base_stats_t *b = NULL;
    /* Find base_stats by dex_id */
    for (int s = 1; s <= 151; s++) {
        if (gBaseStats[s].dex_id == (uint8_t)dex_num) { b = &gBaseStats[s]; break; }
    }

    /* Line 1: №001 BULBASAUR */
    char buf[22];
    const char *name = Pokemon_GetName(dex_num);
    snprintf(buf, sizeof(buf), "#%03d %s", dex_num, name ? name : "?");
    dex_str(1, 1, buf);

    if (!e || !dex_seen(dex_num)) {
        dex_str(1, 3, "???");
        dex_str(1, 5, "Not seen yet.");
        dex_str(1, 15, "A:back");
        return;
    }

    /* Line 2: SEED POKEMON */
    if (e->category) {
        snprintf(buf, sizeof(buf), "%s POKEMON", e->category);
        dex_str(1, 3, buf);
    }

    /* Line 3: TYPE: GRASS/POISON */
    if (b) {
        const char *t1 = type_name(b->type1);
        const char *t2 = type_name(b->type2);
        if (b->type1 == b->type2)
            snprintf(buf, sizeof(buf), "TYPE: %s", t1);
        else
            snprintf(buf, sizeof(buf), "TYPE: %s/%s", t1, t2);
        dex_str(1, 5, buf);
    }

    /* Line 4: HT 2'04"  WT 1.5lb (only if owned) */
    if (dex_owned(dex_num)) {
        int lb_whole = e->weight / 10;
        int lb_frac  = e->weight % 10;
        snprintf(buf, sizeof(buf), "HT %d'%02d\"  WT %d.%dlb",
                 e->height_ft, e->height_in, lb_whole, lb_frac);
        dex_str(1, 7, buf);

        /* Description text (first page only, up to row 13) */
        if (e->description) {
            const char *d = e->description;
            int row = 9;
            int col = 1;
            for (; *d && row <= 13; d++) {
                if (*d == '\\' && *(d+1) == 'f') {
                    d++; row += 2; col = 1; /* page break */
                } else if (*d == ' ' && col > 1) {
                    /* word-wrap at col 18 */
                    dex_put(col, row, (uint8_t)BLANK_TILE_SLOT);
                    col++;
                    if (col > 18) { col = 1; row += 2; }
                } else {
                    if (col > 18) { col = 1; row += 2; }
                    if (row > 13) break;
                    dex_put(col, row, (uint8_t)ascii_tile((unsigned char)*d));
                    col++;
                }
            }
        }
    }

    dex_str(1, 15, "A/B: back");
}

/* ---- Clamp scroll ---------------------------------------------------- */
static void clamp_scroll(void) {
    if (gDexCursor < gDexScroll)
        gDexScroll = gDexCursor;
    if (gDexCursor >= gDexScroll + DEX_VISIBLE)
        gDexScroll = gDexCursor - DEX_VISIBLE + 1;
    if (gDexScroll < 1)       gDexScroll = 1;
    if (gDexScroll > DEX_MAX) gDexScroll = DEX_MAX;
}

/* ---- Public API ------------------------------------------------------ */
void Pokedex_Open(void) {
    gDexOpen   = 1;
    gDexScreen = DEX_SCREEN_LIST;
    if (gDexCursor < 1)    gDexCursor = 1;
    if (gDexCursor > 151)  gDexCursor = 1;
    clamp_scroll();

    /* Hide all sprites (same as Menu_Open) */
    for (int i = 0; i < MAX_SPRITES; i++) wShadowOAM[i].y = 0;
    draw_list();
}

int Pokedex_IsOpen(void) { return gDexOpen; }

void Pokedex_Tick(void) {
    if (!gDexOpen) return;

    extern uint8_t hJoyPressed;

    if (gDexScreen == DEX_SCREEN_LIST) {
        if (hJoyPressed & PAD_UP) {
            if (gDexCursor > 1) gDexCursor--;
            clamp_scroll();
            draw_list();
            return;
        }
        if (hJoyPressed & PAD_DOWN) {
            if (gDexCursor < DEX_MAX) gDexCursor++;
            clamp_scroll();
            draw_list();
            return;
        }
        if (hJoyPressed & PAD_A) {
            /* Open detail for selected entry */
            gDexScreen = DEX_SCREEN_DETAIL;
            draw_detail(gDexCursor);
            return;
        }
        if (hJoyPressed & (PAD_B | PAD_START)) {
            gDexOpen = 0;
            Map_BuildScrollView();
            NPC_BuildView(0, 0);
            return;
        }
    } else {
        /* Detail screen: any A or B returns to list */
        if (hJoyPressed & (PAD_A | PAD_B)) {
            gDexScreen = DEX_SCREEN_LIST;
            draw_list();
            return;
        }
    }
}

/* ---- Seen/Owned flags ------------------------------------------------ */
void Pokedex_SetSeen(int species) {
    if (species < 1 || species > 151) return;
    int dex_num = (int)gBaseStats[species].dex_id;
    if (dex_num < 1 || dex_num > 151) return;
    int bit = dex_num - 1;
    wPokedexSeen[bit >> 3] |= (uint8_t)(1u << (bit & 7));
}

void Pokedex_SetOwned(int species) {
    if (species < 1 || species > 151) return;
    int dex_num = (int)gBaseStats[species].dex_id;
    if (dex_num < 1 || dex_num > 151) return;
    int bit = dex_num - 1;
    wPokedexOwned[bit >> 3] |= (uint8_t)(1u << (bit & 7));
    /* Also mark as seen */
    wPokedexSeen[bit >> 3] |= (uint8_t)(1u << (bit & 7));
}

/* ==== ShowPokedexData — standalone full-screen dex entry ===============
 * Matches engine/menus/pokedex.asm ShowPokedexData / ShowPokedexDataInternal.
 *
 * Layout (20x18 tiles):
 *   Row 0:     border top
 *   (1,1):     7x7 front sprite (flipped)
 *   (9,2):     Pokemon name
 *   (9,4):     Species category + " #MON"
 *   (9,6):     "HT  X'YY"
 *   (2,8):     "No.XXX"
 *   (9,8):     "WT  XXX.Xlb"
 *   Row 9:     horizontal divider
 *   (1,11):    description text (slowtext, 1 char per frame)
 *   Row 17:    border bottom
 * ====================================================================== */

/* ShowData sprite constants — reuse tile slots 0-48, OAM 0-48 */
#define SD_SPR_TILE_BASE  0
#define SD_SPR_OAM_BASE   0
#define SD_SPR_PX_X       8    /* col 1 * 8px */
#define SD_SPR_PX_Y       8    /* row 1 * 8px */

/* ShowData state */
static int         gShowDataActive = 0;
static int         gShowDataDex    = 0;
static const char *gShowDataDesc   = NULL;   /* current position in description */
static int         gShowDataCol    = 1;
static int         gShowDataRow    = 11;
static int         gShowDataDone   = 0;      /* description finished, waiting for button */
static int         gShowDataTimer  = 0;      /* letter delay counter */

static void sd_load_sprite(int dex_num) {
    if (dex_num < 1 || dex_num > 151) return;
    /* Load tiles */
    for (int i = 0; i < POKEMON_FRONT_CANVAS_TILES; i++)
        Display_LoadSpriteTile((uint8_t)(SD_SPR_TILE_BASE + i),
                               gPokemonFrontSprite[dex_num][i]);
    /* Set up OAM — horizontally flipped (LoadFlippedFrontSpriteByMonIndex) */
    for (int ty = 0; ty < 7; ty++) {
        for (int tx = 0; tx < 7; tx++) {
            int idx = SD_SPR_OAM_BASE + ty * 7 + tx;
            wShadowOAM[idx].y    = (uint8_t)(SD_SPR_PX_Y + ty * 8 + OAM_Y_OFS);
            wShadowOAM[idx].x    = (uint8_t)(SD_SPR_PX_X + tx * 8 + OAM_X_OFS);
            wShadowOAM[idx].tile = (uint8_t)(SD_SPR_TILE_BASE + ty * 7 + (6 - tx));
            wShadowOAM[idx].flags = OAM_FLAG_FLIP_X;
        }
    }
}

static void sd_clear_sprite(void) {
    for (int i = SD_SPR_OAM_BASE; i < SD_SPR_OAM_BASE + 49; i++) {
        wShadowOAM[i].y = 0;
        wShadowOAM[i].x = 0;
        wShadowOAM[i].tile = 0;
        wShadowOAM[i].flags = 0;
    }
}

static void sd_draw_screen(int dex_num) {
    extern uint8_t gScrollTileMap[];
    const dex_entry_t *e = (dex_num >= 1 && dex_num <= 151) ? &gDexEntries[dex_num] : NULL;

    /* Clear entire scroll tile map */
    memset(gScrollTileMap, BLANK_TILE_SLOT,
           SCROLL_MAP_W * (SCREEN_HEIGHT + 4));

    /* Border — using standard box tiles ($79-$7E) */
    dex_put(0,  0,  (uint8_t)Font_CharToTile(0x79)); /* top-left */
    dex_put(19, 0,  (uint8_t)Font_CharToTile(0x7B)); /* top-right */
    dex_put(0,  17, (uint8_t)Font_CharToTile(0x7D)); /* bottom-left */
    dex_put(19, 17, (uint8_t)Font_CharToTile(0x7E)); /* bottom-right */
    for (int c = 1; c < 19; c++) {
        dex_put(c, 0,  (uint8_t)Font_CharToTile(0x7A)); /* top edge */
        dex_put(c, 17, (uint8_t)Font_CharToTile(0x7A)); /* bottom edge */
    }
    for (int r = 1; r < 17; r++) {
        dex_put(0,  r, (uint8_t)Font_CharToTile(0x7C)); /* left edge */
        dex_put(19, r, (uint8_t)Font_CharToTile(0x7C)); /* right edge */
    }

    /* Horizontal divider at row 9 */
    for (int c = 0; c < 20; c++)
        dex_put(c, 9, (uint8_t)Font_CharToTile(0x7A));

    /* Pokemon name at (9,2) */
    const char *name = Pokemon_GetName((uint8_t)dex_num);
    if (name) dex_str(9, 2, name);

    /* Species category at (9,4) — e.g. "LIZARD #MON" */
    if (e && e->category) {
        char cat_buf[20];
        snprintf(cat_buf, sizeof(cat_buf), "%s #MON", e->category);
        dex_str(9, 4, cat_buf);
    }

    /* No.XXX at (2,8) */
    {
        char num_buf[10];
        /* # tile = No. symbol, then . then 3-digit number */
        snprintf(num_buf, sizeof(num_buf), "#.%03d", dex_num);
        dex_str(2, 8, num_buf);
    }

    /* Height & Weight (only if owned) */
    if (dex_owned(dex_num) && e) {
        /* HT at (9,6): "HT  X'YY" */
        char ht_buf[12];
        snprintf(ht_buf, sizeof(ht_buf), "HT  %d'%02d\"", e->height_ft, e->height_in);
        dex_str(9, 6, ht_buf);

        /* WT at (9,8): "WT  XXX.Xlb" */
        char wt_buf[14];
        int lb_whole = e->weight / 10;
        int lb_frac  = e->weight % 10;
        snprintf(wt_buf, sizeof(wt_buf), "WT %4d.%dlb", lb_whole, lb_frac);
        dex_str(9, 8, wt_buf);
    }
}

void Pokedex_ShowData(int dex_num) {
    if (dex_num < 1 || dex_num > 151) return;

    const dex_entry_t *e = &gDexEntries[dex_num];

    gShowDataActive = 1;
    gShowDataDex    = dex_num;
    gShowDataDone   = 0;
    gShowDataTimer  = 0;

    /* Set up description pointer (only if owned) */
    if (dex_owned(dex_num) && e->description) {
        gShowDataDesc = e->description;
        gShowDataCol  = 1;
        gShowDataRow  = 11;
    } else {
        gShowDataDesc = NULL;
        gShowDataDone = 1;  /* no description, just wait for button */
    }

    /* Hide all overworld sprites */
    for (int i = 0; i < MAX_SPRITES; i++) wShadowOAM[i].y = 0;

    /* Disable window layer (text box) */
    hWY = 144;

    /* Draw the screen layout */
    sd_draw_screen(dex_num);

    /* Load and display front sprite (flipped) */
    sd_load_sprite(dex_num);

    /* Play cry — Audio_PlayCry takes species ID, not dex number */
    Audio_PlayCry(gDexToSpecies[dex_num]);
}

void Pokedex_ShowDataTick(void) {
    if (!gShowDataActive) return;

    if (!gShowDataDone) {
        /* Slowtext: render one character per frame.
         * Description area: rows 11-16, cols 1-18 (inside border). */
        if (gShowDataDesc && *gShowDataDesc) {
            char c = *gShowDataDesc++;

            if (c == '\\' && *gShowDataDesc == 'f') {
                /* \f = line break in dex description (matches NEXT/LINE) */
                gShowDataDesc++;
                gShowDataRow++;
                gShowDataCol = 1;
            } else if (c == '\n') {
                gShowDataRow++;
                gShowDataCol = 1;
            } else if (c == ' ') {
                /* Word-wrap look-ahead: if the next word won't fit on this
                 * line, wrap now instead of placing the space. */
                int next_word_len = 0;
                const char *p = gShowDataDesc;
                while (*p && *p != ' ' && *p != '\\' && *p != '\n')
                    { next_word_len++; p++; }
                if (gShowDataCol + next_word_len >= 18) {
                    /* Wrap — skip the space, start next line */
                    gShowDataRow++;
                    gShowDataCol = 1;
                } else if (gShowDataCol >= 18) {
                    gShowDataRow++;
                    gShowDataCol = 1;
                } else {
                    if (gShowDataRow <= 16)
                        dex_put(gShowDataCol, gShowDataRow, (uint8_t)BLANK_TILE_SLOT);
                    gShowDataCol++;
                }
            } else {
                if (gShowDataCol >= 18) {
                    gShowDataRow++;
                    gShowDataCol = 1;
                }
                if (gShowDataRow <= 16) {
                    dex_put(gShowDataCol, gShowDataRow,
                            (uint8_t)ascii_tile((unsigned char)c));
                }
                gShowDataCol++;
            }

            /* Stop if we've gone past the displayable area */
            if (gShowDataRow > 16) gShowDataDone = 1;
        } else {
            gShowDataDone = 1;
        }
    }

    if (gShowDataDone) {
        /* Wait for A or B press */
        if (hJoyPressed & (PAD_A | PAD_B)) {
            /* Clean up and exit */
            sd_clear_sprite();
            gShowDataActive = 0;
        }
    }
}

int Pokedex_IsShowingData(void) {
    return gShowDataActive;
}
