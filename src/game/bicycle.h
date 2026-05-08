#pragma once

/* Bicycle field-item behavior and bike movement state. */

/* Called when entering/loading a map to apply forced bike/surf tiles. */
void Bicycle_OnMapLoad(void);

/* Mirror pokered's PlayDefaultMusic behavior for walk/bike/surf state. */
void Bicycle_PlayDefaultMusic(void);

/* Clear forced always-on-bike latch (Route 16/18 gate script parity). */
void Bicycle_ClearAlwaysOnBike(void);

/* Handle ITEM_BICYCLE use from the bag. Returns 1 if item flow consumed. */
int Bicycle_UseFromBag(void);

/* Returns 1 when bike speedup should be active for the current step. */
int Bicycle_IsSpeedupActive(void);

/* Returns 1 when the bike sprite set should be used for player rendering. */
int Bicycle_ShouldUseBikeSprite(void);
