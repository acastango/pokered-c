#pragma once
/* npc.h — NPC sprite loading, movement, and OAM management. */
#include <stdint.h>

/* Load NPC GFX and OAM for the current map. Call after Map_Load(). */
void NPC_Load(void);

/* Per-frame update: advances walk animations and drives random movement
 * for WALK NPCs. Call once per game tick before NPC_BuildView(). */
void NPC_Update(void);

/* Update OAM positions for all NPCs given the current camera scroll offset. */
void NPC_BuildView(int scroll_px_x, int scroll_px_y);

/* Make NPC i face the player (called from check_npc_interact in game.c). */
void NPC_FacePlayer(int npc_idx);

/* Returns 1 if any NPC occupies tile (nx, ny) — used for collision. */
int NPC_IsBlocked(int nx, int ny);

/* Returns the index of the NPC at runtime tile (tx, ty), or -1 if none.
 * Uses live positions so WALK NPCs that have moved are correctly found. */
int NPC_FindAtTile(int tx, int ty);

/* Hide a sprite slot (by NPC array index) by zeroing its OAM Y coords.
 * Used to remove item ball sprites after pickup. */
void NPC_HideSprite(int npc_slot_idx);
