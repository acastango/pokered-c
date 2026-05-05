#pragma once
/* move_anim.h — Move animation runtime (ASM-mirrored scaffold).
 *
 * Source of truth:
 *   pokered-master/engine/battle/animations.asm
 *   pokered-master/data/moves/animations.asm
 */

#include <stdint.h>

typedef struct {
    uint8_t cmd0;  /* subanim header or SE id */
    uint8_t cmd1;  /* sound id (NO_MOVE-1 means none) */
    uint8_t cmd2;  /* subanim id when cmd0 < FIRST_SE_ID */
} move_anim_cmd_t;

typedef struct {
    uint8_t frameblock_id;
    uint8_t basecoord_id;
    uint8_t mode;
} subanim_entry_t;

typedef struct {
    uint8_t type;  /* SUBANIMTYPE_* */
    uint8_t count;
    const subanim_entry_t *entries;
} subanim_def_t;

typedef struct {
    uint8_t animation_id;
    uint8_t animation_type;
    uint8_t anim_sound_id;
    uint8_t which_tileset;
    uint8_t subanim_frame_delay;
    uint8_t subanim_counter;
    uint8_t subanim_postdraw_pending;
    uint8_t subanim_transform;
    uint16_t subanim_entry_index;
    uint8_t fb_mode;
    uint8_t base_x;
    uint8_t base_y;
    uint16_t fb_dest_oam_index;
    uint16_t script_index;
    uint8_t wait_frames;
    uint16_t timing_frac_1e4;
    uint8_t runtime_state;
    uint8_t active_subanim;
    uint8_t active_special_effect;
    uint8_t active_se_id;
    uint8_t se_phase;
    uint8_t se_index;
    uint8_t se_counter0;
    uint8_t pending_oam_clean;
    uint8_t script_done;
} move_anim_ctx_t;

/* MoveAnim_Run — top-level MoveAnimation runtime entrypoint scaffold. */
void MoveAnim_Run(move_anim_ctx_t *ctx);
void MoveAnim_Begin(move_anim_ctx_t *ctx);
int MoveAnim_Tick(move_anim_ctx_t *ctx);
int MoveAnim_IsDone(const move_anim_ctx_t *ctx);
