/* move_anim.c - Move animation runtime.
 *
 * Phase C implements the ASM-mirrored command interpreter core from
 * engine/battle/animations.asm (PlayAnimation + ShareMoveAnimations path).
 */

#include <string.h>
#include <stdio.h>
#include "move_anim.h"
#include "../constants.h"
#include "battle_ui.h"
#include "../../platform/hardware.h"
#include "../../platform/display.h"
#include "../../platform/audio.h"
#include "../../data/move_anim_scripts.h"
#include "../../data/move_anim_subanims.h"
#include "../../data/move_anim_frameblocks.h"
#include "../../data/move_anim_basecoords.h"
#include "../../data/move_anim_tiles.h"
#include "../../data/move_sfx_data.h"
#include "../../data/base_stats.h"
#include "../../data/pokemon_sprites.h"
#include "../../data/font_data.h"
#include "../overworld.h"

#define MOVE_AMNESIA_ID           0x85u
#define MOVE_ANIM_NO_MOVE_MINUS_ONE 0xFFu
#define MOVE_ANIM_SLP_ANIM_ID       189u
#define MOVE_ANIM_CONF_ANIM_ID      191u
#define MOVE_ANIM_TILE_BASE_ID      0x31u
#define MOVE_ANIM_HVFLIP_BASE_Y     136u
#define MOVE_ANIM_HVFLIP_BASE_X     168u

#define MOVE_ANIM_SUBANIMTYPE_NORMAL    0u
#define MOVE_ANIM_SUBANIMTYPE_HVFLIP    1u
#define MOVE_ANIM_SUBANIMTYPE_HFLIP     2u
#define MOVE_ANIM_SUBANIMTYPE_COORDFLIP 3u
#define MOVE_ANIM_SUBANIMTYPE_REVERSE   4u
#define MOVE_ANIM_SUBANIMTYPE_ENEMY     5u

#define MOVE_ANIM_FRAMEBLOCKMODE_02 2u
#define MOVE_ANIM_FRAMEBLOCKMODE_03 3u
#define MOVE_ANIM_FRAMEBLOCKMODE_04 4u
#define MOVE_ANIM_ENEMY_SPR_TILE_BASE 0u
#define MOVE_ANIM_PLAYER_BG_TILE_BASE 53u
#define MOVE_ANIM_PLAYER_BG_COL       1u
#define MOVE_ANIM_PLAYER_BG_ROW       5u

/* Move IDs used by AnimationIdSpecialEffects table in ASM. */
#define MOVE_MEGA_PUNCH_ID        0x05u
#define MOVE_GUILLOTINE_ID        0x0Cu
#define MOVE_MEGA_KICK_ID         0x19u
#define MOVE_HEADBUTT_ID          0x1Du
#define MOVE_DISABLE_ID           0x32u
#define MOVE_HYPER_BEAM_ID        0x3Fu
#define MOVE_REFLECT_ID           0x73u
#define MOVE_SELFDESTRUCT_ID      0x78u
#define MOVE_SPORE_ID             0x93u
#define MOVE_EXPLOSION_ID         0x99u
#define MOVE_ROCK_SLIDE_ID        0x9Du

/* runtime_state values */
#define MOVE_ANIM_RT_UNINITIALIZED 0u
#define MOVE_ANIM_RT_RUNNING       1u
#define MOVE_ANIM_RT_DONE          2u
static int MoveAnim_PlayAnimationStep(move_anim_ctx_t *ctx);
static int MoveAnim_PlaySubanimationStep(move_anim_ctx_t *ctx);
static void MoveAnim_LoadSubanimation(move_anim_ctx_t *ctx, uint8_t subanim_id);
static void MoveAnim_DrawFrameBlock(move_anim_ctx_t *ctx, uint8_t frameblock_id);
static void MoveAnim_LoadTileset(move_anim_ctx_t *ctx, uint8_t tileset_id);
static void MoveAnim_DoSpecialEffectByAnimationId(move_anim_ctx_t *ctx);
static void MoveAnim_PlayApplyingAttackAnimation(move_anim_ctx_t *ctx);
static void MoveAnim_SetAnimationPalette(move_anim_ctx_t *ctx);
static void MoveAnim_ShareMoveAnimations(move_anim_ctx_t *ctx);
static void MoveAnim_RunSpecialEffect(move_anim_ctx_t *ctx, uint8_t se_id);
static int MoveAnim_RunSpecialEffectStep(move_anim_ctx_t *ctx);
static void MoveAnim_PlayCommandSound(move_anim_ctx_t *ctx, uint8_t sound_id_minus_one);
static void MoveAnim_AnimationFlashScreen(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationDarkScreenPalette(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationResetScreenPalette(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationShakeScreen(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationWaterDropletsEverywhere(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationDarkenMonPalette(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationFlashScreenLong(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationSlideMonUp(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationSlideMonDown(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationFlashMonPic(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationSlideMonOff(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationBlinkMon(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationMoveMonHorizontally(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationResetMonPosition(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationLightScreenPalette(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationHideMonPic(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationSquishMonPic(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationShootBallsUpward(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationShootManyBallsUpward(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationBoundUpAndDown(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationMinimizeMon(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationSlideMonDownAndHide(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationTransformMon(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationLeavesFalling(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationPetalsFalling(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationSlideMonHalfOff(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationShakeEnemyHUD(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationSpiralBallsInward(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationDelay10(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationFlashEnemyMonPic(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationHideEnemyMonPic(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationBlinkEnemyMon(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationShowMonPic(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationShowEnemyMonPic(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationSlideEnemyMonOff(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationShakeBackAndForth(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationSubstitute(move_anim_ctx_t *ctx);
static void MoveAnim_AnimationWavyScreen(move_anim_ctx_t *ctx);
static uint8_t MoveAnim_GetSubanimationTransform1(uint8_t subanim_type);
static uint8_t MoveAnim_GetSubanimationTransform2(void);
static uint8_t MoveAnim_HVFlipFlags(uint8_t flags);
static uint8_t MoveAnim_HFlipFlags(uint8_t flags);
static uint8_t MoveAnim_GetFrameBlockYOffset(const move_anim_frameblock_sprite_t *sprite);
static uint8_t MoveAnim_GetFrameBlockXOffset(const move_anim_frameblock_sprite_t *sprite);
static void MoveAnim_DelayFramesAsm(uint8_t frames);
static uint8_t MoveAnim_ConvertAsmFramesToEngineTicks(move_anim_ctx_t *ctx, uint8_t asm_frames);
static void MoveAnim_AnimationCleanOAM(void);
static void MoveAnim_ClearAnimationOAM(void);
static void MoveAnim_SetBGP(uint8_t bgp);
static void MoveAnim_Finalize(move_anim_ctx_t *ctx);
static void MoveAnim_ResetVisualState(void);
static void MoveAnim_ResetEnemyOAMPoseCanonical(void);
static void MoveAnim_SetEnemyVisible(uint8_t visible);
static void MoveAnim_OffsetEnemyY(int8_t delta);
static void MoveAnim_UpdatePerFrameEffects(move_anim_ctx_t *ctx);
static void MoveAnim_LoadEnemyFrontSprite(uint8_t species);
static void MoveAnim_LoadPlayerBackSprite(uint8_t species);
static void MoveAnim_HidePlayerBackSprite(void);
static void MoveAnim_ClearPlayerBackSpriteAt(uint8_t col, uint8_t row);
static void MoveAnim_PlacePlayerBackSpriteAt(uint8_t col, uint8_t row);
static int MoveAnim_SE_FlashScreenLongStep(move_anim_ctx_t *ctx);
static int MoveAnim_SE_SpiralBallsInwardStep(move_anim_ctx_t *ctx);
static int MoveAnim_SE_WaterDropletsEverywhereStep(move_anim_ctx_t *ctx);
static int MoveAnim_SE_SlideMonUpStep(move_anim_ctx_t *ctx);
static int MoveAnim_SE_SlideMonDownStep(move_anim_ctx_t *ctx);
static int MoveAnim_SE_ShootBallsUpwardStep(move_anim_ctx_t *ctx);
static int MoveAnim_SE_ShootManyBallsUpwardStep(move_anim_ctx_t *ctx);
static int MoveAnim_SE_ShakeEnemyHUDStep(move_anim_ctx_t *ctx);
static int MoveAnim_SE_LeavesFallingStep(move_anim_ctx_t *ctx);
static int MoveAnim_SE_PetalsFallingStep(move_anim_ctx_t *ctx);
static int MoveAnim_SE_SquishMonPicStep(move_anim_ctx_t *ctx);
static int MoveAnim_SE_WavyScreenStep(move_anim_ctx_t *ctx);
static int MoveAnim_SE_SlideMonDownAndHideStep(move_anim_ctx_t *ctx);
static uint8_t MoveAnim_IsCryMove(uint8_t animation_id);

static const subanim_def_t *sMoveAnimLoadedSubanimation;
static uint8_t sMoveAnimCurrentBGP = 0xE4u;
static uint8_t sMoveAnimLoadedTileset = 0xFFu;
static move_anim_ctx_t *sMoveAnimExecCtx = 0;
static int8_t sMoveAnimShakeX = 0;
static int8_t sMoveAnimShakeY = 0;
static uint8_t sMoveAnimShakeToggle = 0;
static uint8_t sMoveAnimWavyActive = 0;
static uint8_t sMoveAnimWavyPhase = 0;
static const uint8_t sMoveAnimFlashScreenLongPals[12] = {
    /* ASM FlashScreenLongMonochrome (dc macro packs as:
     * (\1 << 6) | (\2 << 4) | (\3 << 2) | \4). */
    0xF9u, 0xFCu, 0xFFu, 0xFCu, 0xF9u, 0xE4u,
    0x90u, 0x40u, 0x00u, 0x40u, 0x90u, 0xE4u
};
static const uint8_t sMoveAnimSpiralCoords[] = {
    0x38u, 0x28u, 0x40u, 0x18u, 0x50u, 0x10u, 0x60u, 0x18u, 0x68u, 0x28u,
    0x60u, 0x38u, 0x50u, 0x40u, 0x40u, 0x38u, 0x40u, 0x28u, 0x46u, 0x1Eu,
    0x50u, 0x18u, 0x5Bu, 0x1Eu, 0x60u, 0x28u, 0x5Bu, 0x32u, 0x50u, 0x38u,
    0x46u, 0x32u, 0x48u, 0x28u, 0x50u, 0x20u, 0x58u, 0x28u, 0x50u, 0x30u,
    0x50u, 0x28u, 0xFFu
};
static const uint8_t sMoveAnimUpwardBallsXPlayerTurn[] = {0x10u, 0x40u, 0x28u, 0x18u, 0x38u, 0x30u, 0xFFu};
static const uint8_t sMoveAnimUpwardBallsXEnemyTurn[] = {0x60u, 0x90u, 0x78u, 0x68u, 0x88u, 0x80u, 0xFFu};
static const uint8_t sMoveAnimFallingDeltaX[9] = {0u, 1u, 3u, 5u, 7u, 9u, 11u, 13u, 15u};
static const uint8_t sMoveAnimFallingInitialX[20] = {
    0x38u, 0x40u, 0x50u, 0x60u, 0x70u, 0x88u, 0x90u, 0x56u, 0x67u, 0x4Au,
    0x77u, 0x84u, 0x98u, 0x32u, 0x22u, 0x5Cu, 0x6Cu, 0x7Du, 0x8Eu, 0x99u
};
static const uint8_t sMoveAnimFallingInitialMove[20] = {
    0x00u, 0x84u, 0x06u, 0x81u, 0x02u, 0x88u, 0x01u, 0x83u, 0x05u, 0x89u,
    0x09u, 0x80u, 0x07u, 0x87u, 0x03u, 0x82u, 0x04u, 0x85u, 0x08u, 0x86u
};
static uint8_t sMoveAnimWaterBaseX = 0u;
static uint8_t sMoveAnimShootBaseY = 0u;
static uint8_t sMoveAnimShootBaseX = 0u;
static uint8_t sMoveAnimShootBallCount = 0u;
static uint8_t sMoveAnimShootDelay = 0u;
static uint8_t sMoveAnimFallingCount = 0u;
static uint8_t sMoveAnimFallingTile = 0u;
static uint8_t sMoveAnimFallingMove[20];

void MoveAnim_Run(move_anim_ctx_t *ctx) {
    uint16_t guard = 0;
    MoveAnim_Begin(ctx);
    while (!MoveAnim_IsDone(ctx) && guard < 8192u) {
        if (MoveAnim_Tick(ctx)) {
            break;
        }
        guard++;
    }
    if (ctx && !MoveAnim_IsDone(ctx)) {
        MoveAnim_Finalize(ctx);
    }
}

void MoveAnim_Begin(move_anim_ctx_t *ctx) {
    if (!ctx) return;

    ctx->script_index = 0;
    ctx->wait_frames = 0;
    ctx->timing_frac_1e4 = 0;
    ctx->active_subanim = 0;
    ctx->runtime_state = MOVE_ANIM_RT_RUNNING;
    ctx->subanim_entry_index = 0;
    ctx->subanim_transform = 0;
    ctx->subanim_postdraw_pending = 0;
    ctx->anim_sound_id = MOVE_ANIM_NO_MOVE_MINUS_ONE;
    ctx->active_special_effect = 0;
    ctx->active_se_id = 0;
    ctx->se_phase = 0;
    ctx->se_index = 0;
    ctx->se_counter0 = 0;
    ctx->pending_oam_clean = 0;
    ctx->script_done = 0;
    sMoveAnimLoadedSubanimation = 0;
    MoveAnim_ResetVisualState();

    MoveAnim_SetAnimationPalette(ctx);
    if (ctx->animation_id == 0) {
        MoveAnim_Finalize(ctx);
        return;
    }
    MoveAnim_ShareMoveAnimations(ctx);
}

int MoveAnim_Tick(move_anim_ctx_t *ctx) {
    if (!ctx) return 1;
    if (ctx->runtime_state == MOVE_ANIM_RT_DONE) return 1;
    if (ctx->runtime_state != MOVE_ANIM_RT_RUNNING) return 1;

    if (ctx->wait_frames != 0u) {
        MoveAnim_UpdatePerFrameEffects(ctx);
        ctx->wait_frames = (uint8_t)(ctx->wait_frames - 1u);
        if (ctx->wait_frames != 0u) {
            return 0;
        }
        if (ctx->pending_oam_clean != 0u) {
            if (ctx->pending_oam_clean == 1u) {
                /* ASM AnimationCleanOAM does DelayFrame then ClearSprites.
                 * Mirror that extra frame so frameblock cadence matches ROM. */
                ctx->pending_oam_clean = 2u;
                ctx->wait_frames = MoveAnim_ConvertAsmFramesToEngineTicks(ctx, 1u);
                if (ctx->wait_frames == 0u) {
                    ctx->wait_frames = 1u;
                }
                return 0;
            }
            MoveAnim_ClearAnimationOAM();
            ctx->pending_oam_clean = 0u;
            /* Keep the cleared OAM state visible for at least one presented
             * frame before advancing script commands. Otherwise clear+redraw can
             * collapse into one host tick and remove the intended blink cadence. */
            return 0;
        }
        /* Phase-2 cadence parity: once DelayFrames has consumed its final frame,
         * yield this tick so subsequent draw/command work starts on the next
         * frame boundary instead of collapsing into the same host tick. */
        return 0;
    }

    if (ctx->script_done) {
        MoveAnim_PlayApplyingAttackAnimation(ctx);
        MoveAnim_Finalize(ctx);
        return 1;
    }

    sMoveAnimExecCtx = ctx;

    /* Temporarily force battle animations on.
     * The options UI path for toggling BIT_BATTLE_ANIMATION is not fully
     * wired in the current port state, and persisted saves can otherwise
     * suppress all scripted move animation playback. */

    if (MoveAnim_PlayAnimationStep(ctx)) {
        ctx->script_done = 1u;
        sMoveAnimExecCtx = 0;
        if (ctx->wait_frames != 0u || ctx->pending_oam_clean != 0u) {
            return 0;
        }
        MoveAnim_PlayApplyingAttackAnimation(ctx);
        MoveAnim_Finalize(ctx);
        return 1;
    }

    sMoveAnimExecCtx = 0;
    return 0;
}

int MoveAnim_IsDone(const move_anim_ctx_t *ctx) {
    if (!ctx) return 1;
    return ctx->runtime_state == MOVE_ANIM_RT_DONE;
}

static void MoveAnim_Finalize(move_anim_ctx_t *ctx) {
    uint8_t keep_charge_hide = 0u;
    uint8_t player_should_hide =
        (uint8_t)((wPlayerBattleStatus1 & ((1u << BSTAT1_CHARGING_UP) |
                                           (1u << BSTAT1_INVULNERABLE))) != 0u);
    if (!ctx) return;

    if (ctx->pending_oam_clean) {
        MoveAnim_ClearAnimationOAM();
        ctx->pending_oam_clean = 0u;
    }

    /* Mirrors MoveAnimation cleanup of core state bytes. */
    ctx->subanim_entry_index = 0;
    ctx->subanim_transform = 0;
    ctx->subanim_postdraw_pending = 0;
    ctx->anim_sound_id = MOVE_ANIM_NO_MOVE_MINUS_ONE;
    ctx->active_subanim = 0;
    ctx->active_special_effect = 0;
    ctx->active_se_id = 0;
    ctx->se_phase = 0;
    ctx->se_index = 0;
    ctx->se_counter0 = 0;
    ctx->pending_oam_clean = 0;
    ctx->script_done = 0;
    ctx->wait_frames = 0;
    ctx->timing_frac_1e4 = 0;
    ctx->runtime_state = MOVE_ANIM_RT_DONE;
    sMoveAnimLoadedSubanimation = 0;

    /* Charge-turn hide animations (Dig/Fly) must remain hidden until the
     * next turn's execution path clears CHARGING_UP/INVULNERABLE and renders
     * the attacker again.  Do not canonical-restore visuals here. */
    if ((ctx->animation_id == 192u || ctx->animation_id == MOVE_TELEPORT)) {
        if ((hWhoseTurn == 0u && (wPlayerBattleStatus1 & (1u << BSTAT1_CHARGING_UP))) ||
            (hWhoseTurn != 0u && (wEnemyBattleStatus1 & (1u << BSTAT1_CHARGING_UP)))) {
            keep_charge_hide = 1u;
        }
    }
    if (keep_charge_hide) {
        printf("[DIGDBG] finalize keep_hide anim=%u turn=%u p_b1=0x%02X e_b1=0x%02X\n",
               ctx->animation_id, hWhoseTurn, wPlayerBattleStatus1, wEnemyBattleStatus1);
        sMoveAnimShakeX = 0;
        sMoveAnimShakeY = 0;
        sMoveAnimShakeToggle = 0;
        sMoveAnimWavyActive = 0u;
        sMoveAnimWavyPhase = 0u;
        Display_SetWavyPhase(0, 0);
        Display_SetShakeOffset(0, 0);
        return;
    }

    MoveAnim_ResetVisualState();

    /* Player-side mon graphics are BG-tile based in this port.
     * Keep charge/invulnerable hide parity (Dig/Fly) even when another move
     * animation finalizes while the attacker remains underground/invisible. */
    if (player_should_hide) {
        printf("[DIGDBG] finalize hide_player anim=%u turn=%u p_b1=0x%02X e_b1=0x%02X\n",
               ctx->animation_id, hWhoseTurn, wPlayerBattleStatus1, wEnemyBattleStatus1);
        MoveAnim_HidePlayerBackSprite();
    } else {
        MoveAnim_LoadPlayerBackSprite(wBattleMon.species);
    }
}

static int MoveAnim_PlayAnimationStep(move_anim_ctx_t *ctx) {
    const uint8_t *script;

    if (!ctx) return 1;
    if (ctx->animation_id == 0) return 1;
    if (ctx->animation_id > MOVE_ANIM_NUM_ATTACK_ANIMS) return 1;

    script = gMoveAnimAttackAnimationPointers[(uint16_t)ctx->animation_id - 1u];
    if (!script) return 1;

    if (ctx->active_subanim) {
        if (!MoveAnim_PlaySubanimationStep(ctx)) {
            return 0;
        }
        ctx->active_subanim = 0;
        /* If the finishing subanim step queued DelayFrames / OAM clean,
         * yield now (ASM-style) before consuming another command. */
        if (ctx->wait_frames != 0u || ctx->pending_oam_clean != 0u) {
            return 0;
        }
    }
    if (ctx->active_special_effect) {
        if (!MoveAnim_RunSpecialEffectStep(ctx)) {
            return 0;
        }
        ctx->active_special_effect = 0;
        if (ctx->wait_frames != 0u || ctx->pending_oam_clean != 0u) {
            return 0;
        }
    }

    while (script[ctx->script_index] != 0xFFu) {
        uint8_t cmd0 = script[ctx->script_index++];

        if (cmd0 < MOVE_ANIM_FIRST_SE_ID) {
            uint8_t sound_id;
            uint8_t subanim_id;

            /* Subanimation command: cmd0, sound, subanim id. */
            sound_id = script[ctx->script_index++];
            subanim_id = script[ctx->script_index++];

            ctx->subanim_frame_delay = (uint8_t)(cmd0 & 0x3Fu);
            ctx->which_tileset = (uint8_t)(cmd0 >> 6);

            MoveAnim_PlayCommandSound(ctx, sound_id);
            MoveAnim_LoadTileset(ctx, ctx->which_tileset);
            MoveAnim_LoadSubanimation(ctx, subanim_id);
            ctx->active_subanim = 1;
            if (!MoveAnim_PlaySubanimationStep(ctx)) {
                return 0;
            }
            ctx->active_subanim = 0;
            if (ctx->wait_frames != 0u || ctx->pending_oam_clean != 0u) {
                return 0;
            }
            continue;
        }

        /* Special effect command: se id, sound. */
        MoveAnim_PlayCommandSound(ctx, script[ctx->script_index++]);
        ctx->active_special_effect = 1u;
        ctx->active_se_id = cmd0;
        ctx->se_phase = 0u;
        ctx->se_index = 0u;
        ctx->se_counter0 = 0u;
        if (!MoveAnim_RunSpecialEffectStep(ctx)) {
            return 0;
        }
        ctx->active_special_effect = 0u;
        if (ctx->wait_frames != 0u || ctx->pending_oam_clean != 0u) {
            return 0;
        }
    }

    /* ASM parity: PlayAnimation waits for active SFX to finish before
     * returning (AnimationFinished -> WaitForSoundToFinish). */
    if (Audio_IsSFXPlaying()) {
        return 0;
    }
    return 1;
}

static int MoveAnim_RunSpecialEffectStep(move_anim_ctx_t *ctx) {
    if (!ctx) return 1;

    switch (ctx->active_se_id) {
        case 0xFA:
            return MoveAnim_SE_WaterDropletsEverywhereStep(ctx);
        case 0xF8:
            return MoveAnim_SE_FlashScreenLongStep(ctx);
        case 0xF7:
            return MoveAnim_SE_SlideMonUpStep(ctx);
        case 0xF6:
            return MoveAnim_SE_SlideMonDownStep(ctx);
        case 0xEE:
            return MoveAnim_SE_SquishMonPicStep(ctx);
        case 0xED:
            return MoveAnim_SE_ShootBallsUpwardStep(ctx);
        case 0xEC:
            return MoveAnim_SE_ShootManyBallsUpwardStep(ctx);
        case 0xE4:
        case 0xE3:
            return MoveAnim_SE_ShakeEnemyHUDStep(ctx);
        case 0xE7:
            return MoveAnim_SE_LeavesFallingStep(ctx);
        case 0xE6:
            return MoveAnim_SE_PetalsFallingStep(ctx);
        case 0xE9:
            return MoveAnim_SE_SlideMonDownAndHideStep(ctx);
        case 0xE2:
            return MoveAnim_SE_SpiralBallsInwardStep(ctx);
        case 0xD8:
            return MoveAnim_SE_WavyScreenStep(ctx);
        default:
            MoveAnim_RunSpecialEffect(ctx, ctx->active_se_id);
            return 1;
    }
}

static int MoveAnim_PlaySubanimationStep(move_anim_ctx_t *ctx) {
    const subanim_entry_t *entry;
    const move_anim_basecoord_t *basecoord;

    if (!ctx) return 1;
    if (!sMoveAnimLoadedSubanimation) return 1;

    /* ASM PlaySubanimation optionally plays wAnimSoundID here, before looping.
     * The concrete GetMoveSound/PlaySound hookup lands in later phases. */
    if (ctx->anim_sound_id != MOVE_ANIM_NO_MOVE_MINUS_ONE) {
        /* no-op in Phase D */
    }

    if (ctx->subanim_counter != 0u) {
        if (ctx->subanim_entry_index >= sMoveAnimLoadedSubanimation->count) return 1;

        entry = &sMoveAnimLoadedSubanimation->entries[ctx->subanim_entry_index];
        if (!ctx->subanim_postdraw_pending) {
            if (entry->basecoord_id >= MOVE_ANIM_NUM_BASECOORDS) return 1;

            basecoord = &gMoveAnimFrameBlockBaseCoords[entry->basecoord_id];
            ctx->base_y = basecoord->y;
            ctx->base_x = basecoord->x;
            ctx->fb_mode = entry->mode;

            MoveAnim_DrawFrameBlock(ctx, entry->frameblock_id);
            ctx->subanim_postdraw_pending = 1u;
            if (ctx->wait_frames != 0u || ctx->pending_oam_clean != 0u) {
                return 0;
            }
        }

        /* ASM ordering: DoSpecialEffectByAnimationId runs only after
         * DrawFrameBlock (including its DelayFrames/CleanOAM path) returns. */
        MoveAnim_DoSpecialEffectByAnimationId(ctx);
        ctx->subanim_postdraw_pending = 0u;

        /* Mirrors: dec wSubAnimCounter; ret z */
        ctx->subanim_counter = (uint8_t)(ctx->subanim_counter - 1u);
        if (ctx->subanim_counter == 0u) return 1;

        /* Mirrors reverse stepping branch in PlaySubanimation. */
        if (ctx->subanim_transform == MOVE_ANIM_SUBANIMTYPE_REVERSE) {
            ctx->subanim_entry_index = (uint16_t)(ctx->subanim_entry_index - 1u);
        } else {
            ctx->subanim_entry_index = (uint16_t)(ctx->subanim_entry_index + 1u);
        }

        return 0;
    }

    return 1;
}

static void MoveAnim_LoadSubanimation(move_anim_ctx_t *ctx, uint8_t subanim_id) {
    const subanim_def_t *def;
    uint8_t transform;

    if (!ctx) return;
    if (subanim_id >= MOVE_ANIM_NUM_SUBANIMS) {
        sMoveAnimLoadedSubanimation = 0;
        ctx->subanim_counter = 0;
        return;
    }

    def = gMoveAnimSubanimationPointers[subanim_id];
    if (!def) {
        sMoveAnimLoadedSubanimation = 0;
        ctx->subanim_counter = 0;
        return;
    }

    sMoveAnimLoadedSubanimation = def;
    ctx->subanim_counter = def->count;
    ctx->subanim_postdraw_pending = 0u;
    /* ASM PlaySubanimation initializes wFBDestAddr once at subanim start. */
    ctx->fb_dest_oam_index = 0;

    if (def->type == MOVE_ANIM_SUBANIMTYPE_ENEMY) {
        transform = MoveAnim_GetSubanimationTransform2();
    } else {
        transform = MoveAnim_GetSubanimationTransform1(def->type);
    }
    ctx->subanim_transform = transform;

    /* Reverse subanimations start from the last entry. */
    if (transform == MOVE_ANIM_SUBANIMTYPE_REVERSE && ctx->subanim_counter != 0u) {
        ctx->subanim_entry_index = (uint16_t)(ctx->subanim_counter - 1u);
    } else {
        ctx->subanim_entry_index = 0;
    }
}

static void MoveAnim_DrawFrameBlock(move_anim_ctx_t *ctx, uint8_t frameblock_id) {
    const move_anim_frameblock_def_t *frameblock;
    uint16_t dest_oam_index;
    uint8_t i;

    if (!ctx) return;
    if (frameblock_id >= MOVE_ANIM_NUM_FRAMEBLOCKS) return;

    frameblock = gMoveAnimFrameBlockPointers[frameblock_id];
    if (!frameblock) return;

    dest_oam_index = ctx->fb_dest_oam_index;
    for (i = 0; i < frameblock->count; i++) {
        const move_anim_frameblock_sprite_t *sprite = &frameblock->sprites[i];
        uint8_t y_offset = MoveAnim_GetFrameBlockYOffset(sprite);
        uint8_t x_offset = MoveAnim_GetFrameBlockXOffset(sprite);
        uint8_t y_coord;
        uint8_t x_coord;
        uint8_t flags;
        uint8_t tile = (uint8_t)(sprite->tile_id + MOVE_ANIM_TILE_BASE_ID);

        switch (ctx->subanim_transform) {
            case MOVE_ANIM_SUBANIMTYPE_HVFLIP: {
                uint8_t y_sum = (uint8_t)(ctx->base_y + y_offset);
                uint8_t x_sum = (uint8_t)(ctx->base_x + x_offset);
                y_coord = (uint8_t)(MOVE_ANIM_HVFLIP_BASE_Y - y_sum);
                x_coord = (uint8_t)(MOVE_ANIM_HVFLIP_BASE_X - x_sum);
                flags = MoveAnim_HVFlipFlags(sprite->flags);
                break;
            }
            case MOVE_ANIM_SUBANIMTYPE_HFLIP: {
                uint8_t x_sum = (uint8_t)(ctx->base_x + x_offset);
                y_coord = (uint8_t)(ctx->base_y + y_offset + 40u);
                x_coord = (uint8_t)(MOVE_ANIM_HVFLIP_BASE_X - x_sum);
                flags = MoveAnim_HFlipFlags(sprite->flags);
                break;
            }
            case MOVE_ANIM_SUBANIMTYPE_COORDFLIP:
                y_coord = (uint8_t)((uint8_t)(MOVE_ANIM_HVFLIP_BASE_Y - ctx->base_y) + y_offset);
                x_coord = (uint8_t)((uint8_t)(MOVE_ANIM_HVFLIP_BASE_X - ctx->base_x) + x_offset);
                flags = sprite->flags;
                break;
            default:
                y_coord = (uint8_t)(ctx->base_y + y_offset);
                x_coord = (uint8_t)(ctx->base_x + x_offset);
                flags = sprite->flags;
                break;
        }

        if (dest_oam_index < MAX_SPRITES) {
            wShadowOAM[dest_oam_index].y = y_coord;
            wShadowOAM[dest_oam_index].x = x_coord;
            wShadowOAM[dest_oam_index].tile = tile;
            wShadowOAM[dest_oam_index].flags = flags;
        }
        dest_oam_index++;
    }

    if (ctx->fb_mode == MOVE_ANIM_FRAMEBLOCKMODE_02) {
        ctx->fb_dest_oam_index = dest_oam_index;
        return;
    }

    MoveAnim_DelayFramesAsm(ctx->subanim_frame_delay);
    if (ctx->fb_mode == MOVE_ANIM_FRAMEBLOCKMODE_03) {
        ctx->fb_dest_oam_index = dest_oam_index;
        return;
    }
    if (ctx->fb_mode == MOVE_ANIM_FRAMEBLOCKMODE_04) return;

    if (ctx->animation_id != MOVE_GROWL) {
        /* ASM DrawFrameBlock waits first, then clears OAM.
         * In ticked runtime this must be deferred until wait reaches zero. */
        ctx->pending_oam_clean = 1u;
    }
    ctx->fb_dest_oam_index = 0;
}

static void MoveAnim_LoadTileset(move_anim_ctx_t *ctx, uint8_t tileset_id) {
    const uint8_t (*tiles)[16] = 0;
    uint8_t tile_count = 0;
    uint8_t i;

    if (!ctx) return;
    if (tileset_id > 2u) tileset_id = 0u;

    switch (tileset_id) {
        case 0:
            tiles = gMoveAnimTileset0;
            tile_count = MOVE_ANIM_TILESET0_TILES;
            break;
        case 1:
            tiles = gMoveAnimTileset1;
            tile_count = MOVE_ANIM_TILESET1_TILES;
            break;
        default:
            /* ASM tileset2 points at MoveAnimationTiles0 and uses only 64 tiles. */
            tiles = gMoveAnimTileset0;
            tile_count = MOVE_ANIM_TILESET2_TILES;
            break;
    }

    for (i = 0; i < tile_count; i++) {
        Display_LoadSpriteTile((uint8_t)(MOVE_ANIM_TILE_BASE_ID + i), tiles[i]);
    }

    sMoveAnimLoadedTileset = tileset_id;
}

static void MoveAnim_DoSpecialEffectByAnimationId(move_anim_ctx_t *ctx) {
    if (!ctx) return;

    /* Mirrors AnimationIdSpecialEffects table dispatch (subset translated). */
    switch (ctx->animation_id) {
        case MOVE_MEGA_PUNCH_ID:
        case MOVE_GUILLOTINE_ID:
        case MOVE_MEGA_KICK_ID:
        case MOVE_HEADBUTT_ID:
        case MOVE_DISABLE_ID:
        case MOVE_BUBBLEBEAM:
        case MOVE_REFLECT_ID:
        case MOVE_SPORE_ID:
            MoveAnim_AnimationFlashScreen(ctx);
            return;

        case MOVE_HYPER_BEAM_ID:
            if ((ctx->subanim_counter & 3u) == 0u) {
                MoveAnim_AnimationFlashScreen(ctx);
            }
            return;

        case MOVE_THUNDERBOLT:
            if ((ctx->subanim_counter & 7u) == 0u) {
                MoveAnim_AnimationFlashScreen(ctx);
            }
            return;

        case MOVE_BLIZZARD:
            if (ctx->subanim_counter == 13u || ctx->subanim_counter == 9u ||
                ctx->subanim_counter == 5u || ctx->subanim_counter == 1u) {
                MoveAnim_AnimationFlashScreen(ctx);
            }
            return;

        case MOVE_SELFDESTRUCT_ID:
        case MOVE_EXPLOSION_ID:
            if (ctx->subanim_counter == 1u) {
                MoveAnim_AnimationHideMonPic(ctx);
            } else if ((ctx->subanim_counter & 3u) == 0u) {
                MoveAnim_AnimationFlashScreen(ctx);
            }
            return;

        case MOVE_ROCK_SLIDE_ID:
            if (ctx->subanim_counter >= 12u) {
                return;
            }
            if (ctx->subanim_counter >= 8u) {
                sMoveAnimShakeX = 1;
                Display_SetShakeOffset(sMoveAnimShakeX, sMoveAnimShakeY);
                MoveAnim_DelayFramesAsm(1u);
                sMoveAnimShakeX = 0;
                sMoveAnimShakeY = 1;
                Display_SetShakeOffset(sMoveAnimShakeX, sMoveAnimShakeY);
                MoveAnim_DelayFramesAsm(1u);
                sMoveAnimShakeY = 0;
                Display_SetShakeOffset(sMoveAnimShakeX, sMoveAnimShakeY);
            } else if (ctx->subanim_counter == 1u) {
                MoveAnim_AnimationFlashScreen(ctx);
            }
            return;

        case MOVE_GROWL:
            if (MAX_SPRITES >= 8u) {
                memcpy(&wShadowOAM[4], &wShadowOAM[0], sizeof(wShadowOAM[0]) * 4u);
            }
            if (ctx->subanim_counter == 1u) {
                MoveAnim_AnimationCleanOAM();
            }
            return;

        case MOVE_TAIL_WHIP:
            /* TailWhipAnimationUnused: keep ASM timing side effect. */
            ctx->subanim_counter = 1u;
            MoveAnim_DelayFramesAsm(20u);
            return;

        default:
            return;
    }
}

static void MoveAnim_PlayApplyingAttackAnimation(move_anim_ctx_t *ctx) {
    uint8_t i;
    uint8_t rep;
    uint8_t b;
    uint8_t dmg_mult;
    if (!ctx) return;

    /* ASM: wAnimationType 1..6 lookup table. 0 = no generic follow-up anim. */
    if (ctx->animation_type == 0u) return;
    dmg_mult = (uint8_t)(wDamageMultipliers & 0x7Fu);

    switch (ctx->animation_type) {
        case 1u: /* ShakeScreenVertically (enemy damaging no side effect) */
            if (dmg_mult != 0u) Audio_PlaySFX_BattleHit(dmg_mult);
            for (i = 0u; i < 8u; i++) {
                sMoveAnimShakeY = (i & 1u) ? -2 : 2;
                Display_SetShakeOffset(sMoveAnimShakeX, sMoveAnimShakeY);
                MoveAnim_DelayFramesAsm(2u);
            }
            sMoveAnimShakeY = 0;
            Display_SetShakeOffset(sMoveAnimShakeX, sMoveAnimShakeY);
            return;

        case 2u: /* ShakeScreenHorizontallyHeavy */
            if (dmg_mult != 0u) Audio_PlaySFX_BattleHit(dmg_mult);
            for (i = 0u; i < 8u; i++) {
                sMoveAnimShakeX = (i & 1u) ? -4 : 4;
                Display_SetShakeOffset(sMoveAnimShakeX, sMoveAnimShakeY);
                MoveAnim_DelayFramesAsm(2u);
            }
            sMoveAnimShakeX = 0;
            Display_SetShakeOffset(sMoveAnimShakeX, sMoveAnimShakeY);
            return;

        case 3u: /* ShakeScreenHorizontallySlow (b=6,c=2) */
            b = 6u;
            rep = 2u;
            break;

        case 4u: /* BlinkEnemyMonSprite */
            if (dmg_mult != 0u) Audio_PlaySFX_BattleHit(dmg_mult);
            for (i = 0u; i < 6u; i++) {
                MoveAnim_AnimationHideEnemyMonPic(ctx);
                MoveAnim_DelayFramesAsm(5u);
                MoveAnim_AnimationShowEnemyMonPic(ctx);
                MoveAnim_DelayFramesAsm(5u);
            }
            /* Safety postcondition for ASM-equivalent blink-on-hit:
             * enemy sprite must end visible after the flash routine. */
            MoveAnim_SetEnemyVisible(1u);
            BattleUI_EnemySpriteCaptureState();
            return;

        case 5u: /* ShakeScreenHorizontallyLight */
            if (dmg_mult != 0u) Audio_PlaySFX_BattleHit(dmg_mult);
            for (i = 0u; i < 2u; i++) {
                sMoveAnimShakeX = (i & 1u) ? -2 : 2;
                Display_SetShakeOffset(sMoveAnimShakeX, sMoveAnimShakeY);
                MoveAnim_DelayFramesAsm(2u);
            }
            sMoveAnimShakeX = 0;
            Display_SetShakeOffset(sMoveAnimShakeX, sMoveAnimShakeY);
            return;

        case 6u: /* ShakeScreenHorizontallySlow2 (b=3,c=2) */
            b = 3u;
            rep = 2u;
            break;

        default:
            return;
    }

    while (rep-- != 0u) {
        uint8_t k;
        for (k = 0u; k < b; k++) {
            sMoveAnimShakeX = (int8_t)(sMoveAnimShakeX + 1);
            Display_SetShakeOffset(sMoveAnimShakeX, sMoveAnimShakeY);
            MoveAnim_DelayFramesAsm(2u);
        }
        for (k = 0u; k < b; k++) {
            sMoveAnimShakeX = (int8_t)(sMoveAnimShakeX - 1);
            Display_SetShakeOffset(sMoveAnimShakeX, sMoveAnimShakeY);
            MoveAnim_DelayFramesAsm(2u);
        }
    }
    sMoveAnimShakeX = 0;
    Display_SetShakeOffset(sMoveAnimShakeX, sMoveAnimShakeY);
}

static void MoveAnim_SetAnimationPalette(move_anim_ctx_t *ctx) {
    if (!ctx) return;

    /* Non-SGB battle animation palette baseline from ASM SetAnimationPalette. */
    MoveAnim_SetBGP(0xE4u);
}

static void MoveAnim_ShareMoveAnimations(move_anim_ctx_t *ctx) {
    if (!ctx) return;

    /* ShareMoveAnimations: only remap on the opponent's turn. */
    if (hWhoseTurn == 0) return;

    if (ctx->animation_id == MOVE_AMNESIA_ID) {
        ctx->animation_id = MOVE_ANIM_CONF_ANIM_ID;
        return;
    }
    if (ctx->animation_id == MOVE_REST) {
        ctx->animation_id = MOVE_ANIM_SLP_ANIM_ID;
    }
}

static void MoveAnim_RunSpecialEffect(move_anim_ctx_t *ctx, uint8_t se_id) {
    if (!ctx) return;

    /* Mirrors SpecialEffectPointers dispatch targets. */
    switch (se_id) {
        case 0xFE: MoveAnim_AnimationFlashScreen(ctx); break;
        case 0xFD: MoveAnim_AnimationDarkScreenPalette(ctx); break;
        case 0xFC: MoveAnim_AnimationResetScreenPalette(ctx); break;
        case 0xFB: MoveAnim_AnimationShakeScreen(ctx); break;
        case 0xFA: MoveAnim_AnimationWaterDropletsEverywhere(ctx); break;
        case 0xF9: MoveAnim_AnimationDarkenMonPalette(ctx); break;
        case 0xF8: MoveAnim_AnimationFlashScreenLong(ctx); break;
        case 0xF7: MoveAnim_AnimationSlideMonUp(ctx); break;
        case 0xF6: MoveAnim_AnimationSlideMonDown(ctx); break;
        case 0xF5: MoveAnim_AnimationFlashMonPic(ctx); break;
        case 0xF4: MoveAnim_AnimationSlideMonOff(ctx); break;
        case 0xF3: MoveAnim_AnimationBlinkMon(ctx); break;
        case 0xF2: MoveAnim_AnimationMoveMonHorizontally(ctx); break;
        case 0xF1: MoveAnim_AnimationResetMonPosition(ctx); break;
        case 0xF0: MoveAnim_AnimationLightScreenPalette(ctx); break;
        case 0xEF: MoveAnim_AnimationHideMonPic(ctx); break;
        case 0xEE: MoveAnim_AnimationSquishMonPic(ctx); break;
        case 0xED: MoveAnim_AnimationShootBallsUpward(ctx); break;
        case 0xEC: MoveAnim_AnimationShootManyBallsUpward(ctx); break;
        case 0xEB: MoveAnim_AnimationBoundUpAndDown(ctx); break;
        case 0xEA: MoveAnim_AnimationMinimizeMon(ctx); break;
        case 0xE9: MoveAnim_AnimationSlideMonDownAndHide(ctx); break;
        case 0xE8: MoveAnim_AnimationTransformMon(ctx); break;
        case 0xE7: MoveAnim_AnimationLeavesFalling(ctx); break;
        case 0xE6: MoveAnim_AnimationPetalsFalling(ctx); break;
        case 0xE5: MoveAnim_AnimationSlideMonHalfOff(ctx); break;
        case 0xE4: MoveAnim_AnimationShakeEnemyHUD(ctx); break;
        case 0xE3: MoveAnim_AnimationShakeEnemyHUD(ctx); break; /* unused in ASM */
        case 0xE2: MoveAnim_AnimationSpiralBallsInward(ctx); break;
        case 0xE1: MoveAnim_AnimationDelay10(ctx); break;
        case 0xE0: MoveAnim_AnimationFlashEnemyMonPic(ctx); break; /* unused in ASM */
        case 0xDF: MoveAnim_AnimationHideEnemyMonPic(ctx); break;
        case 0xDE: MoveAnim_AnimationBlinkEnemyMon(ctx); break;
        case 0xDD: MoveAnim_AnimationShowMonPic(ctx); break;
        case 0xDC: MoveAnim_AnimationShowEnemyMonPic(ctx); break;
        case 0xDB: MoveAnim_AnimationSlideEnemyMonOff(ctx); break;
        case 0xDA: MoveAnim_AnimationShakeBackAndForth(ctx); break;
        case 0xD9: MoveAnim_AnimationSubstitute(ctx); break;
        case 0xD8: MoveAnim_AnimationWavyScreen(ctx); break;
        default:
            break;
    }
}

#define MOVE_ANIM_SE_NOOP(fn_name) \
    static void fn_name(move_anim_ctx_t *ctx) { (void)ctx; }

MOVE_ANIM_SE_NOOP(MoveAnim_AnimationWaterDropletsEverywhere)
MOVE_ANIM_SE_NOOP(MoveAnim_AnimationFlashScreenLong)
MOVE_ANIM_SE_NOOP(MoveAnim_AnimationShootBallsUpward)
MOVE_ANIM_SE_NOOP(MoveAnim_AnimationShootManyBallsUpward)
MOVE_ANIM_SE_NOOP(MoveAnim_AnimationShakeEnemyHUD)
MOVE_ANIM_SE_NOOP(MoveAnim_AnimationSpiralBallsInward)

static void MoveAnim_AnimationShakeScreen(move_anim_ctx_t *ctx) {
    (void)ctx;
    sMoveAnimShakeToggle ^= 1u;
    sMoveAnimShakeX = sMoveAnimShakeToggle ? 4 : -4;
    Display_SetShakeOffset(sMoveAnimShakeX, sMoveAnimShakeY);
    MoveAnim_DelayFramesAsm(2);
}

static void MoveAnim_AnimationMoveMonHorizontally(move_anim_ctx_t *ctx) {
    (void)ctx;
    MoveAnim_AnimationHideMonPic(ctx);
    if (hWhoseTurn == 0u) {
        /* ASM: hlcoord 2,5 */
        MoveAnim_PlacePlayerBackSpriteAt(2u, 5u);
    } else {
        /* ASM: hlcoord 11,0 for enemy mon */
        /* Enemy-side path uses OAM coordinates. Ensure they're visible before
         * applying the horizontal offset; otherwise we'd offset/capture a
         * hidden (y=0) state and strand the enemy sprite invisible. */
        MoveAnim_SetEnemyVisible(1u);
        MoveAnim_OffsetEnemyY(0);
        {
            uint16_t i;
            uint16_t start = BattleUI_GetEnemyOAMStart();
            uint16_t end = BattleUI_GetEnemyOAMEnd();
            for (i = start; i <= end && i < MAX_SPRITES; i++) {
                if (wShadowOAM[i].y != 0u) {
                    int16_t x = (int16_t)wShadowOAM[i].x - 8;
                    if (x < 0) x = 0;
                    wShadowOAM[i].x = (uint8_t)x;
                }
            }
            BattleUI_EnemySpriteCaptureState();
        }
    }
    MoveAnim_DelayFramesAsm(3u);
}

static void MoveAnim_AnimationResetMonPosition(move_anim_ctx_t *ctx) {
    (void)ctx;
    sMoveAnimShakeX = 0;
    sMoveAnimShakeY = 0;
    Display_SetShakeOffset(0, 0);
    if (hWhoseTurn == 0u) {
        /* Clear any shifted-offset copy from F2 first, then rebuild normal pose. */
        MoveAnim_ClearPlayerBackSpriteAt(2u, 5u);
        MoveAnim_HidePlayerBackSprite();
        MoveAnim_LoadPlayerBackSprite(wBattleMon.species);
    } else {
        MoveAnim_SetEnemyVisible(1u);
        BattleUI_EnemySpriteCaptureState();
    }
}

static void MoveAnim_AnimationSlideMonUp(move_anim_ctx_t *ctx) {
    (void)ctx;
    if (hWhoseTurn != 0u) {
        MoveAnim_OffsetEnemyY(-8);
    } else {
        sMoveAnimShakeY = -8;
        Display_SetShakeOffset(sMoveAnimShakeX, sMoveAnimShakeY);
    }
    MoveAnim_DelayFramesAsm(4);
}

static void MoveAnim_DrawPlayerBackRowsShiftedDown(uint8_t shift_rows) {
    uint8_t ty, tx;
    MoveAnim_HidePlayerBackSprite();
    if (shift_rows >= 7u) return;

    for (ty = 0u; ty < (uint8_t)(7u - shift_rows); ty++) {
        uint8_t dst_ty = (uint8_t)(ty + shift_rows);
        for (tx = 0u; tx < 7u; tx++) {
            uint16_t row = (uint16_t)(MOVE_ANIM_PLAYER_BG_ROW + dst_ty);
            uint16_t col = (uint16_t)(MOVE_ANIM_PLAYER_BG_COL + tx);
            uint16_t idx = (uint16_t)(row * SCREEN_WIDTH + col);
            uint16_t sidx = (uint16_t)(row + 2u) * SCROLL_MAP_W + (uint16_t)(col + 2u);
            uint8_t tile = (uint8_t)(MOVE_ANIM_PLAYER_BG_TILE_BASE + ty * 7u + tx);
            if (idx < SCREEN_AREA) {
                wTileMap[idx] = tile;
            }
            if (sidx < (SCROLL_MAP_W * SCROLL_MAP_H)) {
                gScrollTileMap[sidx] = tile;
            }
        }
    }
}

static void MoveAnim_DrawEnemyFrontRowsShiftedDown(uint8_t shift_rows) {
    uint8_t ty, tx;
    uint16_t start = BattleUI_GetEnemyOAMStart();
    const uint8_t base_x = (uint8_t)(96u + OAM_X_OFS);
    const uint8_t base_y = (uint8_t)(0u + OAM_Y_OFS);

    for (ty = 0u; ty < 7u; ty++) {
        for (tx = 0u; tx < 7u; tx++) {
            uint16_t idx = (uint16_t)(start + (uint16_t)ty * 7u + (uint16_t)tx);
            if (idx >= MAX_SPRITES) {
                return;
            }
            if ((uint8_t)(ty + shift_rows) < 7u) {
                uint8_t dst_ty = (uint8_t)(ty + shift_rows);
                wShadowOAM[idx].y = (uint8_t)(base_y + dst_ty * 8u);
                wShadowOAM[idx].x = (uint8_t)(base_x + tx * 8u);
                wShadowOAM[idx].tile = (uint8_t)(MOVE_ANIM_ENEMY_SPR_TILE_BASE + ty * 7u + tx);
                wShadowOAM[idx].flags = 0u;
            } else {
                wShadowOAM[idx].y = (uint8_t)(SCREEN_HEIGHT_PX + OAM_Y_OFS);
            }
        }
    }
}

static void MoveAnim_AnimationSlideMonDown(move_anim_ctx_t *ctx) {
    uint8_t step;
    (void)ctx;
    for (step = 0u; step < 7u; step++) {
        if (hWhoseTurn == 0u) {
            MoveAnim_DrawPlayerBackRowsShiftedDown(step);
        } else {
            MoveAnim_DrawEnemyFrontRowsShiftedDown(step);
        }
        MoveAnim_DelayFramesAsm(3u);
    }
    MoveAnim_AnimationHideMonPic(ctx);
}

static void MoveAnim_AnimationSlideMonOff(move_anim_ctx_t *ctx) {
    (void)ctx;
    if (hWhoseTurn == 0u) {
        uint8_t step;
        for (step = 0u; step < 8u; step++) {
            uint8_t row;
            for (row = 0u; row < 7u; row++) {
                uint8_t col;
                for (col = 0u; col < 8u; col++) {
                    uint16_t r = (uint16_t)(MOVE_ANIM_PLAYER_BG_ROW + row);
                    uint16_t c = (uint16_t)(MOVE_ANIM_PLAYER_BG_COL + col);
                    uint16_t idx = r * SCREEN_WIDTH + c;
                    uint16_t sidx = (uint16_t)(r + 2u) * SCROLL_MAP_W + (uint16_t)(c + 2u);
                    uint8_t tile = BLANK_TILE_SLOT;
                    if (idx < SCREEN_AREA) {
                        uint8_t a = (uint8_t)(wTileMap[idx] + 7u);
                        if (a < 0x61u) {
                            tile = a;
                        }
                    }
                    if (idx < SCREEN_AREA) wTileMap[idx] = tile;
                    if (sidx < (SCROLL_MAP_W * SCROLL_MAP_H)) gScrollTileMap[sidx] = tile;
                }
            }
            MoveAnim_DelayFramesAsm(3u);
        }
    } else {
        uint8_t step;
        /* Defensive parity with F2/F1 path: if an earlier command left enemy
         * hidden, restore visibility before motion/capture so we don't save
         * an all-zero hidden baseline. */
        MoveAnim_SetEnemyVisible(1u);
        for (step = 0u; step < 8u; step++) {
            uint16_t i;
            uint16_t start = BattleUI_GetEnemyOAMStart();
            uint16_t end = BattleUI_GetEnemyOAMEnd();
            for (i = start; i <= end && i < MAX_SPRITES; i++) {
                if (wShadowOAM[i].y != 0u) {
                    int16_t x = (int16_t)wShadowOAM[i].x + 8;
                    if (x >= 168) {
                        wShadowOAM[i].y = (uint8_t)(SCREEN_HEIGHT_PX + OAM_Y_OFS);
                    } else {
                        wShadowOAM[i].x = (uint8_t)x;
                    }
                }
            }
            MoveAnim_DelayFramesAsm(3u);
        }
        BattleUI_EnemySpriteCaptureState();
        MoveAnim_SetEnemyVisible(0u);
    }
}

static void MoveAnim_AnimationSlideMonHalfOff(move_anim_ctx_t *ctx) {
    (void)ctx;
    if (hWhoseTurn != 0u) {
        MoveAnim_OffsetEnemyY(24);
    } else {
        sMoveAnimShakeY = 24;
        Display_SetShakeOffset(sMoveAnimShakeX, sMoveAnimShakeY);
    }
    MoveAnim_DelayFramesAsm(4);
}

static void MoveAnim_AnimationSlideMonDownAndHide(move_anim_ctx_t *ctx) {
    MoveAnim_AnimationSlideMonDown(ctx);
    if (hWhoseTurn == 0u) {
        MoveAnim_HidePlayerBackSprite();
    } else {
        MoveAnim_SetEnemyVisible(0u);
    }
}

static void MoveAnim_AnimationTransformMon(move_anim_ctx_t *ctx) {
    (void)ctx;
    /* ASM AnimationTransformMon:
     * player turn -> redraw back pic from enemy species
     * enemy turn  -> redraw front pic from player species. */
    if (hWhoseTurn == 0u) {
        MoveAnim_LoadPlayerBackSprite(wEnemyMon.species);
    } else {
        MoveAnim_LoadEnemyFrontSprite(wBattleMon.species);
    }
}

static void MoveAnim_AnimationHideMonPic(move_anim_ctx_t *ctx) {
    (void)ctx;
    if (hWhoseTurn == 0u) {
        MoveAnim_HidePlayerBackSprite();
    } else {
        MoveAnim_SetEnemyVisible(0u);
    }
}

static void MoveAnim_AnimationShowMonPic(move_anim_ctx_t *ctx) {
    (void)ctx;
    if (hWhoseTurn == 0u) {
        MoveAnim_LoadPlayerBackSprite(wBattleMon.species);
    } else {
        MoveAnim_SetEnemyVisible(1u);
    }
    MoveAnim_DelayFramesAsm(3u);
}

static void MoveAnim_AnimationHideEnemyMonPic(move_anim_ctx_t *ctx) {
    (void)ctx;
    MoveAnim_SetEnemyVisible(0u);
    MoveAnim_DelayFramesAsm(3u);
}

static void MoveAnim_AnimationShowEnemyMonPic(move_anim_ctx_t *ctx) {
    (void)ctx;
    MoveAnim_SetEnemyVisible(1u);
    MoveAnim_DelayFramesAsm(3u);
}

static void MoveAnim_AnimationBlinkEnemyMon(move_anim_ctx_t *ctx) {
    uint8_t i;
    (void)ctx;
    for (i = 0u; i < 6u; i++) {
        MoveAnim_SetEnemyVisible(0u);
        MoveAnim_DelayFramesAsm(5u);
        MoveAnim_SetEnemyVisible(1u);
        MoveAnim_DelayFramesAsm(3u);
        MoveAnim_DelayFramesAsm(5u);
    }
    /* Ensure consistent terminal state for callers that rely on this helper. */
    MoveAnim_SetEnemyVisible(1u);
    BattleUI_EnemySpriteCaptureState();
}

static void MoveAnim_AnimationBlinkMon(move_anim_ctx_t *ctx) {
    uint8_t i;
    (void)ctx;
    for (i = 0u; i < 6u; i++) {
        MoveAnim_AnimationHideMonPic(ctx);
        MoveAnim_DelayFramesAsm(5u);
        MoveAnim_AnimationShowMonPic(ctx);
        MoveAnim_DelayFramesAsm(5u);
    }
}

static void MoveAnim_AnimationShakeBackAndForth(move_anim_ctx_t *ctx) {
    (void)ctx;
    sMoveAnimShakeToggle ^= 1u;
    sMoveAnimShakeX = sMoveAnimShakeToggle ? 6 : -6;
    Display_SetShakeOffset(sMoveAnimShakeX, sMoveAnimShakeY);
    MoveAnim_DelayFramesAsm(3);
}

static void MoveAnim_AnimationFlashScreen(move_anim_ctx_t *ctx) {
    uint8_t saved_bgp;
    (void)ctx;

    saved_bgp = sMoveAnimCurrentBGP;
    MoveAnim_SetBGP(0x1Bu); /* 0,1,2,3 (inverted colors) */
    MoveAnim_DelayFramesAsm(2);
    MoveAnim_SetBGP(0x00u); /* white out background */
    MoveAnim_DelayFramesAsm(2);
    MoveAnim_SetBGP(saved_bgp);
}

static void MoveAnim_AnimationDarkScreenPalette(move_anim_ctx_t *ctx) {
    (void)ctx;
    MoveAnim_SetBGP(0x6Fu);
}

static void MoveAnim_AnimationFlashMonPic(move_anim_ctx_t *ctx) {
    (void)ctx;
    /* ASM AnimationFlashMonPic sets change-species bytes to current species and
     * dispatches ChangeMonPic, which effectively redraws the same species pic. */
    if (hWhoseTurn == 0u) {
        MoveAnim_LoadPlayerBackSprite(wBattleMon.species);
    } else {
        MoveAnim_LoadEnemyFrontSprite(wEnemyMon.species);
    }
}

static void MoveAnim_AnimationDarkenMonPalette(move_anim_ctx_t *ctx) {
    (void)ctx;
    /* ASM non-SGB path uses BGP=0xF9. */
    MoveAnim_SetBGP(0xF9u);
}

static void MoveAnim_AnimationResetScreenPalette(move_anim_ctx_t *ctx) {
    (void)ctx;
    MoveAnim_SetBGP(0xE4u);
}

static void MoveAnim_AnimationLightScreenPalette(move_anim_ctx_t *ctx) {
    (void)ctx;
    MoveAnim_SetBGP(0x90u);
}

static void MoveAnim_AnimationDelay10(move_anim_ctx_t *ctx) {
    if (!ctx) return;
    /* Exact ASM semantic: AnimationDelay10 -> DelayFrames(10). */
    MoveAnim_DelayFramesAsm(10);
}

static void MoveAnim_AnimationSquishMonPic(move_anim_ctx_t *ctx) {
    (void)ctx;
}

static void MoveAnim_AnimationBoundUpAndDown(move_anim_ctx_t *ctx) {
    uint8_t i;
    if (!ctx) return;

    /* ASM AnimationBoundUpAndDown: 5x AnimationSlideMonDown, then ShowMonPic. */
    for (i = 0u; i < 5u; i++) {
        MoveAnim_AnimationSlideMonDown(ctx);
    }
    MoveAnim_AnimationShowMonPic(ctx);
}

static void MoveAnim_AnimationMinimizeMon(move_anim_ctx_t *ctx) {
    (void)ctx;
    /* ASM AnimationMinimizeMon ends with Delay3 + AnimationShowMonPic after
     * replacing picture data with the minimized glyph. Full temp-pic rewrite
     * path is pending; preserve exact timing + visibility behavior first. */
    MoveAnim_DelayFramesAsm(3u);
    MoveAnim_AnimationShowMonPic(ctx);
}

static void MoveAnim_AnimationLeavesFalling(move_anim_ctx_t *ctx) {
    (void)ctx;
}

static void MoveAnim_AnimationPetalsFalling(move_anim_ctx_t *ctx) {
    (void)ctx;
}

static void MoveAnim_AnimationSubstitute(move_anim_ctx_t *ctx) {
    (void)ctx;
    /* ASM AnimationSubstitute ends by showing the mon pic and delays 3 frames
     * through AnimationShowMonPic -> Delay3. */
    MoveAnim_AnimationShowMonPic(ctx);
    MoveAnim_DelayFramesAsm(3);
}

static void MoveAnim_AnimationFlashEnemyMonPic(move_anim_ctx_t *ctx) {
    uint8_t saved_turn = hWhoseTurn;
    hWhoseTurn = (uint8_t)(hWhoseTurn ^ 1u);
    MoveAnim_AnimationFlashMonPic(ctx);
    hWhoseTurn = saved_turn;
}

static void MoveAnim_AnimationSlideEnemyMonOff(move_anim_ctx_t *ctx) {
    uint8_t saved_turn = hWhoseTurn;
    hWhoseTurn = (uint8_t)(hWhoseTurn ^ 1u);
    MoveAnim_AnimationSlideMonOff(ctx);
    hWhoseTurn = saved_turn;
}

static void MoveAnim_AnimationWavyScreen(move_anim_ctx_t *ctx) {
    (void)ctx;
    /* Handled by step runner (MoveAnim_SE_WavyScreenStep) to avoid 8-bit
     * delay overflow and to mirror ASM frame pacing. */
}

#undef MOVE_ANIM_SE_NOOP

static void MoveAnim_PlayCommandSound(move_anim_ctx_t *ctx, uint8_t sound_id_minus_one) {
    const move_sfx_data_t *move_sfx;
    uint8_t species;

    if (!ctx) return;

    /* Mirrors wAnimSoundID write in PlayAnimation for every command. */
    ctx->anim_sound_id = sound_id_minus_one;

    /* ASM only plays when sound != NO_MOVE - 1. */
    if (sound_id_minus_one == MOVE_ANIM_NO_MOVE_MINUS_ONE) return;
    if (sound_id_minus_one >= MOVE_SFX_DATA_COUNT) return;

    move_sfx = &gMoveSfxData[sound_id_minus_one];
    if (Audio_IsMoveSfxDebug()) {
        printf("[SFXDBG] anim=0x%02X sound=0x%02X idx=%u sym=%s pitch=0x%02X tempo=0x%02X turn=%u\n",
               (unsigned)ctx->animation_id,
               (unsigned)sound_id_minus_one,
               (unsigned)sound_id_minus_one,
               move_sfx->sfx_symbol ? move_sfx->sfx_symbol : "(null)",
               (unsigned)move_sfx->pitch_mod,
               (unsigned)move_sfx->tempo_mod,
               (unsigned)hWhoseTurn);
    }

    if (MoveAnim_IsCryMove(ctx->animation_id)) {
        if (hWhoseTurn == 0u) {
            species = wBattleMon.species;
        } else {
            species = wEnemyMonSpecies;
        }
        Audio_PlayCryModified(species, (int8_t)move_sfx->pitch_mod, move_sfx->tempo_mod);
        return;
    }

    /* ASM parity (battle engine copy): move/battle SFX apply the per-move
     * frequency + tempo modifiers from data/moves/sfx.asm. */
    if (Audio_PlayMoveSFXBySymbolModified(move_sfx->sfx_symbol, (int8_t)move_sfx->pitch_mod, move_sfx->tempo_mod)) return;

    /* Compatibility fallback for symbols not yet represented in imported
     * move SFX command tables. */
    if (strcmp(move_sfx->sfx_symbol, "SFX_DAMAGE") == 0) {
        Audio_PlaySFX_BattleHit(10u);
    } else if (strcmp(move_sfx->sfx_symbol, "SFX_NOT_VERY_EFFECTIVE") == 0) {
        Audio_PlaySFX_BattleHit(5u);
    } else if (strcmp(move_sfx->sfx_symbol, "SFX_SUPER_EFFECTIVE") == 0) {
        Audio_PlaySFX_BattleHit(20u);
    } else if (strcmp(move_sfx->sfx_symbol, "SFX_FAINT_FALL") == 0) {
        Audio_PlaySFX_FaintFallOnly();
    }
}

static uint8_t MoveAnim_IsCryMove(uint8_t animation_id) {
    if (animation_id == MOVE_GROWL) return 1u;
    if (animation_id == MOVE_ROAR) return 1u;
    return 0u;
}

static uint8_t MoveAnim_GetSubanimationTransform1(uint8_t subanim_type) {
    if (hWhoseTurn != 0u) return subanim_type;
    return MOVE_ANIM_SUBANIMTYPE_NORMAL;
}

static uint8_t MoveAnim_GetSubanimationTransform2(void) {
    if (hWhoseTurn == 0u) return MOVE_ANIM_SUBANIMTYPE_HFLIP;
    return MOVE_ANIM_SUBANIMTYPE_NORMAL;
}

static uint8_t MoveAnim_HVFlipFlags(uint8_t flags) {
    if (flags == 0u) return (uint8_t)(OAM_FLAG_FLIP_Y | OAM_FLAG_FLIP_X);
    if (flags == OAM_FLAG_FLIP_X) return OAM_FLAG_FLIP_Y;
    if (flags == OAM_FLAG_FLIP_Y) return OAM_FLAG_FLIP_X;
    return 0;
}

static uint8_t MoveAnim_HFlipFlags(uint8_t flags) {
    if ((flags & OAM_FLAG_FLIP_X) != 0u) {
        return (uint8_t)(flags & (uint8_t)~OAM_FLAG_FLIP_X);
    }
    return (uint8_t)(flags | OAM_FLAG_FLIP_X);
}

static uint8_t MoveAnim_GetFrameBlockYOffset(const move_anim_frameblock_sprite_t *sprite) {
    if (!sprite) return 0;
    /* Data table preserves dbsprite args (x tile, y tile, x pixel, y pixel).
     * Y offset byte in ASM is y_tile * 8 + y_pixel. */
    return (uint8_t)(sprite->x_offset * 8u + sprite->x_offset_flip);
}

static uint8_t MoveAnim_GetFrameBlockXOffset(const move_anim_frameblock_sprite_t *sprite) {
    if (!sprite) return 0;
    /* X offset byte in ASM is x_tile * 8 + x_pixel. */
    return (uint8_t)(sprite->y_offset * 8u + sprite->y_offset_flip);
}

static void MoveAnim_DelayFramesAsm(uint8_t frames) {
    hFrameCounter = (uint8_t)(hFrameCounter + frames);
    if (sMoveAnimExecCtx && frames != 0u) {
        uint8_t ticks = MoveAnim_ConvertAsmFramesToEngineTicks(sMoveAnimExecCtx, frames);
        /* In tick mode, DelayFrames yields future ticks before continuing.
         * Multiple DelayFrames calls in one ASM routine accumulate. */
        sMoveAnimExecCtx->wait_frames = (uint8_t)(sMoveAnimExecCtx->wait_frames + ticks);
    }
    /* Timing bridge is handled by docs/timing_conversion_protocol.md.
     * Phase D keeps this call site so frameblock mode behavior stays exact. */
}

static uint8_t MoveAnim_ConvertAsmFramesToEngineTicks(move_anim_ctx_t *ctx, uint8_t asm_frames) {
    (void)ctx;
    /* Strict ASM parity for battle move animations: 1 DelayFrame == 1 engine tick. */
    return asm_frames;
}

static void MoveAnim_AnimationCleanOAM(void) {
    MoveAnim_DelayFramesAsm(1);
    MoveAnim_ClearAnimationOAM();
}

static void MoveAnim_ClearAnimationOAM(void) {
    uint16_t i;
    uint16_t oam_start = BattleUI_GetAnimOAMStart();
    uint16_t oam_end = BattleUI_GetAnimOAMEnd();
    if (oam_end >= MAX_SPRITES) {
        oam_end = (uint16_t)(MAX_SPRITES - 1u);
    }
    for (i = oam_start; i <= oam_end; i++) {
        wShadowOAM[i].y = 0u;
    }
}

static void MoveAnim_ResetVisualState(void) {
    uint8_t enemy_should_hide =
        (uint8_t)((wEnemyBattleStatus1 & ((1u << BSTAT1_CHARGING_UP) |
                                          (1u << BSTAT1_INVULNERABLE))) != 0u);
    sMoveAnimShakeX = 0;
    sMoveAnimShakeY = 0;
    sMoveAnimShakeToggle = 0;
    sMoveAnimWavyActive = 0u;
    sMoveAnimWavyPhase = 0u;
    Display_SetWavyPhase(0, 0);
    /* Rebuild enemy tiles + OAM pose from canonical battle state before
     * any visibility restore/capture. If saved coords were stale/zero,
     * restoring first can permanently preserve the bad pose. */
    MoveAnim_LoadEnemyFrontSprite(wEnemyMon.species);
    MoveAnim_ResetEnemyOAMPoseCanonical();
    BattleUI_EnemySpriteSetVisible(1u);
    BattleUI_EnemySpriteCaptureState();
    if (enemy_should_hide) {
        printf("[DIGDBG] reset_visual keep_enemy_hidden e_b1=0x%02X\n", wEnemyBattleStatus1);
        BattleUI_EnemySpriteSetVisible(0u);
    }
    Display_SetShakeOffset(0, 0);
}

static void MoveAnim_ResetEnemyOAMPoseCanonical(void) {
    uint16_t start = BattleUI_GetEnemyOAMStart();
    /* Enemy front pic is a 7x7 OAM canvas at pixel (96,0) in battle UI. */
    const uint8_t base_x = (uint8_t)(96u + OAM_X_OFS);
    const uint8_t base_y = (uint8_t)(0u + OAM_Y_OFS);
    for (uint8_t ty = 0u; ty < 7u; ty++) {
        for (uint8_t tx = 0u; tx < 7u; tx++) {
            uint16_t idx = (uint16_t)(start + (uint16_t)ty * 7u + (uint16_t)tx);
            if (idx >= MAX_SPRITES) return;
            wShadowOAM[idx].y = (uint8_t)(base_y + ty * 8u);
            wShadowOAM[idx].x = (uint8_t)(base_x + tx * 8u);
            wShadowOAM[idx].tile = (uint8_t)(MOVE_ANIM_ENEMY_SPR_TILE_BASE + ty * 7u + tx);
            wShadowOAM[idx].flags = 0u;
        }
    }
}

static void MoveAnim_UpdatePerFrameEffects(move_anim_ctx_t *ctx) {
    (void)ctx;
    if (sMoveAnimWavyActive) {
        Display_SetWavyPhase(1, sMoveAnimWavyPhase);
        sMoveAnimWavyPhase = (uint8_t)((sMoveAnimWavyPhase + 1u) & 31u);
    }
}

static int MoveAnim_SE_WavyScreenStep(move_anim_ctx_t *ctx) {
    if (!ctx) return 1;

    /* Mirrors AnimationWavyScreen timing skeleton:
     * Delay3 -> 255 wave frames -> Delay3 -> end.
     * We model the per-scanline SCX trick as a per-frame phase wobble. */
    switch (ctx->se_phase) {
        case 0:
            sMoveAnimWavyActive = 1u;
            sMoveAnimWavyPhase = 0u;
            Display_SetWavyPhase(1, 0);
            ctx->se_counter0 = 255u;
            ctx->se_phase = 1u;
            MoveAnim_DelayFramesAsm(3u);
            return 0;

        case 1:
            if (ctx->se_counter0 == 0u) {
                ctx->se_phase = 2u;
                return 0;
            }
            ctx->se_counter0 = (uint8_t)(ctx->se_counter0 - 1u);
            MoveAnim_DelayFramesAsm(1u);
            return 0;

        case 2:
            sMoveAnimWavyActive = 0u;
            Display_SetWavyPhase(0, 0);
            ctx->se_phase = 3u;
            MoveAnim_DelayFramesAsm(3u);
            return 0;

        default:
            ctx->se_phase = 0u;
            return 1;
    }
}

static int MoveAnim_SE_SlideMonUpStep(move_anim_ctx_t *ctx) {
    if (!ctx) return 1;
    switch (ctx->se_phase) {
        case 0u:
            /* Reveal from below by replaying slide-down row states in reverse:
             * keep one fully-hidden frame, then shift 6 -> 0 with Delay2 each
             * step (matches _AnimationSlideMonUp cadence without a pre-pop). */
            ctx->se_counter0 = 7u;
            ctx->se_phase = 1u;
            return 0;
        case 1u:
            if (ctx->se_counter0 >= 7u) {
                MoveAnim_AnimationHideMonPic(ctx);
            } else if (hWhoseTurn == 0u) {
                MoveAnim_DrawPlayerBackRowsShiftedDown(ctx->se_counter0);
            } else {
                MoveAnim_DrawEnemyFrontRowsShiftedDown(ctx->se_counter0);
            }
            MoveAnim_DelayFramesAsm(2u);
            if (ctx->se_counter0 == 0u) {
                ctx->se_phase = 0u;
                return 1;
            }
            ctx->se_counter0 = (uint8_t)(ctx->se_counter0 - 1u);
            return 0;
        default:
            ctx->se_phase = 0u;
            return 1;
    }
}

static int MoveAnim_SE_SlideMonDownStep(move_anim_ctx_t *ctx) {
    if (!ctx) return 1;
    switch (ctx->se_phase) {
        case 0u:
            ctx->se_counter0 = 0u;
            ctx->se_phase = 1u;
            return 0;
        case 1u:
            if (hWhoseTurn == 0u) {
                MoveAnim_DrawPlayerBackRowsShiftedDown(ctx->se_counter0);
            } else {
                MoveAnim_DrawEnemyFrontRowsShiftedDown(ctx->se_counter0);
            }
            MoveAnim_DelayFramesAsm(3u);
            ctx->se_counter0 = (uint8_t)(ctx->se_counter0 + 1u);
            if (ctx->se_counter0 >= 7u) {
                MoveAnim_AnimationHideMonPic(ctx);
                ctx->se_phase = 0u;
                return 1;
            }
            return 0;
        default:
            ctx->se_phase = 0u;
            return 1;
    }
}

static void MoveAnim_SetEnemyVisible(uint8_t visible) {
    BattleUI_EnemySpriteSetVisible(visible);
}

static void MoveAnim_OffsetEnemyY(int8_t delta) {
    BattleUI_EnemySpriteOffsetY(delta);
}

static void MoveAnim_LoadEnemyFrontSprite(uint8_t species) {
    uint8_t dex = gSpeciesToDex[species];
    uint8_t i;
    if (dex == 0u || dex > 151u) return;
    for (i = 0; i < POKEMON_FRONT_CANVAS_TILES; i++) {
        Display_LoadSpriteTile((uint8_t)(MOVE_ANIM_ENEMY_SPR_TILE_BASE + i),
                               gPokemonFrontSprite[dex][i]);
    }
}

static void MoveAnim_LoadPlayerBackSprite(uint8_t species) {
    uint8_t dex = gSpeciesToDex[species];
    uint8_t ty;
    if (dex == 0u || dex > 151u) return;

    for (ty = 0; ty < POKEMON_BACK_TILES; ty++) {
        Display_LoadTile((uint8_t)(MOVE_ANIM_PLAYER_BG_TILE_BASE + ty),
                         gPokemonBackSprite[dex][ty]);
    }

    for (ty = 0; ty < 7u; ty++) {
        uint8_t tx;
        for (tx = 0; tx < 7u; tx++) {
            uint16_t row = (uint16_t)(MOVE_ANIM_PLAYER_BG_ROW + ty);
            uint16_t col = (uint16_t)(MOVE_ANIM_PLAYER_BG_COL + tx);
            uint16_t idx = (uint16_t)(row * SCREEN_WIDTH + col);
            uint16_t sidx = (uint16_t)(row + 2u) * SCROLL_MAP_W + (uint16_t)(col + 2u);
            uint8_t tile = (uint8_t)(MOVE_ANIM_PLAYER_BG_TILE_BASE + ty * 7u + tx);
            if (idx < SCREEN_AREA) {
                wTileMap[idx] = tile;
            }
            if (sidx < (SCROLL_MAP_W * SCROLL_MAP_H)) {
                gScrollTileMap[sidx] = tile;
            }
        }
    }
}

static void MoveAnim_HidePlayerBackSprite(void) {
    MoveAnim_ClearPlayerBackSpriteAt(MOVE_ANIM_PLAYER_BG_COL, MOVE_ANIM_PLAYER_BG_ROW);
}

static void MoveAnim_ClearPlayerBackSpriteAt(uint8_t col, uint8_t row) {
    uint8_t ty;
    for (ty = 0; ty < 7u; ty++) {
        uint8_t tx;
        for (tx = 0; tx < 7u; tx++) {
            uint16_t r = (uint16_t)(row + ty);
            uint16_t c = (uint16_t)(col + tx);
            uint16_t idx = (uint16_t)(r * SCREEN_WIDTH + c);
            uint16_t sidx = (uint16_t)(r + 2u) * SCROLL_MAP_W + (uint16_t)(c + 2u);
            if (idx < SCREEN_AREA) wTileMap[idx] = BLANK_TILE_SLOT;
            if (sidx < (SCROLL_MAP_W * SCROLL_MAP_H)) gScrollTileMap[sidx] = BLANK_TILE_SLOT;
        }
    }
}

static void MoveAnim_PlacePlayerBackSpriteAt(uint8_t col, uint8_t row) {
    uint8_t ty;
    for (ty = 0; ty < 7u; ty++) {
        uint8_t tx;
        for (tx = 0; tx < 7u; tx++) {
            uint16_t r = (uint16_t)(row + ty);
            uint16_t c = (uint16_t)(col + tx);
            uint16_t idx = (uint16_t)(r * SCREEN_WIDTH + c);
            uint16_t sidx = (uint16_t)(r + 2u) * SCROLL_MAP_W + (uint16_t)(c + 2u);
            uint8_t tile = (uint8_t)(MOVE_ANIM_PLAYER_BG_TILE_BASE + ty * 7u + tx);
            if (idx < SCREEN_AREA) wTileMap[idx] = tile;
            if (sidx < (SCROLL_MAP_W * SCROLL_MAP_H)) gScrollTileMap[sidx] = tile;
        }
    }
}

static int MoveAnim_SE_FlashScreenLongStep(move_anim_ctx_t *ctx) {
    uint8_t pal;
    if (!ctx) return 1;

    /* ASM AnimationFlashScreenLong:
     * 3 cycles over 12 palettes, delays 2 frames on cycle 1 then 1 frame. */
    if (ctx->se_phase == 0u) {
        ctx->se_counter0 = 3u;
        ctx->se_index = 0u;
        ctx->se_phase = 1u;
    }

    pal = sMoveAnimFlashScreenLongPals[ctx->se_index];
    MoveAnim_SetBGP(pal);
    if (ctx->se_counter0 == 3u) {
        MoveAnim_DelayFramesAsm(2u);
    } else {
        MoveAnim_DelayFramesAsm(1u);
    }

    ctx->se_index = (uint8_t)(ctx->se_index + 1u);
    if (ctx->se_index >= 12u) {
        ctx->se_index = 0u;
        if (ctx->se_counter0 > 0u) {
            ctx->se_counter0 = (uint8_t)(ctx->se_counter0 - 1u);
        }
        if (ctx->se_counter0 == 0u) {
            ctx->se_phase = 0u;
            return 1;
        }
    }
    return 0;
}

static int MoveAnim_SE_SpiralBallsInwardStep(move_anim_ctx_t *ctx) {
    int16_t base_y;
    int16_t base_x;
    uint8_t i;
    if (!ctx) return 1;

    /* ASM AnimationSpiralBallsInward:
     * render 3 balls from a sliding coordinate window, DelayFrames(5) per step,
     * then clean OAM and flash screen. */
    if (ctx->se_phase == 0u) {
        ctx->se_index = 0u;
        ctx->se_phase = 1u;
    }

    base_y = (hWhoseTurn == 0u) ? 0 : -40;
    base_x = (hWhoseTurn == 0u) ? 0 : 80;

    if (sMoveAnimSpiralCoords[(uint16_t)ctx->se_index * 2u] == 0xFFu) {
        ctx->se_phase = 2u;
    }

    if (ctx->se_phase == 1u) {
        for (i = 0u; i < 3u; i++) {
            uint16_t pos = (uint16_t)(ctx->se_index + i) * 2u;
            uint8_t yoff = sMoveAnimSpiralCoords[pos];
            uint8_t xoff;
            int16_t y;
            int16_t x;

            if (yoff == 0xFFu) {
                ctx->se_phase = 2u;
                break;
            }
            xoff = sMoveAnimSpiralCoords[pos + 1u];
            y = (int16_t)((int16_t)(int8_t)yoff + base_y);
            x = (int16_t)((int16_t)(int8_t)xoff + base_x);
            if (i < MAX_SPRITES) {
                if (y < 0) y = 0;
                if (y > 255) y = 255;
                if (x < 0) x = 0;
                if (x > 255) x = 255;
                wShadowOAM[i].y = (uint8_t)y;
                wShadowOAM[i].x = (uint8_t)x;
                wShadowOAM[i].tile = 0x7Au;
                wShadowOAM[i].flags = 0u;
            }
        }
        if (ctx->se_phase == 1u) {
            MoveAnim_DelayFramesAsm(5u);
            ctx->se_index = (uint8_t)(ctx->se_index + 1u);
            return 0;
        }
    }

    MoveAnim_AnimationCleanOAM();
    MoveAnim_AnimationFlashScreen(ctx);
    ctx->se_phase = 0u;
    return 1;
}

static int MoveAnim_SE_WaterDropletsEverywhereStep(move_anim_ctx_t *ctx) {
    uint8_t base_y;
    uint16_t oam;
    if (!ctx) return 1;

    /* ASM AnimationWaterDropletsEverywhere:
     * d=32 outer iterations, each does two _AnimationWaterDroplets passes
     * (start Y 16 then 24), each pass ending with clean+DelayFrame. */
    if (ctx->se_phase == 0u) {
        ctx->se_counter0 = 32u;
        ctx->se_index = 0u;
        ctx->se_phase = 1u;
        sMoveAnimWaterBaseX = (uint8_t)-16;
    }

    base_y = (ctx->se_index == 0u) ? 16u : 24u;
    oam = 0u;
    while (1) {
        uint8_t x;
        sMoveAnimWaterBaseX = (uint8_t)(sMoveAnimWaterBaseX + 27u);
        x = sMoveAnimWaterBaseX;

        if (oam < MAX_SPRITES) {
            wShadowOAM[oam].y = base_y;
            wShadowOAM[oam].x = x;
            wShadowOAM[oam].tile = 0x71u;
            wShadowOAM[oam].flags = 0u;
        }
        oam++;

        if (x < 144u) {
            continue;
        }
        sMoveAnimWaterBaseX = (uint8_t)(sMoveAnimWaterBaseX - 168u);
        base_y = (uint8_t)(base_y + 16u);
        if (base_y < 112u) {
            continue;
        }
        break;
    }

    MoveAnim_AnimationCleanOAM();
    MoveAnim_DelayFramesAsm(1u);

    if (ctx->se_index == 0u) {
        ctx->se_index = 1u;
        return 0;
    }

    ctx->se_index = 0u;
    if (ctx->se_counter0 > 0u) {
        ctx->se_counter0 = (uint8_t)(ctx->se_counter0 - 1u);
    }
    if (ctx->se_counter0 == 0u) {
        ctx->se_phase = 0u;
        return 1;
    }
    return 0;
}

static void MoveAnim_SE_FallingObjectsInit(uint8_t count, uint8_t tile) {
    uint8_t i;
    sMoveAnimFallingCount = count;
    sMoveAnimFallingTile = tile;
    if (sMoveAnimFallingCount > 20u) {
        sMoveAnimFallingCount = 20u;
    }

    for (i = 0u; i < sMoveAnimFallingCount && i < MAX_SPRITES; i++) {
        wShadowOAM[i].y = 0u;
        wShadowOAM[i].x = sMoveAnimFallingInitialX[i];
        wShadowOAM[i].tile = sMoveAnimFallingTile;
        wShadowOAM[i].flags = 0u;
        sMoveAnimFallingMove[i] = sMoveAnimFallingInitialMove[i];
    }

    if (sMoveAnimFallingCount > 0u) {
        wShadowOAM[0].y = 0u;
    }
}

static uint8_t MoveAnim_SE_FallingObjectsUpdateMovementByte(uint8_t movement) {
    uint8_t a = (uint8_t)(movement + 1u);
    if ((a & 0x7Fu) == 9u) {
        a = (uint8_t)(((a & 0x80u) ^ 0x80u));
    }
    return a;
}

static void MoveAnim_SE_FallingObjectsUpdateOAM(uint8_t idx, uint8_t movement) {
    uint8_t y;
    uint8_t delta;
    if (idx >= sMoveAnimFallingCount || idx >= MAX_SPRITES) return;

    y = (uint8_t)(wShadowOAM[idx].y + 2u);
    if (y >= 112u) {
        y = (uint8_t)(SCREEN_HEIGHT_PX + OAM_Y_OFS);
    }
    wShadowOAM[idx].y = y;

    delta = sMoveAnimFallingDeltaX[movement & 0x7Fu];
    if ((movement & 0x80u) != 0u) {
        wShadowOAM[idx].x = (uint8_t)(wShadowOAM[idx].x - delta);
        wShadowOAM[idx].flags = OAM_FLAG_FLIP_X;
    } else {
        wShadowOAM[idx].x = (uint8_t)(wShadowOAM[idx].x + delta);
        wShadowOAM[idx].flags = 0u;
    }
}

static int MoveAnim_SE_LeavesFallingStep(move_anim_ctx_t *ctx) {
    uint8_t i;
    if (!ctx) return 1;

    if (ctx->se_phase == 0u) {
        MoveAnim_SE_FallingObjectsInit(3u, 0x37u);
        ctx->se_phase = 1u;
    }

    for (i = 0u; i < sMoveAnimFallingCount; i++) {
        uint8_t movement = sMoveAnimFallingMove[i];
        movement = MoveAnim_SE_FallingObjectsUpdateMovementByte(movement);
        MoveAnim_SE_FallingObjectsUpdateOAM(i, movement);
        sMoveAnimFallingMove[i] = movement;
    }

    MoveAnim_DelayFramesAsm(3u);
    if (sMoveAnimFallingCount > 0u && wShadowOAM[0].y == 104u) {
        ctx->se_phase = 0u;
        return 1;
    }
    return 0;
}

static int MoveAnim_SE_PetalsFallingStep(move_anim_ctx_t *ctx) {
    uint8_t i;
    if (!ctx) return 1;

    if (ctx->se_phase == 0u) {
        MoveAnim_SE_FallingObjectsInit(20u, 0x71u);
        ctx->se_phase = 1u;
    }

    for (i = 0u; i < sMoveAnimFallingCount; i++) {
        uint8_t movement = sMoveAnimFallingMove[i];
        movement = MoveAnim_SE_FallingObjectsUpdateMovementByte(movement);
        MoveAnim_SE_FallingObjectsUpdateOAM(i, movement);
        sMoveAnimFallingMove[i] = movement;
    }

    MoveAnim_DelayFramesAsm(3u);
    if (sMoveAnimFallingCount > 0u && wShadowOAM[0].y == 104u) {
        MoveAnim_ClearAnimationOAM();
        ctx->se_phase = 0u;
        return 1;
    }
    return 0;
}

static void MoveAnim_SE_SquishCopyRowLeft(uint8_t row, uint8_t start_col) {
    uint8_t k;
    if (row >= SCREEN_HEIGHT) return;
    for (k = 0u; k < 3u; k++) {
        uint8_t src_col = (uint8_t)(start_col + k);
        uint8_t dst_col;
        if (src_col >= SCREEN_WIDTH) continue;
        if (src_col == 0u) continue;
        dst_col = (uint8_t)(src_col - 1u);
        wTileMap[(uint16_t)row * SCREEN_WIDTH + dst_col] =
            wTileMap[(uint16_t)row * SCREEN_WIDTH + src_col];
    }
    if ((uint8_t)(start_col + 2u) < SCREEN_WIDTH) {
        wTileMap[(uint16_t)row * SCREEN_WIDTH + (uint8_t)(start_col + 2u)] = (uint8_t)' ';
    }
}

static void MoveAnim_SE_SquishCopyRowRight(uint8_t row, uint8_t start_col) {
    int8_t k;
    if (row >= SCREEN_HEIGHT) return;
    for (k = 0; k < 3; k++) {
        int16_t src_col = (int16_t)start_col - k;
        int16_t dst_col = src_col + 1;
        if (src_col < 0 || src_col >= SCREEN_WIDTH) continue;
        if (dst_col < 0 || dst_col >= SCREEN_WIDTH) continue;
        wTileMap[(uint16_t)row * SCREEN_WIDTH + (uint16_t)dst_col] =
            wTileMap[(uint16_t)row * SCREEN_WIDTH + (uint16_t)src_col];
    }
    if (start_col >= 2u) {
        wTileMap[(uint16_t)row * SCREEN_WIDTH + (uint8_t)(start_col - 2u)] = (uint8_t)' ';
    }
}

static void MoveAnim_SE_SquishPass(uint8_t direction_right) {
    uint8_t row;
    uint8_t start_row = (hWhoseTurn == 0u) ? 5u : 0u;
    uint8_t left_start_col = (hWhoseTurn == 0u) ? 5u : 16u;
    uint8_t right_start_col = (hWhoseTurn == 0u) ? 3u : 14u;

    for (row = 0u; row < 7u; row++) {
        uint8_t y = (uint8_t)(start_row + row);
        if (y >= SCREEN_HEIGHT) break;
        if (!direction_right) {
            MoveAnim_SE_SquishCopyRowLeft(y, left_start_col);
        } else {
            MoveAnim_SE_SquishCopyRowRight(y, right_start_col);
        }
    }
}

static int MoveAnim_SE_SquishMonPicStep(move_anim_ctx_t *ctx) {
    if (!ctx) return 1;

    /* ASM AnimationSquishMonPic:
     * loop 4x: left-pass (Delay3), right-pass (Delay3), then hide + Delay2. */
    if (ctx->se_phase == 0u) {
        ctx->se_counter0 = 4u;
        ctx->se_index = 0u; /* 0=left, 1=right */
        ctx->se_phase = 1u;
    }

    if (ctx->se_phase == 1u) {
        MoveAnim_SE_SquishPass((uint8_t)(ctx->se_index != 0u));
        MoveAnim_DelayFramesAsm(3u);

        if (ctx->se_index == 0u) {
            ctx->se_index = 1u;
            return 0;
        }

        ctx->se_index = 0u;
        if (ctx->se_counter0 > 0u) {
            ctx->se_counter0 = (uint8_t)(ctx->se_counter0 - 1u);
        }
        if (ctx->se_counter0 == 0u) {
            ctx->se_phase = 2u;
        }
        return 0;
    }

    MoveAnim_AnimationHideMonPic(ctx);
    MoveAnim_DelayFramesAsm(2u);
    ctx->se_phase = 0u;
    return 1;
}

static void MoveAnim_SE_InitShootingBalls(void) {
    uint8_t i;
    uint8_t y = sMoveAnimShootBaseY;

    for (i = 0u; i < sMoveAnimShootBallCount && i < MAX_SPRITES; i++) {
        y = (uint8_t)(y + 8u);
        wShadowOAM[i].y = y;
        wShadowOAM[i].x = sMoveAnimShootBaseX;
        wShadowOAM[i].tile = 0x7Au;
        wShadowOAM[i].flags = 0u;
    }
}

static void MoveAnim_SE_UpdateShootingBalls(move_anim_ctx_t *ctx) {
    uint8_t i;
    uint8_t top_y;

    if (!ctx) return;
    top_y = (uint8_t)(sMoveAnimShootBaseY + 8u);

    for (i = 0u; i < sMoveAnimShootBallCount && i < MAX_SPRITES; i++) {
        uint8_t y = wShadowOAM[i].y;
        if (y == top_y) {
            wShadowOAM[i].y = 0u;
            if (ctx->se_counter0 != 0u) {
                ctx->se_counter0 = (uint8_t)(ctx->se_counter0 - 1u);
            }
        } else {
            wShadowOAM[i].y = (uint8_t)(y - 4u);
        }
    }
}

static int MoveAnim_SE_ShootBallsUpwardStep(move_anim_ctx_t *ctx) {
    if (!ctx) return 1;

    if (ctx->se_phase == 0u) {
        MoveAnim_LoadTileset(ctx, 0u);
        if (hWhoseTurn == 0u) {
            sMoveAnimShootBaseY = 6u * 8u;
            sMoveAnimShootBaseX = 5u * 8u;
        } else {
            sMoveAnimShootBaseY = 0u;
            sMoveAnimShootBaseX = 16u * 8u;
        }
        sMoveAnimShootBallCount = 5u;
        sMoveAnimShootDelay = 1u;
        ctx->se_counter0 = sMoveAnimShootBallCount;
        MoveAnim_SE_InitShootingBalls();
        MoveAnim_DelayFramesAsm(1u);
        ctx->se_phase = 1u;
        return 0;
    }

    if (ctx->se_counter0 == 0u) {
        MoveAnim_AnimationCleanOAM();
        ctx->se_phase = 0u;
        return 1;
    }

    MoveAnim_SE_UpdateShootingBalls(ctx);
    MoveAnim_DelayFramesAsm(sMoveAnimShootDelay);
    return 0;
}

static int MoveAnim_SE_ShootManyBallsUpwardStep(move_anim_ctx_t *ctx) {
    if (!ctx) return 1;

    if (ctx->se_phase == 0u) {
        ctx->se_index = 0u;
        if (hWhoseTurn == 0u) {
            sMoveAnimShootBaseY = 0x50u;
        } else {
            sMoveAnimShootBaseY = 0x28u;
        }
        ctx->se_phase = 1u;
    }

    /* Mirror ASM caller loop: when one pillar finishes, start the next one
     * immediately in the same script step (no extra frame gap). */
    while (1) {
        if (ctx->se_phase == 1u) {
            uint8_t x = (hWhoseTurn == 0u)
                ? sMoveAnimUpwardBallsXPlayerTurn[ctx->se_index]
                : sMoveAnimUpwardBallsXEnemyTurn[ctx->se_index];
            if (x == 0xFFu) {
                MoveAnim_AnimationCleanOAM();
                ctx->se_phase = 0u;
                return 1;
            }
            MoveAnim_LoadTileset(ctx, 0u);
            sMoveAnimShootBaseX = x;
            sMoveAnimShootBallCount = 4u;
            sMoveAnimShootDelay = 1u;
            ctx->se_counter0 = sMoveAnimShootBallCount;
            MoveAnim_SE_InitShootingBalls();
            MoveAnim_DelayFramesAsm(1u);
            ctx->se_phase = 2u;
            return 0;
        }

        if (ctx->se_counter0 == 0u) {
            ctx->se_index = (uint8_t)(ctx->se_index + 1u);
            ctx->se_phase = 1u;
            continue;
        }

        MoveAnim_SE_UpdateShootingBalls(ctx);
        MoveAnim_DelayFramesAsm(sMoveAnimShootDelay);
        if (ctx->se_counter0 != 0u) {
            return 0;
        }
        ctx->se_index = (uint8_t)(ctx->se_index + 1u);
        ctx->se_phase = 1u;
    }
}

static int MoveAnim_SE_ShakeEnemyHUDStep(move_anim_ctx_t *ctx) {
    if (!ctx) return 1;

    /* ASM timing: Delay3, then 8 cycles of (Delay2 right + Delay2 left),
     * then Delay3 restore. */
    switch (ctx->se_phase) {
        case 0u:
            sMoveAnimShakeX = 0;
            Display_SetShakeOffset(sMoveAnimShakeX, sMoveAnimShakeY);
            ctx->se_counter0 = 8u;
            ctx->se_phase = 1u;
            MoveAnim_DelayFramesAsm(3u);
            return 0;
        case 1u:
            sMoveAnimShakeX = 2;
            Display_SetShakeOffset(sMoveAnimShakeX, sMoveAnimShakeY);
            ctx->se_phase = 2u;
            MoveAnim_DelayFramesAsm(2u);
            return 0;
        case 2u:
            sMoveAnimShakeX = -2;
            Display_SetShakeOffset(sMoveAnimShakeX, sMoveAnimShakeY);
            if (ctx->se_counter0 > 0u) {
                ctx->se_counter0 = (uint8_t)(ctx->se_counter0 - 1u);
            }
            if (ctx->se_counter0 == 0u) {
                ctx->se_phase = 3u;
            } else {
                ctx->se_phase = 1u;
            }
            MoveAnim_DelayFramesAsm(2u);
            return 0;
        case 3u:
            sMoveAnimShakeX = 0;
            Display_SetShakeOffset(sMoveAnimShakeX, sMoveAnimShakeY);
            ctx->se_phase = 0u;
            MoveAnim_DelayFramesAsm(3u);
            return 1;
        default:
            ctx->se_phase = 0u;
            return 1;
    }
}

static int MoveAnim_SE_SlideMonDownAndHideStep(move_anim_ctx_t *ctx) {
    uint8_t row, col;
    if (!ctx) return 1;

    /* ASM AnimationSlideMonDownAndHide:
     * TILEMAP_SLIDE_DOWN_MON_PIC_7X5 then next tile list, each delayed 8,
     * then hide mon pic. */
    switch (ctx->se_phase) {
        case 0u:
            ctx->se_counter0 = 0u;
            ctx->se_phase = 1u;
            return 0;
        case 1u:
            if (hWhoseTurn == 0u) {
                /* Player-side self animation (Acid Armor): slide the back pic
                 * down within its 7x7 area. */
                uint8_t top = (ctx->se_counter0 == 0u) ? MOVE_ANIM_PLAYER_BG_ROW : (uint8_t)(MOVE_ANIM_PLAYER_BG_ROW + 1u);
                uint8_t bottom = (uint8_t)(MOVE_ANIM_PLAYER_BG_ROW + 6u);
                if (top > bottom) top = bottom;
                for (row = bottom; row > top; row--) {
                    for (col = 0u; col < 7u; col++) {
                        uint16_t dst = (uint16_t)row * SCREEN_WIDTH + (uint16_t)(MOVE_ANIM_PLAYER_BG_COL + col);
                        uint16_t src = (uint16_t)(row - 1u) * SCREEN_WIDTH + (uint16_t)(MOVE_ANIM_PLAYER_BG_COL + col);
                        uint16_t sdst = (uint16_t)(row + 2u) * SCROLL_MAP_W + (uint16_t)(MOVE_ANIM_PLAYER_BG_COL + col + 2u);
                        uint16_t ssrc = (uint16_t)(row + 1u) * SCROLL_MAP_W + (uint16_t)(MOVE_ANIM_PLAYER_BG_COL + col + 2u);
                        wTileMap[dst] = wTileMap[src];
                        gScrollTileMap[sdst] = gScrollTileMap[ssrc];
                    }
                }
                for (col = 0u; col < 7u; col++) {
                    uint16_t idx = (uint16_t)top * SCREEN_WIDTH + (uint16_t)(MOVE_ANIM_PLAYER_BG_COL + col);
                    uint16_t sidx = (uint16_t)(top + 2u) * SCROLL_MAP_W + (uint16_t)(MOVE_ANIM_PLAYER_BG_COL + col + 2u);
                    wTileMap[idx] = BLANK_TILE_SLOT;
                    gScrollTileMap[sidx] = BLANK_TILE_SLOT;
                }
            } else {
                /* Enemy-side self animation: keep using front-sprite OAM path. */
                MoveAnim_OffsetEnemyY((ctx->se_counter0 == 0u) ? 8 : 16);
            }
            MoveAnim_DelayFramesAsm(8u);
            ctx->se_counter0 = (uint8_t)(ctx->se_counter0 + 1u);
            if (ctx->se_counter0 >= 2u) {
                ctx->se_phase = 2u;
            }
            return 0;
        case 2u:
            if (hWhoseTurn == 0u) {
                for (row = 0u; row < 7u; row++) {
                    uint8_t y = (uint8_t)(MOVE_ANIM_PLAYER_BG_ROW + row);
                    for (col = 0u; col < 7u; col++) {
                        uint16_t idx = (uint16_t)y * SCREEN_WIDTH + (uint16_t)(MOVE_ANIM_PLAYER_BG_COL + col);
                        uint16_t sidx = (uint16_t)(y + 2u) * SCROLL_MAP_W + (uint16_t)(MOVE_ANIM_PLAYER_BG_COL + col + 2u);
                        wTileMap[idx] = BLANK_TILE_SLOT;
                        gScrollTileMap[sidx] = BLANK_TILE_SLOT;
                    }
                }
            } else {
                MoveAnim_AnimationHideMonPic(ctx);
            }
            ctx->se_phase = 0u;
            return 1;
        default:
            ctx->se_phase = 0u;
            return 1;
    }
}

static void MoveAnim_SetBGP(uint8_t bgp) {
    sMoveAnimCurrentBGP = bgp;
    Display_SetBGP(bgp);
}
