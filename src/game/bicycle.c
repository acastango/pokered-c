#include "bicycle.h"
#include "constants.h"
#include "text.h"
#include "player.h"
#include "music.h"
#include "../platform/hardware.h"

/* Map IDs (from pokered constants/map_constants.asm) */
#define MAP_ROUTE16             0x1b
#define MAP_ROUTE17             0x1c
#define MAP_ROUTE18             0x1d
#define MAP_ROUTE23             0x22
#define MAP_INDIGO_PLATEAU      0x24
#define MAP_SEAFOAM_ISLANDS_B3F 0xa1
#define MAP_SEAFOAM_ISLANDS_B4F 0xa2

typedef struct {
    uint8_t map_id;
    uint8_t y;
    uint8_t x;
} forced_bike_surf_t;

static const forced_bike_surf_t kForcedBikeOrSurfMaps[] = {
    { MAP_ROUTE16,             10, 17 },
    { MAP_ROUTE16,             11, 17 },
    { MAP_ROUTE18,              8, 33 },
    { MAP_ROUTE18,              9, 33 },
    { MAP_SEAFOAM_ISLANDS_B3F,  7, 18 },
    { MAP_SEAFOAM_ISLANDS_B3F,  7, 19 },
    { MAP_SEAFOAM_ISLANDS_B4F, 14,  4 },
    { MAP_SEAFOAM_ISLANDS_B4F, 14,  5 },
    { 0xff, 0xff, 0xff },
};

static int s_always_on_bike = 0;

static int is_bike_riding_allowed(void) {
    if (wCurMap == MAP_ROUTE23 || wCurMap == MAP_INDIGO_PLATEAU) return 1;

    switch (wCurMapTileset) {
        case TILESET_OVERWORLD:
        case TILESET_FOREST:
        case TILESET_FOREST_GATE:
        case TILESET_UNDERGROUND:
        case TILESET_GATE:
        case TILESET_SHIP_PORT:
        case TILESET_CAVERN:
            return 1;
        default:
            return 0;
    }
}

void Bicycle_OnMapLoad(void) {
    if (s_always_on_bike) return;

    for (int i = 0; kForcedBikeOrSurfMaps[i].map_id != 0xff; i++) {
        const forced_bike_surf_t *e = &kForcedBikeOrSurfMaps[i];
        if (e->map_id != wCurMap) continue;
        if (e->x != (uint8_t)wXCoord || e->y != (uint8_t)wYCoord) continue;

        if (wCurMap == MAP_SEAFOAM_ISLANDS_B3F || wCurMap == MAP_SEAFOAM_ISLANDS_B4F) {
            wWalkBikeSurfState = 2;
        } else {
            s_always_on_bike = 1;
            wWalkBikeSurfState = 1;
        }
        return;
    }
}

void Bicycle_PlayDefaultMusic(void) {
    if (wWalkBikeSurfState == 2) {
        Music_Play(MUSIC_SURFING);
        return;
    }

    if (wWalkBikeSurfState == 1) {
        Music_Play(MUSIC_BIKE_RIDING);
        return;
    }

    Music_Play(Music_GetMapID(wCurMap));
}

void Bicycle_ClearAlwaysOnBike(void) {
    s_always_on_bike = 0;
}

int Bicycle_UseFromBag(void) {
    if (wIsInBattle) return 0;
    if (wWalkBikeSurfState == 2) return 0; /* surfing */

    if (wWalkBikeSurfState == 1) {
        /* get off bike */
        wWalkBikeSurfState = 0;
        Bicycle_PlayDefaultMusic();
        Text_ShowASCII("Got off\nthe BICYCLE.");
        return 1;
    }

    if (!is_bike_riding_allowed()) {
        Text_ShowASCII("No cycling\nallowed here!");
        return 1;
    }

    hJoyHeld = 0;
    wWalkBikeSurfState = 1;
    Bicycle_PlayDefaultMusic();
    Text_ShowASCII("Got on\nthe BICYCLE.");
    return 1;
}

int Bicycle_IsSpeedupActive(void) {
    if (wWalkBikeSurfState != 1) return 0;
    if (wCurMap == MAP_ROUTE17 && (hJoyHeld & (PAD_UP | PAD_LEFT | PAD_RIGHT))) return 0;
    return 1;
}

int Bicycle_ShouldUseBikeSprite(void) {
    if (wWalkBikeSurfState != 1) return 0;
    if (!is_bike_riding_allowed()) {
        wWalkBikeSurfState = 0;
        return 0;
    }
    return 1;
}
