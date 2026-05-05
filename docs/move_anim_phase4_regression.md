# Move Animation Phase 4 Regression Sweep (ASM-First)

Date: 2026-05-03  
Scope: post-Phase-3 validation for known-problematic groups.

## 1) Bind / Constrict

- ASM references:
  - `engine/battle/animations.asm`
    - `AnimationBlinkMon` (6-cycle hide/show loop + `DelayFrames 5`)
    - `AnimationShowMonPic` (`Delay3`)
  - `data/moves/animations.asm`: `BindAnim`, `ConstrictAnim`
- C references:
  - `src/game/battle/move_anim.c`
    - `MoveAnim_AnimationBlinkMon`
    - `MoveAnim_AnimationShowMonPic`
    - `MoveAnim_AnimationSlideMonOff`
- Status:
  - Updated to ASM-style 6-cycle blink loop.
  - Added `Delay3` after `ShowMonPic`.
  - Updated slide-off player path to 8-column stepping parity.
  - User confirmed sprite no longer permanently vanishes; cadence improved.

## 2) Psychic / Confusion

- ASM references:
  - `data/moves/animations.asm`: `PsychicAnim`, `ConfusionAnim`
  - `engine/battle/animations.asm`: command dispatch + special effects path
- C references:
  - `src/data/move_anim_scripts.c`
  - `src/game/battle/move_anim.c`
- Status:
  - No new delta found in this pass.
  - Keep as verification target after renderer parity changes.

## 3) Swift / Projectile Class

- ASM references:
  - `data/moves/animations.asm`: `SwiftAnim`
  - `engine/battle/animations.asm`: frameblock draw + clean cadence
- C references:
  - `src/data/move_anim_scripts.c`
  - `src/game/battle/move_anim.c`
- Status:
  - Previously fixed persistent projectile residue.
  - No new parity delta found in this pass.

## 4) Bone Club / Bonemerang (SFX + HP timing coupling)

- ASM references:
  - `data/moves/animations.asm`: `BoneClubAnim`, `BonemerangAnim`
  - `engine/battle/animations.asm`: `PlayApplyingAttackAnimation`, sound call timing
- C references:
  - `src/game/battle/move_anim.c`
  - `src/data/move_sfx_data.c`
- Status:
  - No code delta applied in this pass.
  - Keep this group in active validation due known coupling sensitivity.

## Current Timing Policy

- `MoveAnim_ConvertAsmFramesToEngineTicks` is strict literal mapping now:
  - `1 ASM frame == 1 engine tick`
- Any timing change must be justified by ASM call-site parity, not feel-based tuning.

## Test Pass Checklist

- Constrict/Bind:
  - Blink clearly visible (not one-frame mush).
  - Enemy sprite returns correctly after sequence.
- Psychic/Confusion:
  - Distortion + flash ordering preserved.
  - No stuck palette or missing sprite restore.
- Swift:
  - Full projectile sequence renders.
  - No lingering star/object.
- Bonemerang/Bone Club:
  - Correct SFX chain.
  - HP bar progression timing follows ASM cadence.
