/* pokedex.c — Pokédex list + detail screen.
 *
 * Screen layout uses the same gScrollTileMap tile-grid as menu.c /
 * party_menu.c: screen (col, row) → buffer [(row+2)*SCROLL_MAP_W+(col+2)].
 * Screen is 20 cols × 18 rows.
 *
 * LIST SCREEN
 *   CONTENTS + 7 entries on the left, SEEN/OWN and DATA/CRY/AREA/QUIT
 *   side menu on the right, matching engine/menus/pokedex.asm.
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
#include "player.h"
#include "text.h"
#include "pokemon.h"
#include "constants.h"
#include "../data/font_data.h"
#include "../data/base_stats.h"
#include "../data/dex_data.h"
#include "../data/pokedex_tiles.h"
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
    if (c == '>')  return Font_CharToTile(0xED); /* ▶ filled cursor */
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
#define DEX_SCREEN_SIDE   1
#define DEX_VISIBLE       7     /* entries shown at once */
#define DEX_MAX          151

static int gDexOpen   = 0;
static int gDexScreen = DEX_SCREEN_LIST;
static int gDexCursor = 1;  /* current dex number (1-151) */
static int gDexScroll = 1;  /* top of visible window (1-151) */
static int gDexSideCursor = 0; /* 0=DATA,1=CRY,2=AREA,3=QUIT */

enum {
    DEX_SIDE_DATA = 0,
    DEX_SIDE_CRY  = 1,
    DEX_SIDE_AREA = 2,
    DEX_SIDE_QUIT = 3,
};

static void draw_list_cursor(void) {
    for (int i = 0; i < DEX_VISIBLE; i++) {
        int row = 3 + i * 2;
        int dex_num = gDexScroll + i;
        dex_put(0, row, (uint8_t)ascii_tile((gDexScreen == DEX_SCREEN_LIST && dex_num == gDexCursor) ? '>' : ' '));
    }
}

static void draw_side_cursor(void) {
    for (int i = 0; i < 4; i++) {
        int row = 10 + i * 2;
        dex_put(15, row, (uint8_t)ascii_tile((gDexScreen == DEX_SCREEN_SIDE && gDexSideCursor == i) ? '>' : ' '));
    }
}

/* ---- List screen drawing -------------------------------------------- */
static void draw_list(void) {
    extern uint8_t gScrollTileMap[];
    memset(gScrollTileMap, BLANK_TILE_SLOT, SCROLL_MAP_W * (SCREEN_HEIGHT + 4));

    /* Right divider and side box use the dedicated Pokédex tiles from ASM. */
    dex_put(14, 0, 0x71);
    for (int r = 1; r <= 8; r++)
        dex_put(14, r, (uint8_t)(0x71 ^ ((r + 1) & 1)));
    for (int r = 8; r <= 17; r++)
        dex_put(14, r, (uint8_t)(0x71 ^ ((r - 9) & 1)));

    for (int c = 15; c <= 19; c++)
        dex_put(c, 8, (uint8_t)Font_CharToTile(0x7A));
    dex_put(14, 8, 0x70);

    /* 7 visible entries */
    for (int i = 0; i < DEX_VISIBLE; i++) {
        int dex_num = gDexScroll + i;
        int num_row  = 2 + i * 2;
        int name_row = 3 + i * 2;
        if (dex_num > DEX_MAX) {
            dex_fill(1, num_row, 13, (uint8_t)BLANK_TILE_SLOT);
            dex_fill(1, name_row, 13, (uint8_t)BLANK_TILE_SLOT);
            continue;
        }

        /* ASM list behavior: only owned mons get the Pokeball marker, and it
         * sits on the name row rather than beside the number. */
        if (dex_owned(dex_num))
            dex_put(4, name_row, 0x72);
        else
            dex_put(4, name_row, (uint8_t)BLANK_TILE_SLOT);

        /* dex number: №001 */
        dex_num3(1, num_row, dex_num);

        /* name or dashes */
        if (dex_seen(dex_num) || dex_owned(dex_num)) {
            const char *name = Pokemon_GetName(dex_num);
            int name_col = dex_owned(dex_num) ? 5 : 4;
            dex_fill(name_col, name_row, 10 - (name_col - 4), (uint8_t)BLANK_TILE_SLOT);
            dex_str(name_col, name_row, name ? name : "?");
        } else {
            for (int j = 4; j < 14; j++)
                dex_put(j, name_row, (uint8_t)Font_CharToTile(0xE3)); /* --- */
        }
    }

    dex_str(1, 1, "CONTENTS");

    /* SEEN / OWN counts */
    int seen_cnt = count_bits(wPokedexSeen, 19);
    int own_cnt  = count_bits(wPokedexOwned, 19);
    char buf[5];
    dex_str(16, 2, "SEEN");
    dex_fill(16, 3, 3, (uint8_t)BLANK_TILE_SLOT);
    snprintf(buf, sizeof(buf), "%3d", seen_cnt);
    dex_str(16, 3, buf);
    dex_str(16, 5, "OWN");
    dex_fill(16, 6, 3, (uint8_t)BLANK_TILE_SLOT);
    snprintf(buf, sizeof(buf), "%3d", own_cnt);
    dex_str(16, 6, buf);

    dex_str(16, 10, "DATA");
    dex_str(16, 12, "CRY");
    dex_str(16, 14, "AREA");
    dex_str(16, 16, "QUIT");

    draw_list_cursor();
    draw_side_cursor();
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
    gDexSideCursor = 0;
    if (gDexCursor < 1)    gDexCursor = 1;
    if (gDexCursor > 151)  gDexCursor = 1;
    clamp_scroll();

    /* Hide all sprites (same as Menu_Open) */
    for (int i = 0; i < MAX_SPRITES; i++) wShadowOAM[i].y = 0;
    hWY = 144;
    PokedexTiles_Load();
    draw_list();
}

int Pokedex_IsOpen(void) { return gDexOpen; }

void Pokedex_Tick(void) {
    if (!gDexOpen) return;

    extern uint8_t hJoyPressed;

    if (Pokedex_IsShowingData()) {
        Pokedex_ShowDataTick();
        if (!Pokedex_IsShowingData()) {
            gDexScreen = DEX_SCREEN_LIST;
            Display_SetPalette(0xE4, 0xD0, 0xE0);
            PokedexTiles_Load();
            draw_list();
        }
        return;
    }

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
            if (!dex_seen(gDexCursor))
                return;
            gDexSideCursor = DEX_SIDE_DATA;
            gDexScreen = DEX_SCREEN_SIDE;
            draw_list();
            return;
        }
        if (hJoyPressed & (PAD_B | PAD_START)) {
            gDexOpen = 0;
            Display_LoadMapPalette();
            Map_ReloadGfx();
            Font_Load();
            NPC_ReloadTiles();
            Map_BuildScrollView();
            NPC_BuildView(gScrollPxX, gScrollPxY);
            return;
        }
    }

    if (gDexScreen == DEX_SCREEN_SIDE) {
        if (hJoyPressed & PAD_UP) {
            if (gDexSideCursor > 0) gDexSideCursor--;
            draw_list();
            return;
        }
        if (hJoyPressed & PAD_DOWN) {
            if (gDexSideCursor < DEX_SIDE_QUIT) gDexSideCursor++;
            draw_list();
            return;
        }
        if (hJoyPressed & PAD_B) {
            gDexScreen = DEX_SCREEN_LIST;
            draw_list();
            return;
        }
        if (hJoyPressed & PAD_A) {
            switch (gDexSideCursor) {
            case DEX_SIDE_DATA:
                Pokedex_ShowData(gDexCursor);
                return;
            case DEX_SIDE_CRY:
                Audio_PlayCry(gDexToSpecies[gDexCursor]);
                return;
            case DEX_SIDE_AREA:
                gDexScreen = DEX_SCREEN_LIST;
                draw_list();
                return;
            case DEX_SIDE_QUIT:
                gDexScreen = DEX_SCREEN_LIST;
                draw_list();
                return;
            default:
                return;
            }
        }
        if (hJoyPressed & PAD_START) {
            gDexOpen = 0;
            Display_LoadMapPalette();
            Map_ReloadGfx();
            Font_Load();
            NPC_ReloadTiles();
            Map_BuildScrollView();
            NPC_BuildView(gScrollPxX, gScrollPxY);
            return;
        }
    }
}

/* ---- Seen/Owned flags ------------------------------------------------ */
void Pokedex_SetSeen(int species) {
    if (species < 1 || species > 255) return;
    int dex_num = (int)gSpeciesToDex[species];
    if (dex_num < 1 || dex_num > 151) return;
    int bit = dex_num - 1;
    wPokedexSeen[bit >> 3] |= (uint8_t)(1u << (bit & 7));
}

void Pokedex_SetOwned(int species) {
    if (species < 1 || species > 255) return;
    int dex_num = (int)gSpeciesToDex[species];
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
 *   (1,11):    description text (ASM TextCommandProcessor-style pacing)
 *   Row 17:    border bottom
 * ====================================================================== */

/* ShowData picture constants — mirror the ASM's tilemap picture path.
 * Reserve 49 BG tile slots for the 7x7 front sprite stamped at (1,1). */
#define SD_PIC_TILE_BASE  32
#define SD_PIC_COL        1
#define SD_PIC_ROW        1

/* ShowData state */
static int         gShowDataActive = 0;
static int         gShowDataDex    = 0;
static const char *gShowDataDesc   = NULL;   /* current position in description */
static int         gShowDataCol    = 1;
static int         gShowDataRow    = 11;
static int         gShowDataDone   = 0;      /* description finished, waiting for button */
static int         gShowDataWaitCry = 0;     /* ASM PlayCry blocks before text */
static int         gShowDataPageWait = 0;    /* waiting on a <PAGE>-style advance */
static int         gShowDataDelay = 0;       /* ASM Delay3 before sprite load */
static int         gShowDataOwned = 0;
static int         gShowDataPicActive = 0;   /* picture tiles are still streaming in */
static int         gShowDataPicTile = 0;     /* next 7x7 picture tile to upload */
static int         gShowDataOpenWhite = 0;   /* entering data page through whiteout */
static int         gShowDataCloseWhite = 0;  /* leaving data page through whiteout */
static int         gShowDataWhiteTimer = 0;

static int sd_put_desc_token(uint8_t ch) {
    if (gShowDataCol > 18 || gShowDataRow > 15)
        return 0;
    dex_put(gShowDataCol, gShowDataRow, ch);
    gShowDataCol++;
    return 1;
}

static int sd_put_desc_char(unsigned char c) {
    return sd_put_desc_token((uint8_t)ascii_tile(c));
}

static int sd_step_desc(void) {
    if (!gShowDataDesc || !*gShowDataDesc) {
        gShowDataDone = 1;
        return 0;
    }

    if (*gShowDataDesc == '\\' && gShowDataDesc[1] == 'f') {
        gShowDataDesc += 2;
        gShowDataPageWait = 1;
        return 0;
    }

    if (*gShowDataDesc == '\\' && gShowDataDesc[1] == 'n') {
        gShowDataDesc += 2;
        gShowDataRow += 2;
        gShowDataCol = 1;
        return 0;
    }

    if (*gShowDataDesc == '#') {
        gShowDataDesc++;
        sd_put_desc_token((uint8_t)Font_CharToTile(0x8F)); /* P */
        sd_put_desc_token((uint8_t)Font_CharToTile(0x8E)); /* O */
        sd_put_desc_token((uint8_t)Font_CharToTile(0x8A)); /* K */
        sd_put_desc_token((uint8_t)Font_CharToTile(0xBA)); /* é */
        return 1;
    }

    if (sd_put_desc_char((unsigned char)*gShowDataDesc))
        gShowDataDesc++;
    else
        gShowDataDone = 1;
    return 1;
}

static void sd_render_page(void) {
    while (!gShowDataDone && !gShowDataPageWait && gShowDataDesc && *gShowDataDesc) {
        sd_step_desc();
    }
    if (!gShowDataPageWait && gShowDataDesc && *gShowDataDesc == '\0')
        gShowDataDone = 1;
}

static void sd_draw_page_prompt(void) {
    dex_put(18, 16, (uint8_t)Font_CharToTile(0xEE));
}

static void sd_clear_page_prompt(void) {
    dex_put(18, 16, (uint8_t)BLANK_TILE_SLOT);
}

static void sd_clear_desc_area(void) {
    for (int r = 10; r <= 16; r++) {
        for (int c = 1; c <= 18; c++) {
            dex_put(c, r, (uint8_t)BLANK_TILE_SLOT);
        }
    }
}

static void sd_clear_screen(void) {
    extern uint8_t gScrollTileMap[];
    memset(gScrollTileMap, BLANK_TILE_SLOT,
           SCROLL_MAP_W * (SCREEN_HEIGHT + 4));
}

static void sd_fill_owned_details(int dex_num) {
    const dex_entry_t *e = (dex_num >= 1 && dex_num <= 151) ? &gDexEntries[dex_num] : NULL;
    if (!e || !dex_owned(dex_num))
        return;

    int lb_whole = e->weight / 10;
    int lb_frac  = e->weight % 10;
    char ht_digits[3];
    char in_digits[3];
    char wt_whole[5];

    snprintf(ht_digits, sizeof(ht_digits), "%2d", e->height_ft);
    snprintf(in_digits, sizeof(in_digits), "%02d", e->height_in);
    dex_str(12, 6, ht_digits);
    dex_str(15, 6, in_digits);

    snprintf(wt_whole, sizeof(wt_whole), "%4d", lb_whole);
    dex_str(11, 8, wt_whole);
    dex_put(15, 8, (uint8_t)ascii_tile('.'));
    dex_put(16, 8, (uint8_t)ascii_tile((unsigned char)('0' + lb_frac)));
}

static uint8_t sd_reverse_bits(uint8_t v) {
    v = (uint8_t)(((v & 0xF0u) >> 4) | ((v & 0x0Fu) << 4));
    v = (uint8_t)(((v & 0xCCu) >> 2) | ((v & 0x33u) << 2));
    v = (uint8_t)(((v & 0xAAu) >> 1) | ((v & 0x55u) << 1));
    return v;
}

static void sd_prime_sprite_slots(void) {
    static const uint8_t kBlankTile[TILE_SIZE] = {0};

    for (int ty = 0; ty < 7; ty++) {
        for (int tx = 0; tx < 7; tx++) {
            uint8_t tile_id = (uint8_t)(SD_PIC_TILE_BASE + ty * 7 + tx);
            Display_LoadTile(tile_id, kBlankTile);
            dex_put(SD_PIC_COL + tx, SD_PIC_ROW + ty, tile_id);
        }
    }
}

static void sd_begin_sprite_load(void) {
    gShowDataPicActive = 1;
    gShowDataPicTile = 0;
}

static int sd_step_sprite_load(int dex_num) {
    if (dex_num < 1 || dex_num > 151)
        return 0;

    /* The original path goes through CopyVideoData, which transfers 8 2bpp
     * tiles per frame. Mirror that here so the picture becomes visible over
     * several frames instead of appearing all at once. */
    for (int i = 0; i < 8 && gShowDataPicTile < 49; i++, gShowDataPicTile++) {
        int ty = gShowDataPicTile / 7;
        int tx = gShowDataPicTile % 7;
        int src = ty * 7 + (6 - tx);
        uint8_t flipped[TILE_SIZE];

        for (int row = 0; row < 8; row++) {
            flipped[row * 2 + 0] = sd_reverse_bits(gPokemonFrontSprite[dex_num][src][row * 2 + 0]);
            flipped[row * 2 + 1] = sd_reverse_bits(gPokemonFrontSprite[dex_num][src][row * 2 + 1]);
        }
        Display_LoadTile((uint8_t)(SD_PIC_TILE_BASE + ty * 7 + tx), flipped);
    }

    if (gShowDataPicTile >= 49) {
        gShowDataPicActive = 0;
        return 0;
    }
    return 1;
}

static void sd_clear_sprite(void) {
    for (int ty = 0; ty < 7; ty++) {
        for (int tx = 0; tx < 7; tx++) {
            dex_put(SD_PIC_COL + tx, SD_PIC_ROW + ty, (uint8_t)BLANK_TILE_SLOT);
        }
    }
}

static void sd_draw_screen(int dex_num) {
    extern uint8_t gScrollTileMap[];
    const dex_entry_t *e = (dex_num >= 1 && dex_num <= 151) ? &gDexEntries[dex_num] : NULL;
    static const uint8_t kDexDivider[20] = {
        0x68, 0x69, 0x6B, 0x69, 0x6B, 0x69, 0x6B, 0x69, 0x6B, 0x6B,
        0x6B, 0x6B, 0x69, 0x6B, 0x69, 0x6B, 0x69, 0x6B, 0x69, 0x6A
    };

    /* Clear entire scroll tile map */
    memset(gScrollTileMap, BLANK_TILE_SLOT,
           SCROLL_MAP_W * (SCREEN_HEIGHT + 4));

    /* Pokedex frame tiles from engine/menus/pokedex.asm. */
    dex_put(0,  0,  0x63);
    dex_put(19, 0,  0x65);
    dex_put(0,  17, 0x6C);
    dex_put(19, 17, 0x6E);
    for (int c = 1; c < 19; c++) {
        dex_put(c, 0,  0x64);
        dex_put(c, 17, 0x6F);
    }
    for (int r = 1; r < 17; r++) {
        dex_put(0,  r, 0x66);
        dex_put(19, r, 0x67);
    }

    /* Horizontal divider at row 9 */
    for (int c = 0; c < 20; c++)
        dex_put(c, 9, kDexDivider[c]);

    /* Pokemon name at (9,2) */
    const char *name = Pokemon_GetName((uint8_t)dex_num);
    if (name) dex_str(9, 2, name);

    /* Species category at (9,4) — the raw category string only. */
    if (e && e->category) {
        dex_str(9, 4, e->category);
    }

    /* No.XXX at (2,8) */
    {
        char num_buf[10];
        /* # tile = No. symbol, then . then 3-digit number */
        snprintf(num_buf, sizeof(num_buf), "#.%03d", dex_num);
        dex_str(2, 8, num_buf);
    }

    /* Height / Weight template matches HeightWeightText in the ASM.
     * Prime/double-prime glyphs come from the Pokédex tile sheet at $60/$61. */
    dex_str(9, 6, "HT  ?");
    dex_put(14, 6, 0x60);
    dex_put(15, 6, (uint8_t)ascii_tile('?'));
    dex_put(16, 6, (uint8_t)ascii_tile('?'));
    dex_put(17, 6, 0x61);
    dex_str(9, 8, "WT   ???lb");

    (void)e;
}

void Pokedex_ShowData(int dex_num) {
    if (dex_num < 1 || dex_num > 151) return;

    const dex_entry_t *e = &gDexEntries[dex_num];

    gShowDataActive = 1;
    gShowDataDex    = dex_num;
    gShowDataDone   = 0;
    gShowDataWaitCry = 1;
    gShowDataPageWait = 0;
    gShowDataDelay = 6;
    gShowDataOwned = dex_owned(dex_num);
    gShowDataPicActive = 0;
    gShowDataPicTile = 0;
    gShowDataOpenWhite = 1;
    gShowDataCloseWhite = 0;
    gShowDataWhiteTimer = 8;

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
    sd_clear_sprite();

    /* Disable window layer (text box) */
    hWY = 144;
    sd_clear_screen();
    Display_SetPalette(0x00, 0x00, 0x00);
}

void Pokedex_ShowDataTick(void) {
    if (!gShowDataActive) return;

    if (gShowDataOpenWhite) {
        if (--gShowDataWhiteTimer > 0)
            return;
        gShowDataOpenWhite = 0;
        PokedexTiles_Load();
        sd_draw_screen(gShowDataDex);
        sd_prime_sprite_slots();
        Display_SetPalette(0xE4, 0xD0, 0xE0);
        return;
    }

    if (gShowDataCloseWhite) {
        if (--gShowDataWhiteTimer > 0)
            return;
        gShowDataCloseWhite = 0;
        gShowDataActive = 0;
        return;
    }

    if (gShowDataDelay > 0) {
        gShowDataDelay--;
        if (gShowDataDelay == 0) {
            sd_begin_sprite_load();
        }
        return;
    }

    if (gShowDataPicActive) {
        if (!sd_step_sprite_load(gShowDataDex)) {
            Audio_PlayCry(gDexToSpecies[gShowDataDex]);
        }
        return;
    }

    if (gShowDataWaitCry) {
        if (Audio_IsCryPlaying())
            return;
        gShowDataWaitCry = 0;
        if (gShowDataOwned) {
            sd_fill_owned_details(gShowDataDex);
            sd_render_page();
        }
    }

    if (gShowDataPageWait) {
        sd_draw_page_prompt();
        if (!(hJoyPressed & (PAD_A | PAD_B)))
            return;
        sd_clear_page_prompt();
        sd_clear_desc_area();
        gShowDataPageWait = 0;
        gShowDataRow = 11;
        gShowDataCol = 1;
        sd_render_page();
        return;
    }

    if (gShowDataDone) {
        if (hJoyPressed & (PAD_A | PAD_B)) {
            sd_clear_sprite();
            sd_clear_screen();
            Display_SetPalette(0x00, 0x00, 0x00);
            gShowDataCloseWhite = 1;
            gShowDataWhiteTimer = 8;
        }
    }
}

int Pokedex_IsShowingData(void) {
    return gShowDataActive;
}
