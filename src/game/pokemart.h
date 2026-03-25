#pragma once
#include <stdint.h>

/* pokemart.h — Pokémart buy/sell sequence.
 * Source: pokered-master/engine/events/pokemart.asm
 *         pokered-master/data/items/marts.asm
 *         pokered-master/data/items/prices.asm
 */

/* Set the active inventory (null-terminated array of item IDs) then call Start. */
void Pokemart_SetInventory(const uint8_t *inv);

void Pokemart_Start(void);
int  Pokemart_IsActive(void);
void Pokemart_Tick(void);

/* Per-mart callbacks wired to clerk NPC scripts in event_data.c */
void ViridianMart_Start(void);
void PewterMart_Start(void);
void CeruleanMart_Start(void);
void VermilionMart_Start(void);
void LavenderMart_Start(void);
void Celadon2F1Mart_Start(void);
void Celadon2F2Mart_Start(void);
void Celadon4FMart_Start(void);
void Celadon5F1Mart_Start(void);
void Celadon5F2Mart_Start(void);
void FuchsiaMart_Start(void);
void CinnabarMart_Start(void);
void SaffronMart_Start(void);
void IndigoMart_Start(void);
