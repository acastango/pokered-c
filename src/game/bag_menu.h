#pragma once

/* bag_menu.h -- Overworld bag (ITEM) menu.
 *
 * BagMenu_Open()   draw the bag list and enter the menu (call from START menu ITEM).
 * BagMenu_Tick()   process one frame of input; call from GameTick when open.
 * BagMenu_IsOpen() returns 1 while the bag menu is active.
 */
void BagMenu_Open(void);
void BagMenu_Tick(void);
int  BagMenu_IsOpen(void);
