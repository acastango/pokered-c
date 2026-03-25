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

/* Hide / show all NPC slots — used by mart/pokecenter UIs. */
void NPC_HideAll(void);
void NPC_ShowAll(void);

/* Mirror Gen 1 CheckSpriteAvailability: hide any NPC whose 2×2-tile sprite
 * footprint contains a tile slot >= MAP_TILESET_SIZE (96) — i.e. a UI/font
 * tile drawn by a text box.  Call after drawing UI; NPC_BuildView restores. */
void NPC_HideOverUITiles(void);

/* Change the facing direction of NPC i (0=down 1=up 2=left 3=right).
 * Reloads sprite tiles and OAM flags immediately. */
void NPC_SetFacing(int i, int facing);

/* Returns the index of the NPC most recently passed to NPC_FacePlayer,
 * or -1 if no interaction has occurred since map load.
 * Used by Pokecenter_Start to remember which NPC is the nurse. */
int NPC_GetLastInteracted(void);

/* Returns the current screen pixel position (top-left corner) of NPC i's
 * sprite, via the live OAM data.  Assumes NormalOAM layout (down/up/left
 * facing).  Used by pokecenter.c to anchor the healing machine sprites. */
void NPC_GetScreenPos(int i, int *px, int *py);
