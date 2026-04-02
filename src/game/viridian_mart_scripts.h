#pragma once

/* viridian_mart_scripts.h — Oak's Parcel quest (Viridian Mart clerk).
 *
 * Reference: scripts/ViridianMart.asm
 *
 * ViridianMartDefaultScript fires on map entry (not on talking to clerk):
 *   clerk says "Hey! You came from PALLET TOWN?", player auto-walks to
 *   counter, clerk gives OAK's PARCEL.  After that the clerk opens the
 *   normal mart shop.
 */

/* Called on map load — triggers parcel sequence if not yet obtained. */
void ViridianMartScripts_OnMapLoad(void);

/* NPC callback registered for the clerk in kNpcs_ViridianMart[]. */
void ViridianMart_ClerkCallback(void);

/* Call once per overworld frame while on any map. */
void ViridianMartScripts_Tick(void);

/* Returns 1 while the parcel-giving sequence is running. */
int  ViridianMartScripts_IsActive(void);
