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

void Input_Init(void) { /* nothing to init for keyboard */ }
void Input_Quit(void) { }

void Input_Update(void) {
    const uint8_t *keys = SDL_GetKeyboardState(NULL);

    uint8_t raw = 0;
    for (int i = 0; i < 8; i++)
        if (keys[key_map[i]])
            raw |= (uint8_t)(1 << i);

    raw &= ~wJoyIgnore;

    hJoyReleased = hJoyHeld & ~raw;
    hJoyPressed  = raw & ~hJoyHeld;
    hJoyHeld     = raw;
    hJoyInput    = raw;
}
