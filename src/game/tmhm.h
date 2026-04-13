#pragma once
#include <stdint.h>

/* tmhm.h — TM/HM teaching state machine.
 *
 * Entry point: TMHM_Use(item_id) — called from bag_menu when player
 * selects USE on a TM or HM in the overworld.
 *
 * Tick pattern in game.c (same as MtMoon):
 *   if (TMHM_IsActive()) {
 *       TMHM_Tick();
 *       Map_BuildScrollView();
 *       TMHM_PostRender();
 *       NPC_BuildView(gScrollPxX, gScrollPxY);
 *       return;
 *   }
 */

/* Called from bag_menu overworld USE handler.  item_id must be a TM/HM. */
void TMHM_Use(uint8_t item_id);

/* Returns non-zero while teaching is in progress. */
int  TMHM_IsActive(void);

/* Advance one frame.  Call from game.c when TMHM_IsActive(). */
void TMHM_Tick(void);

/* Redraw the current UI overlay (YES/NO box or move-forget menu) after
 * Map_BuildScrollView() has rebuilt the tilemap. */
void TMHM_PostRender(void);
