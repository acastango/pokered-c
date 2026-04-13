/* moves_data.c -- Generated from pokered-master/data/moves/moves.asm */
#include "moves_data.h"

const move_t gMoves[NUM_MOVE_DEFS] = {
    [0] = { .anim=0x00, .effect=0x00, .power=  0, .type=0x00, .accuracy=  0, .pp= 0 },  /* NO_MOVE */
    [1] = { .anim=0x01, .effect=0x00, .power= 40, .type=0x00, .accuracy=255, .pp=35 },  /* POUND */
    [2] = { .anim=0x02, .effect=0x00, .power= 50, .type=0x00, .accuracy=255, .pp=25 },  /* KARATE_CHOP */
    [3] = { .anim=0x03, .effect=0x1D, .power= 15, .type=0x00, .accuracy=217, .pp=10 },  /* DOUBLESLAP */
    [4] = { .anim=0x04, .effect=0x1D, .power= 18, .type=0x00, .accuracy=217, .pp=15 },  /* COMET_PUNCH */
    [5] = { .anim=0x05, .effect=0x00, .power= 80, .type=0x00, .accuracy=217, .pp=20 },  /* MEGA_PUNCH */
    [6] = { .anim=0x06, .effect=0x10, .power= 40, .type=0x00, .accuracy=255, .pp=20 },  /* PAY_DAY */
    [7] = { .anim=0x07, .effect=0x04, .power= 75, .type=0x14, .accuracy=255, .pp=15 },  /* FIRE_PUNCH */
    [8] = { .anim=0x08, .effect=0x05, .power= 75, .type=0x19, .accuracy=255, .pp=15 },  /* ICE_PUNCH */
    [9] = { .anim=0x09, .effect=0x06, .power= 75, .type=0x17, .accuracy=255, .pp=15 },  /* THUNDERPUNCH */
    [10] = { .anim=0x0A, .effect=0x00, .power= 40, .type=0x00, .accuracy=255, .pp=35 },  /* SCRATCH */
    [11] = { .anim=0x0B, .effect=0x00, .power= 55, .type=0x00, .accuracy=255, .pp=30 },  /* VICEGRIP */
    [12] = { .anim=0x0C, .effect=0x1F, .power=  1, .type=0x00, .accuracy= 77, .pp= 5 },  /* GUILLOTINE */
    [13] = { .anim=0x0D, .effect=0x20, .power= 80, .type=0x00, .accuracy=191, .pp=10 },  /* RAZOR_WIND */
    [14] = { .anim=0x0E, .effect=0x2B, .power=  0, .type=0x00, .accuracy=255, .pp=30 },  /* SWORDS_DANCE */
    [15] = { .anim=0x0F, .effect=0x00, .power= 50, .type=0x00, .accuracy=243, .pp=30 },  /* CUT */
    [16] = { .anim=0x10, .effect=0x00, .power= 40, .type=0x00, .accuracy=255, .pp=35 },  /* GUST */
    [17] = { .anim=0x11, .effect=0x00, .power= 35, .type=0x02, .accuracy=255, .pp=35 },  /* WING_ATTACK */
    [18] = { .anim=0x12, .effect=0x1C, .power=  0, .type=0x00, .accuracy=217, .pp=20 },  /* WHIRLWIND */
    [19] = { .anim=0x13, .effect=0x24, .power= 70, .type=0x02, .accuracy=243, .pp=15 },  /* FLY */
    [20] = { .anim=0x14, .effect=0x23, .power= 15, .type=0x00, .accuracy=191, .pp=20 },  /* BIND */
    [21] = { .anim=0x15, .effect=0x00, .power= 80, .type=0x00, .accuracy=191, .pp=20 },  /* SLAM */
    [22] = { .anim=0x16, .effect=0x00, .power= 35, .type=0x16, .accuracy=255, .pp=10 },  /* VINE_WHIP */
    [23] = { .anim=0x17, .effect=0x25, .power= 65, .type=0x00, .accuracy=255, .pp=20 },  /* STOMP */
    [24] = { .anim=0x18, .effect=0x25, .power= 30, .type=0x01, .accuracy=255, .pp=30 },  /* DOUBLE_KICK */
    [25] = { .anim=0x19, .effect=0x00, .power=120, .type=0x00, .accuracy=191, .pp= 5 },  /* MEGA_KICK */
    [26] = { .anim=0x1A, .effect=0x26, .power= 70, .type=0x01, .accuracy=243, .pp=25 },  /* JUMP_KICK */
    [27] = { .anim=0x1B, .effect=0x25, .power= 60, .type=0x01, .accuracy=217, .pp=15 },  /* ROLLING_KICK */
    [28] = { .anim=0x1C, .effect=0x16, .power=  0, .type=0x00, .accuracy=255, .pp=15 },  /* SAND_ATTACK */
    [29] = { .anim=0x1D, .effect=0x25, .power= 70, .type=0x00, .accuracy=255, .pp=15 },  /* HEADBUTT */
    [30] = { .anim=0x1E, .effect=0x00, .power= 65, .type=0x00, .accuracy=255, .pp=25 },  /* HORN_ATTACK */
    [31] = { .anim=0x1F, .effect=0x1D, .power= 15, .type=0x00, .accuracy=217, .pp=20 },  /* FURY_ATTACK */
    [32] = { .anim=0x20, .effect=0x1F, .power=  1, .type=0x00, .accuracy= 77, .pp= 5 },  /* HORN_DRILL */
    [33] = { .anim=0x21, .effect=0x00, .power= 35, .type=0x00, .accuracy=243, .pp=35 },  /* TACKLE */
    [34] = { .anim=0x22, .effect=0x24, .power= 85, .type=0x00, .accuracy=255, .pp=15 },  /* BODY_SLAM */
    [35] = { .anim=0x23, .effect=0x23, .power= 15, .type=0x00, .accuracy=217, .pp=20 },  /* WRAP */
    [36] = { .anim=0x24, .effect=0x29, .power= 90, .type=0x00, .accuracy=217, .pp=20 },  /* TAKE_DOWN */
    [37] = { .anim=0x25, .effect=0x1B, .power= 90, .type=0x00, .accuracy=255, .pp=20 },  /* THRASH */
    [38] = { .anim=0x26, .effect=0x29, .power=100, .type=0x00, .accuracy=255, .pp=15 },  /* DOUBLE_EDGE */
    [39] = { .anim=0x27, .effect=0x13, .power=  0, .type=0x00, .accuracy=255, .pp=30 },  /* TAIL_WHIP */
    [40] = { .anim=0x28, .effect=0x02, .power= 15, .type=0x03, .accuracy=255, .pp=35 },  /* POISON_STING */
    [41] = { .anim=0x29, .effect=0x4d, .power= 25, .type=0x07, .accuracy=255, .pp=20 },  /* TWINEEDLE */
    [42] = { .anim=0x2A, .effect=0x1D, .power= 14, .type=0x07, .accuracy=217, .pp=20 },  /* PIN_MISSILE */
    [43] = { .anim=0x2B, .effect=0x13, .power=  0, .type=0x00, .accuracy=255, .pp=30 },  /* LEER */
    [44] = { .anim=0x2C, .effect=0x1E, .power= 60, .type=0x00, .accuracy=255, .pp=25 },  /* BITE */
    [45] = { .anim=0x2D, .effect=0x12, .power=  0, .type=0x00, .accuracy=255, .pp=40 },  /* GROWL */
    [46] = { .anim=0x2E, .effect=0x1C, .power=  0, .type=0x00, .accuracy=255, .pp=20 },  /* ROAR */
    [47] = { .anim=0x2F, .effect=0x01, .power=  0, .type=0x00, .accuracy=140, .pp=15 },  /* SING */
    [48] = { .anim=0x30, .effect=0x2A, .power=  0, .type=0x00, .accuracy=140, .pp=20 },  /* SUPERSONIC */
    [49] = { .anim=0x31, .effect=0x22, .power=  1, .type=0x00, .accuracy=230, .pp=20 },  /* SONICBOOM */
    [50] = { .anim=0x32, .effect=0x4D, .power=  0, .type=0x00, .accuracy=140, .pp=20 },  /* DISABLE */
    [51] = { .anim=0x33, .effect=0x45, .power= 40, .type=0x03, .accuracy=255, .pp=30 },  /* ACID */
    [52] = { .anim=0x34, .effect=0x04, .power= 40, .type=0x14, .accuracy=255, .pp=25 },  /* EMBER */
    [53] = { .anim=0x35, .effect=0x04, .power= 95, .type=0x14, .accuracy=255, .pp=15 },  /* FLAMETHROWER */
    [54] = { .anim=0x36, .effect=0x27, .power=  0, .type=0x19, .accuracy=255, .pp=30 },  /* MIST */
    [55] = { .anim=0x37, .effect=0x00, .power= 40, .type=0x15, .accuracy=255, .pp=25 },  /* WATER_GUN */
    [56] = { .anim=0x38, .effect=0x00, .power=120, .type=0x15, .accuracy=204, .pp= 5 },  /* HYDRO_PUMP */
    [57] = { .anim=0x39, .effect=0x00, .power= 95, .type=0x15, .accuracy=255, .pp=15 },  /* SURF */
    [58] = { .anim=0x3A, .effect=0x05, .power= 95, .type=0x19, .accuracy=255, .pp=10 },  /* ICE_BEAM */
    [59] = { .anim=0x3B, .effect=0x23, .power=120, .type=0x19, .accuracy=230, .pp= 5 },  /* BLIZZARD */
    [60] = { .anim=0x3C, .effect=0x4c, .power= 65, .type=0x18, .accuracy=255, .pp=20 },  /* PSYBEAM */
    [61] = { .anim=0x3D, .effect=0x46, .power= 65, .type=0x15, .accuracy=255, .pp=20 },  /* BUBBLEBEAM */
    [62] = { .anim=0x3E, .effect=0x44, .power= 65, .type=0x19, .accuracy=255, .pp=20 },  /* AURORA_BEAM */
    [63] = { .anim=0x3F, .effect=0x47, .power=150, .type=0x00, .accuracy=230, .pp= 5 },  /* HYPER_BEAM */
    [64] = { .anim=0x40, .effect=0x00, .power= 35, .type=0x02, .accuracy=255, .pp=35 },  /* PECK */
    [65] = { .anim=0x41, .effect=0x00, .power= 80, .type=0x02, .accuracy=255, .pp=20 },  /* DRILL_PECK */
    [66] = { .anim=0x42, .effect=0x29, .power= 80, .type=0x01, .accuracy=204, .pp=25 },  /* SUBMISSION */
    [67] = { .anim=0x43, .effect=0x25, .power= 50, .type=0x01, .accuracy=230, .pp=20 },  /* LOW_KICK */
    [68] = { .anim=0x44, .effect=0x00, .power=  1, .type=0x01, .accuracy=255, .pp=20 },  /* COUNTER */
    [69] = { .anim=0x45, .effect=0x22, .power=  1, .type=0x01, .accuracy=255, .pp=20 },  /* SEISMIC_TOSS */
    [70] = { .anim=0x46, .effect=0x00, .power= 80, .type=0x00, .accuracy=255, .pp=15 },  /* STRENGTH */
    [71] = { .anim=0x47, .effect=0x03, .power= 20, .type=0x16, .accuracy=255, .pp=20 },  /* ABSORB */
    [72] = { .anim=0x48, .effect=0x03, .power= 40, .type=0x16, .accuracy=255, .pp=10 },  /* MEGA_DRAIN */
    [73] = { .anim=0x49, .effect=0x54, .power=  0, .type=0x16, .accuracy=230, .pp=10 },  /* LEECH_SEED */
    [74] = { .anim=0x4A, .effect=0x0D, .power=  0, .type=0x00, .accuracy=255, .pp=40 },  /* GROWTH */
    [75] = { .anim=0x4B, .effect=0x00, .power= 55, .type=0x16, .accuracy=243, .pp=25 },  /* RAZOR_LEAF */
    [76] = { .anim=0x4C, .effect=0x20, .power=120, .type=0x16, .accuracy=255, .pp=10 },  /* SOLARBEAM */
    [77] = { .anim=0x4D, .effect=0x3A, .power=  0, .type=0x03, .accuracy=191, .pp=35 },  /* POISONPOWDER */
    [78] = { .anim=0x4E, .effect=0x3B, .power=  0, .type=0x16, .accuracy=191, .pp=30 },  /* STUN_SPORE */
    [79] = { .anim=0x4F, .effect=0x01, .power=  0, .type=0x16, .accuracy=191, .pp=15 },  /* SLEEP_POWDER */
    [80] = { .anim=0x50, .effect=0x1B, .power= 70, .type=0x16, .accuracy=255, .pp=20 },  /* PETAL_DANCE */
    [81] = { .anim=0x51, .effect=0x14, .power=  0, .type=0x07, .accuracy=243, .pp=40 },  /* STRING_SHOT */
    [82] = { .anim=0x52, .effect=0x22, .power=  1, .type=0x1A, .accuracy=255, .pp=10 },  /* DRAGON_RAGE */
    [83] = { .anim=0x53, .effect=0x23, .power= 15, .type=0x14, .accuracy=179, .pp=15 },  /* FIRE_SPIN */
    [84] = { .anim=0x54, .effect=0x06, .power= 40, .type=0x17, .accuracy=255, .pp=30 },  /* THUNDERSHOCK */
    [85] = { .anim=0x55, .effect=0x06, .power= 95, .type=0x17, .accuracy=255, .pp=15 },  /* THUNDERBOLT */
    [86] = { .anim=0x56, .effect=0x3B, .power=  0, .type=0x17, .accuracy=255, .pp=20 },  /* THUNDER_WAVE */
    [87] = { .anim=0x57, .effect=0x06, .power=120, .type=0x17, .accuracy=179, .pp=10 },  /* THUNDER */
    [88] = { .anim=0x58, .effect=0x00, .power= 50, .type=0x05, .accuracy=166, .pp=15 },  /* ROCK_THROW */
    [89] = { .anim=0x59, .effect=0x00, .power=100, .type=0x04, .accuracy=255, .pp=10 },  /* EARTHQUAKE */
    [90] = { .anim=0x5A, .effect=0x1F, .power=  1, .type=0x04, .accuracy= 77, .pp= 5 },  /* FISSURE */
    [91] = { .anim=0x5B, .effect=0x20, .power=100, .type=0x04, .accuracy=255, .pp=10 },  /* DIG */
    [92] = { .anim=0x5C, .effect=0x3A, .power=  0, .type=0x03, .accuracy=217, .pp=10 },  /* TOXIC */
    [93] = { .anim=0x5D, .effect=0x4c, .power= 50, .type=0x18, .accuracy=255, .pp=25 },  /* CONFUSION */
    [94] = { .anim=0x5E, .effect=0x47, .power= 90, .type=0x18, .accuracy=255, .pp=10 },  /* PSYCHIC_M */
    [95] = { .anim=0x5F, .effect=0x01, .power=  0, .type=0x18, .accuracy=153, .pp=20 },  /* HYPNOSIS */
    [96] = { .anim=0x60, .effect=0x0A, .power=  0, .type=0x18, .accuracy=255, .pp=40 },  /* MEDITATE */
    [97] = { .anim=0x61, .effect=0x0C, .power=  0, .type=0x18, .accuracy=255, .pp=30 },  /* AGILITY */
    [98] = { .anim=0x62, .effect=0x00, .power= 40, .type=0x00, .accuracy=255, .pp=30 },  /* QUICK_ATTACK */
    [99] = { .anim=0x63, .effect=0x48, .power= 20, .type=0x00, .accuracy=255, .pp=20 },  /* RAGE */
    [100] = { .anim=0x64, .effect=0x1C, .power=  0, .type=0x18, .accuracy=255, .pp=20 },  /* TELEPORT */
    [101] = { .anim=0x65, .effect=0x22, .power=  0, .type=0x08, .accuracy=255, .pp=15 },  /* NIGHT_SHADE */
    [102] = { .anim=0x66, .effect=0x49, .power=  0, .type=0x00, .accuracy=255, .pp=10 },  /* MIMIC */
    [103] = { .anim=0x67, .effect=0x33, .power=  0, .type=0x00, .accuracy=217, .pp=40 },  /* SCREECH */
    [104] = { .anim=0x68, .effect=0x0F, .power=  0, .type=0x00, .accuracy=255, .pp=15 },  /* DOUBLE_TEAM */
    [105] = { .anim=0x69, .effect=0x30, .power=  0, .type=0x00, .accuracy=255, .pp=20 },  /* RECOVER */
    [106] = { .anim=0x6A, .effect=0x0B, .power=  0, .type=0x00, .accuracy=255, .pp=30 },  /* HARDEN */
    [107] = { .anim=0x6B, .effect=0x0F, .power=  0, .type=0x00, .accuracy=255, .pp=20 },  /* MINIMIZE */
    [108] = { .anim=0x6C, .effect=0x16, .power=  0, .type=0x00, .accuracy=255, .pp=20 },  /* SMOKESCREEN */
    [109] = { .anim=0x6D, .effect=0x2A, .power=  0, .type=0x08, .accuracy=255, .pp=10 },  /* CONFUSE_RAY */
    [110] = { .anim=0x6E, .effect=0x0B, .power=  0, .type=0x15, .accuracy=255, .pp=40 },  /* WITHDRAW */
    [111] = { .anim=0x6F, .effect=0x0B, .power=  0, .type=0x00, .accuracy=255, .pp=40 },  /* DEFENSE_CURL */
    [112] = { .anim=0x70, .effect=0x2C, .power=  0, .type=0x18, .accuracy=255, .pp=30 },  /* BARRIER */
    [113] = { .anim=0x71, .effect=0x38, .power=  0, .type=0x18, .accuracy=255, .pp=30 },  /* LIGHT_SCREEN */
    [114] = { .anim=0x72, .effect=0x19, .power=  0, .type=0x19, .accuracy=255, .pp=30 },  /* HAZE */
    [115] = { .anim=0x73, .effect=0x39, .power=  0, .type=0x18, .accuracy=255, .pp=20 },  /* REFLECT */
    [116] = { .anim=0x74, .effect=0x28, .power=  0, .type=0x00, .accuracy=255, .pp=30 },  /* FOCUS_ENERGY */
    [117] = { .anim=0x75, .effect=0x1A, .power=  0, .type=0x00, .accuracy=255, .pp=10 },  /* BIDE */
    [118] = { .anim=0x76, .effect=0x4A, .power=  0, .type=0x00, .accuracy=255, .pp=10 },  /* METRONOME */
    [119] = { .anim=0x77, .effect=0x09, .power=  0, .type=0x02, .accuracy=255, .pp=20 },  /* MIRROR_MOVE */
    [120] = { .anim=0x78, .effect=0x07, .power=130, .type=0x00, .accuracy=255, .pp= 5 },  /* SELFDESTRUCT */
    [121] = { .anim=0x79, .effect=0x00, .power=100, .type=0x00, .accuracy=191, .pp=10 },  /* EGG_BOMB */
    [122] = { .anim=0x7A, .effect=0x24, .power= 20, .type=0x08, .accuracy=255, .pp=30 },  /* LICK */
    [123] = { .anim=0x7B, .effect=0x4E, .power= 20, .type=0x03, .accuracy=179, .pp=20 },  /* SMOG */
    [124] = { .anim=0x7C, .effect=0x4E, .power= 65, .type=0x03, .accuracy=255, .pp=20 },  /* SLUDGE */
    [125] = { .anim=0x7D, .effect=0x1E, .power= 65, .type=0x04, .accuracy=217, .pp=20 },  /* BONE_CLUB */
    [126] = { .anim=0x7E, .effect=0x22, .power=120, .type=0x14, .accuracy=217, .pp= 5 },  /* FIRE_BLAST */
    [127] = { .anim=0x7F, .effect=0x00, .power= 80, .type=0x15, .accuracy=255, .pp=15 },  /* WATERFALL */
    [128] = { .anim=0x80, .effect=0x23, .power= 35, .type=0x15, .accuracy=191, .pp=10 },  /* CLAMP */
    [129] = { .anim=0x81, .effect=0x11, .power= 60, .type=0x00, .accuracy=255, .pp=20 },  /* SWIFT */
    [130] = { .anim=0x82, .effect=0x20, .power=100, .type=0x00, .accuracy=255, .pp=15 },  /* SKULL_BASH */
    [131] = { .anim=0x83, .effect=0x1D, .power= 20, .type=0x00, .accuracy=255, .pp=15 },  /* SPIKE_CANNON */
    [132] = { .anim=0x84, .effect=0x46, .power= 10, .type=0x00, .accuracy=255, .pp=35 },  /* CONSTRICT */
    [133] = { .anim=0x85, .effect=0x2D, .power=  0, .type=0x18, .accuracy=255, .pp=20 },  /* AMNESIA */
    [134] = { .anim=0x86, .effect=0x16, .power=  0, .type=0x18, .accuracy=204, .pp=15 },  /* KINESIS */
    [135] = { .anim=0x87, .effect=0x30, .power=  0, .type=0x00, .accuracy=255, .pp=10 },  /* SOFTBOILED */
    [136] = { .anim=0x88, .effect=0x26, .power= 85, .type=0x01, .accuracy=230, .pp=20 },  /* HI_JUMP_KICK */
    [137] = { .anim=0x89, .effect=0x3B, .power=  0, .type=0x00, .accuracy=191, .pp=30 },  /* GLARE */
    [138] = { .anim=0x8A, .effect=0x08, .power=100, .type=0x18, .accuracy=255, .pp=15 },  /* DREAM_EATER */
    [139] = { .anim=0x8B, .effect=0x3A, .power=  0, .type=0x03, .accuracy=140, .pp=40 },  /* POISON_GAS */
    [140] = { .anim=0x8C, .effect=0x1D, .power= 15, .type=0x00, .accuracy=217, .pp=20 },  /* BARRAGE */
    [141] = { .anim=0x8D, .effect=0x03, .power= 20, .type=0x07, .accuracy=255, .pp=15 },  /* LEECH_LIFE */
    [142] = { .anim=0x8E, .effect=0x01, .power=  0, .type=0x00, .accuracy=191, .pp=10 },  /* LOVELY_KISS */
    [143] = { .anim=0x8F, .effect=0x20, .power=140, .type=0x02, .accuracy=230, .pp= 5 },  /* SKY_ATTACK */
    [144] = { .anim=0x90, .effect=0x31, .power=  0, .type=0x00, .accuracy=255, .pp=10 },  /* TRANSFORM */
    [145] = { .anim=0x91, .effect=0x46, .power= 20, .type=0x15, .accuracy=255, .pp=30 },  /* BUBBLE */
    [146] = { .anim=0x92, .effect=0x00, .power= 70, .type=0x00, .accuracy=255, .pp=10 },  /* DIZZY_PUNCH */
    [147] = { .anim=0x93, .effect=0x01, .power=  0, .type=0x16, .accuracy=255, .pp=15 },  /* SPORE */
    [148] = { .anim=0x94, .effect=0x16, .power=  0, .type=0x00, .accuracy=179, .pp=20 },  /* FLASH */
    [149] = { .anim=0x95, .effect=0x22, .power=  1, .type=0x18, .accuracy=204, .pp=15 },  /* PSYWAVE */
    [150] = { .anim=0x96, .effect=0x4C, .power=  0, .type=0x00, .accuracy=255, .pp=40 },  /* SPLASH */
    [151] = { .anim=0x97, .effect=0x2C, .power=  0, .type=0x03, .accuracy=255, .pp=40 },  /* ACID_ARMOR */
    [152] = { .anim=0x98, .effect=0x00, .power= 90, .type=0x15, .accuracy=217, .pp=10 },  /* CRABHAMMER */
    [153] = { .anim=0x99, .effect=0x07, .power=170, .type=0x00, .accuracy=255, .pp= 5 },  /* EXPLOSION */
    [154] = { .anim=0x9A, .effect=0x1D, .power= 18, .type=0x00, .accuracy=204, .pp=15 },  /* FURY_SWIPES */
    [155] = { .anim=0x9B, .effect=0x25, .power= 50, .type=0x04, .accuracy=230, .pp=10 },  /* BONEMERANG */
    [156] = { .anim=0x9C, .effect=0x30, .power=  0, .type=0x18, .accuracy=255, .pp=10 },  /* REST */
    [157] = { .anim=0x9D, .effect=0x00, .power= 75, .type=0x05, .accuracy=230, .pp=10 },  /* ROCK_SLIDE */
    [158] = { .anim=0x9E, .effect=0x1E, .power= 80, .type=0x00, .accuracy=230, .pp=15 },  /* HYPER_FANG */
    [159] = { .anim=0x9F, .effect=0x0A, .power=  0, .type=0x00, .accuracy=255, .pp=30 },  /* SHARPEN */
    [160] = { .anim=0xA0, .effect=0x18, .power=  0, .type=0x00, .accuracy=255, .pp=30 },  /* CONVERSION */
    [161] = { .anim=0xA1, .effect=0x00, .power= 80, .type=0x00, .accuracy=255, .pp=10 },  /* TRI_ATTACK */
    [162] = { .anim=0xA2, .effect=0x21, .power=  1, .type=0x00, .accuracy=230, .pp=10 },  /* SUPER_FANG */
    [163] = { .anim=0xA3, .effect=0x00, .power= 70, .type=0x00, .accuracy=255, .pp=20 },  /* SLASH */
    [164] = { .anim=0xA4, .effect=0x46, .power=  0, .type=0x00, .accuracy=255, .pp=10 },  /* SUBSTITUTE */
    [165] = { .anim=0xA5, .effect=0x29, .power= 50, .type=0x00, .accuracy=255, .pp=10 },  /* STRUGGLE */
};

/* gMoveNames — pokered-master/data/moves/names.asm
 * Index 0 = no move, indices 1-165 match move IDs. */
const char * const gMoveNames[166] = {
    [0]   = "------",
    [1]   = "POUND",
    [2]   = "KARATE CHOP",
    [3]   = "DOUBLESLAP",
    [4]   = "COMET PUNCH",
    [5]   = "MEGA PUNCH",
    [6]   = "PAY DAY",
    [7]   = "FIRE PUNCH",
    [8]   = "ICE PUNCH",
    [9]   = "THUNDERPUNCH",
    [10]  = "SCRATCH",
    [11]  = "VICEGRIP",
    [12]  = "GUILLOTINE",
    [13]  = "RAZOR WIND",
    [14]  = "SWORDS DANCE",
    [15]  = "CUT",
    [16]  = "GUST",
    [17]  = "WING ATTACK",
    [18]  = "WHIRLWIND",
    [19]  = "FLY",
    [20]  = "BIND",
    [21]  = "SLAM",
    [22]  = "VINE WHIP",
    [23]  = "STOMP",
    [24]  = "DOUBLE KICK",
    [25]  = "MEGA KICK",
    [26]  = "JUMP KICK",
    [27]  = "ROLLING KICK",
    [28]  = "SAND-ATTACK",
    [29]  = "HEADBUTT",
    [30]  = "HORN ATTACK",
    [31]  = "FURY ATTACK",
    [32]  = "HORN DRILL",
    [33]  = "TACKLE",
    [34]  = "BODY SLAM",
    [35]  = "WRAP",
    [36]  = "TAKE DOWN",
    [37]  = "THRASH",
    [38]  = "DOUBLE-EDGE",
    [39]  = "TAIL WHIP",
    [40]  = "POISON STING",
    [41]  = "TWINEEDLE",
    [42]  = "PIN MISSILE",
    [43]  = "LEER",
    [44]  = "BITE",
    [45]  = "GROWL",
    [46]  = "ROAR",
    [47]  = "SING",
    [48]  = "SUPERSONIC",
    [49]  = "SONICBOOM",
    [50]  = "DISABLE",
    [51]  = "ACID",
    [52]  = "EMBER",
    [53]  = "FLAMETHROWER",
    [54]  = "MIST",
    [55]  = "WATER GUN",
    [56]  = "HYDRO PUMP",
    [57]  = "SURF",
    [58]  = "ICE BEAM",
    [59]  = "BLIZZARD",
    [60]  = "PSYBEAM",
    [61]  = "BUBBLEBEAM",
    [62]  = "AURORA BEAM",
    [63]  = "HYPER BEAM",
    [64]  = "PECK",
    [65]  = "DRILL PECK",
    [66]  = "SUBMISSION",
    [67]  = "LOW KICK",
    [68]  = "COUNTER",
    [69]  = "SEISMIC TOSS",
    [70]  = "STRENGTH",
    [71]  = "ABSORB",
    [72]  = "MEGA DRAIN",
    [73]  = "LEECH SEED",
    [74]  = "GROWTH",
    [75]  = "RAZOR LEAF",
    [76]  = "SOLARBEAM",
    [77]  = "POISONPOWDER",
    [78]  = "STUN SPORE",
    [79]  = "SLEEP POWDER",
    [80]  = "PETAL DANCE",
    [81]  = "STRING SHOT",
    [82]  = "DRAGON RAGE",
    [83]  = "FIRE SPIN",
    [84]  = "THUNDERSHOCK",
    [85]  = "THUNDERBOLT",
    [86]  = "THUNDER WAVE",
    [87]  = "THUNDER",
    [88]  = "ROCK THROW",
    [89]  = "EARTHQUAKE",
    [90]  = "FISSURE",
    [91]  = "DIG",
    [92]  = "TOXIC",
    [93]  = "CONFUSION",
    [94]  = "PSYCHIC",
    [95]  = "HYPNOSIS",
    [96]  = "MEDITATE",
    [97]  = "AGILITY",
    [98]  = "QUICK ATTACK",
    [99]  = "RAGE",
    [100] = "TELEPORT",
    [101] = "NIGHT SHADE",
    [102] = "MIMIC",
    [103] = "SCREECH",
    [104] = "DOUBLE TEAM",
    [105] = "RECOVER",
    [106] = "HARDEN",
    [107] = "MINIMIZE",
    [108] = "SMOKESCREEN",
    [109] = "CONFUSE RAY",
    [110] = "WITHDRAW",
    [111] = "DEFENSE CURL",
    [112] = "BARRIER",
    [113] = "LIGHT SCREEN",
    [114] = "HAZE",
    [115] = "REFLECT",
    [116] = "FOCUS ENERGY",
    [117] = "BIDE",
    [118] = "METRONOME",
    [119] = "MIRROR MOVE",
    [120] = "SELFDESTRUCT",
    [121] = "EGG BOMB",
    [122] = "LICK",
    [123] = "SMOG",
    [124] = "SLUDGE",
    [125] = "BONE CLUB",
    [126] = "FIRE BLAST",
    [127] = "WATERFALL",
    [128] = "CLAMP",
    [129] = "SWIFT",
    [130] = "SKULL BASH",
    [131] = "SPIKE CANNON",
    [132] = "CONSTRICT",
    [133] = "AMNESIA",
    [134] = "KINESIS",
    [135] = "SOFTBOILED",
    [136] = "HI JUMP KICK",
    [137] = "GLARE",
    [138] = "DREAM EATER",
    [139] = "POISON GAS",
    [140] = "BARRAGE",
    [141] = "LEECH LIFE",
    [142] = "LOVELY KISS",
    [143] = "SKY ATTACK",
    [144] = "TRANSFORM",
    [145] = "BUBBLE",
    [146] = "DIZZY PUNCH",
    [147] = "SPORE",
    [148] = "FLASH",
    [149] = "PSYWAVE",
    [150] = "SPLASH",
    [151] = "ACID ARMOR",
    [152] = "CRABHAMMER",
    [153] = "EXPLOSION",
    [154] = "FURY SWIPES",
    [155] = "BONEMERANG",
    [156] = "REST",
    [157] = "ROCK SLIDE",
    [158] = "HYPER FANG",
    [159] = "SHARPEN",
    [160] = "CONVERSION",
    [161] = "TRI ATTACK",
    [162] = "SUPER FANG",
    [163] = "SLASH",
    [164] = "SUBSTITUTE",
    [165] = "STRUGGLE",
};
