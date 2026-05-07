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
#include <stdlib.h>

#include "platform/hardware.h"
#include "platform/input.h"
#include "platform/display.h"
#include "platform/audio.h"
#include "platform/save.h"
#include "game/constants.h"
#include "game/overworld.h"      /* gScrollTileMap, SCROLL_MAP_W */
#include "game/player.h"         /* gScrollPxX, gScrollPxY */
#include "game/npc.h"            /* NPC_BuildView */
#include "game/debug_overlay.h"  /* F1 grid, F3 overlay, F7 battle dump, F8 WRAM dump */
#include "game/player.h"         /* gNoClip (F4) */
#include "platform/input.h"      /* Input_IsRecording/Playing (F9/F10) */
#include "platform/compiler.h"
#include "game/debug_cli.h"

extern int gNoWilds;             /* game.c — suppress wild encounters (F6) */

/* ---- Rewind ring buffer ------------------------------------------ */
#define REWIND_SLOTS          1800 /* 30 seconds at true 60fps capture */
#define REWIND_SNAPSHOT_EVERY 1    /* frame-perfect capture */
static int s_rw_seq[REWIND_SLOTS];
static int s_rw_len = 0;
static int s_rw_cursor = -1;       /* index into s_rw_seq */
static int s_rw_next_slot = 0;     /* ring slot index */
static uint32_t s_rw_frame_div = 0;
static uint8_t *s_rw_data = NULL;
static size_t s_rw_state_size = 0;

static int load_state_for_runtime_mem(const uint8_t *src) {
    if (!src) return 0;
    if (Save_StateLoadFromBuffer(src, s_rw_state_size) == 0) {
        extern void Map_Load(uint8_t map_id);
        extern void NPC_Load(void);
        extern void BattleUI_Restore(void);
        static int s_loaded_map = -1;
        if (Save_StateWasBattle()) {
            BattleUI_Restore();
            return 1;
        }

        if (s_loaded_map != (int)wCurMap) {
            Map_Load(wCurMap);
            NPC_Load();
            s_loaded_map = (int)wCurMap;
        } else {
            /* Same-map rewind path: rebuild view from restored camera/scroll only. */
            Map_BuildScrollView();
            NPC_BuildView(gScrollPxX, gScrollPxY);
        }
        return 1;
    }
    return 0;
}

static void rewind_capture_snapshot(void) {
    int slot = s_rw_next_slot;
    uint8_t *dst;

    if (s_rw_cursor >= 0 && s_rw_cursor < s_rw_len - 1) {
        /* Timeline fork after rewind: drop redo tail. */
        s_rw_len = s_rw_cursor + 1;
    }

    dst = s_rw_data + ((size_t)slot * s_rw_state_size);
    s_rw_next_slot = (s_rw_next_slot + 1) % REWIND_SLOTS;
    if (Save_StateCaptureToBuffer(dst, s_rw_state_size) != 0) return;

    if (s_rw_len < REWIND_SLOTS) {
        s_rw_seq[s_rw_len++] = slot;
    } else {
        memmove(&s_rw_seq[0], &s_rw_seq[1], sizeof(int) * (REWIND_SLOTS - 1));
        s_rw_seq[REWIND_SLOTS - 1] = slot;
        if (s_rw_cursor > 0) s_rw_cursor--;
    }
    s_rw_cursor = s_rw_len - 1;
}

static int rewind_step_older(void) {
    if (s_rw_len < 2 || s_rw_cursor <= 0) return 0;
    s_rw_cursor--;
    return load_state_for_runtime_mem(s_rw_data + ((size_t)s_rw_seq[s_rw_cursor] * s_rw_state_size));
}

static int rewind_step_newer(void) {
    if (s_rw_len < 2 || s_rw_cursor < 0 || s_rw_cursor >= s_rw_len - 1) return 0;
    s_rw_cursor++;
    return load_state_for_runtime_mem(s_rw_data + ((size_t)s_rw_seq[s_rw_cursor] * s_rw_state_size));
}

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
extern int gSkipMenu;  /* game.c — bypass title/menu when quickstart flag + save exists */

int main(int argc, char *argv[]) {
    int sfx_debug = 0;
    int debug_render = 0;
    int debug_render_typing = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--skip") == 0 ||
            strcmp(argv[i], "--quickstart") == 0 ||
            strcmp(argv[i], "--skip-title") == 0) {
            gSkipMenu = 1;
        } else if (strcmp(argv[i], "--sfx-debug") == 0) {
            sfx_debug = 1;
        } else if (strcmp(argv[i], "--debug-render") == 0) {
            debug_render = 1;
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    Display_SetDebugRenderMode(debug_render);
    if (Display_Init() != 0) {
        fprintf(stderr, "Display_Init failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    if (Audio_Init() != 0) {
        fprintf(stderr, "Audio_Init failed: %s (continuing without audio)\n",
                SDL_GetError());
    }
    Audio_SetMoveSfxDebug(sfx_debug);
    if (sfx_debug) {
        printf("[debug] Move SFX tracing ON (--sfx-debug)\n");
    }

    Input_Init();
    GameInit();
    if (debug_render) {
        DebugCLI_ConsoleSetOverlayEnabled(0); /* disable old in-map CLI box */
        DebugCLI_ConsoleSetAlwaysOpen(1);      /* keep CLI live at all times */
        SDL_StopTextInput();
    }
    s_rw_state_size = Save_StateSize();
    s_rw_data = (uint8_t *)malloc((size_t)REWIND_SLOTS * s_rw_state_size);
    if (!s_rw_data) {
        fprintf(stderr, "rewind buffer alloc failed\n");
        Audio_Quit();
        Input_Quit();
        Display_Quit();
        SDL_Quit();
        return 1;
    }
    rewind_capture_snapshot(); /* anchor snapshot at startup state */

    const uint32_t FRAME_MS = 1000 / 60;  /* 16ms floor — vsync is OFF in display.c so
                                            * SDL_Delay is the sole frame-rate governor.
                                            * Gives ~62.5 Hz; close enough to GB's 59.73 Hz. */
    uint32_t frame = 0;
    int running = 1;

    while (running) {
        uint32_t frame_start = SDL_GetTicks();
        Input_SetGameplayInputBlocked(debug_render && debug_render_typing);

        /* --- Event pump (quit / window close) --- */
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            int is_cli_toggle = 0;
            if (ev.type == SDL_KEYDOWN && ev.key.repeat == 0) {
                SDL_Scancode sc = ev.key.keysym.scancode;
                SDL_Keycode kc = ev.key.keysym.sym;
                SDL_Keymod km = (SDL_Keymod)ev.key.keysym.mod;
                is_cli_toggle =
                    (sc == SDL_SCANCODE_GRAVE) ||
                    (kc == SDLK_BACKQUOTE)     ||
                    ((km & KMOD_SHIFT) && kc == SDLK_BACKQUOTE);
            }
            if (ev.type == SDL_QUIT) running = 0;
            if (ev.type == SDL_KEYDOWN &&
                ev.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                if (DebugCLI_ConsoleIsOpen()) {
                    DebugCLI_ConsoleClose();
                    if (!debug_render) SDL_StopTextInput();
                } else {
                    running = 0;
                }
            }
            /* ` or Shift+~ = submit command in debug-render, otherwise toggle classic CLI */
            if (is_cli_toggle) {
                if (debug_render) {
                    debug_render_typing = !debug_render_typing;
                    if (debug_render_typing) SDL_StartTextInput();
                    else SDL_StopTextInput();
                } else if (DebugCLI_ConsoleIsOpen()) {
                    DebugCLI_ConsoleClose();
                    SDL_StopTextInput();
                } else {
                    DebugCLI_ConsoleOpen();
                    SDL_StartTextInput();
                }
            }
            /* Console: Enter = execute, Backspace = delete char */
            if (ev.type == SDL_KEYDOWN && DebugCLI_ConsoleIsOpen() && !debug_render) {
                if (ev.key.keysym.scancode == SDL_SCANCODE_RETURN ||
                    ev.key.keysym.scancode == SDL_SCANCODE_KP_ENTER) {
                    DebugCLI_ConsoleExecute();
                    SDL_StopTextInput();
                } else if (ev.key.keysym.scancode == SDL_SCANCODE_BACKSPACE) {
                    DebugCLI_ConsoleBackspace();
                }
            }
            if (ev.type == SDL_KEYDOWN && DebugCLI_ConsoleIsOpen() && debug_render) {
                if (debug_render_typing &&
                    ev.key.keysym.scancode == SDL_SCANCODE_BACKSPACE) {
                    DebugCLI_ConsoleBackspace();
                } else if (debug_render_typing &&
                           (ev.key.keysym.scancode == SDL_SCANCODE_RETURN ||
                            ev.key.keysym.scancode == SDL_SCANCODE_KP_ENTER)) {
                    DebugCLI_ConsoleExecute();
                }
            }
            /* Frame-step hotkeys (when not typing in debug sidebar):
             *   ',' = step backward 1 frame
             *   '.' = step forward 1 frame (within rewind history) */
            if (ev.type == SDL_KEYDOWN && ev.key.repeat == 0 &&
                !(debug_render && debug_render_typing)) {
                if (ev.key.keysym.scancode == SDL_SCANCODE_COMMA) {
                    rewind_step_older();
                } else if (ev.key.keysym.scancode == SDL_SCANCODE_PERIOD) {
                    rewind_step_newer();
                }
            }
            /* Console: typed characters */
            if (ev.type == SDL_TEXTINPUT && DebugCLI_ConsoleIsOpen()) {
                const char *t = ev.text.text;
                /* Skip backtick/tilde — those toggle the console */
                if ((debug_render ? debug_render_typing : 1) &&
                    t[0] != '`' && t[0] != '~' && t[1] == '\0')
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

        /* Hold-to-rewind (emulator-style scrub): step one snapshot per frame. */
        {
            const uint8_t *keys = SDL_GetKeyboardState(NULL);
            int ctrl = (SDL_GetModState() & KMOD_CTRL) != 0;
            int did_scrub = 0;
            if (!DebugCLI_ConsoleIsOpen() && ctrl && keys[SDL_SCANCODE_Z]) {
                did_scrub = rewind_step_older();
            } else if (!DebugCLI_ConsoleIsOpen() && ctrl && keys[SDL_SCANCODE_Y]) {
                did_scrub = rewind_step_newer();
            }
            if (did_scrub) {
                Display_RenderScrolled(gScrollPxX, gScrollPxY, gScrollTileMap, SCROLL_MAP_W);
                uint32_t elapsed = SDL_GetTicks() - frame_start;
                if (elapsed < FRAME_MS) SDL_Delay(FRAME_MS - elapsed);
                frame++;
                continue;
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

        /* Periodic rewind snapshots (timeline history). */
        if (!DebugCLI_IsReplayPlaying()) {
            s_rw_frame_div++;
            if (s_rw_frame_div >= REWIND_SNAPSHOT_EVERY) {
                s_rw_frame_div = 0;
                rewind_capture_snapshot();
            }
        }

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
    free(s_rw_data);
    SDL_Quit();
    return 0;
}
