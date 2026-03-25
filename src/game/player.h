#pragma once
#include <stdint.h>

/* player.h — Player position, input-driven movement, and OAM sprite.
 *
 * Player_Init()   sets start position and loads the sprite.
 * Player_Update() reads hJoyHeld, animates walk, updates OAM each frame.
 * Player_SetPos() repositions after warp (resets scroll state).
 *
 * Walk animation (mirrors AdvancePlayerSprite / wWalkCounter in movement.asm):
 *   - On step start: wXCoord/wYCoord update immediately; gScrollPx{X,Y} = ±TILE_PX.
 *   - Each frame: gScrollPx decrements toward 0 (camera pans to new position).
 *   - Warp check fires when gWalkTimer reaches 0 (step animation complete).
 */
void Player_Init(uint8_t start_x, uint8_t start_y);
void Player_SetPos(uint16_t x, uint16_t y);
void Player_Update(void);
/* Force one step south without collision check (door step-out after warp). */
void Player_ForceStepDown(void);

/* Facing direction: 0=down 1=up 2=left 3=right */
extern int gPlayerFacing;

/* Camera pixel scroll offset — starts at ±TILE_PX on step start, decrements to 0.
 * Read by Display_RenderScrolled and NPC_BuildView each frame. */
extern int gScrollPxX;
extern int gScrollPxY;

/* Get map tile one step in front of the player (for A-button interaction). */
void Player_GetFacingTile(int *out_x, int *out_y);

/* Returns 1 while the player is mid-step (walk animation in progress). */
int Player_IsMoving(void);

/* Set to 1 on the frame a step finishes; read and cleared by game.c. */
extern int gStepJustCompleted;

/* Refresh player OAM after an out-of-band position change (warp map swap). */
void Player_SyncOAM(void);

/* Mirror Gen 1 UpdatePlayerSprite tile check: zero player OAM (slots 0-3)
 * if the lower-left tile of the player's sprite footprint is a UI tile
 * (slot >= 96).  Call after drawing UI boxes (mart, pokecenter). */
void Player_HideIfOverUI(void);

/* Suppress joypad input for n overworld ticks (mirrors wJoyIgnore /
 * IgnoreInputForHalfSecond in EnterMap).  Called by Warp_Execute to
 * prevent "still holding direction" from immediately re-triggering the
 * warp tile just arrived on.  Does NOT block Warp_Check — the warp fires
 * as soon as input is restored and the player walks into the exit. */
void Player_IgnoreInputFrames(int n);
