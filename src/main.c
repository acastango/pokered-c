/* main.c — Pokémon Red native PC port entry point
 *
 * Frame loop (60 fps — matches original GB VBlank rate):
 *   The GB VBlank fires at ~59.73 Hz.  All timing-sensitive systems
 *   (audio, hFrameCounter, battle animations, text delays) run every VBlank.
 *   Overworld IDLE runs at ~30 Hz because OverworldLoop calls DelayFrame
 *   TWICE before processing input; GameTick replicates this with a skip-flag
 *   when not mid-step/dialog/warp.  Mid-step, battle, and menus run at 60 Hz.
 *   1. platform_input()    → hJoyInput / hJoyPressed / hJoyHeld
 *   2. game_tick()         → all game logic (skips idle frames internally)
 *   3. platform_render()   → wTileMap + wShadowOAM → SDL display
 *   4. platform_audio()    → audio engine sequencer tick (always 60 Hz)
 *   5. SDL_Delay / vsync   → 60 fps timing
 *
 * Bank switching is a no-op: all functions are in one address space.
 * HRAM is replaced by plain C globals in platform/hardware.h.
 */
#include <SDL2/SDL.h>
/* SDL.h redefines main as SDL_main for SDL2main integration.
 * We bypass SDL2main (using -mconsole on Windows) so we need plain main. */
#ifdef main
#  undef main
#endif
#include <stdio.h>
#include <stdint.h>

#include "platform/hardware.h"
#include "platform/input.h"
#include "platform/display.h"
#include "platform/audio.h"
#include "platform/save.h"
#include "game/constants.h"
#include "game/overworld.h"  /* gScrollTileMap, SCROLL_MAP_W */
#include "game/player.h"     /* gScrollPxX, gScrollPxY */

/* Forward declarations — implemented as the game logic is ported */
extern void GameInit(void);     /* equivalent to Init in home/init.asm */
extern void GameTick(void);     /* equivalent to OverworldLoop / main game SM */

/* Fallback stubs until game logic is ported */
__attribute__((weak)) void GameInit(void) {
    /* Clear WRAM (home/init.asm zeroes all WRAM on boot) */
    extern void WRAMClear(void);
    WRAMClear();
    /* Try to load save file */
    if (Save_Load() == 0)
        printf("[save] Save loaded OK\n");
    else
        printf("[save] No save file, starting new game\n");
}

__attribute__((weak)) void GameTick(void) {
    /* Placeholder: show a test tile pattern until game logic is ported.
     * Write alternating tile IDs so something visible appears. */
    static uint32_t frame = 0;
    for (int i = 0; i < SCREEN_HEIGHT * SCREEN_WIDTH; i++)
        wTileMap[i] = (uint8_t)((i + frame / 4) & 0xFF);
    frame++;
}

/* ---- Random entropy -------------------------------------- */
static void update_random(uint32_t frame) {
    /* Approximate GB rDIV timer entropy (increments at ~16384 Hz) */
    uint8_t div_approx = (uint8_t)(frame * 273);  /* 16384/60 ≈ 273 */
    hRandomAdd += div_approx;
    hRandomSub -= div_approx;
}

/* ---- Main loop ------------------------------------------- */
int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    if (Display_Init() != 0) {
        fprintf(stderr, "Display_Init failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    if (Audio_Init() != 0) {
        fprintf(stderr, "Audio_Init failed: %s (continuing without audio)\n",
                SDL_GetError());
    }

    Input_Init();
    GameInit();

    const uint32_t FRAME_MS = 1000 / 60;  /* 16ms — matches original ~60 Hz VBlank rate */
    uint32_t frame = 0;
    int running = 1;

    while (running) {
        uint32_t frame_start = SDL_GetTicks();

        /* --- Event pump (quit / window close) --- */
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = 0;
            if (ev.type == SDL_KEYDOWN &&
                ev.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                running = 0;
            /* F5 = quick save */
            if (ev.type == SDL_KEYDOWN &&
                ev.key.keysym.scancode == SDL_SCANCODE_F5) {
                if (Save_Write() == 0) printf("[save] Saved.\n");
            }
        }

        /* --- Frame pipeline (mirrors GB VBlank ISR order) --- */
        update_random(frame);
        Input_Update();         /* hJoyInput → hJoyHeld/Pressed/Released */
        hVBlankOccurred = 1;
        hFrameCounter   = (hFrameCounter - 1) & 0xFF;

        GameTick();             /* all game logic */
        Audio_Update();         /* audio sequencer tick */
        Display_RenderScrolled(gScrollPxX, gScrollPxY,
                               gScrollTileMap, SCROLL_MAP_W);

        hVBlankOccurred = 0;

        /* --- Frame timing (SDL vsync may handle this already) --- */
        uint32_t elapsed = SDL_GetTicks() - frame_start;
        if (elapsed < FRAME_MS)
            SDL_Delay(FRAME_MS - elapsed);

        frame++;
    }

    /* Autosave on quit */
    Save_Write();

    Audio_Quit();
    Input_Quit();
    Display_Quit();
    SDL_Quit();
    return 0;
}
