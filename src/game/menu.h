#pragma once

/* menu.h — START menu
 *
 * Menu_Open()   save tilemap state and draw the start menu (call when START pressed).
 * Menu_Tick()   process one frame of menu input; call from GameTick when open.
 * Menu_IsOpen() returns 1 while the start menu is active.
 */
void Menu_Open(void);
void Menu_Tick(void);
int  Menu_IsOpen(void);
