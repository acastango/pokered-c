#pragma once
/* field_moves.h — Overworld HM field effects (Cut, Flash, etc.)
 *
 * Call FieldMove_TryCut() from the A-press overworld handler.
 * Call FieldMove_TryFlash(slot) from the party menu USE option.
 * While FieldMove_IsActive(), call FieldMove_Tick() + FieldMove_PostRender()
 * each frame (and skip normal overworld input handling).
 */

/* Check if the tile in front of the player is a cuttable tree and the player
 * has the Cascade Badge and a mon that knows Cut.  If so, opens the YesNo
 * prompt and starts the Cut sequence.  No-op if conditions are not met. */
void FieldMove_TryCut(void);

/* Returns 1 if the given party slot knows Flash (MOVE_FLASH). */
int  FieldMove_HasFlash(int slot);

/* Use Flash: requires Boulder Badge + slot must know Flash.
 * Clears gMapPalOffset (lights up dark cave), shows text.
 * Mirrors .flash handler in engine/menus/start_sub_menus.asm. */
void FieldMove_TryFlash(int slot);

/* Returns non-zero while a field move sequence is in progress. */
int  FieldMove_IsActive(void);

/* Per-frame tick — handles YesNo input and state transitions. */
void FieldMove_Tick(void);

/* Re-draw overlay tiles (YesNo box) after Map_BuildScrollView wipes them. */
void FieldMove_PostRender(void);
