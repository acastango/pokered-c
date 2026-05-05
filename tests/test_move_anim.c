#include "test_runner.h"
#include "../src/game/battle/move_anim.h"
#include "../src/platform/hardware.h"
#include <string.h>

extern uint8_t gTestDisplayLastBGP;
extern uint8_t gTestDisplayBGPHistory[256];
extern int gTestDisplayBGPHistoryCount;

static void reset_move_anim_state(void) {
    extern void WRAMClear(void);
    WRAMClear();
    gTestDisplayLastBGP = 0;
    gTestDisplayBGPHistoryCount = 0;
    memset(gTestDisplayBGPHistory, 0, sizeof(gTestDisplayBGPHistory));
    hWhoseTurn = 0;
    hFrameCounter = 0;
}

static int history_contains(uint8_t v) {
    int i;
    for (i = 0; i < gTestDisplayBGPHistoryCount; i++) {
        if (gTestDisplayBGPHistory[i] == v) return 1;
    }
    return 0;
}

TEST(MoveAnimPhaseF, HighFreqSE_LeerAnim_DarkFlashReset) {
    move_anim_ctx_t ctx;
    reset_move_anim_state();
    memset(&ctx, 0, sizeof(ctx));

    /* animation_id=43 -> LeerAnim: FD, FE, FE, FC, -1 */
    ctx.animation_id = 43;
    MoveAnim_Run(&ctx);

    EXPECT_TRUE(history_contains(0x6F));  /* dark screen palette */
    EXPECT_TRUE(history_contains(0x1B));  /* flash invert palette */
    EXPECT_TRUE(history_contains(0x00));  /* flash white-out */
    EXPECT_EQ((int)gTestDisplayLastBGP, 0xE4); /* reset screen palette */
    EXPECT_EQ((int)hFrameCounter, 8); /* two flash cycles, 4 frames each */
}

TEST(MoveAnimPhaseF, HighFreqSE_MistAnim_LightThenReset) {
    move_anim_ctx_t ctx;
    reset_move_anim_state();
    memset(&ctx, 0, sizeof(ctx));

    /* animation_id=54 -> MistAnim: F0, FA, FC, -1 */
    ctx.animation_id = 54;
    MoveAnim_Run(&ctx);

    EXPECT_TRUE(history_contains(0x90));  /* light screen palette */
    EXPECT_EQ((int)gTestDisplayLastBGP, 0xE4); /* reset at end */
}

TEST(MoveAnimPhaseF, HighFreqSE_TailWhipAnim_Delay10Timing) {
    move_anim_ctx_t ctx;
    reset_move_anim_state();
    memset(&ctx, 0, sizeof(ctx));

    /* animation_id=39 -> TailWhipAnim: F2,E1,F1,E1,F2,E1,F1,-1 */
    ctx.animation_id = 39;
    MoveAnim_Run(&ctx);

    /* Three SE_DELAY_ANIMATION_10 commands. */
    EXPECT_EQ((int)hFrameCounter, 30);
}

TEST(MoveAnimPhaseF, HighFreqSubanims_RunThroughExpectedScripts) {
    static const struct {
        uint8_t animation_id;
        uint8_t expected_tileset;
        uint8_t expected_delay;
    } kCases[] = {
        /* SUBANIM_1_STAR_BIG_MOVING via MegaPunchAnim */
        { 5, 1, 0x06 },
        /* SUBANIM_0_STAR_TWICE via DoubleSlapAnim */
        { 3, 0, 0x05 },
        /* SUBANIM_0_STAR_THRICE via CometPunchAnim */
        { 4, 0, 0x04 },
        /* SUBANIM_0_BIND via BindAnim */
        { 20, 0, 0x04 },
        /* SUBANIM_1_STAR_BIG via StompAnim */
        { 23, 1, 0x08 },
    };
    int i;

    for (i = 0; i < (int)(sizeof(kCases) / sizeof(kCases[0])); i++) {
        move_anim_ctx_t ctx;
        reset_move_anim_state();
        memset(&ctx, 0, sizeof(ctx));

        ctx.animation_id = kCases[i].animation_id;
        MoveAnim_Run(&ctx);

        EXPECT_EQ((int)ctx.animation_id, (int)kCases[i].animation_id);
        EXPECT_EQ((int)ctx.which_tileset, (int)kCases[i].expected_tileset);
        EXPECT_EQ((int)ctx.subanim_frame_delay, (int)kCases[i].expected_delay);
        EXPECT_EQ((int)ctx.subanim_counter, 0);
    }
}

TEST(MoveAnimPhaseG, AnimationsDisabled_SkipsScriptButKeepsDelayContract) {
    move_anim_ctx_t ctx;
    reset_move_anim_state();
    memset(&ctx, 0, sizeof(ctx));

    /* wOptions bit 7 set: battle animations disabled. */
    wOptions = 0x80u;

    /* animation_id=43 -> LeerAnim (would otherwise run dark+flash+reset SEs). */
    ctx.animation_id = 43;
    MoveAnim_Run(&ctx);

    /* MoveAnimation disabled path uses DelayFrames(30) instead of PlayAnimation. */
    EXPECT_EQ((int)hFrameCounter, 30);
    EXPECT_FALSE(history_contains(0x6F)); /* no dark-screen palette SE */
    EXPECT_FALSE(history_contains(0x1B)); /* no flash-screen SE */
    EXPECT_EQ((int)gTestDisplayLastBGP, 0xE4); /* baseline palette still applied */
}

TEST(MoveAnimPhaseG, RuntimeCleanup_MatchesAsmEndState) {
    move_anim_ctx_t ctx;
    reset_move_anim_state();
    memset(&ctx, 0, sizeof(ctx));

    ctx.animation_id = 5; /* MegaPunchAnim */
    MoveAnim_Run(&ctx);

    EXPECT_EQ((int)ctx.subanim_entry_index, 0);
    EXPECT_EQ((int)ctx.subanim_transform, 0);
    EXPECT_EQ((int)ctx.anim_sound_id, 0xFF);
}

TEST(MoveAnimPhaseG, SwiftAnim_WritesProjectileOAMDuringTicks) {
    move_anim_ctx_t ctx;
    int saw_visible = 0;
    int saw_star_tile = 0;
    int guard = 0;
    reset_move_anim_state();
    memset(&ctx, 0, sizeof(ctx));

    /* animation_id=129 -> SwiftAnim: 0x43, 0x80, 0x3F, 0xFF */
    ctx.animation_id = 129;
    MoveAnim_Begin(&ctx);

    while (!MoveAnim_IsDone(&ctx) && guard < 256) {
        if (MoveAnim_Tick(&ctx)) break;

        for (int i = 0; i < MAX_SPRITES; i++) {
            if (wShadowOAM[i].y != 0u && wShadowOAM[i].y < 160u) {
                saw_visible = 1;
            }
            /* FrameBlock68 uses tile IDs 0x03/0x13 with +0x31 runtime base. */
            if (wShadowOAM[i].tile == 0x34u || wShadowOAM[i].tile == 0x44u) {
                saw_star_tile = 1;
            }
        }
        guard++;
    }

    EXPECT_TRUE(saw_visible);
    EXPECT_TRUE(saw_star_tile);
}
