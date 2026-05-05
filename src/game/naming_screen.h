#pragma once
#include <stdint.h>

/* naming_screen.h — Gen 1 naming screen UI (first-pass C port)
 *
 * Screen types mirror constants/menu_constants.asm:
 *   0 = player name, 1 = rival name, 2 = mon nickname.
 *
 * Open semantics:
 * - `name_buf` is a GB-encoded NAME_LENGTH buffer (0x50 terminator).
 * - The current contents are edited in-place and written back on submit.
 */

enum {
    NAME_PLAYER_SCREEN = 0,
    NAME_RIVAL_SCREEN  = 1,
    NAME_MON_SCREEN    = 2,
};

void NamingScreen_Open(uint8_t type, uint8_t species, uint8_t *name_buf);
int  NamingScreen_IsOpen(void);
void NamingScreen_Tick(void);
