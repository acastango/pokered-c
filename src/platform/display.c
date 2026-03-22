/* display.c — Render wTileMap (BG) + wShadowOAM (sprites) via SDL2
 *
 * Rendering pipeline:
 *   1. wTileMap[20×18] tile IDs → lookup tile_gfx[tile_id][16 bytes]
 *   2. 2bpp decode → 4-color index → palette → RGBA pixel
 *   3. wShadowOAM[40] sprites blit on top (Y-16, X-8 offsets)
 *   4. Scale by DISPLAY_SCALE and present
 */
#include "display.h"
#include "hardware.h"
#include "../game/constants.h"
#include <SDL2/SDL.h>
#include <string.h>

/* SDL objects */
static SDL_Window   *window   = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture  *fb_tex   = NULL;  /* 160×144 framebuffer */

/* BG tile GFX cache: up to 256 tiles × 16 bytes (tileset, font, box) */
static uint8_t  tile_gfx[256][TILE_SIZE];
static int      num_tiles_loaded = 0;

/* Sprite (OBJ) tile GFX cache: separate from BG, mirrors GB hardware OBJ table */
static uint8_t  sprite_tile_gfx[256][TILE_SIZE];

/* 4-color palettes (RGBA) decoded from rBGP/rOBP0/rOBP1
 * GB color 0 = lightest (white), 3 = darkest (black) */
static SDL_Color bg_palette[4];
static SDL_Color obp0_palette[4];
static SDL_Color obp1_palette[4];

/* Raw framebuffer: 160×144 RGBA */
static uint32_t fb[SCREEN_WIDTH_PX * SCREEN_HEIGHT_PX];

/* BG color-index buffer: tracks the 2bpp palette index (0-3) written to each
 * screen pixel by the BG tile pass.  Used to implement OAM_FLAG_PRIORITY:
 * sprites with bit 7 set only draw where BG index == 0 (transparent/white),
 * matching the GB hardware's OBJ-behind-BG behavior for grass tiles. */
static uint8_t bg_idx[SCREEN_WIDTH_PX * SCREEN_HEIGHT_PX];

/* Default DMG palette shades */
static const SDL_Color DMG_SHADES[4] = {
    {0xE0, 0xF8, 0xD0, 0xFF},  /* color 0: lightest */
    {0x88, 0xC0, 0x70, 0xFF},  /* color 1 */
    {0x34, 0x68, 0x56, 0xFF},  /* color 2 */
    {0x08, 0x18, 0x20, 0xFF},  /* color 3: darkest */
};

static void decode_palette(uint8_t reg, SDL_Color out[4]) {
    for (int i = 0; i < 4; i++)
        out[i] = DMG_SHADES[(reg >> (i * 2)) & 3];
}

int Display_Init(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return -1;

    window = SDL_CreateWindow(
        "Pokémon Red",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH_PX * DISPLAY_SCALE,
        SCREEN_HEIGHT_PX * DISPLAY_SCALE,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!window) return -1;

    renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) return -1;

    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH_PX, SCREEN_HEIGHT_PX);
    SDL_RenderSetIntegerScale(renderer, SDL_TRUE);

    fb_tex = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH_PX, SCREEN_HEIGHT_PX);
    if (!fb_tex) return -1;

    /* Default palette: rBGP = 0xE4 (00=white 01=light 10=dark 11=black) */
    Display_SetPalette(0xE4, 0xE4, 0xE4);
    return 0;
}

void Display_LoadTileset(const uint8_t *gfx, int num_tiles) {
    if (num_tiles > 256) num_tiles = 256;
    memcpy(tile_gfx, gfx, num_tiles * TILE_SIZE);
    num_tiles_loaded = num_tiles;
}

void Display_LoadTile(uint8_t tile_id, const uint8_t *gfx) {
    memcpy(tile_gfx[tile_id], gfx, TILE_SIZE);
}

void Display_GetTile(uint8_t tile_id, uint8_t out[16]) {
    memcpy(out, tile_gfx[tile_id & 0xFF], TILE_SIZE);
}

void Display_LoadSpriteTile(uint8_t tile_id, const uint8_t *gfx) {
    memcpy(sprite_tile_gfx[tile_id], gfx, TILE_SIZE);
}

void Display_SetPalette(uint8_t bgp, uint8_t obp0, uint8_t obp1) {
    decode_palette(bgp,  bg_palette);
    decode_palette(obp0, obp0_palette);
    decode_palette(obp1, obp1_palette);
}

/* Decode one 8×8 tile into fb at pixel (px, py) */
static void blit_tile(int px, int py, uint8_t tile_id,
                      const SDL_Color pal[4], int flip_x, int flip_y,
                      int behind_bg) {
    const uint8_t *t = tile_gfx[tile_id & 0xFF];
    for (int row = 0; row < 8; row++) {
        int sy = flip_y ? (7 - row) : row;
        uint8_t lo = t[sy * 2];
        uint8_t hi = t[sy * 2 + 1];
        int dy = py + row;
        if (dy < 0 || dy >= SCREEN_HEIGHT_PX) continue;
        for (int col = 0; col < 8; col++) {
            int sx    = flip_x ? col : (7 - col);
            int color = ((hi >> sx) & 1) << 1 | ((lo >> sx) & 1);
            int dx    = px + col;
            if (dx < 0 || dx >= SCREEN_WIDTH_PX) continue;
            SDL_Color c = pal[color];
            fb[dy * SCREEN_WIDTH_PX + dx] =
                ((uint32_t)c.r << 24) | ((uint32_t)c.g << 16) |
                ((uint32_t)c.b <<  8) | 0xFF;
            bg_idx[dy * SCREEN_WIDTH_PX + dx] = (uint8_t)color;
        }
    }
}

void Display_Render(void) {
    /* 1. Draw background tiles from wTileMap (20×18 visible) */
    for (int ty = 0; ty < SCREEN_HEIGHT; ty++) {
        for (int tx = 0; tx < SCREEN_WIDTH; tx++) {
            uint8_t tile_id = wTileMap[ty * SCREEN_WIDTH + tx];
            blit_tile(tx * TILE_PX, ty * TILE_PX, tile_id,
                      bg_palette, 0, 0, 0);
        }
    }

    /* 2. Draw sprites from wShadowOAM, lower index = higher priority.
     * Process in reverse order so sprite 0 appears on top.
     * Sprite tiles read from sprite_tile_gfx (separate from BG tile_gfx).
     * OAM_FLAG_PRIORITY (bit 7): sprite renders behind BG colors 1-3, matching
     * the GB hardware OBJ-behind-BG behavior used for the grass overlay effect. */
    for (int i = MAX_SPRITES - 1; i >= 0; i--) {
        oam_entry_t *s = &wShadowOAM[i];
        if (s->y == 0 || s->y >= 160) continue;  /* off-screen */
        int px      = (int)s->x - OAM_X_OFS;
        int py      = (int)s->y - OAM_Y_OFS;
        int flipx   = (s->flags & OAM_FLAG_FLIP_X) != 0;
        int flipy   = (s->flags & OAM_FLAG_FLIP_Y) != 0;
        int behind  = (s->flags & OAM_FLAG_PRIORITY) != 0;
        const SDL_Color *pal = (s->flags & OAM_FLAG_PALETTE) ? obp1_palette : obp0_palette;
        const uint8_t *t = sprite_tile_gfx[s->tile & 0xFF];
        for (int row = 0; row < 8; row++) {
            int sy = flipy ? (7 - row) : row;
            uint8_t lo = t[sy * 2];
            uint8_t hi = t[sy * 2 + 1];
            int dy = py + row;
            if (dy < 0 || dy >= SCREEN_HEIGHT_PX) continue;
            for (int col = 0; col < 8; col++) {
                int sx    = flipx ? col : (7 - col);
                int color = ((hi >> sx) & 1) << 1 | ((lo >> sx) & 1);
                if (color == 0) continue;  /* transparent */
                int dx = px + col;
                if (dx < 0 || dx >= SCREEN_WIDTH_PX) continue;
                if (behind && bg_idx[dy * SCREEN_WIDTH_PX + dx] != 0) continue;
                SDL_Color c = pal[color];
                fb[dy * SCREEN_WIDTH_PX + dx] =
                    ((uint32_t)c.r << 24) | ((uint32_t)c.g << 16) |
                    ((uint32_t)c.b <<  8) | 0xFF;
            }
        }
    }

    /* 3. Upload framebuffer to GPU texture and present */
    SDL_UpdateTexture(fb_tex, NULL, fb, SCREEN_WIDTH_PX * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, fb_tex, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void Display_RenderScrolled(int px, int py, const uint8_t *tile_map, int stride) {
    /* Draw background: tile_map is (SCREEN_WIDTH+4) x (SCREEN_HEIGHT+4).
     * Buffer tile (bx,by) renders at screen pixel (bx*8 - 16 + px, by*8 - 16 + py).
     * At px=0,py=0: buffer tile (2,2) -> pixel (0,0) — normal 20x18 view.
     * At px=+16: buffer tile (0,2) -> pixel (0,0) — shows two tiles left (old pos).
     * Two-tile margin required because each step shifts the camera by 2 tiles. */
    for (int by = 0; by < SCREEN_HEIGHT + 4; by++) {
        for (int bx = 0; bx < SCREEN_WIDTH + 4; bx++) {
            int sx = bx * TILE_PX - 2 * TILE_PX + px;
            int sy = by * TILE_PX - 2 * TILE_PX + py;
            if (sx + TILE_PX <= 0 || sx >= SCREEN_WIDTH_PX) continue;
            if (sy + TILE_PX <= 0 || sy >= SCREEN_HEIGHT_PX) continue;
            uint8_t tid = tile_map[by * stride + bx];
            blit_tile(sx, sy, tid, bg_palette, 0, 0, 0);
        }
    }

    /* Draw sprites — same as Display_Render, reads sprite_tile_gfx */
    for (int i = MAX_SPRITES - 1; i >= 0; i--) {
        oam_entry_t *s = &wShadowOAM[i];
        if (s->y == 0 || s->y >= 160) continue;
        int spx    = (int)s->x - OAM_X_OFS;
        int spy    = (int)s->y - OAM_Y_OFS;
        int flipx  = (s->flags & OAM_FLAG_FLIP_X) != 0;
        int flipy  = (s->flags & OAM_FLAG_FLIP_Y) != 0;
        int behind = (s->flags & OAM_FLAG_PRIORITY) != 0;
        const SDL_Color *pal = (s->flags & OAM_FLAG_PALETTE) ? obp1_palette : obp0_palette;
        const uint8_t *t = sprite_tile_gfx[s->tile & 0xFF];
        for (int row = 0; row < 8; row++) {
            int sy2 = flipy ? (7 - row) : row;
            uint8_t lo = t[sy2 * 2];
            uint8_t hi = t[sy2 * 2 + 1];
            int dy = spy + row;
            if (dy < 0 || dy >= SCREEN_HEIGHT_PX) continue;
            for (int col = 0; col < 8; col++) {
                int sx2   = flipx ? col : (7 - col);
                int color = ((hi >> sx2) & 1) << 1 | ((lo >> sx2) & 1);
                if (color == 0) continue;
                int dx = spx + col;
                if (dx < 0 || dx >= SCREEN_WIDTH_PX) continue;
                if (behind && bg_idx[dy * SCREEN_WIDTH_PX + dx] != 0) continue;
                SDL_Color c = pal[color];
                fb[dy * SCREEN_WIDTH_PX + dx] =
                    ((uint32_t)c.r << 24) | ((uint32_t)c.g << 16) |
                    ((uint32_t)c.b <<  8) | 0xFF;
            }
        }
    }

    /* Text box overlay: wTileMap rows 12–17 (6-row box matching original).
     * Detect open box by checking the top-left border tile at row 12.
     * draw_box_border sets it to the ┌ tile (≠0); Text_Close clears it to 0. */
    if (wTileMap[12 * SCREEN_WIDTH] != 0) {
        for (int ty = 12; ty < 18; ty++) {
            for (int tx = 0; tx < SCREEN_WIDTH; tx++) {
                uint8_t tid = wTileMap[ty * SCREEN_WIDTH + tx];
                blit_tile(tx * TILE_PX, ty * TILE_PX, tid, bg_palette, 0, 0, 0);
            }
        }
    }

    SDL_UpdateTexture(fb_tex, NULL, fb, SCREEN_WIDTH_PX * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, fb_tex, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void Display_Quit(void) {
    if (fb_tex)   SDL_DestroyTexture(fb_tex);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window)   SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}
