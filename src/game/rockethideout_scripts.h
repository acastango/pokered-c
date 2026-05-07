#pragma once

/* Rocket Hideout map behavior scripts:
 * - B1F entrance door gate
 * - B2F/B3F spinner-arrow movement
 * - B4F locked door gate
 */

void RocketHideoutScripts_OnMapLoad(void);
void RocketHideoutScripts_Tick(void);
int  RocketHideoutScripts_IsActive(void);
void RocketHideoutScripts_StepCheck(void);
