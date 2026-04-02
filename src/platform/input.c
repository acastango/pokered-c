/* input.c — SDL keyboard → GB joypad byte
 *
 * GB hJoyInput bit layout (SET = pressed):
 *   bit 7 = D_DOWN   bit 3 = START
 *   bit 6 = D_UP     bit 2 = SELECT
 *   bit 5 = D_LEFT   bit 1 = B
 *   bit 4 = D_RIGHT  bit 0 = A
 */
#include "input.h"
#include "hardware.h"
#include "../game/constants.h"
#include <SDL2/SDL.h>
#include <stdio.h>

/* Input recorder / playback state */
static FILE *s_rec_fp  = NULL;
static FILE *s_play_fp = NULL;
static int   s_playing = 0;

/* Default key bindings — can be made configurable later */
static SDL_Scancode key_map[8] = {
    SDL_SCANCODE_Z,         /* PAD_A      bit 0 */
    SDL_SCANCODE_X,         /* PAD_B      bit 1 */
    SDL_SCANCODE_RSHIFT,    /* PAD_SELECT bit 2 */
    SDL_SCANCODE_RETURN,    /* PAD_START  bit 3 */
    SDL_SCANCODE_RIGHT,     /* PAD_RIGHT  bit 4 */
    SDL_SCANCODE_LEFT,      /* PAD_LEFT   bit 5 */
    SDL_SCANCODE_UP,        /* PAD_UP     bit 6 */
    SDL_SCANCODE_DOWN,      /* PAD_DOWN   bit 7 */
};

void Input_Init(void) { }
void Input_Quit(void) { Input_StopRecording(); Input_StopPlayback(); }

/* CLI injection — set by debug_cli.c */
extern uint8_t gCliButtons;
extern int     gCliFrames;

void Input_Update(void) {
    uint8_t raw;

    if (gCliFrames > 0) {
        raw = gCliButtons;
        gCliFrames--;
    } else if (s_playing) {
        if (fread(&raw, 1, 1, s_play_fp) != 1) {
            Input_StopPlayback();
            raw = 0;
        }
    } else {
        const uint8_t *keys = SDL_GetKeyboardState(NULL);
        raw = 0;
        for (int i = 0; i < 8; i++)
            if (keys[key_map[i]])
                raw |= (uint8_t)(1 << i);
        raw &= ~wJoyIgnore;
    }

    if (s_rec_fp) fwrite(&raw, 1, 1, s_rec_fp);

    hJoyReleased = hJoyHeld & ~raw;
    hJoyPressed  = raw & ~hJoyHeld;
    hJoyHeld     = raw;
    hJoyInput    = raw;
}

void Input_StartRecording(const char *path) {
    Input_StopRecording();
    s_rec_fp = fopen(path, "wb");
    if (s_rec_fp) printf("[input] Recording: %s\n", path);
    else          printf("[input] Record open failed: %s\n", path);
}
void Input_StopRecording(void) {
    if (!s_rec_fp) return;
    fclose(s_rec_fp); s_rec_fp = NULL;
    printf("[input] Recording stopped.\n");
}
void Input_StartPlayback(const char *path) {
    Input_StopPlayback();
    s_play_fp = fopen(path, "rb");
    if (s_play_fp) { s_playing = 1; printf("[input] Playback: %s\n", path); }
    else           printf("[input] Playback open failed: %s\n", path);
}
void Input_StopPlayback(void) {
    if (s_play_fp) { fclose(s_play_fp); s_play_fp = NULL; }
    if (s_playing) { s_playing = 0; printf("[input] Playback stopped.\n"); }
}
int Input_IsRecording(void) { return s_rec_fp != NULL; }
int Input_IsPlaying(void)   { return s_playing; }
