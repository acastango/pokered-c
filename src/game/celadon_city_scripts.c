#include "celadon_city_scripts.h"
#include "constants.h"
#include "inventory.h"
#include "npc.h"
#include "pokemon.h"
#include "pokedex.h"
#include "text.h"
#include "yesno.h"
#include "naming_screen.h"
#include "elevator_menu.h"
#include "overworld.h"
#include "../data/font_data.h"
#include "../platform/audio.h"
#include "../platform/hardware.h"
#include "../data/event_constants.h"
#include <stdio.h>

/* Item IDs are contiguous in Gen 1:
 * HM01..HM05 = 0xC4..0xC8, then TM01 starts at 0xC9. */
#define ITEM_TM01  (ITEM_HM01 + 5)
#define ITEM_TM41  (ITEM_TM01 + 40)
#define ITEM_TM18  (ITEM_TM01 + 17)
#define ITEM_COIN_CASE 0x45
#define ITEM_HM02  (ITEM_HM01 + 1)
#define ITEM_PP_UP 0x3F
#define ITEM_FRESH_WATER 0x3C
#define ITEM_SODA_POP 0x3D
#define ITEM_LEMONADE 0x3E
#define ITEM_TM13 (ITEM_TM01 + 12)
#define ITEM_TM48 (ITEM_TM01 + 47)
#define ITEM_TM49 (ITEM_TM01 + 48)

/* Missing from constants.h in this branch; Gen 1 species index value. */
#define SPECIES_POLIWRATH 0x14
#define MAP_CELADON_MANSION_ROOF_HOUSE 0x84
#define MAP_CELADON_MART_ROOF 0x7E
#define MAP_ROUTE16_FLY_HOUSE 0xBC
#define NPC_CELADON_MANSION_EEVEE_BALL 1
#define POKE_SPACE  0x7F
#define POKE_TL     0x79
#define POKE_H      0x7A
#define POKE_TR     0x7B
#define POKE_V      0x7C
#define POKE_BL     0x7D
#define POKE_BR     0x7E
#define POKE_CURSOR 0xED

#define TMIDX(r, c) (((r) + 2) * SCROLL_MAP_W + ((c) + 2))

typedef enum {
    CG_IDLE = 0,
    CG_COINCASE_WAIT_PRETEXT_CLOSE,
    CG_COINCASE_WAIT_JINGLE,
    CG_EEVEE_WAIT_RECEIVED_CLOSE,
    CG_EEVEE_WAIT_NICK_YESNO,
    CG_EEVEE_WAIT_NAMING,
    CG_TM18_WAIT_PRETEXT_CLOSE,
    CG_ROOF_WAIT_DRINK_YN,
    CG_ROOF_WAIT_SELECT_DRINK,
    CG_ROOF_WAIT_GIRL_REWARD,
} CeladonGiftState;

static CeladonGiftState sGiftState = CG_IDLE;
static int sEeveePartySlot = -1;
static int sCursor = 0;
static int sDrinkCount = 0;
static uint8_t sDrinkList[3];
static uint8_t sPendingDrinkItem = 0;
static uint8_t sMenuSaved[6 * 14];
static int sMenuSavedValid = 0;

static const char kTM41Gifted[] =
    "Hello, there!\f"
    "I've seen you,\nbut I never had a\nchance to talk!\f"
    "Here's a gift for\ndropping by!\f"
    "{PLAYER} got\nTM41!@\f"
    "TM41 teaches\nSOFTBOILED!\f"
    "Only one POKeMON\ncan use it!\f"
    "That POKeMON is\nCHANSEY!";

static const char kTM41Explain[] =
    "TM41 teaches\nSOFTBOILED!\f"
    "Only one POKeMON\ncan use it!\f"
    "That POKeMON is\nCHANSEY!";

static const char kTM41NoRoom[] =
    "Oh, your pack is\nfull of items!";
static const char kTM18PreReceive[] =
    "Oh, hi! I finally\nfinished POKeMON!\f"
    "Not done yet?\nThis might be\nuseful!";
static const char kTM18Received[] =
    "{PLAYER} received\nTM18!@";
static const char kTM18Explain[] =
    "TM18 is COUNTER!\nNot like the one\nI'm leaning on,\nmind you!";
static const char kTM18NoRoom[] =
    "Your pack is full\nof items!";

static const char kCoinCaseGifted[] =
    "{PLAYER} got\na COIN CASE!@";

static const char kCoinCasePreGift[] =
    "Go ahead! Laugh!\f"
    "I'm flat out\nbusted!\f"
    "No more slots for\nme! I'm going\nstraight!\f"
    "Here! I won't be\nneeding this any-\nmore!";

static const char kCoinCaseNoRoom[] =
    "Make room for\nthis!";

static const char kCoinCaseAfter[] =
    "I always thought\nI was going to\nwin it back...";

static const char kEeveeReceived[] =
    "{PLAYER} got\nEEVEE!@";
static const char kEeveeNickPrompt[] =
    "Do you want to\ngive a nickname\nto EEVEE?";

static const char kEeveePartyFull[] =
    "You can't carry\nany more POKeMON!";

static const char kHM02Gifted[] =
    "Oh, you found my\nsecret retreat!\f"
    "Please don't tell\nanyone I'm here.\nI'll make it up\nto you with this!\f"
    "{PLAYER} got\nHM02!@\f"
    "HM02 is FLY.\nIt will take you\nback to any town.\f"
    "Put it to good\nuse!";

static const char kHM02Explain[] =
    "HM02 is FLY.\nIt will take you\nback to any town.\f"
    "Put it to good\nuse!";

static const char kHM02NoRoom[] =
    "You don't have any\nroom for this.";

static const char kFoundPPUp[] =
    "Found PP UP!@";

static const char kNoRoomForPPUp[] =
    "No room for\nmore items!";
static const char kGirlGiveDrinkPrompt[] =
    "I'm thirsty!\nI want something\nto drink!\f"
    "Give her a drink?";
static const char kGirlGiveWhichDrink[] =
    "Give her which\ndrink?";
static const char kGirlImThirsty[] =
    "I'm thirsty!\nI want something\nto drink!";
static const char kGirlNotThirsty[] =
    "No thank you!\nI'm not thirsty\nafter all!";
static const char kGirlNoRoom[] =
    "You don't have\nspace for this!";
static const char kGirlYayFresh[] =
    "Yay!\f"
    "FRESH WATER!\f"
    "Thank you!\f"
    "You can have this\nfrom me!@";
static const char kGirlYaySoda[] =
    "Yay!\f"
    "SODA POP!\f"
    "Thank you!\f"
    "You can have this\nfrom me!@";
static const char kGirlYayLemon[] =
    "Yay!\f"
    "LEMONADE!\f"
    "Thank you!\f"
    "You can have this\nfrom me!@";
static const char kGirlGotTM13[] =
    "{PLAYER} received\nTM13!@\f"
    "TM13 contains\nICE BEAM!\f"
    "It can freeze the\ntarget sometimes!@";
static const char kGirlGotTM48[] =
    "{PLAYER} received\nTM48!@\f"
    "TM48 contains\nROCK SLIDE!\f"
    "It can spook the\ntarget sometimes!@";
static const char kGirlGotTM49[] =
    "{PLAYER} received\nTM49!@\f"
    "TM49 contains\nTRI ATTACK!@";
static const char kVendingIntro[] =
    "A vending machine!\nHere's the menu!";

static uint8_t poke_char(unsigned char c) {
    if (c >= 'A' && c <= 'Z') return (uint8_t)Font_CharToTile((uint8_t)(0x80 + (c - 'A')));
    if (c >= 'a' && c <= 'z') return (uint8_t)Font_CharToTile((uint8_t)(0xA0 + (c - 'a')));
    if (c >= '0' && c <= '9') return (uint8_t)Font_CharToTile((uint8_t)(0xF6 + (c - '0')));
    if (c == '-') return (uint8_t)Font_CharToTile(0xE3);
    if (c == ' ') return (uint8_t)Font_CharToTile(POKE_SPACE);
    return (uint8_t)Font_CharToTile(POKE_SPACE);
}

static void menu_set(int row, int col, uint8_t tile) {
    gScrollTileMap[TMIDX(row, col)] = tile;
}

static void menu_draw_box(int row, int col, int w, int h) {
    menu_set(row, col, (uint8_t)Font_CharToTile(POKE_TL));
    for (int c = 1; c < w - 1; c++) menu_set(row, col + c, (uint8_t)Font_CharToTile(POKE_H));
    menu_set(row, col + w - 1, (uint8_t)Font_CharToTile(POKE_TR));
    for (int r = 1; r < h - 1; r++) {
        menu_set(row + r, col, (uint8_t)Font_CharToTile(POKE_V));
        for (int c = 1; c < w - 1; c++) menu_set(row + r, col + c, (uint8_t)Font_CharToTile(POKE_SPACE));
        menu_set(row + r, col + w - 1, (uint8_t)Font_CharToTile(POKE_V));
    }
    menu_set(row + h - 1, col, (uint8_t)Font_CharToTile(POKE_BL));
    for (int c = 1; c < w - 1; c++) menu_set(row + h - 1, col + c, (uint8_t)Font_CharToTile(POKE_H));
    menu_set(row + h - 1, col + w - 1, (uint8_t)Font_CharToTile(POKE_BR));
}

static void menu_put(int row, int col, const char *s) {
    for (int i = 0; s[i]; i++) menu_set(row, col + i, poke_char((unsigned char)s[i]));
}


static void build_roof_drink_list(void) {
    sDrinkCount = 0;
    if (Inventory_GetQty(ITEM_FRESH_WATER) > 0) sDrinkList[sDrinkCount++] = ITEM_FRESH_WATER;
    if (Inventory_GetQty(ITEM_SODA_POP) > 0) sDrinkList[sDrinkCount++] = ITEM_SODA_POP;
    if (Inventory_GetQty(ITEM_LEMONADE) > 0) sDrinkList[sDrinkCount++] = ITEM_LEMONADE;
}

static void menu_save(void) {
    for (int r = 0; r < 6; r++)
        for (int c = 0; c < 14; c++)
            sMenuSaved[r * 14 + c] = gScrollTileMap[TMIDX(r + 2, c)];
    sMenuSavedValid = 1;
}

static void menu_restore(void) {
    if (!sMenuSavedValid) return;
    for (int r = 0; r < 6; r++)
        for (int c = 0; c < 14; c++)
            gScrollTileMap[TMIDX(r + 2, c)] = sMenuSaved[r * 14 + c];
    sMenuSavedValid = 0;
}

static void draw_drink_list_menu(void) {
    if (!sMenuSavedValid) menu_save();
    menu_draw_box(2, 0, 14, 2 + sDrinkCount + 1);
    char name[16];
    for (int i = 0; i < sDrinkCount; i++) {
        Inventory_DecodeASCII(sDrinkList[i], name, sizeof(name));
        menu_set(3 + i, 1, (i == sCursor) ? (uint8_t)Font_CharToTile(POKE_CURSOR) : (uint8_t)Font_CharToTile(POKE_SPACE));
        menu_put(3 + i, 2, name);
    }
}

void CeladonCity_Gramps3Script(void) {
    if (CheckEvent(EVENT_GOT_TM41)) {
        Text_ShowASCII(kTM41Explain);
        return;
    }

    if (Inventory_Add(ITEM_TM41, 1) != 0) {
        Text_ShowASCII(kTM41NoRoom);
        return;
    }

    SetEvent(EVENT_GOT_TM41);
    Audio_PlaySFX_GetKeyItem();
    Text_ShowASCII(kTM41Gifted);
}

void CeladonMart3F_ClerkScript(void) {
    if (sGiftState != CG_IDLE) return;

    if (CheckEvent(EVENT_GOT_TM18)) {
        Text_ShowASCII(kTM18Explain);
        return;
    }

    Text_ShowASCII(kTM18PreReceive);
    sGiftState = CG_TM18_WAIT_PRETEXT_CLOSE;
}

void CeladonCity_PoliwrathScript(void) {
    Text_ShowASCII("POLIWRATH: Ribi\nribit!@");
    Audio_PlayCry(SPECIES_POLIWRATH);
}

void CeladonCity_HiddenPPUpScript(void) {
    if (CheckEvent(EVENT_FOUND_CELADON_CITY_HIDDEN_PP_UP))
        return;
    if (Inventory_Add(ITEM_PP_UP, 1) != 0) {
        Text_ShowASCII(kNoRoomForPPUp);
        return;
    }
    SetEvent(EVENT_FOUND_CELADON_CITY_HIDDEN_PP_UP);
    Audio_PlaySFX_GetItem1();
    Text_ShowASCII(kFoundPPUp);
}

void CeladonDiner_GymGuideScript(void) {
    if (sGiftState != CG_IDLE) return;

    if (CheckEvent(EVENT_GOT_COIN_CASE)) {
        Text_ShowASCII(kCoinCaseAfter);
        return;
    }

    if (Inventory_Add(ITEM_COIN_CASE, 1) != 0) {
        Text_ShowASCII(kCoinCaseNoRoom);
        return;
    }

    SetEvent(EVENT_GOT_COIN_CASE);
    Text_ShowASCII(kCoinCasePreGift);
    sGiftState = CG_COINCASE_WAIT_PRETEXT_CLOSE;
}

void CeladonMansionRoofHouse_EeveeScript(void) {
    if (CheckEvent(EVENT_GOT_EEVEE)) {
        NPC_HideSprite(NPC_CELADON_MANSION_EEVEE_BALL);
        return;
    }

    uint8_t before_count = wPartyCount;
    Pokemon_AddToParty(SPECIES_EEVEE, 25);
    if (wPartyCount == before_count) {
        Text_ShowASCII(kEeveePartyFull);
        return;
    }

    Pokedex_SetOwned(SPECIES_EEVEE);
    SetEvent(EVENT_GOT_EEVEE);
    NPC_HideSprite(NPC_CELADON_MANSION_EEVEE_BALL);
    Audio_PlaySFX_GetKeyItem();
    Text_ShowASCII(kEeveeReceived);
    sEeveePartySlot = (int)wPartyCount - 1;
    sGiftState = CG_EEVEE_WAIT_RECEIVED_CLOSE;
}

void Route16FlyHouse_BrunetteScript(void) {
    if (CheckEvent(EVENT_GOT_HM02)) {
        Text_ShowASCII(kHM02Explain);
        return;
    }

    if (Inventory_Add(ITEM_HM02, 1) != 0) {
        Text_ShowASCII(kHM02NoRoom);
        return;
    }

    SetEvent(EVENT_GOT_HM02);
    Audio_PlaySFX_GetKeyItem();
    Text_ShowASCII(kHM02Gifted);
}

void Route16FlyHouse_FearowScript(void) {
    Text_ShowASCII("FEAROW: Kyueen!");
    Audio_PlayCry(SPECIES_FEAROW);
}

void CeladonMartRoof_LittleGirlScript(void) {
    if (sGiftState != CG_IDLE) return;
    build_roof_drink_list();
    if (sDrinkCount == 0) {
        Text_ShowASCII(kGirlImThirsty);
        return;
    }
    YesNo_Show(kGirlGiveDrinkPrompt);
    sGiftState = CG_ROOF_WAIT_DRINK_YN;
}

void CeladonMartRoof_VendingMachineScript(void) {
    if (sGiftState != CG_IDLE) return;
    wDoNotWaitForButtonPress = 1;
    Text_ShowASCII(kVendingIntro);
    ElevatorMenu_QueueOpenVending();
}

void CeladonGiftScripts_OnMapLoad(void) {
    if (wCurMap == MAP_CELADON_MANSION_ROOF_HOUSE && CheckEvent(EVENT_GOT_EEVEE))
        NPC_HideSprite(NPC_CELADON_MANSION_EEVEE_BALL);
}

void CeladonGiftScripts_Tick(void) {
    switch (sGiftState) {
    case CG_IDLE:
        return;
    case CG_COINCASE_WAIT_PRETEXT_CLOSE:
        if (Text_IsOpen()) return;
        Audio_PlaySFX_GetKeyItem();
        Text_ShowASCII(kCoinCaseGifted);
        sGiftState = CG_COINCASE_WAIT_JINGLE;
        return;
    case CG_COINCASE_WAIT_JINGLE:
        if (Text_IsOpen()) return;
        if (Audio_IsSFXPlaying_GetKeyItem()) return;
        sGiftState = CG_IDLE;
        return;
    case CG_EEVEE_WAIT_RECEIVED_CLOSE:
        if (Text_IsOpen()) return;
        YesNo_Show(kEeveeNickPrompt);
        sGiftState = CG_EEVEE_WAIT_NICK_YESNO;
        return;
    case CG_EEVEE_WAIT_NICK_YESNO:
        YesNo_Tick();
        if (YesNo_IsOpen()) return;
        if (YesNo_GetResult() && sEeveePartySlot >= 0 && sEeveePartySlot < PARTY_LENGTH) {
            NamingScreen_Open(NAME_MON_SCREEN, SPECIES_EEVEE, wPartyMonNicks[sEeveePartySlot]);
            sGiftState = CG_EEVEE_WAIT_NAMING;
            return;
        }
        sGiftState = CG_IDLE;
        return;
    case CG_EEVEE_WAIT_NAMING:
        if (NamingScreen_IsOpen()) return;
        sGiftState = CG_IDLE;
        return;
    case CG_TM18_WAIT_PRETEXT_CLOSE:
        if (Text_IsOpen()) return;
        if (Inventory_Add(ITEM_TM18, 1) != 0) {
            Text_ShowASCII(kTM18NoRoom);
            sGiftState = CG_IDLE;
            return;
        }
        SetEvent(EVENT_GOT_TM18);
        Audio_PlaySFX_GetItem1();
        Text_ShowASCII(kTM18Received);
        sGiftState = CG_IDLE;
        return;
    case CG_ROOF_WAIT_DRINK_YN:
        YesNo_Tick();
        if (YesNo_IsOpen()) return;
        if (!YesNo_GetResult()) {
            Text_ShowASCII(kGirlNotThirsty);
            sGiftState = CG_IDLE;
            return;
        }
        Text_ShowASCII(kGirlGiveWhichDrink);
        sCursor = 0;
        sMenuSavedValid = 0;
        sGiftState = CG_ROOF_WAIT_SELECT_DRINK;
        return;
    case CG_ROOF_WAIT_SELECT_DRINK:
        if (Text_IsOpen()) return;
        if (sDrinkCount <= 0) {
            sGiftState = CG_IDLE;
            return;
        }
        if (hJoyPressed & PAD_UP) {
            if (sCursor > 0) sCursor--;
        }
        if (hJoyPressed & PAD_DOWN) {
            if (sCursor < (sDrinkCount - 1)) sCursor++;
        }
        if (hJoyPressed & PAD_B) {
            menu_restore();
            Text_ShowASCII(kGirlNotThirsty);
            sGiftState = CG_IDLE;
            return;
        }
        if (hJoyPressed & PAD_A) {
            uint8_t item = sDrinkList[sCursor];
            menu_restore();
            sPendingDrinkItem = item;
            if (item == ITEM_FRESH_WATER && CheckEvent(EVENT_GOT_TM13)) {
                Text_ShowASCII(kGirlNotThirsty);
                sGiftState = CG_IDLE;
                return;
            }
            if (item == ITEM_SODA_POP && CheckEvent(EVENT_GOT_TM48)) {
                Text_ShowASCII(kGirlNotThirsty);
                sGiftState = CG_IDLE;
                return;
            }
            if (item == ITEM_LEMONADE && CheckEvent(EVENT_GOT_TM49)) {
                Text_ShowASCII(kGirlNotThirsty);
                sGiftState = CG_IDLE;
                return;
            }
            if (item == ITEM_FRESH_WATER) Text_ShowASCII(kGirlYayFresh);
            else if (item == ITEM_SODA_POP) Text_ShowASCII(kGirlYaySoda);
            else Text_ShowASCII(kGirlYayLemon);
            sGiftState = CG_ROOF_WAIT_GIRL_REWARD;
            return;
        }
        return;
    case CG_ROOF_WAIT_GIRL_REWARD:
        if (Text_IsOpen()) return;
        if (Inventory_Remove(sPendingDrinkItem, 1) != 0) {
            sGiftState = CG_IDLE;
            return;
        }
        if (sPendingDrinkItem == ITEM_FRESH_WATER) {
            if (Inventory_Add(ITEM_TM13, 1) != 0) {
                Text_ShowASCII(kGirlNoRoom);
                sGiftState = CG_IDLE;
                return;
            }
            SetEvent(EVENT_GOT_TM13);
            Audio_PlaySFX_GetItem1();
            Text_ShowASCII(kGirlGotTM13);
        } else if (sPendingDrinkItem == ITEM_SODA_POP) {
            if (Inventory_Add(ITEM_TM48, 1) != 0) {
                Text_ShowASCII(kGirlNoRoom);
                sGiftState = CG_IDLE;
                return;
            }
            SetEvent(EVENT_GOT_TM48);
            Audio_PlaySFX_GetItem1();
            Text_ShowASCII(kGirlGotTM48);
        } else {
            if (Inventory_Add(ITEM_TM49, 1) != 0) {
                Text_ShowASCII(kGirlNoRoom);
                sGiftState = CG_IDLE;
                return;
            }
            SetEvent(EVENT_GOT_TM49);
            Audio_PlaySFX_GetItem1();
            Text_ShowASCII(kGirlGotTM49);
        }
        sGiftState = CG_IDLE;
        return;
    }
}

int CeladonGiftScripts_IsActive(void) {
    return sGiftState != CG_IDLE;
}

void CeladonGiftScripts_PostRender(void) {
    if (sGiftState == CG_EEVEE_WAIT_NICK_YESNO && YesNo_IsOpen())
        YesNo_PostRender();
    if (sGiftState == CG_ROOF_WAIT_SELECT_DRINK && !Text_IsOpen())
        draw_drink_list_menu();
}
