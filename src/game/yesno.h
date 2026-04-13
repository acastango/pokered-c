#pragma once

/* yesno.h — Simple YES/NO prompt overlay.
 *
 * Flow:
 *   YesNo_Show(prompt)   — show text and YES/NO box.
 *   each tick:  if (YesNo_IsOpen()) YesNo_Tick();
 *   after close: result = YesNo_GetResult()  (1=YES, 0=NO)
 *
 * Mirrors AskQuestion / DisplayYesNoMenu from pokered-master
 * engine/menus/yes_no_menu.asm.
 */

/* Start the yes/no flow.  prompt is shown via Text_ShowASCII, then a
 * YES/NO box appears at the bottom-right corner. */
void YesNo_Show(const char *prompt);

/* Returns non-zero while the prompt is active. */
int  YesNo_IsOpen(void);

/* Advance one game tick. */
void YesNo_Tick(void);

/* 1=YES, 0=NO.  Valid only after YesNo_IsOpen() returns 0. */
int  YesNo_GetResult(void);

/* Redraw the YES/NO box tiles.  Call after Map_BuildScrollView() to restore
 * the overlay that the tilemap rebuild would have overwritten. */
void YesNo_PostRender(void);
