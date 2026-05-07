#pragma once
#include <stdint.h>

/* Rocket Hideout elevator floor menu (ASM-equivalent interaction target).
 * Opened from elevator control panel when Lift Key is in bag.
 */

void ElevatorMenu_OpenRocketHideout(void);
void ElevatorMenu_QueueOpenRocketHideout(void);
void ElevatorMenu_QueueOpenVending(void);
void ElevatorMenu_TryOpenQueued(void);
void ElevatorMenu_Tick(void);
int  ElevatorMenu_IsOpen(void);
int  ElevatorMenu_IsBusy(void);

/* Deprecated immediate-teleport path (kept for compatibility; now unused). */
int  ElevatorMenu_ConsumeTeleport(uint8_t *map_out, int *x_out, int *y_out);
