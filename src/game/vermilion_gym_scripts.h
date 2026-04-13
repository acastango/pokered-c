#pragma once
/* vermilion_gym_scripts.h — Vermilion Gym trashcan puzzle and NPC callbacks.
 *
 * Trashcan puzzle: 15 hidden events, two-stage randomized lock.
 * Faithfully replicates the Gen 1 bug where offset underflow → second lock = 0.
 * Reference: engine/events/hidden_events/vermilion_gym_trash.asm
 */

/* Call from fire_map_onload_callbacks in game.c. */
void VermilionGymScripts_OnMapLoad(void);

/* NPC callbacks — set in kNpcs_VermilionGym in event_data.c */
void VermilionGymScripts_GentlemanInteract(void);
void VermilionGymScripts_RockerInteract(void);
void VermilionGymScripts_SailorInteract(void);

/* Hidden event callbacks — one per trash can (indices 0-14) */
void VermilionGymScripts_Trash0(void);
void VermilionGymScripts_Trash1(void);
void VermilionGymScripts_Trash2(void);
void VermilionGymScripts_Trash3(void);
void VermilionGymScripts_Trash4(void);
void VermilionGymScripts_Trash5(void);
void VermilionGymScripts_Trash6(void);
void VermilionGymScripts_Trash7(void);
void VermilionGymScripts_Trash8(void);
void VermilionGymScripts_Trash9(void);
void VermilionGymScripts_Trash10(void);
void VermilionGymScripts_Trash11(void);
void VermilionGymScripts_Trash12(void);
void VermilionGymScripts_Trash13(void);
void VermilionGymScripts_Trash14(void);
