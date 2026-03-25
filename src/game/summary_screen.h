#pragma once
/* summary_screen.h — Two-page Gen 1 Pokémon summary screen.
 * Ports StatusScreen / StatusScreen2 (engine/pokemon/status_screen.asm).
 *
 * Page 1: sprite, name, №, level, HP bar, status, types, OT/ID, stats.
 * Page 2: move names, PP, EXP points, EXP to next level.
 *
 * A on page 1 → page 2.  A or B on page 2 → close.  B on page 1 → close.
 */

void SummaryScreen_Open(int slot);   /* open for party slot 0-5 */
int  SummaryScreen_IsOpen(void);
void SummaryScreen_Tick(void);
