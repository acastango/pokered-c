#pragma once
#include <stdint.h>
#include "../game/types.h"
#include "../game/constants.h"

/* ============================================================
 * hardware.h — Game Boy hardware abstraction layer for PC port.
 *
 * On the original GB, these were hardware registers or HRAM
 * locations. On PC they are plain C globals updated each frame
 * by the platform layer before game logic runs.
 * ============================================================ */

/* ---- HRAM shadow registers (fast-access on GB) ----------- */
/* Joypad — written by platform/input.c each frame */
extern uint8_t hJoyInput;      /* raw: SET bit = pressed */
extern uint8_t hJoyHeld;       /* currently held */
extern uint8_t hJoyPressed;    /* newly pressed this frame */
extern uint8_t hJoyReleased;   /* newly released this frame */
extern uint8_t wJoyIgnore;     /* mask: bits set here are ignored */

/* Random number state — updated every frame by platform */
extern uint8_t hRandomAdd;     /* pseudo-random byte A */
extern uint8_t hRandomSub;     /* pseudo-random byte B */

/* Frame counter */
extern uint8_t hFrameCounter;  /* decremented each VBlank */
extern uint8_t hVBlankOccurred;

/* Scroll registers (shadows of rSCX/rSCY) */
extern uint8_t hSCX;
extern uint8_t hSCY;
extern uint8_t hWY;            /* window Y */

/* Loaded ROM bank (no-op on PC — all code is in one address space) */
extern uint8_t hLoadedROMBank;

/* Audio DMA / OAM DMA flags */
extern uint8_t hAutoBGTransferEnabled;
extern uint8_t hTileAnimations;

/* Multiply/divide workspace */
extern uint32_t hDividend;
extern uint8_t  hDivisor;
extern uint32_t hQuotient;
extern uint8_t  hMultiplicand[3];
extern uint8_t  hMultiplier;
extern uint8_t  hProduct[3];

/* Text / display */
extern uint8_t hTextID;
extern uint8_t hItemAlreadyFound;

/* ---- WRAM: Video buffers ---------------------------------- */

/* wTileMap: 20×18 = 360 tile IDs visible on screen.
 * Written by LoadCurrentMapView, transferred to VRAM each VBlank.
 * On PC: read by display renderer to draw the background. */
extern uint8_t wTileMap[TILEMAP_WIDTH * TILEMAP_HEIGHT];

/* wSurroundingTiles: 24×20 buffer used during block expansion */
extern uint8_t wSurroundingTiles[SURROUNDING_WIDTH * SURROUNDING_HEIGHT];

/* wOverworldMap: full map in block IDs (max ~1300 bytes) */
extern uint8_t wOverworldMap[1300];

/* wTileMapBackup / wTileMapBackup2: saved screen buffers */
extern uint8_t wTileMapBackup[SCREEN_AREA];
extern uint8_t wTileMapBackup2[SCREEN_AREA];

/* wShadowOAM: 40 sprite entries updated by PrepareOAMData each frame.
 * On PC: read by display renderer to draw sprites. */
extern oam_entry_t wShadowOAM[MAX_SPRITES];

/* ---- WRAM: Tileset state ---------------------------------- */
extern uint8_t  wTilesetBank;
extern uint16_t wTilesetBlocksPtr;  /* points into loaded block data */
extern uint16_t wTilesetGfxPtr;     /* points into loaded tile graphics */
extern uint16_t wTilesetCollisionPtr; /* passability list */
extern uint8_t  wTilesetTalkingOverTiles[3];
extern uint8_t  wGrassTile;
extern uint8_t  wCurMapTileset;

/* ---- WRAM: Map state ------------------------------------- */
extern uint8_t  wCurMap;
extern uint8_t  wLastMap;
extern uint16_t wYCoord;   /* tile coord; uint16 for large routes (>64 blocks) */
extern uint16_t wXCoord;
extern uint8_t  wYBlockCoord;
extern uint8_t  wXBlockCoord;
extern uint16_t wCurrentTileBlockMapViewPointer;
extern uint16_t wMapViewVRAMPointer;

/* Current map header fields (unpacked from ROM) */
extern uint8_t  wCurMapHeight;
extern uint8_t  wCurMapWidth;
extern uint16_t wCurMapDataPtr;
extern uint16_t wCurMapTextPtr;
extern uint16_t wCurMapScriptPtr;
extern uint8_t  wCurMapConnections;

/* Warp / sign / sprite counts */
extern uint8_t wNumberOfWarps;
extern uint8_t wNumSigns;
extern uint8_t wNumSprites;
extern uint8_t wDestinationWarpID;
extern uint8_t wMapBackgroundTile;

/* ---- WRAM: Player state ---------------------------------- */
extern uint8_t wPlayerMovingDirection;
extern uint8_t wPlayerLastStopDirection;
extern uint8_t wPlayerDirection;
extern uint8_t wWalkBikeSurfState;
extern uint8_t wWalkCounter;
extern uint8_t wStepCounter;

/* ---- WRAM: Party / inventory ----------------------------- */
extern uint8_t    wPartyCount;
extern party_mon_t wPartyMons[PARTY_LENGTH];
extern uint8_t    wPartyMonOT[PARTY_LENGTH][NAME_LENGTH];
extern uint8_t    wPartyMonNicks[PARTY_LENGTH][NAME_LENGTH];

extern uint8_t    wNumBagItems;
extern uint8_t    wBagItems[BAG_ITEM_CAPACITY * 2 + 1]; /* item_id, qty pairs + 0xFF */

extern uint8_t    wPlayerMoney[3];  /* 3-byte BCD */
extern uint8_t    wPlayerName[NAME_LENGTH];
extern uint8_t    wRivalName[NAME_LENGTH];
extern uint16_t   wPlayerID;
extern uint8_t    wObtainedBadges;

/* ---- WRAM: Pokédex --------------------------------------- */
extern uint8_t wPokedexOwned[19];   /* flag_array for 151 Pokémon */
extern uint8_t wPokedexSeen[19];

/* ---- WRAM: Event flags ----------------------------------- */
#define NUM_EVENTS          507
#define EVENT_FLAGS_BYTES   ((NUM_EVENTS + 7) / 8)  /* 64 bytes */
extern uint8_t wEventFlags[EVENT_FLAGS_BYTES];

/* Event flag accessors */
static inline int  CheckEvent(int n)  { return (wEventFlags[n/8] >> (n%8)) & 1; }
static inline void SetEvent(int n)    { wEventFlags[n/8] |=  (1 << (n%8)); }
static inline void ClearEvent(int n)  { wEventFlags[n/8] &= ~(1 << (n%8)); }

/* ---- WRAM: Item pickup flags ----------------------------- */
/* Bit i of wPickedUpItems[map_id] = item ball i on that map is gone. */
extern uint16_t wPickedUpItems[248]; /* NUM_MAPS = 248 */

/* ---- WRAM: Wild encounters ------------------------------- */
extern uint8_t wGrassRate;
extern uint8_t wWaterRate;
extern uint8_t wGrassMons[NUM_WILD_SLOTS * 2];  /* level, species pairs */
extern uint8_t wWaterMons[NUM_WILD_SLOTS * 2];

/* ---- WRAM: Battle state ---------------------------------- */
extern uint8_t  wIsInBattle;            /* 0=none, 1=trainer, 2=wild */
extern uint8_t  wBattleType;
extern uint8_t  wCurEnemyLevel;
extern uint8_t  wCurPartySpecies;
extern uint8_t  wEnemyMonSpecies;
extern uint8_t  wTrainerClass;
extern uint8_t  wPlayerMonStatMods[NUM_STAT_MODS];
extern uint8_t  wEnemyMonStatMods[NUM_STAT_MODS];
extern uint8_t  wCriticalHitOrOHKO;
extern uint16_t wDamage;
extern uint8_t  wDamageMultipliers;
extern uint8_t  wMoveMissed;
extern uint8_t  wPlayerSelectedMove;
extern uint8_t  wEnemySelectedMove;
extern uint8_t  wRepelRemainingSteps;

/* ---- WRAM: Audio ---------------------------------------- */
extern uint8_t  wAudioROMBank;
extern uint16_t wMusicTempo;
extern uint16_t wSfxTempo;
extern uint8_t  wChannelCommandPointers[NUM_CHANNELS * 2];
extern uint8_t  wChannelReturnAddresses[NUM_CHANNELS * 2];
extern uint8_t  wChannelSoundIDs[NUM_CHANNELS];
extern uint8_t  wChannelFlags1[NUM_CHANNELS];
extern uint8_t  wChannelFlags2[NUM_CHANNELS];
extern uint8_t  wChannelDutyCycles[NUM_CHANNELS];
extern uint8_t  wChannelOctaves[NUM_CHANNELS];
extern uint8_t  wChannelNoteDelayCounters[NUM_CHANNELS];
extern uint8_t  wChannelNoteSpeeds[NUM_CHANNELS];
extern uint8_t  wChannelVolumes[NUM_CHANNELS];

/* ---- Utility --------------------------------------------- */
/* Convert 3-byte big-endian GB exp to uint32_t */
static inline uint32_t exp_to_u32(const uint8_t e[3]) {
    return ((uint32_t)e[0] << 16) | ((uint32_t)e[1] << 8) | e[2];
}
static inline void u32_to_exp(uint32_t v, uint8_t e[3]) {
    e[0] = (v >> 16) & 0xFF;
    e[1] = (v >>  8) & 0xFF;
    e[2] =  v        & 0xFF;
}

/* GB stat formula: ((Base + DV) * 2 + sqrt(StatExp)/4) * Level/100 + 5
 * HP adds Level+10 instead of +5 */
uint16_t CalcStat(uint8_t base, uint8_t dv, uint16_t stat_exp, uint8_t level, int is_hp);

/* Battle damage formula: (Level*2/5+2) * BP * Atk / Def / 50, capped at 997 */
uint16_t CalcDamage(uint8_t level, uint8_t bp, uint8_t attack, uint8_t defense);

/* Stat stage modifier: base * num[stage-1] / den[stage-1] */
uint16_t ModifyStat(uint16_t base, uint8_t stage);

/* Save checksum: ~(sum of bytes) */
uint8_t CalcCheckSum(const uint8_t *data, uint16_t len);

/* Random (mirrors GB engine: add/sub from rDIV analogue) */
uint8_t BattleRandom(void);
