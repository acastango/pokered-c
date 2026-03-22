/* save.c — File-based save replacing GB SRAM */
#include "save.h"
#include "hardware.h"
#include "../game/constants.h"
#include "../game/types.h"
#include <stdio.h>
#include <string.h>

/* The save block mirrors what the GB writes to SRAM bank 1.
 * We serialize it as a flat struct matching the original layout. */
typedef struct __attribute__((packed)) {
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
    uint8_t     y_coord;
    uint8_t     x_coord;
    uint8_t     event_flags[EVENT_FLAGS_BYTES];
    uint8_t     game_progress_flags[256]; /* wGameProgressFlags region */
    /* --- sPartyData region --- */
    uint8_t     party_count;
    party_mon_t party_mons[PARTY_LENGTH];
    uint8_t     party_ot[PARTY_LENGTH][NAME_LENGTH];
    uint8_t     party_nicks[PARTY_LENGTH][NAME_LENGTH];
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
    save.y_coord    = wYCoord;
    save.x_coord    = wXCoord;
    memcpy(save.event_flags,    wEventFlags,     EVENT_FLAGS_BYTES);
    save.party_count = wPartyCount;
    memcpy(save.party_mons,     wPartyMons,      sizeof(wPartyMons));
    memcpy(save.party_ot,       wPartyMonOT,     sizeof(wPartyMonOT));
    memcpy(save.party_nicks,    wPartyMonNicks,  sizeof(wPartyMonNicks));

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
    wYCoord         = save.y_coord;
    wXCoord         = save.x_coord;
    memcpy(wEventFlags,    save.event_flags,   EVENT_FLAGS_BYTES);
    wPartyCount = save.party_count;
    memcpy(wPartyMons,    save.party_mons,     sizeof(wPartyMons));
    memcpy(wPartyMonOT,   save.party_ot,       sizeof(wPartyMonOT));
    memcpy(wPartyMonNicks,save.party_nicks,    sizeof(wPartyMonNicks));
}

int Save_ValidateChecksum(void) {
    uint8_t calc = CalcCheckSum((uint8_t *)&save,
                                 (uint16_t)(sizeof(save) - 1));
    return calc == save.checksum ? 0 : -1;
}

int Save_Load(void) {
    FILE *f = fopen(SAVE_FILE, "rb");
    if (!f) return -1;
    size_t n = fread(&save, 1, sizeof(save), f);
    fclose(f);
    if (n != sizeof(save)) return -1;
    if (Save_ValidateChecksum() != 0) return -1;
    unpack_save();
    return 0;
}

int Save_Write(void) {
    pack_save();
    FILE *f = fopen(SAVE_FILE, "wb");
    if (!f) return -1;
    fwrite(&save, 1, sizeof(save), f);
    fclose(f);
    return 0;
}
