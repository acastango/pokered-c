#pragma once
/* player_sprite.h -- Generated from pokered-master gfx/sprites/red.png */
#include <stdint.h>

/* 6 frames, 4 tiles each (2x2 = TL,TR,BL,BR), 16 bytes per tile.
 * Frame index: 0=StandingDown 1=StandingUp 2=StandingLeft
 *              3=WalkingDown  4=WalkingUp  5=WalkingLeft
 * Mirrors data/sprites/facings.asm tile layout: $00-$0B standing, $80-$8B walking. */
#define PLAYER_FRAMES      6
#define PLAYER_TILE_BYTES  16
extern const uint8_t gPlayerGfx[PLAYER_FRAMES][4][PLAYER_TILE_BYTES];
