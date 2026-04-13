/* route2gate_scripts.h — Route 2 Gate Oak's aide HM05 FLASH event.
 *
 * ASM reference: scripts/Route2Gate.asm, engine/events/oaks_aide.asm
 * Map ID: ROUTE_2_GATE = 0x31
 */
#pragma once

void Route2GateScripts_OnMapLoad(void);
int  Route2GateScripts_IsActive(void);
void Route2GateScripts_Tick(void);

/* NPC script callback — registered for the aide NPC in event_data.c */
void Route2GateScripts_AideInteract(void);
