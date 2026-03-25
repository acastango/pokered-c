#pragma once

/* pokecenter.h — Pokémon Center nurse NPC healing sequence.
 *
 * Mirrors DisplayPokemonCenterDialogue (engine/events/pokecenter.asm):
 *   Welcome text → YES/NO → "We'll need your POKeMON" →
 *   HealParty → brief anim delay → "Thank you! fighting fit!" → "See you again!"
 *
 * Usage:
 *   Wire nurse NPCs in event_data.c to Pokecenter_Start.
 *   Call Pokecenter_IsActive() / Pokecenter_Tick() from GameTick each frame.
 */

/* Called by nurse NPC script — begins the heal sequence. */
void Pokecenter_Start(void);

/* Returns 1 while the sequence is running (text or YES/NO or healing). */
int  Pokecenter_IsActive(void);

/* Advance one frame.  Call from GameTick when IsActive() and !Text_IsOpen(). */
void Pokecenter_Tick(void);
