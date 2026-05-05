#pragma once

/* title_screen.h — Startup title sequence derived from pokered title flow.
 *
 * Sequence:
 *   1) Legal splash card
 *   2) Game Freak logo + shooting-star intro (first pass)
 *   3) Intro battle cutscene placeholder
 *   4) Title screen with logo bounce + cycling title mons
 *   5) Interrupt on START/A or UP+SELECT+B
 *   6) Return control to startup menu flow
 *
 * Result values from TitleScreen_GetResult():
 *   TITLE_SCREEN_PENDING    still running
 *   TITLE_SCREEN_MAIN_MENU  proceed to main menu
 *   TITLE_SCREEN_CLEAR_SAVE UP+SELECT+B was used (clear-save combo path)
 */

#define TITLE_SCREEN_PENDING     0
#define TITLE_SCREEN_MAIN_MENU   1
#define TITLE_SCREEN_CLEAR_SAVE  2

void TitleScreen_Open(void);
int  TitleScreen_IsOpen(void);
int  TitleScreen_GetResult(void);
void TitleScreen_Tick(void);
