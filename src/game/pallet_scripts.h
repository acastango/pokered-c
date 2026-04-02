#pragma once

/* pallet_scripts.h — Pallet Town position/map scripts.
 *
 * Mirrors PalletTown_Script / PalletTownDefaultScript / PalletMovementScript
 * in scripts/PalletTown.asm and engine/overworld/auto_movement.asm.
 *
 * Call PalletScripts_Tick() every overworld frame.
 * Call PalletScripts_OnMapLoad() after NPC_Load on Pallet Town.
 */

int  PalletScripts_IsActive(void);
void PalletScripts_Tick(void);

/* Called after NPC_Load() when entering Pallet Town.
 * Hides Oak's sprite if EVENT_OAK_APPEARED_IN_PALLET hasn't fired yet,
 * matching the original toggleable_objects system (TOGGLE_PALLET_TOWN_OAK
 * starts OFF). */
void PalletScripts_OnMapLoad(void);
