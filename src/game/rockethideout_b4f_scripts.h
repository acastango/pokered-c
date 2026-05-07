#pragma once
#include <stdint.h>

/* Rocket Hideout B4F scripted encounters (Giovanni + Rocket3 drop). */

void RocketHideoutB4FScripts_OnMapLoad(void);
void RocketHideoutB4FScripts_Tick(void);
int  RocketHideoutB4FScripts_IsActive(void);
int  RocketHideoutB4FScripts_GetPendingBattle(uint8_t *class_out, uint8_t *no_out);
int  RocketHideoutB4FScripts_ConsumeBattle(void);
void RocketHideoutB4FScripts_OnVictory(void);
void RocketHideoutB4FScripts_OnDefeat(void);

/* NPC callbacks wired from event_data.c */
void RocketHideoutB4F_GiovanniScript(void);
void RocketHideoutB4F_Rocket3Script(void);
