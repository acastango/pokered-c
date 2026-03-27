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
extern uint32_t   wAmountMoneyWon;  /* prize money for current trainer battle */
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
extern uint8_t  wIsInBattle;            /* -1=lost, 0=none, 1=wild, 2=trainer */
extern uint8_t  wBattleType;            /* 0=normal, 1=old man, 2=safari */
extern uint8_t  wCurEnemyLevel;
extern uint8_t  wCurPartySpecies;
extern uint8_t  wEnemyMonSpecies;
extern uint8_t  wTrainerClass;
/* wLoneAttackNo — 1-based index into LoneMoves table; written by gym scripts
 * before battle. 0 = no lone move (most trainers). */
extern uint8_t  wLoneAttackNo;
/* wRivalStarter — SPECIES_* of the player's chosen starter (set at game start).
 * Used by RIVAL3 to decide which signature move his 6th mon gets. */
extern uint8_t  wRivalStarter;

/* hWhoseTurn: 0 = player's turn, 1 = enemy's turn (HRAM in original, global here) */
extern uint8_t  hWhoseTurn;

/* Battle copies of the active mons (wBattleMon / wEnemyMon in pokered wram.asm) */
extern battle_mon_t wBattleMon;
extern battle_mon_t wEnemyMon;

/* Current-move vars — filled by GetCurrentMove before damage calculation.
 * Layout mirrors pokered wram.asm:
 *   wPlayerMoveNum, wPlayerMoveEffect, wPlayerMovePower,
 *   wPlayerMoveType, wPlayerMoveAccuracy (scaled by CalcHitChance),
 *   wPlayerMoveMaxPP  (and identical set for enemy) */
extern uint8_t  wPlayerMoveNum;
extern uint8_t  wPlayerMoveEffect;
extern uint8_t  wPlayerMovePower;
extern uint8_t  wPlayerMoveType;
extern uint8_t  wPlayerMoveAccuracy;    /* scaled hit chance after CalcHitChance */
extern uint8_t  wPlayerMoveMaxPP;

extern uint8_t  wEnemyMoveNum;
extern uint8_t  wEnemyMoveEffect;
extern uint8_t  wEnemyMovePower;
extern uint8_t  wEnemyMoveType;
extern uint8_t  wEnemyMoveAccuracy;
extern uint8_t  wEnemyMoveMaxPP;
/* wMoveType — temporary copy of the active move's type used during
 * AdjustDamageForMoveType (core.asm:5086/5100).  Set from wPlayerMoveType
 * or wEnemyMoveType depending on hWhoseTurn. */
extern uint8_t  wMoveType;

/* Battle status flag bytes (see BSTAT1_x / BSTAT2_x / BSTAT3_x bit positions) */
extern uint8_t  wPlayerBattleStatus1;
extern uint8_t  wPlayerBattleStatus2;
extern uint8_t  wPlayerBattleStatus3;
extern uint8_t  wEnemyBattleStatus1;
extern uint8_t  wEnemyBattleStatus2;
extern uint8_t  wEnemyBattleStatus3;

/* Per-turn counters */
extern uint8_t  wPlayerConfusedCounter;
extern uint8_t  wPlayerToxicCounter;
extern uint8_t  wPlayerDisabledMove;    /* high nibble=which move (1-4), low=turns left */
extern uint8_t  wEnemyConfusedCounter;
extern uint8_t  wEnemyToxicCounter;
extern uint8_t  wEnemyDisabledMove;

/* Stat modifier arrays: wPlayerMonStatMods[MOD_*] and wEnemyMonStatMods[MOD_*]
 * Range 1-13 (7 = normal).  Indices: 0=Atk, 1=Def, 2=Spd, 3=Spc, 4=Acc, 5=Eva */
extern uint8_t  wPlayerMonStatMods[NUM_STAT_MODS];
extern uint8_t  wEnemyMonStatMods[NUM_STAT_MODS];

/* wPlayerMonNumber — which party slot (0-5) the active player mon is in.
 * Set when a player mon enters battle (wPlayerMonNumber in pokered wram.asm:246). */
extern uint8_t  wPlayerMonNumber;

/* wCalculateWhoseStats — controls CalculateModifiedStats target:
 * 0 = recalculate player wBattleMon stats;  non-zero = enemy wEnemyMon stats.
 * (wCalculateWhoseStats in pokered wram.asm:1597) */
extern uint8_t  wCalculateWhoseStats;

/* Unmodified (pre-stage) battle stats — stored from party mon at switch-in.
 * CalculateModifiedStats reads these, applies the stage ratios, and writes back
 * to wBattleMon/wEnemyMon.  Order: ATK, DEF, SPD, SPC (matches battle_struct). */
extern uint16_t wPlayerMonUnmodifiedAttack;
extern uint16_t wPlayerMonUnmodifiedDefense;
extern uint16_t wPlayerMonUnmodifiedSpeed;
extern uint16_t wPlayerMonUnmodifiedSpecial;
extern uint16_t wEnemyMonUnmodifiedAttack;
extern uint16_t wEnemyMonUnmodifiedDefense;
extern uint16_t wEnemyMonUnmodifiedSpeed;
extern uint16_t wEnemyMonUnmodifiedSpecial;

/* CalcStat — defined in wram.c; used by GetDamageVarsFor*Attack crit bypass */
extern uint16_t CalcStat(uint8_t base, uint8_t dv, uint16_t stat_exp,
                          uint8_t level, int is_hp);

extern uint8_t  wCriticalHitOrOHKO;    /* 0=normal, 1=crit, 2=OHKO hit, 0xFF=OHKO miss */
extern uint16_t wDamage;
extern uint8_t  wDamageMultipliers;     /* bits 0-6=effectiveness (×10), bit7=STAB */
extern uint8_t  wMoveMissed;
extern uint8_t  wPlayerSelectedMove;
extern uint8_t  wEnemySelectedMove;
extern uint8_t  wRepelRemainingSteps;

/* ---- WRAM: Extended battle state (Phase 5 turn execution) ---------- */
/* wActionResultOrTookBattleTurn — non-zero if the player's turn was already
 * consumed (used an item, tried to run, switched) so ExecutePlayerMove skips. */
extern uint8_t  wActionResultOrTookBattleTurn;

/* wMonIsDisobedient — set to 1 by CheckForDisobedience when a traded mon
 * disobeys and uses a random move instead of the selected one. */
extern uint8_t  wMonIsDisobedient;

/* wInHandlePlayerMonFainted — prevents RemoveFaintedPlayerMon from playing
 * the cry and faint text when called from HandleEnemyMonFainted (simultaneous
 * faint: enemy mon detected first, player mon detected second). */
extern uint8_t  wInHandlePlayerMonFainted;

/* wPlayerUsedMove / wEnemyUsedMove — last move number actually executed.
 * Cleared when asleep or frozen so Mirror Move doesn't copy those turns. */
extern uint8_t  wPlayerUsedMove;
extern uint8_t  wEnemyUsedMove;

/* ---- WRAM: Extended battle state (Phase 4 effects engine) ---------- */
/* wLinkState — 0=not linked, LINK_STATE_BATTLING=0x04 in a link battle.
 * StatModifierDownEffect checks this to skip the 25% accuracy roll. */
extern uint8_t  wLinkState;

/* wEscapedFromBattle — set to 1 when a mon successfully uses Teleport/Roar/Whirlwind
 * in a wild battle, signaling the main battle loop to end. */
extern uint8_t  wEscapedFromBattle;

/* wMoveDidntMiss — set to 1 by the main execute loop after a successful hit.
 * ConditionalPrintButItFailed returns without printing if this is set. */
extern uint8_t  wMoveDidntMiss;

/* wChargeMoveNum — move ID stored at start of charge turn (ChargeEffect).
 * Read by the text handler to print the correct charge-turn message. */
extern uint8_t  wChargeMoveNum;

/* Multi-hit counters (TwoToFiveAttacksEffect, TrappingEffect) */
extern uint8_t  wPlayerNumAttacksLeft;  /* remaining hits in current multi-hit sequence */
extern uint8_t  wEnemyNumAttacksLeft;
extern uint8_t  wPlayerNumHits;         /* total hits landed (displayed at end) */
extern uint8_t  wEnemyNumHits;

/* Bide accumulated damage — 16-bit total damage absorbed during Bide turns.
 * Released as wDamage when Bide fires (handled in core.asm, not effects.c). */
extern uint16_t wPlayerBideAccumulatedDamage;
extern uint16_t wEnemyBideAccumulatedDamage;

/* Substitute HP — low byte of max_hp/4, as stored by SubstituteEffect.
 * 0 = no substitute.  Assumes max_hp ≤ 1023 (fits in uint8_t). */
extern uint8_t  wPlayerSubstituteHP;
extern uint8_t  wEnemySubstituteHP;

/* Minimized flag — set to 1 by StatModifierUpEffect when the move is Minimize.
 * Used by evasion check in MoveHitTest. */
extern uint8_t  wPlayerMonMinimized;
extern uint8_t  wEnemyMonMinimized;

/* Disabled move number — move ID of the currently disabled move (for display).
 * Separate from wPlayerDisabledMove (which packs move slot + turn counter). */
extern uint8_t  wPlayerDisabledMoveNumber;
extern uint8_t  wEnemyDisabledMoveNumber;

/* Move list indices — which slot (0-3) the currently selected move occupies.
 * Used by MimicEffect to know which slot to overwrite. */
extern uint8_t  wPlayerMoveListIndex;
extern uint8_t  wEnemyMoveListIndex;

/* wTotalPayDayMoney — 3-byte packed BCD accumulator for Pay Day coins.
 * Layout: [0]=MSB (hundred-thousands/ten-thousands), [1]=middle,
 * [2]=LSB (ones/tens).  Displayed at battle end. */
extern uint8_t  wTotalPayDayMoney[3];

/* wTransformedEnemyMonOriginalDVs — saved DVs of the enemy mon before Transform.
 * Restored when the transformed mon switches out. */
extern uint16_t wTransformedEnemyMonOriginalDVs;

/* ---- WRAM: Battle loop state (Phase 6) ------------------ */
/* wFirstMonsNotOutYet — 1 during battle setup; cleared at the start of the
 * first turn (core.asm:138, MainInBattleLoop:292). */
extern uint8_t  wFirstMonsNotOutYet;

/* wBattleResult — outcome byte written by faint/victory handlers.
 * See BATTLE_OUTCOME_* constants in battle_loop.h. */
extern uint8_t  wBattleResult;

/* ---- WRAM: Enemy party (trainer battles) ---------------- */
/* wEnemyPartyCount — number of trainer's Pokémon (0 = wild battle). */
extern uint8_t     wEnemyPartyCount;

/* wEnemyMons — enemy trainer's party (same layout as wPartyMons).
 * For wild battles this is unused; wEnemyMon is set directly from wild data. */
extern party_mon_t wEnemyMons[PARTY_LENGTH];

/* wEnemyMonPartyPos — which slot in wEnemyMons is currently active.
 * Updated by Battle_EnemySendOut_State when a new enemy mon enters. */
extern uint8_t     wEnemyMonPartyPos;

/* wNumRunAttempts — incremented each time the player tries to run.
 * Reset to 0 at battle start.  Used by TryRunningFromBattle flee formula. */
extern uint8_t     wNumRunAttempts;

/* wForcePlayerToChooseMon — set to 1 when the active player mon fainted
 * and a trainer enemy was also replaced (simultaneous faint path in
 * HandleEnemyMonFainted).  Driver/UI must prompt for a new mon. */
extern uint8_t     wForcePlayerToChooseMon;

/* wAICount — AI state counter (wMiscBattleData in pokered wram.asm).
 * Used by the trainer AI move-selection layers. */
extern uint8_t     wAICount;

/* wAILayer2Encouragement — AI layer 2 encouragement flag.
 * Cleared by ReplaceFaintedEnemyMon after each switch-in. */
extern uint8_t     wAILayer2Encouragement;

/* wLastSwitchInEnemyMonHP — HP of the enemy mon at the time it switched in.
 * Saved by Battle_EnemySendOut_State; used by the trainer AI. */
extern uint16_t    wLastSwitchInEnemyMonHP;

/* ---- WRAM: Experience / level-up system (experience.asm) ---------- */
/* wPartySpecies — species ID per party slot (PARTY_LENGTH+1 bytes, 0xFF terminator).
 * Mirrors wPartySpecies in wram.asm (used by GainExperience to look up growth rate). */
extern uint8_t  wPartySpecies[PARTY_LENGTH + 1];

/* wPartyGainExpFlags — 6-bit flag (bit n = party slot n gained exp this turn).
 * Set when a mon is sent out; read by GainExperience; cleared+reset at end. */
extern uint8_t  wPartyGainExpFlags;

/* wPartyFoughtCurrentEnemyFlags — tracks which mons attacked this enemy. */
extern uint8_t  wPartyFoughtCurrentEnemyFlags;

/* wWhichPokemon — party slot index loop variable in GainExperience. */
extern uint8_t  wWhichPokemon;

/* wExpAmountGained — exp gained this turn (for display, 2 bytes). */
extern uint16_t wExpAmountGained;

/* wGainBoostedExp — 1 if exp was boosted (traded mon or trainer battle). */
extern uint8_t  wGainBoostedExp;

/* wBoostExpByExpAll — 1 if Exp. All is active (not yet implemented). */
extern uint8_t  wBoostExpByExpAll;

/* wCurSpecies — temp species ID used during level-up/exp calculations. */
extern uint8_t  wCurSpecies;

/* ---- WRAM: Evolution system (engine/pokemon/evos_moves.asm) -------------- */
/* wCanEvolveFlags — 6-bit flag (bit n = party slot n can evolve after battle).
 * Set in GainExperience after a level-up; read by EvolutionAfterBattle. */
extern uint8_t  wCanEvolveFlags;

/* wEvolutionOccurred — set to 1 if at least one mon evolved this battle. */
extern uint8_t  wEvolutionOccurred;

/* wEvoOldSpecies / wEvoNewSpecies — species before/after evolution. */
extern uint8_t  wEvoOldSpecies;
extern uint8_t  wEvoNewSpecies;

/* wForceEvolution — nonzero suppresses level-based evo check (used by stones). */
extern uint8_t  wForceEvolution;

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
