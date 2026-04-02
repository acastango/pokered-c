#pragma once
/* warp.h — Warp trigger and map transition system.
 *
 * After each successful player move, Warp_Check() compares the player's
 * current tile position against the current map's warp event table.
 * On a match: destination map is loaded and the player is re-positioned
 * at the destination warp exit tile (one step south of the warp entry).
 *
 * Mirrors CheckWarpsCollision / EnterMap logic from home/overworld.asm.
 */
#include <stdint.h>

/* Check if the player is standing on a warp tile.
 * On match: stores the pending destination (does NOT load the map yet) and
 * returns 1.  The caller (GameTick) must call Warp_Execute() at peak-black
 * to actually switch maps — this keeps the old map's tile GFX alive during
 * the fade-out so Display_LoadTileset doesn't corrupt the fading screen.
 * Skips the check for one call after a successful warp (cooldown).
 *
 * Mirrors CheckWarpsNoCollision (home/overworld.asm) — fires after the
 * player successfully steps onto the warp position; includes tile-type gating
 * (IsPlayerStandingOnDoorTileOrWarpTile / ExtraWarpCheck). */
int Warp_Check(void);

/* Collision variant: mirrors CheckWarpsCollision (home/overworld.asm).
 * Called when the player's step is blocked by a solid tile — the player
 * is still AT the warp event position but cannot advance further.
 * Pure position match only; no tile-type gating. */
int Warp_CheckCollision(void);

/* Execute the pending warp: Map_Load → Player_SetPos → NPC_Load.
 * Call at peak black (fully-dark frame) so the tileset switch is invisible.
 * No-op if no warp is pending. */
void Warp_Execute(void);

/* Reset the warp cooldown.  Called by Map_Load() so each fresh map load
 * starts with a clean warp state (avoids stale cooldown from previous map). */
void Warp_Reset(void);

/* Returns 1 (and clears the flag) if a warp was detected during the last
 * Warp_Check call.  Used by GameTick to start the fade-out transition. */
int Warp_JustHappened(void);

/* Returns 1 (and clears the flag) if Warp_Execute placed the player on a
 * door tile.  GameTick calls this on the first normal frame after fade-in
 * and triggers Player_ForceStepDown() to play the step-out animation.
 * Mirrors BIT_STANDING_ON_DOOR / PlayerStepOutFromDoor in the original. */
int Warp_HasDoorStep(void);

/* Returns 1 if tile_id is a door tile for the current map's tileset.
 * Uses the same per-tileset lookup as the warp system. */
int Warp_IsDoorTile(uint8_t tile_id);

/* Returns 1 if there is a warp event at tile position (x, y) on the current map.
 * Used by player.c to allow stepping onto warp tiles that are also collision tiles
 * (e.g. gym exits: tile 0x16 is in Gym_Coll but is also the exit warp position). */
int Warp_HasEventAt(int x, int y);

/* Debug teleport: immediately load map_id and place the player at (tile_x, tile_y).
 * Bypasses the normal warp fade/queue system.  Pass tile_x=-1,tile_y=-1 to land
 * at the map centre.  Intended for the bugs/teleport.txt file-based trigger. */
void Warp_ForceTeleport(uint8_t map_id, int tile_x, int tile_y);
