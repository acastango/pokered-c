#pragma once
/* tileset_data.h -- Generated from pokered-master tileset binaries.
 *
 * Each tileset entry has:
 *   blocks[]     -- block table: 16 tile IDs per block (4x4), indexed 0..N-1
 *   gfx[]        -- 2bpp tile graphics: 16 bytes per tile
 *   gfx_tiles    -- number of tiles in gfx[]
 *   num_blocks   -- number of blocks
 *   coll_tiles[] -- passable tile ID list ($FF-terminated)
 */
#include <stdint.h>

#define NUM_TILESETS 24

typedef struct {
    const uint8_t *blocks;      /* block table (16 bytes per block) */
    uint16_t       num_blocks;
    const uint8_t *gfx;         /* 2bpp tile graphics (16 bytes per tile) */
    uint16_t       gfx_tiles;
    const uint8_t *coll_tiles;  /* passable tile IDs, 0xFF-terminated */
    uint8_t        grass_tile;  /* expanded tile ID of grass ($FF = no grass) */
    uint8_t        anim_type;   /* TILEANIM_* (0=none, 1=water, 2=water+flower) */
} tileset_info_t;

extern const tileset_info_t gTilesets[NUM_TILESETS];
