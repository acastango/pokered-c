#pragma once

/* Gate_LoadMap — call after NPC_Load on every map transition.
 * Hides badge-check guards whose event flags are already set,
 * mirroring the pokered position-script system that checks event flags
 * before triggering a guard interaction. */
void Gate_LoadMap(void);

/* Per-step position trigger for the Viridian City old man block.
 * Call inside the gStepJustCompleted block in game.c.
 * Mirrors ViridianCityCheckGotPokedexScript: fires when the player steps
 * to the trigger tile north of the sleeping old man without EVENT_GOT_POKEDEX,
 * shows the blocking text, and pushes the player one step south. */
void Gate_ViridianStepCheck(void);
void Gate_ViridianDoPush(void); /* call before Player_Update when !Text_IsOpen — deferred push to avoid camera snap */

/* Pewter City east-exit block — mirrors PewterCityCheckPlayerLeavingEastScript.
 * Fires on any of the four east-edge trigger tiles if EVENT_BEAT_BROCK not set. */
void Gate_PewterEastCheck(void);
void Gate_PewterTick(void);     /* call every frame before Player_Update — escort state machine */
int  Gate_PewterIsActive(void); /* true while escort sequence is running */

/* NPC script callbacks for badge/event-gated NPCs.
 * Each is a npc_script_fn (void(*)(void)) wired into event_data.c. */
void Gate_ViridianOldMan(void);     /* Viridian north — checks EVENT_GOT_POKEDEX */
void Gate_Route22_Guard(void);      /* Route 22 Gate — A-press on guard NPC */
void Gate_Route22StepCheck(void);   /* step trigger — blocks corridor (4,2)/(5,2) until Boulder Badge */
void Gate_Route22DoPush(void);      /* deferred push — call before Player_Update each frame */
void Gate_Route23_Earth(void);      /* Route 23 guard 0 (northernmost) — BADGE_EARTH */
void Gate_Route23_Volcano(void);    /* Route 23 guard 1 — BADGE_VOLCANO */
void Gate_Route23_Marsh(void);      /* Route 23 guard 2 (Swimmer) — BADGE_MARSH */
void Gate_Route23_Soul(void);       /* Route 23 guard 3 (Swimmer) — BADGE_SOUL */
void Gate_Route23_Rainbow(void);    /* Route 23 guard 4 — BADGE_RAINBOW */
void Gate_Route23_Thunder(void);    /* Route 23 guard 5 — BADGE_THUNDER */
void Gate_Route23_Cascade(void);    /* Route 23 guard 6 (southernmost) — BADGE_CASCADE */
void Gate_SaffronGuard_Interact(void); /* Routes 5/6/7/8 counter guard A-press */
void Gate_SaffronStepCheck(void);      /* step trigger — blocks corridor until drink given */
void Gate_SaffronDoPush(void);         /* deferred push — call before Player_Update each frame */
