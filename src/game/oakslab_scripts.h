#pragma once
#include <stdint.h>

/* Oaks Lab scripts (scripts/OaksLab.asm).
 *
 * Covers:
 *   Oak enters / swaps / player walks into lab (intro chain)
 *   Speech sequence through EVENT_OAK_ASKED_TO_CHOOSE_MON
 *   Starter ball interaction (yes/no, give mon)
 *   Rival picks his starter → challenges player → battle
 *   Rival exits after battle
 */

void OaksLabScripts_OnMapLoad(void);
void OaksLabScripts_Tick(void);
int  OaksLabScripts_IsActive(void);

/* NPC callbacks for the three starter Pokéball objects.
 * Registered in kNpcs_OaksLab[] in event_data.c. */
void OaksLab_CharmanderCallback(void);
void OaksLab_SquirtleCallback(void);
void OaksLab_BulbasaurCallback(void);

/* Talk callback for Oak (slot 4).  Handles parcel delivery and Pokédex giving.
 * Reference: OaksLabOak1Text / OaksLabOakGivesPokedexScript in scripts/OaksLab.asm. */
void OaksLab_OakCallback(void);

/* Returns 1 (and fills *class_out / *no_out) when the rival battle
 * is ready to start.  Called by game.c each overworld tick.
 * Clears the pending flag after returning 1. */
int OaksLabScripts_GetPendingBattle(uint8_t *class_out, uint8_t *no_out);

/* Redraws any active UI overlays (yes/no box) that may have been
 * overwritten by Map_BuildScrollView.  Call after Map_BuildScrollView
 * and NPC_BuildView whenever OaksLabScripts_IsActive() is true. */
void OaksLabScripts_PostRender(void);
