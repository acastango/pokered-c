#include "rockethideout_scripts.h"
#include "player.h"
#include "overworld.h"
#include "constants.h"
#include "../platform/hardware.h"
#include "../platform/audio.h"
#include "../data/event_constants.h"
#include <stdint.h>
#include <stdio.h>

#define MAP_ROCKET_HIDEOUT_B1F 0xc7
#define MAP_ROCKET_HIDEOUT_B2F 0xc8
#define MAP_ROCKET_HIDEOUT_B3F 0xc9
#define MAP_ROCKET_HIDEOUT_B4F 0xca

#define DIR_DOWN  0
#define DIR_UP    1
#define DIR_LEFT  2
#define DIR_RIGHT 3

typedef struct {
    uint8_t x, y;
    const int8_t *seq;
} spin_coord_t;

static int s_spin_active = 0;
static const int8_t *s_spin_seq = 0;
static int s_spin_idx = 0;
static uint8_t s_b4f_door_block = 0xFF;
static int s_b4f_dbg_ticks = 0;

/* ASM DecodeArrowMovementRLE consumes from the terminal marker backwards. */
static int seq_last_idx(const int8_t *seq) {
    int i = 0;
    while (seq[i] != -1) i++;
    return i - 1;
}

/* ---- B2F spinner paths (scripts/RocketHideoutB2F.asm) ---- */
static const int8_t kB2M1[]  = { DIR_LEFT, DIR_LEFT, -1 };
static const int8_t kB2M2[]  = { DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, -1 };
static const int8_t kB2M3[]  = { DIR_UP, DIR_UP, DIR_UP, DIR_UP, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, -1 };
static const int8_t kB2M4[]  = { DIR_UP, DIR_UP, DIR_UP, DIR_UP, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_UP, -1 };
static const int8_t kB2M5[]  = { DIR_LEFT, DIR_LEFT, DIR_UP, DIR_UP, DIR_UP, -1 };
static const int8_t kB2M6[]  = { DIR_DOWN, DIR_DOWN, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, -1 };
static const int8_t kB2M7[]  = { DIR_UP, DIR_UP, -1 };
static const int8_t kB2M8[]  = { DIR_UP, DIR_UP, DIR_UP, DIR_UP, -1 };
static const int8_t kB2M9[]  = { DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, -1 };
static const int8_t kB2M10[] = { DIR_UP, -1 };
static const int8_t kB2M11[] = { DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_UP, DIR_UP, DIR_UP, DIR_UP, -1 };
static const int8_t kB2M12[] = { DIR_DOWN, DIR_DOWN, -1 };
static const int8_t kB2M13[] = { DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, -1 };
static const int8_t kB2M14[] = { DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_UP, -1 };
static const int8_t kB2M15[] = { DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_UP, DIR_UP, DIR_UP, DIR_UP, DIR_UP, DIR_UP, -1 };
static const int8_t kB2M16[] = { DIR_UP, DIR_UP, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, -1 };
static const int8_t kB2M17[] = { DIR_UP, DIR_UP, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_UP, DIR_UP, -1 };
static const int8_t kB2M18[] = { DIR_DOWN, DIR_DOWN, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_DOWN, DIR_DOWN, -1 };
static const int8_t kB2M19[] = { DIR_DOWN, DIR_DOWN, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, -1 };
static const int8_t kB2M20[] = { DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, -1 };
static const int8_t kB2M21[] = { DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_UP, DIR_UP, -1 };
static const int8_t kB2M22[] = { DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_UP, DIR_UP, DIR_UP, DIR_UP, -1 };
static const int8_t kB2M23[] = { DIR_UP, DIR_UP, DIR_RIGHT, DIR_RIGHT, -1 };
static const int8_t kB2M24[] = { DIR_RIGHT, DIR_DOWN, DIR_DOWN, -1 };
static const int8_t kB2M25[] = { DIR_RIGHT, -1 };
static const int8_t kB2M26[] = { DIR_DOWN, DIR_DOWN, DIR_RIGHT, DIR_RIGHT, -1 };
static const int8_t kB2M27[] = { DIR_DOWN, DIR_DOWN, DIR_LEFT, DIR_LEFT, -1 };
static const int8_t kB2M28[] = { DIR_UP, DIR_UP, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_UP, DIR_UP, DIR_LEFT, DIR_LEFT, DIR_LEFT, -1 };
static const int8_t kB2M29[] = { DIR_DOWN, DIR_DOWN, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, -1 };
static const int8_t kB2M30[] = { DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_UP, DIR_UP, DIR_UP, DIR_UP, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, -1 };
static const int8_t kB2M31[] = { DIR_UP, DIR_UP, -1 };
static const int8_t kB2M32[] = { DIR_UP, -1 };
static const int8_t kB2M33[] = { DIR_UP, DIR_UP, DIR_UP, -1 };
static const int8_t kB2M34[] = { DIR_UP, DIR_UP, DIR_UP, DIR_UP, DIR_UP, -1 };
static const int8_t kB2M35[] = { DIR_RIGHT, DIR_DOWN, DIR_DOWN, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, -1 };
static const int8_t kB2M36[] = { DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_UP, DIR_UP, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, DIR_LEFT, -1 };

static const spin_coord_t kB2Spin[] = {
    { 4,  9, kB2M1  }, { 4, 11, kB2M2  }, { 4, 15, kB2M3  }, { 4, 16, kB2M4  },
    { 4, 19, kB2M1  }, { 4, 22, kB2M5  }, { 5, 14, kB2M6  }, { 6, 22, kB2M7  },
    { 6, 24, kB2M8  }, { 8,  9, kB2M9  }, { 8, 12, kB2M10 }, { 8, 15, kB2M8  },
    { 8, 19, kB2M9  }, { 8, 23, kB2M11 }, { 9, 14, kB2M12 }, { 9, 22, kB2M12 },
    {10,  9, kB2M13 }, {10, 10, kB2M14 }, {10, 15, kB2M15 }, {10, 17, kB2M16 },
    {10, 19, kB2M17 }, {10, 25, kB2M2  }, {11, 14, kB2M18 }, {11, 16, kB2M19 },
    {11, 18, kB2M12 }, {12,  9, kB2M20 }, {12, 11, kB2M21 }, {12, 13, kB2M22 },
    {12, 17, kB2M23 }, {13, 10, kB2M24 }, {13, 12, kB2M25 }, {13, 16, kB2M26 },
    {13, 18, kB2M27 }, {13, 19, kB2M28 }, {13, 22, kB2M29 }, {13, 23, kB2M30 },
    {14, 17, kB2M31 }, {15, 16, kB2M12 }, {16, 14, kB2M32 }, {16, 16, kB2M33 },
    {16, 18, kB2M34 }, {17, 10, kB2M35 }, {17, 11, kB2M36 },
};

/* ---- B3F spinner paths (scripts/RocketHideoutB3F.asm) ---- */
static const int8_t kB3M1[]  = { DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_UP, DIR_UP, DIR_UP, DIR_UP, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, -1 };
static const int8_t kB3M2[]  = { DIR_DOWN, DIR_DOWN, DIR_DOWN, DIR_DOWN, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, -1 };
static const int8_t kB3M3[]  = { DIR_LEFT, DIR_LEFT, -1 };
static const int8_t kB3M4[]  = { DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_UP, DIR_UP, DIR_RIGHT, DIR_RIGHT, -1 };
static const int8_t kB3M5[]  = { DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_UP, DIR_UP, DIR_RIGHT, DIR_RIGHT, DIR_UP, DIR_UP, DIR_UP, -1 };
static const int8_t kB3M6[]  = { DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, -1 };
static const int8_t kB3M7[]  = { DIR_RIGHT, DIR_RIGHT, -1 };
static const int8_t kB3M8[]  = { DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_UP, DIR_UP, -1 };
static const int8_t kB3M9[]  = { DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_UP, DIR_UP, DIR_UP, DIR_UP, -1 };
static const int8_t kB3M10[] = { DIR_DOWN, DIR_DOWN, DIR_DOWN, DIR_DOWN, -1 };
static const int8_t kB3M11[] = { DIR_UP, DIR_UP, -1 };
static const int8_t kB3M12[] = { DIR_UP, -1 };

static const spin_coord_t kB3Spin[] = {
    {10, 13, kB3M6  }, {10, 19, kB3M1  }, {11, 18, kB3M2  }, {12, 11, kB3M3  },
    {12, 17, kB3M4  }, {12, 20, kB3M5  }, {13, 16, kB3M6  }, {14, 11, kB3M7  },
    {14, 15, kB3M6  }, {14, 17, kB3M8  }, {14, 19, kB3M9  }, {15, 16, kB3M7  },
    {15, 18, kB3M10 }, {16, 13, kB3M11 }, {17, 12, kB3M10 }, {18, 16, kB3M12 },
};

static const int8_t *find_spin_seq(uint8_t map, uint8_t x, uint8_t y) {
    const spin_coord_t *tbl = 0;
    int n = 0;
    if (map == MAP_ROCKET_HIDEOUT_B2F) {
        tbl = kB2Spin;
        n = (int)(sizeof(kB2Spin) / sizeof(kB2Spin[0]));
    } else if (map == MAP_ROCKET_HIDEOUT_B3F) {
        tbl = kB3Spin;
        n = (int)(sizeof(kB3Spin) / sizeof(kB3Spin[0]));
    } else {
        return 0;
    }
    for (int i = 0; i < n; i++) {
        if (tbl[i].x == x && tbl[i].y == y) return tbl[i].seq;
    }
    return 0;
}

static void apply_b1f_door(void) {
    if (CheckEvent(EVENT_ENTERED_ROCKET_HIDEOUT)) {
        Map_SetBlock(8, 12, 0x0e);
        return;
    }
    if (CheckEvent(EVENT_BEAT_ROCKET_HIDEOUT_1_TRAINER_4)) {
        Audio_PlaySFX_GoInside();
        Map_SetBlock(8, 12, 0x0e);
        return;
    }
    Map_SetBlock(8, 12, 0x54);
}

static void apply_b4f_door(void) {
    uint8_t want;
    if (!CheckEvent(EVENT_ROCKET_HIDEOUT_4_DOOR_UNLOCKED) &&
        CheckEvent(EVENT_BEAT_ROCKET_HIDEOUT_4_TRAINER_0) &&
        CheckEvent(EVENT_BEAT_ROCKET_HIDEOUT_4_TRAINER_1)) {
        SetEvent(EVENT_ROCKET_HIDEOUT_4_DOOR_UNLOCKED);
        Audio_PlaySFX_GoInside();
    }

    /* ASM ReplaceTileBlock: b=Y, c=X => (y=5, x=12). */
    want = CheckEvent(EVENT_ROCKET_HIDEOUT_4_DOOR_UNLOCKED) ? 0x0e : 0x2d;
    if (s_b4f_door_block != want) {
        s_b4f_door_block = want;
        Map_SetBlock(12, 5, want);
        /* Match ASM ReplaceTileBlock behavior: redraw view when block changes. */
        Map_BuildScrollView();
    } else {
        Map_SetBlock(12, 5, want);
    }

    /* Temporary deep-debug for B4F door mismatch reports (disabled by default). */
    if (0 && (++s_b4f_dbg_ticks % 30) == 0) {
        printf("[rh4-door] flags t0=%d t1=%d unlocked=%d want=%02x b(12,5)=%02x b(11,5)=%02x b(12,6)=%02x b(13,5)=%02x\n",
               CheckEvent(EVENT_BEAT_ROCKET_HIDEOUT_4_TRAINER_0),
               CheckEvent(EVENT_BEAT_ROCKET_HIDEOUT_4_TRAINER_1),
               CheckEvent(EVENT_ROCKET_HIDEOUT_4_DOOR_UNLOCKED),
               want,
               (uint8_t)Map_GetBlockIdRaw(12, 5),
               (uint8_t)Map_GetBlockIdRaw(11, 5),
               (uint8_t)Map_GetBlockIdRaw(12, 6),
               (uint8_t)Map_GetBlockIdRaw(13, 5));
    }
}

void RocketHideoutScripts_OnMapLoad(void) {
    s_spin_active = 0;
    s_spin_seq = 0;
    s_spin_idx = 0;
    s_b4f_door_block = 0xFF;
    s_b4f_dbg_ticks = 0;
    Player_SetSpinnerSpin(0);
    if (wCurMap == MAP_ROCKET_HIDEOUT_B1F) {
        apply_b1f_door();
    } else if (wCurMap == MAP_ROCKET_HIDEOUT_B4F) {
        apply_b4f_door();
    }
}

void RocketHideoutScripts_Tick(void) {
    /* ASM parity safety: B4F door script runs via map script callback path.
     * Keep block state synced each tick so the door cannot remain default-open
     * if a one-shot map-load callback path was skipped. */
    if (wCurMap == MAP_ROCKET_HIDEOUT_B4F)
        apply_b4f_door();

    if (!s_spin_active && !Player_IsMoving())
        RocketHideoutScripts_StepCheck();

    if (s_spin_active && !Player_IsSimulatingMovement()) {
        s_spin_active = 0;
        s_spin_seq = 0;
        s_spin_idx = 0;
        Player_SetSpinnerSpin(0);
    }
}

int RocketHideoutScripts_IsActive(void) {
    return s_spin_active || Player_IsSimulatingMovement();
}

void RocketHideoutScripts_StepCheck(void) {
    const int8_t *seq;
    if (s_spin_active) return;

    seq = find_spin_seq((uint8_t)wCurMap, (uint8_t)wXCoord, (uint8_t)wYCoord);
    if (!seq) return;

    s_spin_active = 1;
    s_spin_seq = seq;
    s_spin_idx = seq_last_idx(seq);
    if (s_spin_idx < 0) {
        s_spin_active = 0;
        s_spin_seq = 0;
        return;
    }

    Audio_PlaySFX_ArrowTiles();
    Player_SetSpinnerSpin(1);
    Player_StartSimulatedMovement(s_spin_seq, s_spin_idx);
}
