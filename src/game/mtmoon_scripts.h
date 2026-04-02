#pragma once

/* Mt. Moon B2F fossil choice event — mirrors scripts/MtMoonB2F.asm.
 * After defeating the Super Nerd, player can pick Dome or Helix fossil.
 * Super Nerd takes the other one. */

void MtMoon_FossilCallback_Dome(void);   /* NPC 5 A-press callback */
void MtMoon_FossilCallback_Helix(void);  /* NPC 6 A-press callback */
void MtMoon_OnMapLoad(void);             /* hide fossils if already taken */
void MtMoon_StepCheck(void);             /* step trigger: auto-engage Super Nerd */
void MtMoon_Tick(void);                  /* call from game tick */
int  MtMoon_IsActive(void);              /* true during fossil sequence */
void MtMoon_PostRender(void);            /* redraw yes/no after Map_BuildScrollView */
