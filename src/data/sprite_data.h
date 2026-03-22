#pragma once
/* sprite_data.h -- Generated from pokered-master gfx/sprites/ PNGs. */
#include <stdint.h>

#define NUM_SPRITES  73
#define SPRITE_TILES 24      /* tiles per overworld sprite (2x12): 12 stand + 12 walk */
#define SPRITE_GFX_SIZE (SPRITE_TILES * 16)  /* bytes */

/* gSpriteGfx[sprite_id][tile][16 bytes].                         */
/* sprite_id 0 = SPRITE_NONE (empty).  Use first SPRITE_TILES tiles */
/* (front-facing animation frame) for overworld rendering.         */
extern const uint8_t gSpriteGfx[NUM_SPRITES][SPRITE_GFX_SIZE];
