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
#include <string.h>
#include <time.h>

#include "platform/hardware.h"
#include "platform/input.h"
#include "platform/display.h"
#include "platform/audio.h"
#include "platform/save.h"
#include "game/constants.h"
#include "game/overworld.h"      /* gScrollTileMap, SCROLL_MAP_W */
#include "game/player.h"         /* gScrollPxX, gScrollPxY */
#include "game/debug_overlay.h"  /* F1 grid, F3 overlay, F7 battle dump, F8 WRAM dump */
#include "game/player.h"         /* gNoClip (F4) */
#include "platform/input.h"      /* Input_IsRecording/Playing (F9/F10) */
#include "platform/compiler.h"
#include "game/debug_cli.h"

extern int gNoWilds;             /* game.c — suppress wild encounters (F6) */

/* Forward declarations — implemented as the game logic is ported */
extern void GameInit(void);     /* equivalent to Init in home/init.asm */
extern void GameTick(void);     /* equivalent to OverworldLoop / main game SM */

/* Fallback stubs until game logic is ported */
#ifndef _MSC_VER
WEAK void GameInit(void) {
    /* Clear WRAM (home/init.asm zeroes all WRAM on boot) */
    extern void WRAMClear(void);
    WRAMClear();
    /* Try to load save file */
    if (Save_Load() == 0)
        printf("[save] Save loaded OK\n");
    else
        printf("[save] No save file, starting new game\n");
}

WEAK void GameTick(void) {
    /* Placeholder: show a test tile pattern until game logic is ported.
     * Write alternating tile IDs so something visible appears. */
    static uint32_t frame = 0;
    for (int i = 0; i < SCREEN_HEIGHT * SCREEN_WIDTH; i++)
        wTileMap[i] = (uint8_t)((i + frame / 4) & 0xFF);
    frame++;
}
#endif

/* ---- Random entropy -------------------------------------- */
static void update_random(uint32_t frame) {
    /* Approximate GB rDIV timer entropy (increments at ~16384 Hz) */
    uint8_t div_approx = (uint8_t)(frame * 273);  /* 16384/60 ≈ 273 */
    hRandomAdd += div_approx;
    hRandomSub -= div_approx;
}

/* ---- Main loop ------------------------------------------- */
extern int gSkipMenu;  /* game.c — bypass main menu when --skip and save exists */

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--skip") == 0) gSkipMenu = 1;
    }

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

    const uint32_t FRAME_MS = 1000 / 60;  /* 16ms floor — vsync is OFF in display.c so
                                            * SDL_Delay is the sole frame-rate governor.
                                            * Gives ~62.5 Hz; close enough to GB's 59.73 Hz. */
    uint32_t frame = 0;
    int running = 1;

    while (running) {
        uint32_t frame_start = SDL_GetTicks();

        /* --- Event pump (quit / window close) --- */
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = 0;
            if (ev.type == SDL_KEYDOWN &&
                ev.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                if (DebugCLI_ConsoleIsOpen()) {
                    DebugCLI_ConsoleClose();
                    SDL_StopTextInput();
                } else {
                    running = 0;
                }
            }
            /* GRAVE (~) = toggle in-game debug console */
            if (ev.type == SDL_KEYDOWN &&
                ev.key.keysym.scancode == SDL_SCANCODE_GRAVE) {
                if (DebugCLI_ConsoleIsOpen()) {
                    DebugCLI_ConsoleClose();
                    SDL_StopTextInput();
                } else {
                    DebugCLI_ConsoleOpen();
                    SDL_StartTextInput();
                }
            }
            /* Console: Enter = execute, Backspace = delete char */
            if (ev.type == SDL_KEYDOWN && DebugCLI_ConsoleIsOpen()) {
                if (ev.key.keysym.scancode == SDL_SCANCODE_RETURN ||
                    ev.key.keysym.scancode == SDL_SCANCODE_KP_ENTER) {
                    DebugCLI_ConsoleExecute();
                    SDL_StopTextInput();
                } else if (ev.key.keysym.scancode == SDL_SCANCODE_BACKSPACE) {
                    DebugCLI_ConsoleBackspace();
                }
            }
            /* Console: typed characters */
            if (ev.type == SDL_TEXTINPUT && DebugCLI_ConsoleIsOpen()) {
                const char *t = ev.text.text;
                /* Skip backtick/tilde — those toggle the console */
                if (t[0] != '`' && t[0] != '~' && t[1] == '\0')
                    DebugCLI_ConsoleAddChar(t[0]);
            }
            /* F1 = debug grid */
            if (ev.type == SDL_KEYDOWN &&
                ev.key.keysym.scancode == SDL_SCANCODE_F1)
                Debug_PrintGrid();
            /* F2 = bug report: screenshot + interactive prompt */
            if (ev.type == SDL_KEYDOWN &&
                ev.key.keysym.scancode == SDL_SCANCODE_F2) {
                /* Generate timestamped screenshot path in bugs/inbox/ */
                char ss_path[256];
                time_t now = time(NULL);
                snprintf(ss_path, sizeof(ss_path),
                         "bugs/inbox/screenshot_%lu.bmp", (unsigned long)now);
#ifdef _WIN32
                system("if not exist bugs\\inbox mkdir bugs\\inbox");
#else
                system("mkdir -p bugs/inbox");
#endif
                if (Display_SaveScreenshot(ss_path) == 0) {
                    printf("[bug] Screenshot saved: %s\n", ss_path);
                    char cmd[512];
#ifdef _WIN32
                    snprintf(cmd, sizeof(cmd),
                             "start cmd /k python tools\\bug_report.py \"%s\"", ss_path);
#else
                    snprintf(cmd, sizeof(cmd),
                             "python3 tools/bug_report.py \"%s\" &", ss_path);
#endif
                    system(cmd);
                } else {
                    printf("[bug] Screenshot failed.\n");
                }
            }
            /* F3 = toggle block-ID overlay; Shift+F3 = toggle collision overlay */
            if (ev.type == SDL_KEYDOWN &&
                ev.key.keysym.scancode == SDL_SCANCODE_F3) {
                if (ev.key.keysym.mod & KMOD_SHIFT) {
                    int on = !Debug_CollisionOverlayOn();
                    Debug_SetCollisionOverlay(on);
                    printf("[debug] Collision overlay %s\n", on ? "ON" : "OFF");
                } else {
                    int on = !Display_GetBlockIDOverlay();
                    Display_SetBlockIDOverlay(on);
                    printf("[debug] Block-ID overlay %s\n", on ? "ON" : "OFF");
                }
            }
            /* F4 = toggle no-clip */
            if (ev.type == SDL_KEYDOWN &&
                ev.key.keysym.scancode == SDL_SCANCODE_F4) {
                gNoClip = !gNoClip;
                printf("[debug] No-clip %s\n", gNoClip ? "ON" : "OFF");
            }
            /* F6 = toggle wild encounter suppression */
            if (ev.type == SDL_KEYDOWN &&
                ev.key.keysym.scancode == SDL_SCANCODE_F6) {
                gNoWilds = !gNoWilds;
                printf("[debug] Wild encounters %s\n", gNoWilds ? "OFF" : "ON");
            }
            /* F5 = quick save */
            if (ev.type == SDL_KEYDOWN &&
                ev.key.keysym.scancode == SDL_SCANCODE_F5) {
                if (Save_Write() == 0) printf("[save] Saved.\n");
            }
            /* F7 = battle state dump  (Shift+F7 toggles combat log) */
            if (ev.type == SDL_KEYDOWN &&
                ev.key.keysym.scancode == SDL_SCANCODE_F7) {
                if (ev.key.keysym.mod & KMOD_SHIFT) {
                    int on = !Debug_CombatLogOn();
                    Debug_SetCombatLog(on);
                } else {
                    Debug_PrintBattleState();
                }
            }
            /* F8 = WRAM dump */
            if (ev.type == SDL_KEYDOWN &&
                ev.key.keysym.scancode == SDL_SCANCODE_F8) {
                Debug_DumpWRAM();
            }
            /* F9 = toggle input recording */
            if (ev.type == SDL_KEYDOWN &&
                ev.key.keysym.scancode == SDL_SCANCODE_F9) {
                if (Input_IsRecording()) {
                    Input_StopRecording();
                } else {
                    char path[256];
                    snprintf(path, sizeof(path),
                             "bugs/recording_%lu.bin", (unsigned long)time(NULL));
                    Input_StartRecording(path);
                }
            }
            /* F10 = toggle input playback (most recent recording) */
            if (ev.type == SDL_KEYDOWN &&
                ev.key.keysym.scancode == SDL_SCANCODE_F10) {
                if (Input_IsPlaying()) {
                    Input_StopPlayback();
                } else {
                    Input_StartPlayback("bugs/recording_latest.bin");
                }
            }
            /* F11 = save state */
            if (ev.type == SDL_KEYDOWN &&
                ev.key.keysym.scancode == SDL_SCANCODE_F11) {
                if (Save_StateWrite("bugs/savestate.bin") == 0)
                    printf("[state] State saved.\n");
                else
                    printf("[state] State save failed.\n");
            }
            /* F12 = load state */
            if (ev.type == SDL_KEYDOWN &&
                ev.key.keysym.scancode == SDL_SCANCODE_F12) {
                if (Save_StateLoad("bugs/savestate.bin") == 0) {
                    extern void Map_Load(uint8_t map_id);
                    extern void NPC_Load(void);
                    extern void BattleUI_Restore(void);
                    Map_Load(wCurMap);
                    NPC_Load();
                    if (Save_StateWasBattle()) BattleUI_Restore();
                    Player_IgnoreInputFrames(4);
                    printf("[state] State loaded.\n");
                } else {
                    printf("[state] No save state found.\n");
                }
            }
        }

        /* --- Frame pipeline (mirrors GB VBlank ISR order) --- */
        update_random(frame);
        Input_Update();         /* hJoyInput → hJoyHeld/Pressed/Released */
        hVBlankOccurred = 1;
        hFrameCounter   = (hFrameCounter - 1) & 0xFF;

        GameTick();             /* all game logic */
        Audio_Update();         /* audio sequencer tick */
        if (Debug_CollisionOverlayOn()) Debug_UpdateOverlay();
        DebugCLI_ConsoleRender();
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
