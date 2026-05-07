#pragma once
#include <stdint.h>

void GameCornerScripts_OnMapLoad(void);
void GameCornerScripts_Tick(void);
int  GameCornerScripts_IsActive(void);

int  GameCornerScripts_GetPendingBattle(uint8_t *class_out, uint8_t *no_out);
int  GameCornerScripts_ConsumeRocketBattle(void);
void GameCornerScripts_OnVictory(void);
void GameCornerScripts_OnDefeat(void);

/* event_data callbacks */
void GameCorner_RocketScript(void);
void GameCorner_PosterScript(void);

