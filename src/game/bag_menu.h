#pragma once
#include <stdint.h>

/* bag_menu.h -- Overworld bag (ITEM) menu.
 *
 * BagMenu_Open()        draw the bag list and enter the menu (call from START menu ITEM).
 * BagMenu_Tick()        process one frame of input; call from GameTick when open.
 * BagMenu_IsOpen()      returns 1 while the bag menu is active.
 *
 * Battle-mode API (called from battle_ui.c):
 * BagMenu_OpenBattle()  open the bag during a battle (skips map/NPC rebuild on close).
 * BagMenu_GetSelected() returns item ID of the last USE selection (0 = cancelled/none).
 */
void    BagMenu_Open(void);
void    BagMenu_Tick(void);
int     BagMenu_IsOpen(void);

void    BagMenu_OpenBattle(void);
uint8_t BagMenu_GetSelected(void);
