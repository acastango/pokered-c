#pragma once
/* map_data.h -- Generated from pokered-master map data.
 *
 * gMapTable is indexed by map ID (0x00 = PALLET_TOWN ... 0xF7 = AGATHAS_ROOM).
 * For unused/invalid map IDs, blocks==NULL.
 */
#include <stdint.h>

typedef struct {
    uint8_t        width;       /* map width in blocks */
    uint8_t        height;      /* map height in blocks */
    uint8_t        tileset_id;  /* index into gTilesets[] */
    uint8_t        border_block;/* block ID to use for out-of-bounds */
    const uint8_t *blocks;      /* raw block data (width*height bytes) */
    const char    *name;        /* map name for debug */
} map_info_t;

#define NUM_MAPS 248

extern const map_info_t gMapTable[NUM_MAPS];
