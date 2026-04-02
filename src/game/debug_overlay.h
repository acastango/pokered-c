#pragma once
/* debug_overlay.h — Debug tools for overworld navigation.
 *
 * Debug_PrintGrid()   — F1: prints a 10×9 ASCII grid to stdout.
 * Debug_SetCollisionOverlay(on) — F3: live in-game tile tint overlay.
 * Debug_UpdateOverlay()         — call each frame when overlay is on.
 *
 * Collision overlay color legend (semi-transparent tints over tiles):
 *   RED     solid / impassable     CYAN    warp / door
 *   GREEN   grass (wild encounter) YELLOW  ledge
 *   MAGENTA NPC position
 *   (passable tiles get no tint so the screen looks normal for open areas)
 *
 * ASCII grid symbol legend (F1):
 *   @  player   o  NPC   D  warp/door   g  grass
 *   v  ledgeS   <  ledgeW   >  ledgeE   #  solid   .  walkable
 */
void Debug_PrintGrid(void);

void Debug_SetCollisionOverlay(int on);
int  Debug_CollisionOverlayOn(void);
/* Recompute per-tile overlay colors from current game state.
 * Call each frame (after GameTick, before Display_RenderScrolled). */
void Debug_UpdateOverlay(void);

/* Battle state dump — prints full battle state to stdout + bugs/battle_<ts>.txt.
 * No-op when not in battle.  Trigger with F7. */
void Debug_PrintBattleState(void);

/* Combat log — routes all BLOG lines to bugs/combat_log_<ts>.txt while enabled.
 * Toggle with F7-shift or dedicated key.  A new file opens each time. */
void Debug_SetCombatLog(int on);
int  Debug_CombatLogOn(void);

/* WRAM dump — writes party, position, bag, and key flags to bugs/wram_<ts>.txt.
 * Trigger with F8. */
void Debug_DumpWRAM(void);
