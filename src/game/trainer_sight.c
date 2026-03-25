/* trainer_sight.c — Gen 1 trainer line-of-sight and engagement sequence.
 *
 * Ports:
 *   CheckFightingMapTrainers   (home/trainers.asm:128)
 *   TrainerEngage              (engine/overworld/trainer_sight.asm:163)
 *   CheckSpriteCanSeePlayer    (engine/overworld/trainer_sight.asm:257)
 *   CheckPlayerIsInFrontOfSprite (engine/overworld/trainer_sight.asm:293)
 *   TrainerWalkUpToPlayer      (engine/overworld/trainer_sight.asm:77)
 *   DisplayEnemyTrainerTextAndStartBattle (home/trainers.asm:162)
 *
 * Coordinate system:
 *   wXCoord / wYCoord and npc tile positions use 2-unit steps per visible
 *   block (asm_x*2).  One "sight tile" in pokered (16px block) = 2 units here.
 *   Distance: abs(diff) / 2 <= sight_dist.
 *
 * Facing / in-front rules (from CheckPlayerIsInFrontOfSprite):
 *   DOWN  (0): trainer_y < player_y  (player south of trainer)
 *   UP    (1): trainer_y >= player_y (player north of trainer; ">=" because
 *              pokered uses "jr nc" = jump-if-carry-clear = A >= B)
 *   LEFT  (2): trainer_x >= player_x (player west; trainer right of player)
 *   RIGHT (3): trainer_x < player_x  (player east; trainer left of player)
 *
 * ALWAYS refer to pokered-master ASM before modifying anything here.
 */

#include "trainer_sight.h"
#include "npc.h"
#include "text.h"
#include "overworld.h"
#include "../platform/hardware.h"
#include "../data/event_data.h"
#include <stddef.h>
#include <stdio.h>

/* ---- Engagement state machine ------------------------------------- */
typedef enum {
    TS_IDLE    = 0,
    TS_SPOTTED,   /* brief pause after spotting (mirrors EmotionBubble timing) */
    TS_WALKING,   /* trainer walks toward player */
    TS_TALKING,   /* before-battle text displayed */
    TS_READY,     /* battle ready — return 1 to caller */
} TrainerSightState;

static TrainerSightState ts_state   = TS_IDLE;
static int               ts_npc_idx = -1;   /* which NPC is the trainer */
static int               ts_map_trainer_idx = -1; /* index in map's trainer table */
static int               ts_timer   = 0;

/* Public: engaged trainer's class and party number */
uint8_t gEngagedTrainerClass = 0;
uint8_t gEngagedTrainerNo    = 0;

/* ---- Helpers ------------------------------------------------------ */

/* Check wEventFlags bit: return 1 if bit is set (trainer defeated). */
static int event_flag_get(uint16_t bit) {
    return (wEventFlags[bit / 8] >> (bit % 8)) & 1;
}

/* Set wEventFlags bit (mark trainer as defeated). */
static void event_flag_set(uint16_t bit) {
    wEventFlags[bit / 8] |= (uint8_t)(1u << (bit % 8));
}

/* ---- TrainerEngage / CheckSpriteCanSeePlayer / CheckPlayerIsInFrontOfSprite
 *
 * Returns 1 if trainer t can see the player and the player is in front of
 * the trainer (faithful port of the three functions above combined).
 *
 * Sight algorithm:
 *   1. Trainer and player must be on the same column (facing UP/DOWN) or
 *      same row (facing LEFT/RIGHT).
 *   2. Distance <= sight_dist blocks.
 *   3. Player must be in front of the trainer (facing direction check).
 */
static int trainer_can_see_player(int tx, int ty, int facing, int sight_dist) {
    int px = (int)wXCoord;
    int py = (int)wYCoord;
    int dx = px - tx;
    int dy = py - ty;
    int dist;

    if (facing == 0 || facing == 1) {
        /* UP or DOWN: must be same column */
        if (dx != 0) return 0;
        if (dy == 0) return 0;  /* same tile — never engage (pokered: jr z, .noEngage) */
        dist = (dy < 0 ? -dy : dy) >> 1;  /* divide by 2: 2 units = 1 block */
        if (dist > sight_dist) return 0;
        /* CheckPlayerIsInFrontOfSprite:
         *   DOWN (0): trainer_y < player_y  → ty < py → dy > 0 */
        if (facing == 0 && dy <= 0) return 0;
        /* UP   (1): trainer_y >= player_y → ty >= py → dy <= 0  (strict: dy < 0 since dy!=0) */
        if (facing == 1 && dy > 0) return 0;
    } else {
        /* LEFT or RIGHT: must be same row */
        if (dy != 0) return 0;
        if (dx == 0) return 0;
        dist = (dx < 0 ? -dx : dx) >> 1;
        if (dist > sight_dist) return 0;
        /* CheckPlayerIsInFrontOfSprite:
         *   LEFT  (2): trainer_x >= player_x → tx >= px → dx <= 0 (strict: dx < 0) */
        if (facing == 2 && dx > 0) return 0;
        /* RIGHT (3): trainer_x < player_x  → tx < px  → dx > 0 */
        if (facing == 3 && dx <= 0) return 0;
    }
    return 1;
}

/* ---- Public API --------------------------------------------------- */

void Trainer_LoadMap(void) {
    ts_state           = TS_IDLE;
    ts_npc_idx         = -1;
    ts_map_trainer_idx = -1;
    ts_timer           = 0;
    gEngagedTrainerClass = 0;
    gEngagedTrainerNo    = 0;

    if (wCurMap >= NUM_MAPS) return;
    const map_events_t *ev = &gMapEvents[wCurMap];
    if (!ev->trainers) return;

    /* Apply initial facing directions for trainer NPCs (from object_event facing
     * field in pokered map data — trainers have a fixed sight direction). */
    for (int i = 0; i < ev->num_trainers; i++) {
        const map_trainer_t *t = &ev->trainers[i];
        NPC_SetFacing(t->npc_idx, t->facing);
    }
}

void Trainer_CheckSight(void) {
    if (ts_state != TS_IDLE) return;  /* already engaging */

    if (wCurMap >= NUM_MAPS) return;
    const map_events_t *ev = &gMapEvents[wCurMap];
    if (!ev->trainers) return;

    for (int i = 0; i < ev->num_trainers; i++) {
        const map_trainer_t *t = &ev->trainers[i];

        /* Skip defeated trainers */
        if (event_flag_get(t->flag_bit)) continue;

        int tx, ty;
        NPC_GetTilePos(t->npc_idx, &tx, &ty);

        if (!trainer_can_see_player(tx, ty, t->facing, t->sight_dist)) continue;

        /* Trainer spotted the player — start engagement.
         * Mirrors: set BIT_SEEN_BY_TRAINER, show exclamation, suppress input. */
        ts_state           = TS_SPOTTED;
        ts_npc_idx         = t->npc_idx;
        ts_map_trainer_idx = i;
        ts_timer           = 30;  /* ~0.5 s pause (mirrors EmotionBubble timing) */
        gEngagedTrainerClass = t->trainer_class;
        gEngagedTrainerNo    = t->trainer_no;

        printf("[trainer] Trainer %d (class %d, no %d) spotted player at (%d,%d)\n",
               i, t->trainer_class, t->trainer_no, (int)wXCoord, (int)wYCoord);
        return;
    }
}

int Trainer_SightTick(void) {
    if (ts_state == TS_IDLE) return 0;

    if (wCurMap >= NUM_MAPS) { ts_state = TS_IDLE; return 0; }
    const map_events_t *ev = &gMapEvents[wCurMap];
    if (!ev->trainers || ts_map_trainer_idx < 0) { ts_state = TS_IDLE; return 0; }
    const map_trainer_t *t = &ev->trainers[ts_map_trainer_idx];

    switch (ts_state) {

    case TS_SPOTTED:
        /* Count down the "!" pause before trainer walks. */
        if (--ts_timer > 0) return 0;
        ts_state = TS_WALKING;
        return 0;

    case TS_WALKING: {
        /* Walk trainer toward player one step at a time (TrainerWalkUpToPlayer).
         * Stop when 1 tile adjacent (|diff| == 2 in our coord system). */
        if (NPC_IsWalking(ts_npc_idx)) return 0;

        int tx, ty;
        NPC_GetTilePos(ts_npc_idx, &tx, &ty);
        int px = (int)wXCoord;
        int py = (int)wYCoord;
        int dx = px - tx;
        int dy = py - ty;
        int facing = t->facing;

        /* Check adjacency: stop when 1 block apart (2 units) */
        int adj = (facing == 0 || facing == 1) ? (dy < 0 ? -dy : dy) : (dx < 0 ? -dx : dx);
        if (adj <= 2) {
            /* Adjacent — face player and transition to talking. */
            NPC_SetFacing(ts_npc_idx, facing);
            ts_state = TS_TALKING;
            return 0;
        }

        /* Step one block closer */
        NPC_DoScriptedStep(ts_npc_idx, facing);
        return 0;
    }

    case TS_TALKING:
        /* Show before-battle text once; wait for player to dismiss it.
         * ts_timer: 0 = text not yet shown, 1 = text shown (may still be open). */
        if (Text_IsOpen()) return 0;  /* Text_Update() handling it; wait */
        if (ts_timer == 0) {
            if (t->before_text)
                Text_ShowASCII(t->before_text);
            ts_timer = 1;   /* mark shown whether or not text was non-NULL */
            return 0;
        }
        /* ts_timer == 1 and !Text_IsOpen(): text was dismissed (or there was none) */
        ts_state = TS_READY;
        return 0;

    case TS_READY:
        ts_state = TS_IDLE;
        return 1;  /* signal: start battle */

    default:
        ts_state = TS_IDLE;
        return 0;
    }
}

int Trainer_IsEngaging(void) {
    return ts_state != TS_IDLE;
}

void Trainer_MarkCurrentDefeated(void) {
    if (wCurMap >= NUM_MAPS) return;
    const map_events_t *ev = &gMapEvents[wCurMap];
    if (!ev->trainers || ts_map_trainer_idx < 0 ||
        ts_map_trainer_idx >= ev->num_trainers) return;
    event_flag_set(ev->trainers[ts_map_trainer_idx].flag_bit);
    ts_map_trainer_idx = -1;
}
