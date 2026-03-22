#pragma once
#include <stdint.h>

/* ============================================================
 * types.h — C struct definitions derived from pokered macros/ram.asm
 * All structs are byte-exact matches to the original Game Boy layout.
 * ============================================================ */

/* ---- Pokémon data structs --------------------------------- */

/* box_mon_t: 33 bytes — matches box_struct in macros/ram.asm
 * Used in PC boxes and as the base for party mons. */
typedef struct __attribute__((packed)) {
    uint8_t  species;           /* species ID (internal dex order) */
    uint16_t hp;                /* current HP (big-endian on GB, use BE accessors) */
    uint8_t  box_level;         /* level as stored in box (may differ from stat level) */
    uint8_t  status;            /* status condition byte (see STATUS_* constants) */
    uint8_t  type1;             /* primary type */
    uint8_t  type2;             /* secondary type */
    uint8_t  catch_rate;        /* catch rate (also used as held item in Gen 2) */
    uint8_t  moves[4];          /* move IDs */
    uint16_t ot_id;             /* original trainer ID */
    uint8_t  exp[3];            /* experience points (24-bit big-endian) */
    uint16_t stat_exp_hp;       /* HP stat experience */
    uint16_t stat_exp_atk;      /* Attack stat experience */
    uint16_t stat_exp_def;      /* Defense stat experience */
    uint16_t stat_exp_spd;      /* Speed stat experience */
    uint16_t stat_exp_spc;      /* Special stat experience */
    uint16_t dvs;               /* DVs packed: Atk[15:12] Def[11:8] Spd[7:4] Spc[3:0] */
    uint8_t  pp[4];             /* current PP for each move (PP_UP bits in high 2 bits) */
} box_mon_t; /* 33 bytes */

/* party_mon_t: 44 bytes (0x2C) — matches party_struct in macros/ram.asm
 * Extends box_mon_t with computed stats and actual level. */
typedef struct __attribute__((packed)) {
    box_mon_t base;             /* all box fields (33 bytes) */
    uint8_t   level;            /* actual level used for stat calculation */
    uint16_t  max_hp;           /* max HP */
    uint16_t  atk;              /* Attack stat */
    uint16_t  def;              /* Defense stat */
    uint16_t  spd;              /* Speed stat */
    uint16_t  spc;              /* Special stat */
} party_mon_t; /* 44 bytes */

/* battle_mon_t — battle copy of mon data (in wBattleMon / wEnemyMon WRAM) */
typedef struct __attribute__((packed)) {
    uint8_t  species;
    uint16_t hp;
    uint8_t  party_pos;
    uint8_t  status;
    uint8_t  type1;
    uint8_t  type2;
    uint8_t  catch_rate;
    uint8_t  moves[4];
    uint16_t dvs;
    uint8_t  level;
    uint16_t max_hp;
    uint16_t atk;
    uint16_t def;
    uint16_t spd;
    uint16_t spc;
    uint8_t  pp[4];
} battle_mon_t;

/* ---- Move data -------------------------------------------- */

/* move_t: 6 bytes — matches Moves table in data/moves/moves.asm */
typedef struct __attribute__((packed)) {
    uint8_t anim;       /* animation ID (same as move ID) */
    uint8_t effect;     /* effect ID (see EFFECT_* constants) */
    uint8_t power;      /* base power (0 = no damage) */
    uint8_t type;       /* type ID */
    uint8_t accuracy;   /* accuracy (stored as value*255/100 via percent macro) */
    uint8_t pp;         /* max PP (must be ≤40) */
} move_t; /* 6 bytes */

/* ---- Sprite state ----------------------------------------- */

/* sprite_state_data1_t: 16 bytes — per-sprite, indexed by sprite slot (0–15) */
typedef struct __attribute__((packed)) {
    uint8_t picture_id;         /* sprite picture ID */
    uint8_t movement_status;    /* 0=ready, 1=moving, 2=delayed */
    uint8_t image_index;        /* current frame/facing */
    uint8_t y_disp;             /* Y pixel displacement from tile */
    uint8_t x_disp;             /* X pixel displacement from tile */
    uint8_t map_y;              /* Y position on map (in tiles) */
    uint8_t map_x;              /* X position on map (in tiles) */
    uint8_t movement_byte_2;    /* movement type */
    uint8_t grass_priority;     /* grass overlay flag */
    uint8_t y_pixels;           /* Y pixel coordinate (screen) */
    uint8_t x_pixels;           /* X pixel coordinate (screen) */
    uint8_t intra_anim_frame;   /* within-tile movement frame counter */
    uint8_t collision_data;     /* direction bits blocking movement */
    uint8_t facing_direction;   /* current facing (1=down,2=up,4=left,8=right) */
    uint8_t _pad[2];
} sprite_state_data1_t; /* 16 bytes */

/* sprite_state_data2_t: 16 bytes */
typedef struct __attribute__((packed)) {
    uint8_t walk_animation_counter;
    uint8_t movement_byte1;     /* movement script type ($FF=no move) */
    uint8_t _unk1;
    uint8_t text_id;            /* which text/script to trigger */
    uint8_t trainer_class_or_item;
    uint8_t trainer_set_id;
    uint8_t _unk2[2];
    uint8_t y_displacement;     /* wandering counter Y (init $08) */
    uint8_t x_displacement;     /* wandering counter X (init $08) */
    uint8_t _unk3[6];
} sprite_state_data2_t; /* 16 bytes */

/* OAM entry — one hardware sprite */
typedef struct __attribute__((packed)) {
    uint8_t y;          /* Y position + 16 */
    uint8_t x;          /* X position + 8 */
    uint8_t tile;       /* tile index */
    uint8_t flags;      /* attribute flags (priority, flip, palette) */
} oam_entry_t;

/* ---- Map structures --------------------------------------- */

/* map_header_t — mirrors wCurMapHeader in WRAM */
typedef struct __attribute__((packed)) {
    uint8_t  tileset;           /* tileset ID */
    uint8_t  height;            /* map height in blocks */
    uint8_t  width;             /* map width in blocks */
    uint16_t blocks_ptr;        /* pointer to block data */
    uint16_t text_ptr;          /* pointer to text/script pointers */
    uint16_t script_ptr;        /* pointer to map script */
    uint8_t  connections;       /* bitmask of adjacent connections (N/S/E/W) */
} map_header_t; /* 9 bytes */

/* map_connection_t — one adjacent map connection strip (11 bytes) */
typedef struct __attribute__((packed)) {
    uint8_t  map_id;
    uint16_t strip_src_ptr;     /* source strip pointer in connected map */
    uint16_t strip_dst_ptr;     /* destination strip pointer in current map */
    uint8_t  strip_width;       /* width of connection strip */
    uint8_t  window;            /* SCX/SCY scroll offset for connection */
    uint8_t  map_pos;           /* position in block units */
    uint8_t  _pad[2];
} map_connection_t; /* 11 bytes */

/* warp_event_t: 4 bytes */
typedef struct __attribute__((packed)) {
    uint8_t y;
    uint8_t x;
    uint8_t dest_warp_id;       /* warp index in destination map (0-based) */
    uint8_t dest_map;           /* destination map ID */
} warp_event_t;

/* bg_event_t (sign): 3 bytes */
typedef struct __attribute__((packed)) {
    uint8_t y;
    uint8_t x;
    uint8_t text_id;
} bg_event_t;

/* ---- Tileset ---------------------------------------------- */

/* tileset_header_t: 12 bytes — matches Tilesets table */
typedef struct __attribute__((packed)) {
    uint8_t  bank;              /* ROM bank containing tileset GFX */
    uint16_t blocks_ptr;        /* pointer to block→tile mapping data */
    uint16_t gfx_ptr;           /* pointer to 2bpp tile graphics */
    uint16_t coll_ptr;          /* pointer to passability list ($FF-terminated) */
    uint8_t  counter[3];        /* counter tile IDs ($FF = none) */
    uint8_t  grass_tile;        /* grass tile ID ($FF = no grass) */
    uint8_t  anim;              /* TILEANIM_* value */
} tileset_header_t; /* 12 bytes */

/* ---- Base stats ------------------------------------------- */
#define NUM_TM_HM_BYTES  7      /* ceil(55 TMs + HMs / 8) */

typedef struct __attribute__((packed)) {
    uint8_t  dex_id;
    uint8_t  hp, atk, def, spd, spc;
    uint8_t  type1, type2;
    uint8_t  catch_rate;
    uint8_t  base_exp;
    uint8_t  sprite_dim;        /* packed WxH (upper nibble W, lower nibble H, in tiles) */
    uint16_t front_ptr;
    uint16_t back_ptr;
    uint8_t  start_moves[4];
    uint8_t  growth_rate;
    uint8_t  tmhm[NUM_TM_HM_BYTES];
} base_stats_t;

/* ---- Wild encounter data ---------------------------------- */
#define NUM_WILD_SLOTS  10

typedef struct __attribute__((packed)) {
    uint8_t encounter_rate;
    struct { uint8_t level; uint8_t species; } slots[NUM_WILD_SLOTS];
} wild_data_t; /* 21 bytes */

/* ---- Inventory item slot ---------------------------------- */
typedef struct __attribute__((packed)) {
    uint8_t item_id;
    uint8_t quantity;
} item_slot_t;
