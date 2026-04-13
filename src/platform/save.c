/* save.c — File-based save replacing GB SRAM */
#include "save.h"
#include "compiler.h"
#include "hardware.h"
#include "../game/constants.h"
#include "../game/types.h"
#include "../game/player.h"        /* gPlayerFacing */
#include "../game/trainer_sight.h" /* gEngagedTrainerClass */
#include <stdio.h>
#include <string.h>

extern int  Game_GetScene(void);
extern void Game_SetScene(int);

/* The save block mirrors what the GB writes to SRAM bank 1.
 * We serialize it as a flat struct matching the original layout. */
#if defined(_MSC_VER)
#pragma pack(push, 1)
#endif

typedef struct PACKED {
    uint8_t     player_name[NAME_LENGTH];
    /* --- sMainData region (checksum covers this) --- */
    uint8_t     pokedex_owned[19];
    uint8_t     pokedex_seen[19];
    uint8_t     num_bag_items;
    uint8_t     bag_items[BAG_ITEM_CAPACITY * 2 + 1];
    uint8_t     player_money[3];
    uint8_t     rival_name[NAME_LENGTH];
    uint8_t     options;
    uint8_t     badges;
    uint8_t     _pad1;
    uint8_t     letter_delay_flags;
    uint16_t    player_id;
    uint8_t     cur_map;
    uint8_t     last_map;
    uint8_t     y_coord;
    uint8_t     x_coord;
    uint8_t     event_flags[EVENT_FLAGS_BYTES];
    uint8_t     game_progress_flags[256]; /* wGameProgressFlags region */
    uint16_t    picked_up_items[248];     /* wPickedUpItems — item ball pickup flags */
    /* --- sPartyData region --- */
    uint8_t     party_count;
    party_mon_t party_mons[PARTY_LENGTH];
    uint8_t     party_ot[PARTY_LENGTH][NAME_LENGTH];
    uint8_t     party_nicks[PARTY_LENGTH][NAME_LENGTH];
    /* --- end of core save data --- */
    uint8_t     checksum;
} save_block_v1_t;

typedef struct PACKED {
    uint8_t     player_name[NAME_LENGTH];
    /* --- sMainData region (checksum covers this) --- */
    uint8_t     pokedex_owned[19];
    uint8_t     pokedex_seen[19];
    uint8_t     num_bag_items;
    uint8_t     bag_items[BAG_ITEM_CAPACITY * 2 + 1];
    uint8_t     player_money[3];
    uint8_t     rival_name[NAME_LENGTH];
    uint8_t     options;
    uint8_t     badges;
    uint8_t     _pad1;
    uint8_t     letter_delay_flags;
    uint16_t    player_id;
    uint8_t     cur_map;
    uint8_t     last_map;
    uint8_t     y_coord;
    uint8_t     x_coord;
    uint8_t     event_flags[EVENT_FLAGS_BYTES];
    uint8_t     game_progress_flags[256];
    uint16_t    picked_up_items[248];
    /* --- sPartyData region --- */
    uint8_t     party_count;
    party_mon_t party_mons[PARTY_LENGTH];
    uint8_t     party_ot[PARTY_LENGTH][NAME_LENGTH];
    uint8_t     party_nicks[PARTY_LENGTH][NAME_LENGTH];
    /* --- box data extension for the port --- */
    uint8_t     current_box_num;
    uint8_t     box_count[NUM_BOXES];
    uint8_t     box_species[NUM_BOXES][BOX_CAPACITY + 1];
    box_mon_t   box_mons[NUM_BOXES][BOX_CAPACITY];
    uint8_t     box_ot[NUM_BOXES][BOX_CAPACITY][NAME_LENGTH];
    uint8_t     box_nicks[NUM_BOXES][BOX_CAPACITY][NAME_LENGTH];
    /* --- end of core save data --- */
    uint8_t     checksum;
} save_block_t;

static save_block_t save;

static void pack_save(void) {
    memcpy(save.player_name,    wPlayerName,     NAME_LENGTH);
    memcpy(save.pokedex_owned,  wPokedexOwned,   sizeof(wPokedexOwned));
    memcpy(save.pokedex_seen,   wPokedexSeen,    sizeof(wPokedexSeen));
    save.num_bag_items = wNumBagItems;
    memcpy(save.bag_items,      wBagItems,       sizeof(wBagItems));
    memcpy(save.player_money,   wPlayerMoney,    3);
    memcpy(save.rival_name,     wRivalName,      NAME_LENGTH);
    save.badges     = wObtainedBadges;
    save.player_id  = wPlayerID;
    save.cur_map    = wCurMap;
    save.last_map   = wLastMap;
    save.y_coord    = wYCoord;
    save.x_coord    = wXCoord;
    memcpy(save.event_flags,    wEventFlags,     EVENT_FLAGS_BYTES);
    memcpy(save.picked_up_items, wPickedUpItems, sizeof(wPickedUpItems));
    save.party_count = wPartyCount;
    memcpy(save.party_mons,     wPartyMons,      sizeof(wPartyMons));
    memcpy(save.party_ot,       wPartyMonOT,     sizeof(wPartyMonOT));
    memcpy(save.party_nicks,    wPartyMonNicks,  sizeof(wPartyMonNicks));
    save.current_box_num = wCurrentBoxNum;
    memcpy(save.box_count,      wBoxCount,       sizeof(wBoxCount));
    memcpy(save.box_species,    wBoxSpecies,     sizeof(wBoxSpecies));
    memcpy(save.box_mons,       wBoxMons,        sizeof(wBoxMons));
    memcpy(save.box_ot,         wBoxMonOT,       sizeof(wBoxMonOT));
    memcpy(save.box_nicks,      wBoxMonNicks,    sizeof(wBoxMonNicks));

    /* Checksum covers everything except the checksum byte itself */
    save.checksum = CalcCheckSum((uint8_t *)&save,
                                  (uint16_t)(sizeof(save) - 1));
}

static void unpack_save(void) {
    memcpy(wPlayerName,    save.player_name,   NAME_LENGTH);
    memcpy(wPokedexOwned,  save.pokedex_owned, sizeof(wPokedexOwned));
    memcpy(wPokedexSeen,   save.pokedex_seen,  sizeof(wPokedexSeen));
    wNumBagItems = save.num_bag_items;
    memcpy(wBagItems,      save.bag_items,     sizeof(wBagItems));
    memcpy(wPlayerMoney,   save.player_money,  3);
    memcpy(wRivalName,     save.rival_name,    NAME_LENGTH);
    wObtainedBadges = save.badges;
    wPlayerID       = save.player_id;
    wCurMap         = save.cur_map;
    wLastMap        = save.last_map;
    wYCoord         = save.y_coord;
    wXCoord         = save.x_coord;
    memcpy(wEventFlags,    save.event_flags,   EVENT_FLAGS_BYTES);
    memcpy(wPickedUpItems, save.picked_up_items, sizeof(wPickedUpItems));
    wPartyCount = save.party_count;
    memcpy(wPartyMons,    save.party_mons,     sizeof(wPartyMons));
    memcpy(wPartyMonOT,   save.party_ot,       sizeof(wPartyMonOT));
    memcpy(wPartyMonNicks,save.party_nicks,    sizeof(wPartyMonNicks));
    wCurrentBoxNum = save.current_box_num;
    memcpy(wBoxCount,     save.box_count,      sizeof(wBoxCount));
    memcpy(wBoxSpecies,   save.box_species,    sizeof(wBoxSpecies));
    memcpy(wBoxMons,      save.box_mons,       sizeof(wBoxMons));
    memcpy(wBoxMonOT,     save.box_ot,         sizeof(wBoxMonOT));
    memcpy(wBoxMonNicks,  save.box_nicks,      sizeof(wBoxMonNicks));
}

int Save_ValidateChecksum(void) {
    uint8_t calc = CalcCheckSum((uint8_t *)&save,
                                 (uint16_t)(sizeof(save) - 1));
    return calc == save.checksum ? 0 : -1;
}

int Save_Load(void) {
    FILE *f = fopen(SAVE_FILE, "rb");
    long sz;
    size_t n;
    if (!f) return -1;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return -1; }
    sz = ftell(f);
    if (sz < 0) { fclose(f); return -1; }
    rewind(f);

    if ((size_t)sz == sizeof(save_block_t)) {
        n = fread(&save, 1, sizeof(save), f);
        fclose(f);
        if (n != sizeof(save)) return -1;
        if (Save_ValidateChecksum() != 0) return -1;
        unpack_save();
        return 0;
    }

    if ((size_t)sz == sizeof(save_block_v1_t)) {
        save_block_v1_t old_save;
        uint8_t calc;
        memset(&old_save, 0, sizeof(old_save));
        n = fread(&old_save, 1, sizeof(old_save), f);
        fclose(f);
        if (n != sizeof(old_save)) return -1;
        calc = CalcCheckSum((uint8_t *)&old_save, (uint16_t)(sizeof(old_save) - 1));
        if (calc != old_save.checksum) return -1;

        memset(&save, 0, sizeof(save));
        memcpy(save.player_name,   old_save.player_name,   sizeof(old_save.player_name));
        memcpy(save.pokedex_owned, old_save.pokedex_owned, sizeof(old_save.pokedex_owned));
        memcpy(save.pokedex_seen,  old_save.pokedex_seen,  sizeof(old_save.pokedex_seen));
        save.num_bag_items = old_save.num_bag_items;
        memcpy(save.bag_items,     old_save.bag_items,     sizeof(old_save.bag_items));
        memcpy(save.player_money,  old_save.player_money,  sizeof(old_save.player_money));
        memcpy(save.rival_name,    old_save.rival_name,    sizeof(old_save.rival_name));
        save.options = old_save.options;
        save.badges  = old_save.badges;
        save._pad1   = old_save._pad1;
        save.letter_delay_flags = old_save.letter_delay_flags;
        save.player_id = old_save.player_id;
        save.cur_map   = old_save.cur_map;
        save.last_map  = old_save.last_map;
        save.y_coord   = old_save.y_coord;
        save.x_coord   = old_save.x_coord;
        memcpy(save.event_flags,   old_save.event_flags,   sizeof(old_save.event_flags));
        memcpy(save.game_progress_flags, old_save.game_progress_flags, sizeof(old_save.game_progress_flags));
        memcpy(save.picked_up_items, old_save.picked_up_items, sizeof(old_save.picked_up_items));
        save.party_count = old_save.party_count;
        memcpy(save.party_mons,    old_save.party_mons,    sizeof(old_save.party_mons));
        memcpy(save.party_ot,      old_save.party_ot,      sizeof(old_save.party_ot));
        memcpy(save.party_nicks,   old_save.party_nicks,   sizeof(old_save.party_nicks));
        save.current_box_num = 0;
        memset(save.box_count, 0, sizeof(save.box_count));
        memset(save.box_species, 0xFF, sizeof(save.box_species));
        memset(save.box_mons, 0, sizeof(save.box_mons));
        memset(save.box_ot, 0, sizeof(save.box_ot));
        memset(save.box_nicks, 0, sizeof(save.box_nicks));
        unpack_save();
        return 0;
    }

    fclose(f);
    return -1;
}

int Save_Write(void) {
    pack_save();
    FILE *f = fopen(SAVE_FILE, "wb");
    if (!f) return -1;
    fwrite(&save, 1, sizeof(save), f);
    fclose(f);
    return 0;
}

/* ================================================================
 * Save states — full runtime snapshot (wram globals + scene + RNG)
 * ================================================================ */

#define STATE_MAGIC   0x504B5354u   /* "PKST" */
#define STATE_VERSION 1u

typedef struct PACKED {
    uint32_t magic;
    uint32_t version;

    /* Map & player */
    uint8_t  wCurMap, wLastMap;
    uint16_t wYCoord, wXCoord;
    uint8_t  wYBlockCoord, wXBlockCoord;
    uint8_t  wDestinationWarpID, wMapBackgroundTile;
    uint8_t  wPlayerMovingDirection, wPlayerLastStopDirection, wPlayerDirection;
    uint8_t  wWalkBikeSurfState, wWalkCounter, wStepCounter;
    uint8_t  wRepelRemainingSteps;
    int8_t   gPlayerFacing;
    int8_t   gScene;

    /* RNG */
    uint8_t  hRandomAdd, hRandomSub, hFrameCounter;

    /* Player identity */
    uint8_t  wPlayerName[NAME_LENGTH];
    uint8_t  wRivalName[NAME_LENGTH];
    uint16_t wPlayerID;
    uint8_t  wObtainedBadges;
    uint8_t  wRivalStarter;

    /* Inventory */
    uint8_t  wPlayerMoney[3];
    uint32_t wAmountMoneyWon;
    uint8_t  wNumBagItems;
    uint8_t  wBagItems[BAG_ITEM_CAPACITY * 2 + 1];

    /* Party */
    uint8_t     wPartyCount;
    party_mon_t wPartyMons[PARTY_LENGTH];
    uint8_t     wPartyMonOT[PARTY_LENGTH][NAME_LENGTH];
    uint8_t     wPartyMonNicks[PARTY_LENGTH][NAME_LENGTH];
    uint8_t     wPartySpecies[PARTY_LENGTH + 1];

    /* Pokédex & flags */
    uint8_t  wPokedexOwned[19];
    uint8_t  wPokedexSeen[19];
    uint8_t  wEventFlags[EVENT_FLAGS_BYTES];
    uint16_t wPickedUpItems[248];

    /* Battle core */
    uint8_t  wIsInBattle, wBattleType;
    uint8_t  wCurEnemyLevel, wTrainerClass, wLoneAttackNo;
    uint8_t  hWhoseTurn;
    uint8_t  wPlayerMonNumber, wCalculateWhoseStats;
    battle_mon_t wBattleMon, wEnemyMon;
    uint8_t  wPlayerMonStatMods[NUM_STAT_MODS];
    uint8_t  wEnemyMonStatMods[NUM_STAT_MODS];
    uint8_t  wPlayerBattleStatus1, wPlayerBattleStatus2, wPlayerBattleStatus3;
    uint8_t  wEnemyBattleStatus1,  wEnemyBattleStatus2,  wEnemyBattleStatus3;
    uint8_t  wPlayerConfusedCounter, wPlayerToxicCounter, wPlayerDisabledMove;
    uint8_t  wEnemyConfusedCounter,  wEnemyToxicCounter,  wEnemyDisabledMove;
    uint8_t  wPlayerSelectedMove,  wEnemySelectedMove;
    uint8_t  wPlayerMoveNum,       wEnemyMoveNum;
    uint8_t  wCriticalHitOrOHKO;
    uint16_t wDamage;
    uint8_t  wMoveMissed;
    uint8_t  wActionResultOrTookBattleTurn;
    uint8_t  wPlayerUsedMove,      wEnemyUsedMove;
    uint8_t  wPlayerNumAttacksLeft, wEnemyNumAttacksLeft;
    uint8_t  wPlayerNumHits,       wEnemyNumHits;
    uint16_t wPlayerBideAccumulatedDamage, wEnemyBideAccumulatedDamage;
    uint8_t  wPlayerSubstituteHP,  wEnemySubstituteHP;
    uint8_t  wPlayerMonMinimized,  wEnemyMonMinimized;
    uint8_t  wPlayerDisabledMoveNumber, wEnemyDisabledMoveNumber;
    uint16_t wPlayerMonUnmodifiedAttack,  wPlayerMonUnmodifiedDefense;
    uint16_t wPlayerMonUnmodifiedSpeed,   wPlayerMonUnmodifiedSpecial;
    uint16_t wEnemyMonUnmodifiedAttack,   wEnemyMonUnmodifiedDefense;
    uint16_t wEnemyMonUnmodifiedSpeed,    wEnemyMonUnmodifiedSpecial;
    uint16_t wTransformedEnemyMonOriginalDVs;
    uint8_t  wFirstMonsNotOutYet, wBattleResult;

    /* Enemy party (trainer battles) */
    uint8_t     wEnemyPartyCount;
    party_mon_t wEnemyMons[PARTY_LENGTH];
    uint8_t     wEnemyMonPartyPos;
    uint8_t     wNumRunAttempts, wForcePlayerToChooseMon;
    uint8_t     wAICount, wAILayer2Encouragement;
    uint16_t    wLastSwitchInEnemyMonHP;
    uint8_t     gEngagedTrainerClass;

    /* Exp flags */
    uint8_t  wPartyGainExpFlags, wPartyFoughtCurrentEnemyFlags;
    uint8_t  wGainBoostedExp, wCanEvolveFlags, wEvolutionOccurred;
} state_block_t;

static state_block_t st;
static int s_last_load_in_battle = 0;

#if defined(_MSC_VER)
#pragma pack(pop)
#endif

static void pack_state(void) {
    st.magic   = STATE_MAGIC;
    st.version = STATE_VERSION;

    st.wCurMap = wCurMap;   st.wLastMap = wLastMap;
    st.wYCoord = wYCoord;   st.wXCoord  = wXCoord;
    st.wYBlockCoord = wYBlockCoord; st.wXBlockCoord = wXBlockCoord;
    st.wDestinationWarpID = wDestinationWarpID;
    st.wMapBackgroundTile = wMapBackgroundTile;
    st.wPlayerMovingDirection    = wPlayerMovingDirection;
    st.wPlayerLastStopDirection  = wPlayerLastStopDirection;
    st.wPlayerDirection          = wPlayerDirection;
    st.wWalkBikeSurfState = wWalkBikeSurfState;
    st.wWalkCounter       = wWalkCounter;
    st.wStepCounter       = wStepCounter;
    st.wRepelRemainingSteps = wRepelRemainingSteps;
    st.gPlayerFacing = (int8_t)gPlayerFacing;
    st.gScene        = (int8_t)Game_GetScene();

    st.hRandomAdd     = hRandomAdd;
    st.hRandomSub     = hRandomSub;
    st.hFrameCounter  = hFrameCounter;

    memcpy(st.wPlayerName, wPlayerName, NAME_LENGTH);
    memcpy(st.wRivalName,  wRivalName,  NAME_LENGTH);
    st.wPlayerID      = wPlayerID;
    st.wObtainedBadges = wObtainedBadges;
    st.wRivalStarter   = wRivalStarter;

    memcpy(st.wPlayerMoney, wPlayerMoney, 3);
    st.wAmountMoneyWon = wAmountMoneyWon;
    st.wNumBagItems    = wNumBagItems;
    memcpy(st.wBagItems, wBagItems, sizeof(wBagItems));

    st.wPartyCount = wPartyCount;
    memcpy(st.wPartyMons,    wPartyMons,    sizeof(wPartyMons));
    memcpy(st.wPartyMonOT,   wPartyMonOT,   sizeof(wPartyMonOT));
    memcpy(st.wPartyMonNicks,wPartyMonNicks,sizeof(wPartyMonNicks));
    memcpy(st.wPartySpecies, wPartySpecies, sizeof(wPartySpecies));

    memcpy(st.wPokedexOwned, wPokedexOwned, sizeof(wPokedexOwned));
    memcpy(st.wPokedexSeen,  wPokedexSeen,  sizeof(wPokedexSeen));
    memcpy(st.wEventFlags,   wEventFlags,   EVENT_FLAGS_BYTES);
    memcpy(st.wPickedUpItems,wPickedUpItems,sizeof(wPickedUpItems));

    st.wIsInBattle = wIsInBattle; st.wBattleType = wBattleType;
    st.wCurEnemyLevel = wCurEnemyLevel; st.wTrainerClass = wTrainerClass;
    st.wLoneAttackNo  = wLoneAttackNo;
    st.hWhoseTurn = hWhoseTurn;
    st.wPlayerMonNumber = wPlayerMonNumber;
    st.wCalculateWhoseStats = wCalculateWhoseStats;
    st.wBattleMon = wBattleMon; st.wEnemyMon = wEnemyMon;
    memcpy(st.wPlayerMonStatMods, wPlayerMonStatMods, NUM_STAT_MODS);
    memcpy(st.wEnemyMonStatMods,  wEnemyMonStatMods,  NUM_STAT_MODS);
    st.wPlayerBattleStatus1 = wPlayerBattleStatus1;
    st.wPlayerBattleStatus2 = wPlayerBattleStatus2;
    st.wPlayerBattleStatus3 = wPlayerBattleStatus3;
    st.wEnemyBattleStatus1  = wEnemyBattleStatus1;
    st.wEnemyBattleStatus2  = wEnemyBattleStatus2;
    st.wEnemyBattleStatus3  = wEnemyBattleStatus3;
    st.wPlayerConfusedCounter = wPlayerConfusedCounter;
    st.wPlayerToxicCounter    = wPlayerToxicCounter;
    st.wPlayerDisabledMove    = wPlayerDisabledMove;
    st.wEnemyConfusedCounter  = wEnemyConfusedCounter;
    st.wEnemyToxicCounter     = wEnemyToxicCounter;
    st.wEnemyDisabledMove     = wEnemyDisabledMove;
    st.wPlayerSelectedMove = wPlayerSelectedMove;
    st.wEnemySelectedMove  = wEnemySelectedMove;
    st.wPlayerMoveNum = wPlayerMoveNum; st.wEnemyMoveNum = wEnemyMoveNum;
    st.wCriticalHitOrOHKO = wCriticalHitOrOHKO;
    st.wDamage = wDamage; st.wMoveMissed = wMoveMissed;
    st.wActionResultOrTookBattleTurn = wActionResultOrTookBattleTurn;
    st.wPlayerUsedMove = wPlayerUsedMove; st.wEnemyUsedMove = wEnemyUsedMove;
    st.wPlayerNumAttacksLeft = wPlayerNumAttacksLeft;
    st.wEnemyNumAttacksLeft  = wEnemyNumAttacksLeft;
    st.wPlayerNumHits = wPlayerNumHits; st.wEnemyNumHits = wEnemyNumHits;
    st.wPlayerBideAccumulatedDamage = wPlayerBideAccumulatedDamage;
    st.wEnemyBideAccumulatedDamage  = wEnemyBideAccumulatedDamage;
    st.wPlayerSubstituteHP = wPlayerSubstituteHP;
    st.wEnemySubstituteHP  = wEnemySubstituteHP;
    st.wPlayerMonMinimized = wPlayerMonMinimized;
    st.wEnemyMonMinimized  = wEnemyMonMinimized;
    st.wPlayerDisabledMoveNumber = wPlayerDisabledMoveNumber;
    st.wEnemyDisabledMoveNumber  = wEnemyDisabledMoveNumber;
    st.wPlayerMonUnmodifiedAttack  = wPlayerMonUnmodifiedAttack;
    st.wPlayerMonUnmodifiedDefense = wPlayerMonUnmodifiedDefense;
    st.wPlayerMonUnmodifiedSpeed   = wPlayerMonUnmodifiedSpeed;
    st.wPlayerMonUnmodifiedSpecial = wPlayerMonUnmodifiedSpecial;
    st.wEnemyMonUnmodifiedAttack   = wEnemyMonUnmodifiedAttack;
    st.wEnemyMonUnmodifiedDefense  = wEnemyMonUnmodifiedDefense;
    st.wEnemyMonUnmodifiedSpeed    = wEnemyMonUnmodifiedSpeed;
    st.wEnemyMonUnmodifiedSpecial  = wEnemyMonUnmodifiedSpecial;
    st.wTransformedEnemyMonOriginalDVs = wTransformedEnemyMonOriginalDVs;
    st.wFirstMonsNotOutYet = wFirstMonsNotOutYet;
    st.wBattleResult = wBattleResult;

    st.wEnemyPartyCount = wEnemyPartyCount;
    memcpy(st.wEnemyMons, wEnemyMons, sizeof(wEnemyMons));
    st.wEnemyMonPartyPos       = wEnemyMonPartyPos;
    st.wNumRunAttempts         = wNumRunAttempts;
    st.wForcePlayerToChooseMon = wForcePlayerToChooseMon;
    st.wAICount                = wAICount;
    st.wAILayer2Encouragement  = wAILayer2Encouragement;
    st.wLastSwitchInEnemyMonHP = wLastSwitchInEnemyMonHP;
    st.gEngagedTrainerClass    = gEngagedTrainerClass;

    st.wPartyGainExpFlags             = wPartyGainExpFlags;
    st.wPartyFoughtCurrentEnemyFlags  = wPartyFoughtCurrentEnemyFlags;
    st.wGainBoostedExp   = wGainBoostedExp;
    st.wCanEvolveFlags   = wCanEvolveFlags;
    st.wEvolutionOccurred = wEvolutionOccurred;
}

static void unpack_state(void) {
    wCurMap = st.wCurMap; wLastMap = st.wLastMap;
    wYCoord = st.wYCoord; wXCoord  = st.wXCoord;
    wYBlockCoord = st.wYBlockCoord; wXBlockCoord = st.wXBlockCoord;
    wDestinationWarpID = st.wDestinationWarpID;
    wMapBackgroundTile = st.wMapBackgroundTile;
    wPlayerMovingDirection   = st.wPlayerMovingDirection;
    wPlayerLastStopDirection = st.wPlayerLastStopDirection;
    wPlayerDirection         = st.wPlayerDirection;
    wWalkBikeSurfState = st.wWalkBikeSurfState;
    wWalkCounter       = st.wWalkCounter;
    wStepCounter       = st.wStepCounter;
    wRepelRemainingSteps = st.wRepelRemainingSteps;
    gPlayerFacing = (int)st.gPlayerFacing;
    Game_SetScene((int)st.gScene);

    hRandomAdd    = st.hRandomAdd;
    hRandomSub    = st.hRandomSub;
    hFrameCounter = st.hFrameCounter;

    memcpy(wPlayerName, st.wPlayerName, NAME_LENGTH);
    memcpy(wRivalName,  st.wRivalName,  NAME_LENGTH);
    wPlayerID       = st.wPlayerID;
    wObtainedBadges = st.wObtainedBadges;
    wRivalStarter   = st.wRivalStarter;

    memcpy(wPlayerMoney, st.wPlayerMoney, 3);
    wAmountMoneyWon = st.wAmountMoneyWon;
    wNumBagItems    = st.wNumBagItems;
    memcpy(wBagItems, st.wBagItems, sizeof(wBagItems));

    wPartyCount = st.wPartyCount;
    memcpy(wPartyMons,    st.wPartyMons,    sizeof(wPartyMons));
    memcpy(wPartyMonOT,   st.wPartyMonOT,   sizeof(wPartyMonOT));
    memcpy(wPartyMonNicks,st.wPartyMonNicks,sizeof(wPartyMonNicks));
    memcpy(wPartySpecies, st.wPartySpecies, sizeof(wPartySpecies));

    memcpy(wPokedexOwned, st.wPokedexOwned, sizeof(wPokedexOwned));
    memcpy(wPokedexSeen,  st.wPokedexSeen,  sizeof(wPokedexSeen));
    memcpy(wEventFlags,   st.wEventFlags,   EVENT_FLAGS_BYTES);
    memcpy(wPickedUpItems,st.wPickedUpItems,sizeof(wPickedUpItems));

    wIsInBattle = st.wIsInBattle; wBattleType = st.wBattleType;
    wCurEnemyLevel = st.wCurEnemyLevel; wTrainerClass = st.wTrainerClass;
    wLoneAttackNo  = st.wLoneAttackNo;
    hWhoseTurn = st.hWhoseTurn;
    wPlayerMonNumber    = st.wPlayerMonNumber;
    wCalculateWhoseStats = st.wCalculateWhoseStats;
    wBattleMon = st.wBattleMon; wEnemyMon = st.wEnemyMon;
    memcpy(wPlayerMonStatMods, st.wPlayerMonStatMods, NUM_STAT_MODS);
    memcpy(wEnemyMonStatMods,  st.wEnemyMonStatMods,  NUM_STAT_MODS);
    wPlayerBattleStatus1 = st.wPlayerBattleStatus1;
    wPlayerBattleStatus2 = st.wPlayerBattleStatus2;
    wPlayerBattleStatus3 = st.wPlayerBattleStatus3;
    wEnemyBattleStatus1  = st.wEnemyBattleStatus1;
    wEnemyBattleStatus2  = st.wEnemyBattleStatus2;
    wEnemyBattleStatus3  = st.wEnemyBattleStatus3;
    wPlayerConfusedCounter = st.wPlayerConfusedCounter;
    wPlayerToxicCounter    = st.wPlayerToxicCounter;
    wPlayerDisabledMove    = st.wPlayerDisabledMove;
    wEnemyConfusedCounter  = st.wEnemyConfusedCounter;
    wEnemyToxicCounter     = st.wEnemyToxicCounter;
    wEnemyDisabledMove     = st.wEnemyDisabledMove;
    wPlayerSelectedMove = st.wPlayerSelectedMove;
    wEnemySelectedMove  = st.wEnemySelectedMove;
    wPlayerMoveNum = st.wPlayerMoveNum; wEnemyMoveNum = st.wEnemyMoveNum;
    wCriticalHitOrOHKO = st.wCriticalHitOrOHKO;
    wDamage = st.wDamage; wMoveMissed = st.wMoveMissed;
    wActionResultOrTookBattleTurn = st.wActionResultOrTookBattleTurn;
    wPlayerUsedMove = st.wPlayerUsedMove; wEnemyUsedMove = st.wEnemyUsedMove;
    wPlayerNumAttacksLeft = st.wPlayerNumAttacksLeft;
    wEnemyNumAttacksLeft  = st.wEnemyNumAttacksLeft;
    wPlayerNumHits = st.wPlayerNumHits; wEnemyNumHits = st.wEnemyNumHits;
    wPlayerBideAccumulatedDamage = st.wPlayerBideAccumulatedDamage;
    wEnemyBideAccumulatedDamage  = st.wEnemyBideAccumulatedDamage;
    wPlayerSubstituteHP = st.wPlayerSubstituteHP;
    wEnemySubstituteHP  = st.wEnemySubstituteHP;
    wPlayerMonMinimized = st.wPlayerMonMinimized;
    wEnemyMonMinimized  = st.wEnemyMonMinimized;
    wPlayerDisabledMoveNumber = st.wPlayerDisabledMoveNumber;
    wEnemyDisabledMoveNumber  = st.wEnemyDisabledMoveNumber;
    wPlayerMonUnmodifiedAttack  = st.wPlayerMonUnmodifiedAttack;
    wPlayerMonUnmodifiedDefense = st.wPlayerMonUnmodifiedDefense;
    wPlayerMonUnmodifiedSpeed   = st.wPlayerMonUnmodifiedSpeed;
    wPlayerMonUnmodifiedSpecial = st.wPlayerMonUnmodifiedSpecial;
    wEnemyMonUnmodifiedAttack   = st.wEnemyMonUnmodifiedAttack;
    wEnemyMonUnmodifiedDefense  = st.wEnemyMonUnmodifiedDefense;
    wEnemyMonUnmodifiedSpeed    = st.wEnemyMonUnmodifiedSpeed;
    wEnemyMonUnmodifiedSpecial  = st.wEnemyMonUnmodifiedSpecial;
    wTransformedEnemyMonOriginalDVs = st.wTransformedEnemyMonOriginalDVs;
    wFirstMonsNotOutYet = st.wFirstMonsNotOutYet;
    wBattleResult = st.wBattleResult;

    wEnemyPartyCount = st.wEnemyPartyCount;
    memcpy(wEnemyMons, st.wEnemyMons, sizeof(wEnemyMons));
    wEnemyMonPartyPos       = st.wEnemyMonPartyPos;
    wNumRunAttempts         = st.wNumRunAttempts;
    wForcePlayerToChooseMon = st.wForcePlayerToChooseMon;
    wAICount               = st.wAICount;
    wAILayer2Encouragement = st.wAILayer2Encouragement;
    wLastSwitchInEnemyMonHP = st.wLastSwitchInEnemyMonHP;
    gEngagedTrainerClass    = st.gEngagedTrainerClass;

    wPartyGainExpFlags            = st.wPartyGainExpFlags;
    wPartyFoughtCurrentEnemyFlags = st.wPartyFoughtCurrentEnemyFlags;
    wGainBoostedExp    = st.wGainBoostedExp;
    wCanEvolveFlags    = st.wCanEvolveFlags;
    wEvolutionOccurred = st.wEvolutionOccurred;
}

int Save_StateWrite(const char *path) {
    pack_state();
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    fwrite(&st, 1, sizeof(st), f);
    fclose(f);
    return 0;
}

int Save_StateLoad(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    size_t n = fread(&st, 1, sizeof(st), f);
    fclose(f);
    if (n != sizeof(st)) return -1;
    if (st.magic != STATE_MAGIC || st.version != STATE_VERSION) return -1;
    unpack_state();
    s_last_load_in_battle = (st.wIsInBattle != 0);
    return 0;
}

int Save_StateWasBattle(void) { return s_last_load_in_battle; }
