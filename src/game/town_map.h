#pragma once

/* town_map.h — START->ITEM Town Map screen.
 *
 * Base behavior mirrors DisplayTownMap in engine/items/town_map.asm:
 * - Opens a full-screen map view
 * - Shows current/selected location name
 * - UP/DOWN cycles TownMapOrder entries
 * - A/B exits
 */

void TownMap_Open(void);
int  TownMap_IsOpen(void);
void TownMap_Tick(void);

