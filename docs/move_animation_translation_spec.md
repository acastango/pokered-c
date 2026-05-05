# Move Animation Translation Spec (ASM -> C)

## Goal
Build the move-animation system in C as a direct behavioral mirror of `pokered-master`, with ASM as source of truth and no creative reinterpretation.

This document is intentionally split into two passes:

1. ASM system inventory and execution model.
2. C-side contract (types, function names, state fields, and file layout) that maps 1:1 to ASM behavior.

---

## Source Of Truth (ASM)

- `C:/Users/Anthony/pokered/pokered-master/engine/battle/animations.asm`
- `C:/Users/Anthony/pokered/pokered-master/data/moves/animations.asm`
- `C:/Users/Anthony/pokered/pokered-master/data/battle_anims/special_effect_pointers.asm`
- `C:/Users/Anthony/pokered/pokered-master/data/battle_anims/subanimations.asm`
- `C:/Users/Anthony/pokered/pokered-master/data/battle_anims/frame_blocks.asm`
- `C:/Users/Anthony/pokered/pokered-master/data/battle_anims/base_coords.asm`
- `C:/Users/Anthony/pokered/pokered-master/constants/move_animation_constants.asm`

---

## Pass 1: ASM System Inventory

## 1) Top-Level Animation Pipeline

Top-level routine is `MoveAnimation` in `engine/battle/animations.asm`.

High-level order:

1. `WaitForSoundToFinish`
2. `SetAnimationPalette`
3. early out for `wAnimationID == 0`
4. if toss animation, route to `TossBallAnimation`
5. if battle animations enabled:
6. `ShareMoveAnimations`
7. `PlayAnimation`
8. `PlayApplyingAttackAnimation`
9. cleanup/reset control vars
10. `WaitForSoundToFinish`

Core command-stream interpreter is `PlayAnimation`:

1. resolve `wAnimationID` through `AttackAnimationPointers`
2. read command bytes until `-1`
3. if command byte `< FIRST_SE_ID`, treat as subanimation command
4. if command byte `>= FIRST_SE_ID`, treat as special effect command
5. optional sound playback through `GetMoveSound`/`PlaySound`
6. execute effect routine or `LoadSubanimation + PlaySubanimation`

---

## 2) Command Encoding

From macro in `data/moves/animations.asm`:

- Subanimation command form (`battle_anim SOUND, SUBANIM, TILESET, DELAY`):
  - byte0: `(TILESET << 6) | DELAY`
  - byte1: `SOUND - 1`
  - byte2: `SUBANIM`
- Special effect command form (`battle_anim SOUND, SE_*`):
  - byte0: `SE_*` id (`>= FIRST_SE_ID`)
  - byte1: `SOUND - 1`
- Terminator:
  - `db -1`

---

## 3) Special Effect Pointer Table (ASM)

From `data/battle_anims/special_effect_pointers.asm`:

| SE ID | ASM Routine |
|---|---|
| SE_DARK_SCREEN_FLASH | AnimationFlashScreen |
| SE_DARK_SCREEN_PALETTE | AnimationDarkScreenPalette |
| SE_RESET_SCREEN_PALETTE | AnimationResetScreenPalette |
| SE_SHAKE_SCREEN | AnimationShakeScreen |
| SE_WATER_DROPLETS_EVERYWHERE | AnimationWaterDropletsEverywhere |
| SE_DARKEN_MON_PALETTE | AnimationDarkenMonPalette |
| SE_FLASH_SCREEN_LONG | AnimationFlashScreenLong |
| SE_SLIDE_MON_UP | AnimationSlideMonUp |
| SE_SLIDE_MON_DOWN | AnimationSlideMonDown |
| SE_FLASH_MON_PIC | AnimationFlashMonPic |
| SE_SLIDE_MON_OFF | AnimationSlideMonOff |
| SE_BLINK_MON | AnimationBlinkMon |
| SE_MOVE_MON_HORIZONTALLY | AnimationMoveMonHorizontally |
| SE_RESET_MON_POSITION | AnimationResetMonPosition |
| SE_LIGHT_SCREEN_PALETTE | AnimationLightScreenPalette |
| SE_HIDE_MON_PIC | AnimationHideMonPic |
| SE_SQUISH_MON_PIC | AnimationSquishMonPic |
| SE_SHOOT_BALLS_UPWARD | AnimationShootBallsUpward |
| SE_SHOOT_MANY_BALLS_UPWARD | AnimationShootManyBallsUpward |
| SE_BOUNCE_UP_AND_DOWN | AnimationBoundUpAndDown |
| SE_MINIMIZE_MON | AnimationMinimizeMon |
| SE_SLIDE_MON_DOWN_AND_HIDE | AnimationSlideMonDownAndHide |
| SE_TRANSFORM_MON | AnimationTransformMon |
| SE_LEAVES_FALLING | AnimationLeavesFalling |
| SE_PETALS_FALLING | AnimationPetalsFalling |
| SE_SLIDE_MON_HALF_OFF | AnimationSlideMonHalfOff |
| SE_SHAKE_ENEMY_HUD | AnimationShakeEnemyHUD |
| SE_SHAKE_ENEMY_HUD_2 | AnimationShakeEnemyHUD |
| SE_SPIRAL_BALLS_INWARD | AnimationSpiralBallsInward |
| SE_DELAY_ANIMATION_10 | AnimationDelay10 |
| SE_FLASH_ENEMY_MON_PIC | AnimationFlashEnemyMonPic |
| SE_HIDE_ENEMY_MON_PIC | AnimationHideEnemyMonPic |
| SE_BLINK_ENEMY_MON | AnimationBlinkEnemyMon |
| SE_SHOW_MON_PIC | AnimationShowMonPic |
| SE_SHOW_ENEMY_MON_PIC | AnimationShowEnemyMonPic |
| SE_SLIDE_ENEMY_MON_OFF | AnimationSlideEnemyMonOff |
| SE_SHAKE_BACK_AND_FORTH | AnimationShakeBackAndForth |
| SE_SUBSTITUTE_MON | AnimationSubstitute |
| SE_WAVY_SCREEN | AnimationWavyScreen |

---

## 4) Subanimation System (ASM)

Subanimation pointer table: `SubanimationPointers` in `data/battle_anims/subanimations.asm`.

Subanimation header byte format:

- upper 3 bits: subanimation type
- lower 5 bits: number of frame-block entries

Each frame-block entry is exactly 3 bytes:

1. frame block id (`FRAMEBLOCK_*`)
2. base coord id (`BASECOORD_*`)
3. frame block mode (`FRAMEBLOCKMODE_*`)

Transform handling path:

- `LoadSubanimation`
- `GetSubanimationTransform1`
- `GetSubanimationTransform2`
- `DrawFrameBlock` transform branches:
  - normal
  - `SUBANIMTYPE_HVFLIP`
  - `SUBANIMTYPE_HFLIP`
  - `SUBANIMTYPE_COORDFLIP`
  - reverse stepping logic

Critical behavior notes to preserve:

- tile base offset `+ $31` for battle anim tiles
- `FRAMEBLOCKMODE_02/03/04` influence delay, OAM cleanup, and destination advancing
- special per-animation hooks executed after frame block draw:
  - `DoSpecialEffectByAnimationId`

---

## 5) Subanimation Pointer Inventory (ASM Names)

From `SubanimationPointers` table:

```text
Subanim_0Star
Subanim_0StarTwice
Subanim_0StarThrice
Subanim_0StarDescending
Subanim_1StarBigMoving
Subanim_1StarBig
Subanim_0BallTossHigh
Subanim_0BallTossMiddle
Subanim_0BallTossLow
Subanim_0BallShakeEnemy
Subanim_0BallPoofEnemy
Subanim_0BallBlock
Subanim_1FlameColumn1
Subanim_1FlameColumn2
Subanim_1FlameColumn3
Subanim_0Scratches
Subanim_1Tornado
Subanim_1Flames
Subanim_0Heart_1Music
Subanim_1BlobToss
Subanim_1BlobDripEnemy
Subanim_1Shout
Subanim_0Slice
Subanim_0BirdiesCirclingEnemy
Subanim_1SwordsCircling
Subanim_1CloudToss
Subanim_0WaterColumns
Subanim_1SeedToss
Subanim_1SeedLand
Subanim_0RocksLift
Subanim_0RocksToss
Subanim_1FlameBeam
Subanim_1FlameStar
Subanim_0Circles_1Squares_CenteringEnemy
Subanim_0Circle_1Square_TossBack
Subanim_0Bind
Subanim_0StatusParalyzed
Subanim_0StatusConfused
Subanim_0StatusConfusedEnemy
Subanim_0StatusPoisoned
Subanim_1Sand
Subanim_1LightningBall
Subanim_0SliceBothSides
Subanim_1Lightning
Subanim_0WaterDroplets
Subanim_0CirclesCentering
Subanim_0Beam
Subanim_0IceRise
Subanim_0RocksFallEnemy
Subanim_0SoundWave
Subanim_0Circle_1Square_HalfToss
Subanim_1Barrier
Subanim_1Selfdestruct
Subanim_0WaterBubbles
Subanim_0CirclesFalling
Subanim_0StringShot
Subanim_0IceFall
Subanim_0Circle_1Square_Appears
Subanim_0StatusSleep
Subanim_0StatusSleepEnemy
Subanim_0Water_1Fire_Barrier
Subanim_0Water_1Fire_Geyser
Subanim_1StarBigToss
Subanim_1StarsSmallToss
Subanim_1MusicCirclingEnemy
Subanim_1CircleBlackToss
Subanim_1ExplosionSmallEnemy
Subanim_0Circle_1Square_Closing
Subanim_1LeavesToss
Subanim_0HornJabTwice
Subanim_0HornJabThrice
Subanim_0BallPoof
Subanim_2TradeBallDrop
Subanim_2TradeBallShake
Subanim_2TradeBallAppear
Subanim_2TradeBallPoof
Subanim_0EggShaking
Subanim_1TriangleToss
Subanim_1SphereBig
Subanim_1SphereBigRise
Subanim_1SphereBigFall
Subanim_0Shell
Subanim_0CoinBounce
Subanim_0SafariRock
Subanim_0SafariBait
Subanim_0StarHigh
```

---

## Pass 2: C Port Contract

## 6) Proposed C File Layout

- `src/game/battle/move_anim.h`
- `src/game/battle/move_anim.c`
- `src/data/move_anim_scripts.h` (generated or hand-authored data)
- `src/data/move_anim_subanims.h` (generated from ASM tables)
- `src/data/move_anim_frameblocks.h` (generated from ASM tables)
- `src/data/move_anim_basecoords.h` (generated from ASM tables)

---

## 7) Proposed C Runtime Types

```c
typedef struct {
    uint8_t cmd0;  // subanim header or SE id
    uint8_t cmd1;  // sound id (NO_MOVE-1 means none)
    uint8_t cmd2;  // subanim id when cmd0 < FIRST_SE_ID
} move_anim_cmd_t;

typedef struct {
    uint8_t frameblock_id;
    uint8_t basecoord_id;
    uint8_t mode;
} subanim_entry_t;

typedef struct {
    uint8_t type;         // SUBANIMTYPE_*
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
    uint8_t subanim_transform;
    uint16_t subanim_entry_index;
    uint8_t fb_mode;
    uint8_t base_x;
    uint8_t base_y;
    uint16_t fb_dest_oam_index;
} move_anim_ctx_t;
```

---

## 8) ASM Variable To C Field Mapping

- `wAnimationID` -> `ctx.animation_id`
- `wAnimationType` -> `ctx.animation_type`
- `wAnimSoundID` -> `ctx.anim_sound_id`
- `wWhichBattleAnimTileset` -> `ctx.which_tileset`
- `wSubAnimFrameDelay` -> `ctx.subanim_frame_delay`
- `wSubAnimCounter` -> `ctx.subanim_counter`
- `wSubAnimTransform` -> `ctx.subanim_transform`
- `wSubAnimSubEntryAddr` -> `ctx.subanim_entry_index`
- `wFBMode` -> `ctx.fb_mode`
- `wBaseCoordX` -> `ctx.base_x`
- `wBaseCoordY` -> `ctx.base_y`
- `wFBDestAddr` -> `ctx.fb_dest_oam_index`

---

## 9) Proposed C Function Surface (Mirrored Names)

- `void MoveAnim_Run(move_anim_ctx_t *ctx);`
- `static void MoveAnim_PlayAnimation(move_anim_ctx_t *ctx);`
- `static void MoveAnim_PlaySubanimation(move_anim_ctx_t *ctx);`
- `static void MoveAnim_LoadSubanimation(move_anim_ctx_t *ctx, uint8_t subanim_id);`
- `static void MoveAnim_DrawFrameBlock(move_anim_ctx_t *ctx, uint8_t frameblock_id);`
- `static void MoveAnim_LoadTileset(move_anim_ctx_t *ctx, uint8_t tileset_id);`
- `static void MoveAnim_DoSpecialEffectByAnimationId(move_anim_ctx_t *ctx);`
- `static void MoveAnim_PlayApplyingAttackAnimation(move_anim_ctx_t *ctx);`
- `static void MoveAnim_SetAnimationPalette(move_anim_ctx_t *ctx);`
- `static void MoveAnim_ShareMoveAnimations(move_anim_ctx_t *ctx);`

SE dispatch function contract:

- `static void MoveAnim_RunSpecialEffect(move_anim_ctx_t *ctx, uint8_t se_id);`

One C function per ASM special effect routine (same names with `MoveAnim_` prefix), for example:

- `MoveAnim_AnimationShakeScreen`
- `MoveAnim_AnimationSlideMonUp`
- `MoveAnim_AnimationSubstitute`
- `MoveAnim_AnimationWavyScreen`

---

## 10) Implementation Constraints

- Preserve ASM timing semantics (`DelayFrames`) using conversion protocol in `docs/timing_conversion_protocol.md`.
- Preserve OAM write order and transform branch behavior from `DrawFrameBlock`.
- Preserve SE/subanim command ordering and early exits.
- Preserve enemy/player transform rules:
  - `GetSubanimationTransform1`
  - `GetSubanimationTransform2`
- Preserve special per-animation post-draw hooks (`DoSpecialEffectByAnimationId`).

---

## 11) Next Turn Implementation Plan

1. Create `move_anim.h/.c` runtime shell with `move_anim_ctx_t`.
2. Implement interpreter skeleton:
3. command decode
4. SE dispatch
5. subanim dispatch
6. Implement `DrawFrameBlock` with exact transform/mode branches.
7. Add data loaders for:
8. `SpecialEffectPointers`
9. `SubanimationPointers`
10. `FrameBlockPointers`
11. `FrameBlockBaseCoords`
12. Implement highest-frequency SE first:
13. `SE_DARK_SCREEN_FLASH`
14. `SE_RESET_SCREEN_PALETTE`
15. `SE_DELAY_ANIMATION_10`
16. `SE_LIGHT_SCREEN_PALETTE`
17. Implement highest-frequency subanims first:
18. `SUBANIM_1_STAR_BIG_MOVING`
19. `SUBANIM_0_STAR_TWICE`
20. `SUBANIM_0_STAR_THRICE`
21. `SUBANIM_0_BIND`

This gives fast functional coverage while keeping the system mirrored to ASM at inception.

---

## 12) Implementation Checklist

## Phase A: Scaffolding

- [X] Create `src/game/battle/move_anim.h`.
- [X] Create `src/game/battle/move_anim.c`.
- [X] Define `move_anim_ctx_t` and command/data structs exactly as in this spec.
- [X] Add public entrypoint `MoveAnim_Run`.
- [X] Wire build includes for new files.

## Phase B: Data Plumbing (ASM mirrored)

- [X] Add C data table for `AttackAnimationPointers` command streams.
- [X] Add C data table for `SpecialEffectPointers` (`SE_* -> function`).
- [X] Add C data table for `SubanimationPointers`.
- [X] Add C data table for `FrameBlockPointers`.
- [X] Add C data table for `FrameBlockBaseCoords`.
- [X] Add C data table for move animation tileset pointers/counts.

## Phase C: Interpreter Core

- [X] Implement `MoveAnim_PlayAnimation` command loop (`-1` terminated).
- [X] Implement subanim command decode (`cmd0 < FIRST_SE_ID`).
- [X] Implement SE command decode (`cmd0 >= FIRST_SE_ID`).
- [X] Implement per-command sound hookup (`GetMoveSound` equivalent path).
- [X] Implement `MoveAnim_ShareMoveAnimations` enemy-turn substitutions.

## Phase D: Subanimation Runtime

- [X] Implement `MoveAnim_LoadSubanimation`.
- [X] Implement transform selection logic equivalent to:
- [X] `GetSubanimationTransform1`
- [X] `GetSubanimationTransform2`
- [X] Implement `MoveAnim_DrawFrameBlock` with all transform branches:
- [X] normal
- [X] `SUBANIMTYPE_HVFLIP`
- [X] `SUBANIMTYPE_HFLIP`
- [X] `SUBANIMTYPE_COORDFLIP`
- [X] reverse stepping
- [X] Implement frame-block mode behavior:
- [X] `FRAMEBLOCKMODE_02`
- [X] `FRAMEBLOCKMODE_03`
- [X] `FRAMEBLOCKMODE_04`
- [X] Implement `DoSpecialEffectByAnimationId` hook path after each frame block.

## Phase E: Special Effects (SE_*)

- [X] Implement SE dispatcher `MoveAnim_RunSpecialEffect`.
- [ ] Implement all `SE_*` handlers with mirrored names.
- [X] Mark and preserve ASM-flagged unused effects without deleting them.
- [X] Verify exact timing behavior for `SE_DELAY_ANIMATION_10`.

## Phase F: High-Coverage First (frequency-based)

- [X] Implement and verify high-frequency SE first:
- [X] `SE_DARK_SCREEN_FLASH`
- [X] `SE_RESET_SCREEN_PALETTE`
- [X] `SE_DELAY_ANIMATION_10`
- [X] `SE_LIGHT_SCREEN_PALETTE`
- [X] `SE_DARK_SCREEN_PALETTE`
- [X] Implement and verify high-frequency subanims first:
- [X] `SUBANIM_1_STAR_BIG_MOVING`
- [X] `SUBANIM_0_STAR_TWICE`
- [X] `SUBANIM_0_STAR_THRICE`
- [X] `SUBANIM_0_BIND`
- [X] `SUBANIM_1_STAR_BIG`

## Phase G: Integration

- [X] Route battle move animation calls through new runtime.
- [X] Preserve toss/ball/status/trade animation branches.
- [X] Preserve animation-disable option behavior.
- [X] Preserve applying-attack animation phase behavior.

## Phase H: Verification

- [ ] Script-by-script compare against ASM command streams.
- [ ] Validate transforms on player and enemy turns.
- [ ] Validate OAM cleanup behavior between frame blocks.
- [ ] Validate palette transitions and reset paths.
- [ ] Validate key move samples per archetype:
- [ ] projectile (`Ember`, `WaterGun`)
- [ ] contact (`Tackle`, `BodySlam`)
- [ ] screen/palette (`Leer`, `Confusion`)
- [ ] mon-transform (`Transform`, `Minimize`, `Substitute`)
- [ ] heavy FX (`Earthquake`, `Explosion`, `Blizzard`)

## Tracking Rule

- [X] For every completed checklist item, link the implementing commit or file/line proof in this section.

Completed item proofs:

- [X] Phase A / Create `src/game/battle/move_anim.h`:
  `src/game/battle/move_anim.h:1`
- [X] Phase A / Create `src/game/battle/move_anim.c`:
  `src/game/battle/move_anim.c:1`
- [X] Phase A / Define `move_anim_ctx_t` and command/data structs exactly as in this spec:
  `src/game/battle/move_anim.h:11`
  `src/game/battle/move_anim.h:17`
  `src/game/battle/move_anim.h:23`
  `src/game/battle/move_anim.h:29`
- [X] Phase A / Add public entrypoint `MoveAnim_Run`:
  `src/game/battle/move_anim.h:45`
  `src/game/battle/move_anim.c:95`
- [X] Phase A / Wire build includes for new files:
  `CMakeLists.txt:79`
  `CMakeLists.txt:204`
- [X] Phase B / Add C data table for `AttackAnimationPointers` command streams:
  `src/data/move_anim_scripts.c:207`
- [X] Phase B / Add C data table for `SpecialEffectPointers` (`SE_* -> function`):
  `src/data/move_anim_scripts.c:413`
- [X] Phase B / Add C data table for `SubanimationPointers`:
  `src/data/move_anim_subanims.c:946`
- [X] Phase B / Add C data table for `FrameBlockPointers`:
  `src/data/move_anim_frameblocks.c:1234`
- [X] Phase B / Add C data table for `FrameBlockBaseCoords`:
  `src/data/move_anim_basecoords.c:3`
- [X] Phase B / Add C data table for move animation tileset pointers/counts:
  `src/data/move_anim_scripts.c:455`
  `src/data/move_anim_scripts.c:456`
- [X] Phase C / Implement `MoveAnim_PlayAnimation` command loop (`-1` terminated):
  `src/game/battle/move_anim.c:106`
  `src/game/battle/move_anim.c:121`
- [X] Phase C / Implement subanim command decode (`cmd0 < FIRST_SE_ID`):
  `src/game/battle/move_anim.c:124`
  `src/game/battle/move_anim.c:137`
- [X] Phase C / Implement SE command decode (`cmd0 >= FIRST_SE_ID`):
  `src/game/battle/move_anim.c:143`
  `src/game/battle/move_anim.c:144`
- [X] Phase C / Implement per-command sound hookup (`GetMoveSound` equivalent path):
  `src/game/battle/move_anim.c:137`
  `src/game/battle/move_anim.c:143`
  `src/game/battle/move_anim.c:437`
- [X] Phase C / Implement `MoveAnim_ShareMoveAnimations` enemy-turn substitutions:
  `src/game/battle/move_anim.c:320`
  `src/game/battle/move_anim.c:326`
  `src/game/battle/move_anim.c:330`
- [X] Phase D / Implement `MoveAnim_LoadSubanimation`:
  `src/game/battle/move_anim.c:190`
  `src/game/battle/move_anim.c:219`
- [X] Phase D / Implement transform selection logic equivalent to `GetSubanimationTransform1`:
  `src/game/battle/move_anim.c:214`
  `src/game/battle/move_anim.c:446`
- [X] Phase D / Implement transform selection logic equivalent to `GetSubanimationTransform2`:
  `src/game/battle/move_anim.c:211`
  `src/game/battle/move_anim.c:451`
- [X] Phase D / Implement `MoveAnim_DrawFrameBlock` normal branch:
  `src/game/battle/move_anim.c:226`
  `src/game/battle/move_anim.c:268`
- [X] Phase D / Implement `MoveAnim_DrawFrameBlock` `SUBANIMTYPE_HVFLIP` branch:
  `src/game/battle/move_anim.c:248`
- [X] Phase D / Implement `MoveAnim_DrawFrameBlock` `SUBANIMTYPE_HFLIP` branch:
  `src/game/battle/move_anim.c:256`
- [X] Phase D / Implement `MoveAnim_DrawFrameBlock` `SUBANIMTYPE_COORDFLIP` branch:
  `src/game/battle/move_anim.c:263`
- [X] Phase D / Implement reverse stepping:
  `src/game/battle/move_anim.c:182`
  `src/game/battle/move_anim.c:219`
- [X] Phase D / Implement frame-block mode `FRAMEBLOCKMODE_02`:
  `src/game/battle/move_anim.c:284`
- [X] Phase D / Implement frame-block mode `FRAMEBLOCKMODE_03`:
  `src/game/battle/move_anim.c:291`
- [X] Phase D / Implement frame-block mode `FRAMEBLOCKMODE_04`:
  `src/game/battle/move_anim.c:295`
- [X] Phase D / Implement `DoSpecialEffectByAnimationId` hook path after each frame block:
  `src/game/battle/move_anim.c:175`
- [X] Phase E / Implement SE dispatcher `MoveAnim_RunSpecialEffect`:
  `src/game/battle/move_anim.c:335`
  `src/game/battle/move_anim.c:339`
- [ ] Phase E / Implement all `SE_*` handlers with mirrored names:
  `src/game/battle/move_anim.c:387`
  `src/game/battle/move_anim.c:423`
- [X] Phase E / Mark and preserve ASM-flagged unused effects:
  `src/game/battle/move_anim.c:367`
  `src/game/battle/move_anim.c:370`
- [X] Phase E / Verify exact timing behavior for `SE_DELAY_ANIMATION_10`:
  `src/game/battle/move_anim.c:425`
  `src/game/battle/move_anim.c:428`
- [X] Phase F / Implement and verify high-frequency `SE_DARK_SCREEN_FLASH`:
  `src/game/battle/move_anim.c:427`
  `tests/test_move_anim.c:28`
- [X] Phase F / Implement and verify high-frequency `SE_RESET_SCREEN_PALETTE`:
  `src/game/battle/move_anim.c:444`
  `tests/test_move_anim.c:28`
- [X] Phase F / Implement and verify high-frequency `SE_DELAY_ANIMATION_10`:
  `src/game/battle/move_anim.c:454`
  `tests/test_move_anim.c:57`
- [X] Phase F / Implement and verify high-frequency `SE_LIGHT_SCREEN_PALETTE`:
  `src/game/battle/move_anim.c:449`
  `tests/test_move_anim.c:44`
- [X] Phase F / Implement and verify high-frequency `SE_DARK_SCREEN_PALETTE`:
  `src/game/battle/move_anim.c:439`
  `tests/test_move_anim.c:28`
- [X] Phase F / Implement and verify high-frequency `SUBANIM_1_STAR_BIG_MOVING`:
  `src/data/move_anim_subanims.c:29`
  `tests/test_move_anim.c:70`
- [X] Phase F / Implement and verify high-frequency `SUBANIM_0_STAR_TWICE`:
  `src/data/move_anim_subanims.c:8`
  `tests/test_move_anim.c:70`
- [X] Phase F / Implement and verify high-frequency `SUBANIM_0_STAR_THRICE`:
  `src/data/move_anim_subanims.c:14`
  `tests/test_move_anim.c:70`
- [X] Phase F / Implement and verify high-frequency `SUBANIM_0_BIND`:
  `src/data/move_anim_subanims.c:372`
  `tests/test_move_anim.c:70`
- [X] Phase F / Implement and verify high-frequency `SUBANIM_1_STAR_BIG`:
  `src/data/move_anim_subanims.c:36`
  `tests/test_move_anim.c:70`
- [X] Phase G / Route battle move animation calls through new runtime:
  `src/game/battle/battle_ui.c:1609`
  `src/game/battle/battle_ui.c:1742`
  `src/game/battle/battle_ui.c:1758`
  `src/game/battle/battle_ui.c:1924`
  `src/game/battle/battle_ui.c:1933`
  `src/game/battle/battle_ui.c:3791`
- [X] Phase G / Preserve toss/ball/status/trade animation branches:
  `src/game/battle/battle_ui.c:3410`
  `src/game/battle/battle_ui.c:3474`
  `src/game/battle/battle_ui.c:3533`
- [X] Phase G / Preserve animation-disable option behavior:
  `src/game/constants.h:44`
  `src/platform/hardware.h:288`
  `src/game/wram.c:189`
  `src/platform/save.c:98`
  `src/game/battle/move_anim.c:106`
  `tests/test_move_anim.c:104`
- [X] Phase G / Preserve applying-attack animation phase behavior:
  `src/game/battle/battle_ui.c:1654`
  `src/game/battle/battle_ui.c:2618`
