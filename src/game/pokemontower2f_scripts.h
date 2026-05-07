#pragma once
#include <stdint.h>

void PokemonTower2FScripts_OnMapLoad(void);
void PokemonTower2FScripts_StepCheck(void);
void PokemonTower2FScripts_Tick(void);
int  PokemonTower2FScripts_IsActive(void);
int  PokemonTower2FScripts_GetPendingBattle(uint8_t *class_out, uint8_t *no_out);
int  PokemonTower2FScripts_ConsumeRivalBattle(void);
void PokemonTower2FScripts_OnVictory(void);
void PokemonTower2FScripts_OnDefeat(void);

/* NPC callback bound from event_data.c for tower rival interaction. */
void PokemonTower2F_RivalScript(void);
