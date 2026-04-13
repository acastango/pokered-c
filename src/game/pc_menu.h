#pragma once

/* Generic Pokémon Center PC / Bill's PC menu flow.
 * ASM reference:
 *   - engine/menus/pc.asm
 *   - engine/pokemon/bills_pc.asm
 */

void PCMenu_Activate(void);   /* hidden-event entry point for Pokémon Center PCs */
int  PCMenu_IsOpen(void);
void PCMenu_Tick(void);
