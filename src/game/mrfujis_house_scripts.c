#include "mrfujis_house_scripts.h"
#include "text.h"
#include "npc.h"
#include "inventory.h"
#include "../platform/audio.h"
#include "../platform/hardware.h"
#include "../data/event_constants.h"

#define MAP_MR_FUJIS_HOUSE 0x95
#define NPC_MR_FUJI        4

static const char kSuperNerdNotHere[] =
    "That's odd, MR.FUJI\nisn't here.\nWhere'd he go?";
static const char kSuperNerdPraying[] =
    "MR.FUJI had been\npraying alone for\nCUBONE's mother.";

static const char kLittleGirlHouse[] =
    "This is really\nMR.FUJI's house.\fHe's really kind!\f"
    "He looks after\nabandoned and\norphaned POKEMON!";
static const char kLittleGirlHug[] =
    "It's so warm!\nPOKEMON are so\nnice to hug!";

static const char kFujiIntro[] =
    "MR.FUJI: {PLAYER}.\fYour POKEDEX quest\nmay fail without\nlove for your\nPOKEMON.\f"
    "I think this may\nhelp your quest.";
static const char kFujiReceived[] =
    "{PLAYER} received\na POKE FLUTE!\fUpon hearing\nPOKE FLUTE,\n"
    "sleeping POKEMON\nwill spring awake.\fIt works on all\nsleeping POKEMON.";
static const char kFujiNoRoom[] =
    "You must make\nroom for this!";
static const char kFujiAfter[] =
    "MR.FUJI: Has my\nFLUTE helped you?";

void MrFujisHouseScripts_OnMapLoad(void) {
    if (wCurMap != MAP_MR_FUJIS_HOUSE) return;
    if (CheckEvent(EVENT_RESCUED_MR_FUJI)) NPC_ShowSprite(NPC_MR_FUJI);
    else NPC_HideSprite(NPC_MR_FUJI);
}

void MrFujisHouse_SuperNerdScript(void) {
    if (CheckEvent(EVENT_RESCUED_MR_FUJI)) Text_ShowASCII(kSuperNerdPraying);
    else Text_ShowASCII(kSuperNerdNotHere);
}

void MrFujisHouse_LittleGirlScript(void) {
    if (CheckEvent(EVENT_RESCUED_MR_FUJI)) Text_ShowASCII(kLittleGirlHug);
    else Text_ShowASCII(kLittleGirlHouse);
}

void MrFujisHouse_MrFujiScript(void) {
    if (!CheckEvent(EVENT_RESCUED_MR_FUJI)) return;
    if (CheckEvent(EVENT_GOT_POKE_FLUTE)) {
        Text_ShowASCII(kFujiAfter);
        return;
    }
    Text_ShowASCII(kFujiIntro);
    if (Inventory_Add(ITEM_POKE_FLUTE, 1) == 0) {
        SetEvent(EVENT_GOT_POKE_FLUTE);
        Audio_PlaySFX_GetKeyItem();
        Text_ShowASCII(kFujiReceived);
    } else {
        Text_ShowASCII(kFujiNoRoom);
    }
}
