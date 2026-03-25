#pragma once
/* party_menu.h — Gen 1 party menu.
 *
 * Matches DisplayPartyMenu / DrawPartyMenu_ from pokered-master.
 *
 * Screen layout (20×18 tiles):
 *   Slot N, row N*2   (name row): col 3=nickname[10], col 13=level, col 17=status
 *   Slot N, row N*2+1 (HP row):   col 0=cursor, col 4=HP bar (9 tiles), col 13=HP fraction
 *   Rows 12-17: message box ("Choose a POKeMON.")
 *
 * Caller responsibilities:
 *   - After close from OVERWORLD: call Map_ReloadGfx() + Font_Load() +
 *     Map_BuildScrollView() + NPC_BuildView() to restore the map.
 *   - After close from BATTLE: bui_state = BUI_DRAW_HUD handles the redraw.
 */

/* Open the party menu.
 * force=0: B cancels and returns -1.
 * force=1: B ignored (battle forced switch). */
void PartyMenu_Open(int force);

/* Returns non-zero while the menu is active. */
int  PartyMenu_IsOpen(void);

/* Per-frame update.  Call each tick when PartyMenu_IsOpen(). */
void PartyMenu_Tick(void);

/* Selected slot (0-5) after the menu closes via A, or -1 if cancelled via B.
 * Valid only after PartyMenu_IsOpen() transitions to 0. */
int  PartyMenu_GetSelected(void);
