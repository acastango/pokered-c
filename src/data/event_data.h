#pragma once
/* event_data.h -- Generated from pokered-master map object files. */
#include <stdint.h>

typedef struct {
    uint16_t x, y;          /* tile coords: x = asm_x*2, y = asm_y*2+1 */
    uint8_t  dest_map;
    uint8_t  dest_warp_idx; /* 0-indexed */
} map_warp_t;

/* NPC script callback type — mirrors pokered text_id dispatch.
 * If set, called instead of showing text.  NULL = plain text NPC. */
typedef void (*npc_script_fn)(void);

typedef struct {
    uint16_t      x, y;     /* tile coords: x = asm_x*2, y = asm_y*2+1 */
    uint8_t       sprite_id; /* SPRITE_* numeric value */
    uint8_t       movement;  /* 0=STAY, 1=WALK */
    const char   *text;      /* decoded dialogue, or NULL */
    npc_script_fn script;    /* NULL = plain text; non-NULL = call this instead */
} npc_event_t;

typedef struct {
    uint16_t    x, y;       /* tile coords: x = asm_x*2, y = asm_y*2+1 */
    const char *text;       /* decoded sign text, or NULL */
} sign_event_t;

typedef struct {
    uint16_t x, y;          /* tile coords: x = asm_x*2, y = asm_y*2+1 */
    uint8_t  item_id;       /* item ID (from item_constants.asm) */
} item_event_t;

typedef struct {
    const map_warp_t  *warps;
    uint8_t            num_warps;
    const npc_event_t *npcs;
    uint8_t            num_npcs;
    const sign_event_t *signs;
    uint8_t             num_signs;
    const item_event_t *items;
    uint8_t             num_items;
    uint8_t            border_block;
} map_events_t;

#define NUM_MAPS 248
extern const map_events_t gMapEvents[NUM_MAPS];
