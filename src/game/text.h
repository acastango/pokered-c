#pragma once
/* text.h — Text box rendering and input-driven dialog.
 *
 * Text_ShowBox(str)  draws a 2-line dialog box in the bottom 4 tile rows.
 * Text_IsOpen()      returns 1 while a dialog is visible.
 * Text_Update()      advances the dialog on A-press; call from GameTick.
 * Text_Close()       dismisses the current box.
 *
 * pokered character encoding (charmap.asm):
 *   $80-$FF  A-Z a-z digits punctuation  (main font, tile = FONT_TILE_BASE + c - $80)
 *   $7F      space                        (tile = BLANK_TILE_SLOT)
 *   $79-$7E  ┌─┐│└┘ box-drawing          (tile = BOX_TILE_BASE + c - $79)
 *   $50      '@' — string terminator
 *   $4E      <LINE> — advance to second text line
 *   $4F      <PARA> / wait for A then clear
 */
#include <stdint.h>

void Text_ShowBox(const uint8_t *str);    /* pokered-encoded string */
void Text_ShowASCII(const char *str);     /* ASCII: \n=line \f=paragraph @=end */
int  Text_IsOpen(void);
void Text_Update(void);
void Text_Close(void);
