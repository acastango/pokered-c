#pragma once
#include <stdint.h>

/* ============================================================
 * constants.h — All game constants derived from pokered constants/
 * ============================================================ */

/* ---- Screen / display ------------------------------------ */
#define SCREEN_WIDTH        20      /* tiles */
#define SCREEN_HEIGHT       18      /* tiles */
#define SCREEN_AREA         360     /* tiles */
#define SCREEN_WIDTH_PX     160
#define SCREEN_HEIGHT_PX    144
#define TILEMAP_WIDTH       32      /* BG map width in tiles */
#define TILEMAP_HEIGHT      32
#define TILE_SIZE           16      /* bytes per 2bpp 8x8 tile */
#define TILE_PX             8       /* pixels per tile side */
#define OAM_Y_OFS           16      /* OAM Y coordinate offset */
#define OAM_X_OFS           8       /* OAM X coordinate offset */
#define MAX_SPRITES         80      /* extended OAM: player(4) + 16 NPCs(64) + slack */

/* ---- Block / map rendering ------------------------------- */
#define BLOCK_WIDTH         4       /* tiles per block (horizontal) */
#define BLOCK_HEIGHT        4       /* tiles per block (vertical) */
#define SCREEN_BLOCK_WIDTH  6       /* blocks visible horizontally */
#define SCREEN_BLOCK_HEIGHT 5       /* blocks visible vertically */
#define SURROUNDING_WIDTH   24      /* tiles in wSurroundingTiles row */
#define SURROUNDING_HEIGHT  20      /* tiles in wSurroundingTiles col */
#define MAP_BORDER          3       /* block border padding in wOverworldMap */

/* ---- Joypad ---------------------------------------------- */
#define PAD_A               0x01
#define PAD_B               0x02
#define PAD_SELECT          0x04
#define PAD_START           0x08
#define PAD_RIGHT           0x10
#define PAD_LEFT            0x20
#define PAD_UP              0x40
#define PAD_DOWN            0x80
#define PAD_BUTTONS         0x0F    /* A | B | SELECT | START */
#define PAD_CTRL_PAD        0xF0    /* direction bits */

/* ---- Mon status byte ------------------------------------- */
#define STATUS_SLP_MASK     0x07    /* bits 2-0: sleep turn counter */
#define STATUS_PSN          (1<<3)
#define STATUS_BRN          (1<<4)
#define STATUS_FRZ          (1<<5)
#define STATUS_PAR          (1<<6)
#define IS_ASLEEP(s)        ((s) & STATUS_SLP_MASK)
#define IS_POISONED(s)      ((s) & STATUS_PSN)
#define IS_BURNED(s)        ((s) & STATUS_BRN)
#define IS_FROZEN(s)        ((s) & STATUS_FRZ)
#define IS_PARALYZED(s)     ((s) & STATUS_PAR)

/* ---- Battle stat stages ---------------------------------- */
#define STAT_STAGE_MIN      1
#define STAT_STAGE_NORMAL   7
#define STAT_STAGE_MAX      13
#define NUM_STAT_MODS       6       /* Atk/Def/Spd/Spc/Acc/Eva */
/* StatModifierRatios[stage-1] = {numerator, denominator}
 * Stage 1=0.25, 7=1.00, 13=4.00 */
static const uint16_t STAT_MOD_NUM[13] = {25,28,33,40,50,66,100,150,200,250,300,350,400};
static const uint16_t STAT_MOD_DEN[13] = {100,100,100,100,100,100,100,100,100,100,100,100,100};

/* ---- Party / box sizes ----------------------------------- */
#define PARTY_LENGTH        6
#define BOX_CAPACITY        20
#define NUM_BOXES           12
#define BAG_ITEM_CAPACITY   20
#define PC_ITEM_CAPACITY    50
#define NUM_MOVES           4       /* moves per mon */
#define MAX_LEVEL           100
#define NUM_POKEMON         151
#define NUM_WILD_SLOTS      10

/* ---- Name lengths ---------------------------------------- */
#define NAME_LENGTH         11      /* mon/player name buffer (includes terminator) */
#define PLAYER_NAME_LENGTH  8
#define MOVE_NAME_LENGTH    14
#define ITEM_NAME_LENGTH    13

/* ---- Move struct ----------------------------------------- */
#define MOVE_LENGTH         6

/* ---- PP -------------------------------------------------- */
#define PP_UP_MASK          0xC0    /* high 2 bits of PP byte = PP UP count */
#define PP_MASK             0x3F    /* low 6 bits = current PP */

/* ---- Type IDs -------------------------------------------- */
#define TYPE_NORMAL         0
#define TYPE_FIGHTING       1
#define TYPE_FLYING         2
#define TYPE_POISON         3
#define TYPE_GROUND         4
#define TYPE_ROCK           5
#define TYPE_BIRD           6       /* unused */
#define TYPE_BUG            7
#define TYPE_GHOST          8
/* 9-19 unused gap */
#define TYPE_FIRE           20
#define TYPE_WATER          21
#define TYPE_GRASS          22
#define TYPE_ELECTRIC       23
#define TYPE_PSYCHIC        24
#define TYPE_ICE            25
#define TYPE_DRAGON         26
#define NUM_TYPES           27

/* Type effectiveness multipliers */
#define TYPE_SUPER_EFFECTIVE    2
#define TYPE_NORMAL_EFFECTIVE   1
#define TYPE_NOT_VERY_EFFECTIVE 0   /* stored as 0.5 in ASM; applied as >>1 */
#define TYPE_NO_EFFECT          0   /* zero damage */

/* ---- Tileset IDs ----------------------------------------- */
#define TILESET_OVERWORLD   0
#define TILESET_REDS_HOUSE1 1
#define TILESET_MART        2
#define TILESET_FOREST      3
#define TILESET_REDS_HOUSE2 4
#define TILESET_DOJO        5
#define TILESET_POKECENTER  6
#define TILESET_GYM         7
#define TILESET_HOUSE       8
#define TILESET_FOREST_GATE 9
#define TILESET_MUSEUM      10
#define TILESET_UNDERGROUND 11
#define TILESET_GATE        12
#define TILESET_SHIP        13
#define TILESET_SHIP_PORT   14
#define TILESET_CEMETERY    15
#define TILESET_INTERIOR    16
#define TILESET_CAVERN      17
#define TILESET_LOBBY       18
#define TILESET_MANSION     19
#define TILESET_LAB         20
#define TILESET_CLUB        21
#define TILESET_FACILITY    22
#define TILESET_PLATEAU     23
#define NUM_TILESETS        24

/* Tile animation types */
#define TILEANIM_NONE           0
#define TILEANIM_WATER          1
#define TILEANIM_WATER_FLOWER   2

/* ---- Growth rates ---------------------------------------- */
#define GROWTH_MEDIUM_FAST      0
#define GROWTH_SOMEWHAT_FAST    3
#define GROWTH_MEDIUM_SLOW      4
#define GROWTH_FAST             5
#define GROWTH_SLOW             1   /* actually index varies */
#define GROWTH_SLOW2            2

/* ---- Evolution types ------------------------------------- */
#define EVOLVE_LEVEL        1
#define EVOLVE_ITEM         2
#define EVOLVE_TRADE        3

/* ---- Audio channels -------------------------------------- */
#define CHAN1               0   /* Square 1 (music) */
#define CHAN2               1   /* Square 2 (music) */
#define CHAN3               2   /* Wave    (music) */
#define CHAN4               3   /* Noise   (music) */
#define CHAN5               4   /* Square 1 (SFX) */
#define CHAN6               5   /* Square 2 (SFX) */
#define CHAN7               6   /* Wave    (SFX) */
#define CHAN8               7   /* Noise   (SFX) */
#define NUM_CHANNELS        8
#define NUM_MUSIC_CHANS     4
#define SFX_STOP_ALL_MUSIC  0xFF
/* GB square channel freq → Hz: 131072 / (2048 - gb_freq) */
/* GB wave channel freq → Hz:    65536 / (2048 - gb_freq) */

/* ---- OAM flags ------------------------------------------- */
#define OAM_FLAG_PRIORITY   0x80    /* sprite behind BG */
#define OAM_FLAG_FLIP_Y     0x40
#define OAM_FLAG_FLIP_X     0x20
#define OAM_FLAG_PALETTE    0x10    /* OBJ palette 0 or 1 */

/* ---- Map/warp constants ---------------------------------- */
#define MAX_WARP_EVENTS     32
#define MAX_BG_EVENTS       16
#define MAX_OBJECT_EVENTS   16
#define SPRITE_SET_LENGTH   11
#define DEST_WARP_NONE      0xFF

/* ---- Battle constants ------------------------------------ */
#define MIN_NEUTRAL_DAMAGE  2
#define MAX_NEUTRAL_DAMAGE  999
#define MAX_PREDEF          112

/* ---- Save ------------------------------------------------ */
#define HOF_TEAM_CAPACITY   50
/* Checksum: ~(sum of all bytes in sMainData section) */

/* ---- Text engine opcodes --------------------------------- */
#define TX_START            0x00
#define TX_RAM              0x01
#define TX_BCD              0x02
#define TX_MOVE             0x03
#define TX_BOX              0x04
#define TX_LOW              0x05
#define TX_PROMPT_BUTTON    0x06
#define TX_SCROLL           0x07
#define TX_START_ASM        0x08
#define TX_NUM              0x09
#define TX_PAUSE            0x0A
#define TX_SOUND_GET_ITEM1  0x0B
#define TX_DOTS             0x0C
#define TX_WAIT_BUTTON      0x0D
#define TX_END              0x50
/* Script IDs (high byte dispatch) */
#define TX_SCRIPT_VENDING_MACHINE           0xF5
#define TX_SCRIPT_CABLE_CLUB_RECEPTIONIST   0xF6
#define TX_SCRIPT_PRIZE_VENDOR              0xF7
#define TX_SCRIPT_POKECENTER_PC             0xF9
#define TX_SCRIPT_PLAYERS_PC                0xFC
#define TX_SCRIPT_BILLS_PC                  0xFD
#define TX_SCRIPT_MART                      0xFE
#define TX_SCRIPT_POKECENTER_NURSE          0xFF

/* ---- Move effects ---------------------------------------- */
#define EFFECT_NONE                     0x00
#define EFFECT_SLEEP                    0x20
#define EFFECT_POISON_SIDE1             0x02
#define EFFECT_DRAIN_HP                 0x03
#define EFFECT_BURN_SIDE                0x04
#define EFFECT_FREEZE_SIDE              0x05
#define EFFECT_PARALYZE_SIDE            0x06
#define EFFECT_EXPLODE                  0x07
#define EFFECT_DREAM_EATER              0x08
#define EFFECT_MIRROR_MOVE              0x09
#define EFFECT_ATTACK_UP1               0x0A
#define EFFECT_DEFENSE_UP1              0x0B
#define EFFECT_SPEED_UP1                0x0C
#define EFFECT_SPECIAL_UP1              0x0D
#define EFFECT_ACCURACY_UP1             0x0E
#define EFFECT_EVASION_UP1              0x0F
#define EFFECT_PAY_DAY                  0x10
#define EFFECT_SWIFT                    0x11
#define EFFECT_OHKO                     0x26
#define EFFECT_CHARGE                   0x27
#define EFFECT_SUPER_FANG               0x28
#define EFFECT_SPECIAL_DAMAGE           0x29
#define EFFECT_TRAPPING                 0x2A
#define EFFECT_FLY                      0x2B
#define EFFECT_ATTACK_TWICE             0x2C
#define EFFECT_JUMP_KICK                0x2D
#define EFFECT_MIST                     0x2E
#define EFFECT_FOCUS_ENERGY             0x2F
#define EFFECT_RECOIL                   0x30
#define EFFECT_CONFUSION                0x31
#define EFFECT_ATTACK_UP2               0x32
#define EFFECT_DEFENSE_UP2              0x33
#define EFFECT_SPEED_UP2                0x34
#define EFFECT_SPECIAL_UP2              0x35
#define EFFECT_HEAL                     0x38
#define EFFECT_TRANSFORM                0x39
#define EFFECT_LIGHT_SCREEN             0x40
#define EFFECT_REFLECT                  0x41
#define EFFECT_POISON                   0x42
#define EFFECT_PARALYZE                 0x43
#define EFFECT_CONFUSION_SIDE           0x4C
#define EFFECT_TWINEEDLE                0x4D
#define EFFECT_SUBSTITUTE               0x4F
#define EFFECT_HYPER_BEAM               0x50
#define EFFECT_RAGE                     0x51
#define EFFECT_MIMIC                    0x52
#define EFFECT_METRONOME                0x53
#define EFFECT_LEECH_SEED               0x54
#define EFFECT_SPLASH                   0x55
#define EFFECT_DISABLE                  0x56
