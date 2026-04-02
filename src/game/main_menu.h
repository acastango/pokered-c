#pragma once

/* main_menu.h — Title / main-menu screen (CONTINUE / NEW GAME).
 *
 * Mirrors engine/menus/main_menu.asm:MainMenu.
 * Shown at startup: CONTINUE if a save file was found, then NEW GAME.
 *
 * Result (from MainMenu_GetResult):
 *   MAIN_MENU_PENDING  — still waiting for input
 *   MAIN_MENU_CONTINUE — player chose to continue saved game
 *   MAIN_MENU_NEW_GAME — player chose to start a new game
 */

#define MAIN_MENU_PENDING   0
#define MAIN_MENU_CONTINUE  1
#define MAIN_MENU_NEW_GAME  2

void MainMenu_Open(int has_save);
int  MainMenu_IsOpen(void);
int  MainMenu_GetResult(void);
void MainMenu_Tick(void);
