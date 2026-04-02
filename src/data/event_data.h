#pragma once
/* event_data.h -- Generated from pokered-master map object files. */
#include <stdint.h>

typedef struct {
    uint16_t x, y;          /* ASM block coords (1:1 with pokered source) */
    uint8_t  dest_map;
    uint8_t  dest_warp_idx; /* 0-indexed */
} map_warp_t;

/* NPC script callback type — mirrors pokered text_id dispatch.
 * If set, called instead of showing text.  NULL = plain text NPC. */
typedef void (*npc_script_fn)(void);

typedef struct {
    uint16_t      x, y;     /* ASM block coords (1:1 with pokered source) */
    uint8_t       sprite_id; /* SPRITE_* numeric value */
    uint8_t       movement;  /* 0=STAY, 1=WALK */
    const char   *text;      /* decoded dialogue, or NULL */
    npc_script_fn script;    /* NULL = plain text; non-NULL = call this instead */
} npc_event_t;

/* Per-trainer data — one per trainer NPC on a map.
 * Stored separately from npc_event_t to avoid breaking all existing NPC data.
 *
 * npc_idx:       index into the map's npc array that this trainer occupies
 * facing:        initial facing direction (0=down 1=up 2=left 3=right)
 * trainer_class: 1-based class index for Battle_ReadTrainer (1=YOUNGSTER, etc.)
 * trainer_no:    1-based party index within that class
 * sight_dist:    line-of-sight in visible tiles (pokered engage_dist blocks)
 * flag_bit:      bit index in wEventFlags[] — set when trainer is defeated
 * before_text:   shown when trainer spots/confronts the player (battle start)
 * after_text:    shown on re-interaction after defeat
 */
typedef struct {
    uint8_t      npc_idx;
    uint8_t      facing;
    uint8_t      trainer_class;
    uint8_t      trainer_no;
    uint8_t      sight_dist;
    uint16_t     flag_bit;
    const char  *before_text;
    const char  *after_text;
} map_trainer_t;

typedef struct {
    uint16_t    x, y;       /* ASM block coords (1:1 with pokered source) */
    const char *text;       /* decoded sign text, or NULL */
} sign_event_t;

typedef struct {
    uint16_t x, y;          /* ASM block coords (1:1 with pokered source) */
    uint8_t  item_id;       /* item ID (from item_constants.asm) */
} item_event_t;

/* Hidden event — triggered when player faces tile (x, y) and presses A.
 * Mirrors CheckForHiddenEvent / CheckIfCoordsInFrontOfPlayerMatch from
 * engine/overworld/hidden_events.asm.  No sprite is involved; the trigger
 * tile is typically a background tile that looks interactive (bench, sign,
 * PC terminal).
 * Coords: x = orig_x*2, y = orig_y*2+1 — same system as npc_event_t. */
typedef void (*hidden_script_fn)(void);
typedef struct {
    int16_t           x, y;    /* facing tile that triggers this event */
    const char       *text;    /* ASCII text, or NULL if script handles it */
    hidden_script_fn  script;  /* optional callback; overrides text if non-NULL */
} hidden_event_t;

typedef struct {
    const map_warp_t      *warps;
    uint8_t                num_warps;
    const npc_event_t     *npcs;
    uint8_t                num_npcs;
    const sign_event_t    *signs;
    uint8_t                num_signs;
    const item_event_t    *items;
    uint8_t                num_items;
    uint8_t                border_block;
    const map_trainer_t   *trainers;
    uint8_t                num_trainers;
    const hidden_event_t  *hidden_events;
    uint8_t                num_hidden_events;
} map_events_t;

#define NUM_MAPS 248
extern const map_events_t gMapEvents[NUM_MAPS];
