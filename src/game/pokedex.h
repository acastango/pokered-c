#pragma once

/* pokedex.h — Pokédex list + detail screen.
 *
 * Open from start menu (only when EVENT_GOT_POKEDEX is set).
 * Flow: list screen → press A → detail screen → press A/B → list screen.
 */

void Pokedex_Open(void);
int  Pokedex_IsOpen(void);
void Pokedex_Tick(void);

/* Call when a Pokémon appears in battle (wild or trainer).
 * species = internal species ID (gBaseStats index). */
void Pokedex_SetSeen(int species);
void Pokedex_SetOwned(int species);

/* ShowPokedexData — standalone full-screen dex entry display.
 * Used by StarterDex (Oak's Lab) and item_effects (Pokédex item).
 * Matches engine/menus/pokedex.asm ShowPokedexData/ShowPokedexDataInternal.
 * dex_num = Pokédex number (1-151). */
void Pokedex_ShowData(int dex_num);
void Pokedex_ShowDataTick(void);
int  Pokedex_IsShowingData(void);
