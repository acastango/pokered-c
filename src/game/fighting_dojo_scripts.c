#include "fighting_dojo_scripts.h"
#include "constants.h"
#include "npc.h"
#include "pokemon.h"
#include "pokedex.h"
#include "text.h"
#include "yesno.h"
#include "naming_screen.h"
#include "../platform/audio.h"
#include "../platform/hardware.h"
#include "../data/event_constants.h"

#define MAP_FIGHTING_DOJO 0xB1
#define NPC_HITMONLEE_BALL 5
#define NPC_HITMONCHAN_BALL 6

typedef enum {
    FD_IDLE = 0,
    FD_WAIT_TAKE_YESNO,
    FD_WAIT_RECEIVED_CLOSE,
    FD_WAIT_NICK_YESNO,
    FD_WAIT_NAMING,
} FightingDojoState;

static FightingDojoState sState = FD_IDLE;
static int sPartySlot = -1;
static int sPendingBallIdx = -1;
static uint16_t sPendingGotEvent = 0;
static uint8_t sPendingSpecies = 0;

static const char kNeedMasterDefeat[] =
    "Beat the KARATE\nMASTER first!";
static const char kAlreadyChosen[] =
    "You already chose\nyour prize!";
static const char kPartyFull[] =
    "You can't carry\nany more POKeMON!";
static const char kTakeHitmonlee[] =
    "Do you want the\nhard-kicking\nHITMONLEE?";
static const char kTakeHitmonchan[] =
    "Do you want the\npiston-punching\nHITMONCHAN?";
static const char kGotHitmonlee[] =
    "{PLAYER} got\nHITMONLEE!@";
static const char kGotHitmonchan[] =
    "{PLAYER} got\nHITMONCHAN!@";
static const char kNickHitmonlee[] =
    "Do you want to\ngive a nickname\nto HITMONLEE?";
static const char kNickHitmonchan[] =
    "Do you want to\ngive a nickname\nto HITMONCHAN?";

static void hide_selected_ball(int ball_idx) {
    if (ball_idx >= 0) NPC_HideSprite(ball_idx);
}

static void start_hitmon_offer(uint8_t species, uint16_t got_event, int ball_idx, const char *offer_text) {
    if (!CheckEvent(EVENT_BEAT_KARATE_MASTER)) {
        Text_ShowASCII(kNeedMasterDefeat);
        return;
    }
    if (CheckEvent(EVENT_GOT_HITMONLEE) || CheckEvent(EVENT_GOT_HITMONCHAN)) {
        Text_ShowASCII(kAlreadyChosen);
        return;
    }

    sPendingSpecies = species;
    sPendingGotEvent = got_event;
    sPendingBallIdx = ball_idx;
    YesNo_Show(offer_text);
    sState = FD_WAIT_TAKE_YESNO;
}

void FightingDojo_HitmonleeBallScript(void) {
    if (sState != FD_IDLE) return;
    start_hitmon_offer(SPECIES_HITMONLEE, EVENT_GOT_HITMONLEE, NPC_HITMONLEE_BALL, kTakeHitmonlee);
}

void FightingDojo_HitmonchanBallScript(void) {
    if (sState != FD_IDLE) return;
    start_hitmon_offer(SPECIES_HITMONCHAN, EVENT_GOT_HITMONCHAN, NPC_HITMONCHAN_BALL, kTakeHitmonchan);
}

void FightingDojoScripts_OnMapLoad(void) {
    if (wCurMap != MAP_FIGHTING_DOJO) return;
    if (CheckEvent(EVENT_GOT_HITMONLEE))
        NPC_HideSprite(NPC_HITMONLEE_BALL);
    if (CheckEvent(EVENT_GOT_HITMONCHAN))
        NPC_HideSprite(NPC_HITMONCHAN_BALL);
}

void FightingDojoScripts_Tick(void) {
    switch (sState) {
    case FD_IDLE:
        return;
    case FD_WAIT_TAKE_YESNO:
        YesNo_Tick();
        if (YesNo_IsOpen()) return;
        if (!YesNo_GetResult()) {
            sState = FD_IDLE;
            sPendingSpecies = 0;
            sPendingBallIdx = -1;
            sPendingGotEvent = 0;
            return;
        }
        if (wPartyCount >= PARTY_LENGTH) {
            Text_ShowASCII(kPartyFull);
            sState = FD_IDLE;
            sPendingSpecies = 0;
            sPendingBallIdx = -1;
            sPendingGotEvent = 0;
            return;
        }
        Pokemon_AddToParty(sPendingSpecies, 30);
        if (wPartyCount == 0) {
            Text_ShowASCII(kPartyFull);
            sState = FD_IDLE;
            sPendingSpecies = 0;
            sPendingBallIdx = -1;
            sPendingGotEvent = 0;
            return;
        }
        Pokedex_SetOwned(sPendingSpecies);
        SetEvent(sPendingGotEvent);
        SetEvent(EVENT_DEFEATED_FIGHTING_DOJO);
        hide_selected_ball(sPendingBallIdx);
        Audio_PlaySFX_GetKeyItem();
        if (sPendingSpecies == SPECIES_HITMONLEE) Text_ShowASCII(kGotHitmonlee);
        else Text_ShowASCII(kGotHitmonchan);
        sPartySlot = (int)wPartyCount - 1;
        sState = FD_WAIT_RECEIVED_CLOSE;
        return;
    case FD_WAIT_RECEIVED_CLOSE:
        if (Text_IsOpen()) return;
        if (sPendingSpecies == SPECIES_HITMONLEE) YesNo_Show(kNickHitmonlee);
        else YesNo_Show(kNickHitmonchan);
        sState = FD_WAIT_NICK_YESNO;
        return;
    case FD_WAIT_NICK_YESNO:
        YesNo_Tick();
        if (YesNo_IsOpen()) return;
        if (YesNo_GetResult() && sPartySlot >= 0 && sPartySlot < PARTY_LENGTH) {
            NamingScreen_Open(NAME_MON_SCREEN, sPendingSpecies, wPartyMonNicks[sPartySlot]);
            sState = FD_WAIT_NAMING;
            return;
        }
        sState = FD_IDLE;
        sPendingSpecies = 0;
        return;
    case FD_WAIT_NAMING:
        if (NamingScreen_IsOpen()) return;
        sState = FD_IDLE;
        sPendingSpecies = 0;
        sPendingBallIdx = -1;
        sPendingGotEvent = 0;
        return;
    }
}

int FightingDojoScripts_IsActive(void) {
    return sState != FD_IDLE;
}

void FightingDojoScripts_PostRender(void) {
    if ((sState == FD_WAIT_TAKE_YESNO || sState == FD_WAIT_NICK_YESNO) && YesNo_IsOpen())
        YesNo_PostRender();
}
