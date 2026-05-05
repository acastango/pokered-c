# Move Animation ASM Gap Report

Source of truth:
- `pokered-master/engine/battle/animations.asm`
- `src/game/battle/move_anim.c`
- `src/data/move_anim_scripts.c`

Generated on: 2026-04-25

## 1) Special Effects Still NOOP (with script usage counts)

`uses` = number of opcode occurrences in `gMoveAnimAttackAnimationPointers`.

- `0xE2` `MoveAnim_AnimationSpiralBallsInward` — uses: 11
- `0xF9` `MoveAnim_AnimationDarkenMonPalette` — uses: 9
- `0xF8` `MoveAnim_AnimationFlashScreenLong` — uses: 7
- `0xFA` `MoveAnim_AnimationWaterDropletsEverywhere` — uses: 5
- `0xEB` `MoveAnim_AnimationBoundUpAndDown` — uses: 2
- `0xDB` `MoveAnim_AnimationSlideEnemyMonOff` — uses: 2
- `0xF5` `MoveAnim_AnimationFlashMonPic` — uses: 2
- `0xE0` `MoveAnim_AnimationFlashEnemyMonPic` — uses: 1
- `0xE3` `MoveAnim_AnimationShakeEnemyHUD` (unused in ASM table comment) — uses: 1

## 2) Core ASM Hooks Remaining

- `MoveAnim_PlayCommandSound` still lacks full `GetMoveSound/PlaySound` table lookup parity.
  - Command-byte transport is in place, but original move-sound dispatch is still pending.

## 3) Implemented but Not ASM-Equivalent Yet (behavior placeholders)

These are implemented, but not in tilemap/OAM-exact ASM semantics yet:

- `0xF2` MoveMonHorizontally
- `0xF1` ResetMonPosition
- `0xF7` SlideMonUp
- `0xF6` SlideMonDown
- `0xF4` SlideMonOff
- `0xF3` BlinkMon
- `0xDA` ShakeBackAndForth
- `0xD9` Substitute (timing present, sprite construction not ASM exact)
- `0xD8` WavyScreen (timing present; scanline SCX behavior approximated)
- `0xE8` Transform (species swap path implemented; full ChangeMonPic path not yet ASM-complete)
- `0xEF/0xDD/0xDF/0xDC/0xDE` hide/show/blink pic family currently OAM-visibility centric, not full ASM tilemap semantics.

## 4) Recommended Execution Order (ASM-first)

1. `0xE2` Spiral balls inward
2. `0xF8` FlashScreenLong
3. `0xFA` WaterDropletsEverywhere
4. `0xF5/0xE0` Flash mon/enemy mon pic
5. `0xDB` SlideEnemyMonOff
6. Complete move-sound lookup (`GetMoveSound`/`PlaySound`) parity

## 5) Newly Landed in This Pass

- `0xED` `AnimationShootBallsUpward` translated as stepped runtime (ASM init/delay/update/cleanup flow).
- `0xEC` `AnimationShootManyBallsUpward` translated as stepped multi-pillar runtime with ASM x-coordinate tables.
- `0xE4`/`0xE3` `AnimationShakeEnemyHUD` no longer a NOOP; timing loop translated (Delay3 + 8 shake cycles + Delay3).
- `0xEA` `AnimationMinimizeMon` no longer a NOOP; ASM timing/visibility sequence (`Delay3` + `ShowMonPic`) translated.
- `0xE7`/`0xE6` `AnimationLeavesFalling`/`AnimationPetalsFalling` now run via stepped falling-objects engine using ASM movement-byte and delta-X tables.
- `0xEE` `AnimationSquishMonPic` now runs as a stepped 4-cycle left/right squish loop with ASM-equivalent `Delay3` cadence and final hide+delay.
- `DoSpecialEffectByAnimationId` translated from `AnimationIdSpecialEffects` for high-impact move IDs (flash/interval flash/blizzard/explode/rock slide/growl/tail whip unused path).
- `PlayApplyingAttackAnimation` translated from `wAnimationType` pointer-table behavior (type-specific shake/blink timing paths).

This order maximizes visible parity first while following ASM function dependencies.
