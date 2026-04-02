/* bills_house_scripts.h
 *
 * Bill's House transformation sequence (map 0x58).
 * Ports scripts/BillsHouse.asm and
 * engine/events/hidden_events/bills_house_pc.asm.
 */
#pragma once

void BillsHouseScripts_OnMapLoad(void);
int  BillsHouseScripts_IsActive(void);
void BillsHouseScripts_Tick(void);

/* NPC script callbacks — registered in event_data.c */
void BillsHouseScripts_BillPokemonInteract(void);
void BillsHouseScripts_BillInteract(void);
void BillsHouseScripts_Bill2Interact(void);

/* Hidden event callback — registered for PC tile (1,4) */
void BillsHouseScripts_PCActivate(void);
