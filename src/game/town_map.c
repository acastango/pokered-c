#include "town_map.h"

#include "overworld.h"
#include "npc.h"
#include "player.h"
#include "../data/town_map_data.h"
#include "../data/font_data.h"
#include "../platform/hardware.h"

/* Display labels in exact TownMapOrder sequence from ASM. */
static const char *kTownMapOrderNames[] = {
    "PALLET TOWN", "ROUTE 1", "VIRIDIAN CITY", "ROUTE 2", "VIRIDIAN FOREST",
    "DIGLETTS CAVE", "PEWTER CITY", "ROUTE 3", "MT.MOON", "ROUTE 4",
    "CERULEAN CITY", "ROUTE 24", "ROUTE 25", "SEA COTTAGE", "ROUTE 5",
    "ROUTE 6", "VERMILION CITY", "S.S.ANNE", "ROUTE 9", "ROCK TUNNEL",
    "ROUTE 10", "LAVENDER TOWN", "POKEMON TOWER", "ROUTE 8", "ROUTE 7",
    "CELADON CITY", "SAFFRON CITY", "ROUTE 11", "ROUTE 12", "ROUTE 13",
    "ROUTE 14", "ROUTE 15", "ROUTE 16", "ROUTE 17", "ROUTE 18",
    "FUCHSIA CITY", "SAFARI ZONE", "ROUTE 19", "SEAFOAM ISLANDS",
    "ROUTE 20", "CINNABAR ISLAND", "ROUTE 21", "ROUTE 22", "ROUTE 23",
    "VICTORY ROAD", "INDIGO PLATEAU", "POWER PLANT",
};
#define TM_NAME_COUNT ((int)(sizeof(kTownMapOrderNames) / sizeof(kTownMapOrderNames[0])))

/* Cursor sprite uses OAM slots 4..7 and sprite tile slots 4..7 (ASM-style) */
#define TM_CURSOR_OAM_BASE 4
#define TM_CURSOR_TILE_BASE 4

/* Marker blink cadence (matches classic map cursor blink feel). */
#define BLINK_HALF_PERIOD 25

static int gTownMapOpen = 0;
static int gTownMapIdx = 0;
static int gBlinkCounter = 0;
static int gBlinkOn = 1;

static int tm_order_count(void) {
    return (gTownMapOrderCount < TM_NAME_COUNT) ? gTownMapOrderCount : TM_NAME_COUNT;
}

static void tm_set(int col, int row, uint8_t tile) {
    if (col < 0 || col >= SCREEN_WIDTH || row < 0 || row >= SCREEN_HEIGHT) return;
    gScrollTileMap[(row + 2) * SCROLL_MAP_W + (col + 2)] = tile;
}

static void tm_clear(void) {
    for (int r = 0; r < SCREEN_HEIGHT; r++)
        for (int c = 0; c < SCREEN_WIDTH; c++)
            tm_set(c, r, BLANK_TILE_SLOT);
}

static void tm_clear_row(int row) {
    for (int c = 0; c < SCREEN_WIDTH; c++)
        tm_set(c, row, BLANK_TILE_SLOT);
}

static uint8_t tm_char(unsigned char c) {
    if (c >= 'A' && c <= 'Z') return (uint8_t)Font_CharToTile((uint8_t)(0x80 + (c - 'A')));
    if (c >= 'a' && c <= 'z') return (uint8_t)Font_CharToTile((uint8_t)(0xA0 + (c - 'a')));
    if (c >= '0' && c <= '9') return (uint8_t)Font_CharToTile((uint8_t)(0xF6 + (c - '0')));
    if (c == ' ') return (uint8_t)Font_CharToTile(0x7F);
    if (c == '.') return (uint8_t)Font_CharToTile(0xE8);
    if (c == '-') return (uint8_t)Font_CharToTile(0xE3);
    return (uint8_t)Font_CharToTile(0x7F);
}

static void tm_puts(int col, int row, const char *s) {
    while (*s && col < SCREEN_WIDTH) {
        tm_set(col++, row, tm_char((unsigned char)*s));
        s++;
    }
}

static void tm_draw_world_map(void) {
    int out = 0;
    for (int i = 0; gTownMapCompressedMap[i] != 0 && out < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        uint8_t packed = gTownMapCompressedMap[i];
        int run = packed & 0x0F;
        uint8_t tile = (uint8_t)(TOWN_MAP_WORLD_TILE_BASE + ((packed >> 4) & 0x0F));
        while (run-- > 0 && out < SCREEN_WIDTH * SCREEN_HEIGHT) {
            int row = out / SCREEN_WIDTH;
            int col = out % SCREEN_WIDTH;
            tm_set(col, row, tile);
            out++;
        }
    }
}

static void tm_hide_cursor_oam(void) {
    for (int i = 0; i < 4; i++) {
        wShadowOAM[TM_CURSOR_OAM_BASE + i].y = 0;
        wShadowOAM[TM_CURSOR_OAM_BASE + i].x = 0;
        wShadowOAM[TM_CURSOR_OAM_BASE + i].tile = 0;
        wShadowOAM[TM_CURSOR_OAM_BASE + i].flags = 0;
    }
}

static uint8_t tm_get_map_coords_packed(uint8_t map_id) {
    if (map_id < TOWN_MAP_FIRST_INDOOR_MAP) {
        return gTownMapExternalCoords[map_id];
    }
    for (int i = 0; i < gTownMapInternalCoordsCount; i++) {
        if (map_id < gTownMapInternalCoords[i].group_end) {
            return gTownMapInternalCoords[i].coords;
        }
    }
    return gTownMapExternalCoords[0];
}

static void tm_get_map_coords(uint8_t map_id, int *x, int *y) {
    uint8_t packed = tm_get_map_coords_packed(map_id);
    *x = packed & 0x0F;
    *y = (packed >> 4) & 0x0F;
}

static void tm_set_cursor_oam(int map_x, int map_y) {
    /* Exact ASM coordinate domain: b/c are OAM bytes, not screen pixels.
     * TownMapCoordsToOAMCoords: b=y*8+24, c=x*8+24
     * WriteTownMapSpriteOAM: y=b-3, x=c-4 for top-left tile of the 16x16 cursor. */
    int oy = 24 + (map_y * 8) - 3;
    int ox = 24 + (map_x * 8) - 4;

    wShadowOAM[TM_CURSOR_OAM_BASE + 0].y = (uint8_t)oy;
    wShadowOAM[TM_CURSOR_OAM_BASE + 0].x = (uint8_t)ox;
    wShadowOAM[TM_CURSOR_OAM_BASE + 0].tile = (uint8_t)(TM_CURSOR_TILE_BASE + 0);
    wShadowOAM[TM_CURSOR_OAM_BASE + 0].flags = 0;

    wShadowOAM[TM_CURSOR_OAM_BASE + 1].y = (uint8_t)oy;
    wShadowOAM[TM_CURSOR_OAM_BASE + 1].x = (uint8_t)(ox + 8);
    wShadowOAM[TM_CURSOR_OAM_BASE + 1].tile = (uint8_t)(TM_CURSOR_TILE_BASE + 1);
    wShadowOAM[TM_CURSOR_OAM_BASE + 1].flags = 0;

    wShadowOAM[TM_CURSOR_OAM_BASE + 2].y = (uint8_t)(oy + 8);
    wShadowOAM[TM_CURSOR_OAM_BASE + 2].x = (uint8_t)ox;
    wShadowOAM[TM_CURSOR_OAM_BASE + 2].tile = (uint8_t)(TM_CURSOR_TILE_BASE + 2);
    wShadowOAM[TM_CURSOR_OAM_BASE + 2].flags = 0;

    wShadowOAM[TM_CURSOR_OAM_BASE + 3].y = (uint8_t)(oy + 8);
    wShadowOAM[TM_CURSOR_OAM_BASE + 3].x = (uint8_t)(ox + 8);
    wShadowOAM[TM_CURSOR_OAM_BASE + 3].tile = (uint8_t)(TM_CURSOR_TILE_BASE + 3);
    wShadowOAM[TM_CURSOR_OAM_BASE + 3].flags = 0;
}

static int tm_pick_index_for_map(uint8_t map_id) {
    int cx, cy;
    int count = tm_order_count();
    tm_get_map_coords(map_id, &cx, &cy);
    for (int i = 0; i < count; i++) {
        int ox, oy;
        tm_get_map_coords(gTownMapOrderMapIds[i], &ox, &oy);
        if (ox == cx && oy == cy) return i;
    }
    return 0;
}

static void tm_render(void) {
    int map_x, map_y;
    int count = tm_order_count();
    if (count <= 0) return;
    if (gTownMapIdx >= count) gTownMapIdx = 0;
    if (gTownMapIdx < 0) gTownMapIdx = count - 1;

    tm_get_map_coords(gTownMapOrderMapIds[gTownMapIdx], &map_x, &map_y);
    tm_clear();
    tm_draw_world_map();
    tm_clear_row(0);
    tm_puts(1, 0, kTownMapOrderNames[gTownMapIdx]);

    if (gBlinkOn) tm_set_cursor_oam(map_x, map_y);
    else tm_hide_cursor_oam();
}

static void tm_close(void) {
    gTownMapOpen = 0;
    hWY = SCREEN_HEIGHT_PX;
    Map_BuildScrollView();
    NPC_BuildView(gScrollPxX, gScrollPxY);
}

void TownMap_Open(void) {
    gTownMapOpen = 1;
    gTownMapIdx = tm_pick_index_for_map(wCurMap);
    gBlinkCounter = 0;
    gBlinkOn = 1;
    TownMapData_LoadTiles();
    for (int i = 0; i < MAX_SPRITES; i++) wShadowOAM[i].y = 0;
    hWY = SCREEN_HEIGHT_PX; /* no window layer */
    tm_render();
}

int TownMap_IsOpen(void) {
    return gTownMapOpen;
}

void TownMap_Tick(void) {
    if (!gTownMapOpen) return;
    int count = tm_order_count();
    if (count <= 0) return;

    if (hJoyPressed & PAD_UP) {
        gTownMapIdx++;
        if (gTownMapIdx >= count) gTownMapIdx = 0;
        gBlinkCounter = 0;
        gBlinkOn = 1;
        tm_render();
        return;
    }
    if (hJoyPressed & PAD_DOWN) {
        gTownMapIdx--;
        if (gTownMapIdx < 0) gTownMapIdx = count - 1;
        gBlinkCounter = 0;
        gBlinkOn = 1;
        tm_render();
        return;
    }
    if (hJoyPressed & (PAD_A | PAD_B)) {
        tm_hide_cursor_oam();
        tm_close();
        return;
    }

    gBlinkCounter++;
    if (gBlinkCounter >= BLINK_HALF_PERIOD) {
        gBlinkCounter = 0;
        gBlinkOn ^= 1;
        tm_render();
    }
}
