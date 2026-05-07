# Move DB + Effects + Atlas Diff (ASM -> C)

Date: 2026-05-03  
Method: ASM-first verification against `pokered-master`, then C tables/dispatch.

## Scope

1. Move database parity
- ASM: `pokered-master/data/moves/moves.asm`
- C: `src/data/moves_data.c`

2. Move animation script label parity
- ASM: `pokered-master/data/moves/animations.asm`
- C: `src/data/move_anim_scripts.c`

3. Move effect dispatch parity
- ASM: `pokered-master/data/moves/effects_pointers.asm`
- C: `src/game/battle/battle_effects.c` (`Battle_JumpMoveEffect`)

4. Atlas consistency
- `docs/asm_c_battle_animation_atlas.md`

## Results

## Move DB (`moves.asm` vs `moves_data.c`)

- Table shape and field mapping format are aligned (`anim/effect/power/type/accuracy/pp`).
- Accuracy values in C are expected Gen1 byte conversion (percent -> 0-255 scale), so raw numbers intentionally differ from percent literals in ASM.
- No immediate structural mismatch found in this pass.

## Move Animation Label Presence

Checked labels (requested move-work tranche + key regressions):

- `AbsorbAnim`, `AcidAnim`, `AcidArmorAnim`, `AgilityAnim`
- `AmnesiaAnim`, `AuroraBeamAnim`, `BarrageAnim`, `BarrierAnim`
- `BideAnim`, `BindAnim`, `BiteAnim`, `BlizzardAnim`
- `BodySlamAnim`, `BoneClubAnim`, `BonemerangAnim`
- `BubbleAnim`, `BubbleBeamAnim`, `ClampAnim`, `CometPunchAnim`
- `ConfuseRayAnim`, `ConfusionAnim`, `ConstrictAnim`
- `ConversionAnim`, `CounterAnim`, `PsychicAnim`, `SwiftAnim`

Status: all above were found `1:1` (exactly one label in ASM and one generated script entry in C).

## Move Effect Dispatch (`effects_pointers.asm` vs `Battle_JumpMoveEffect`)

- Dispatch families are present in C:
  - Sleep/Poison/Drain/FreezeBurnParalyze
  - Stat up/down groups
  - Multi-hit, flinch, OHKO, charge, trapping
  - Mist/focus/recoil/confusion/heal/transform
  - Reflect/light-screen, paralyze/confusion-side
  - Substitute/hyper beam/rage/mimic/leech-seed/splash/disable
- `NULL` pointer semantics in ASM are represented as no-op or handled in core paths in C (documented inline in `Battle_JumpMoveEffect`).

No missing effect-family dispatch was identified in this sweep.

## Atlas Status

- Existing atlas mappings remain valid for this phase.
- No new file-pair mapping was required; this pass was validation-oriented.

## What This Means

- Current issues are more likely runtime semantic/order/timing behavior in specific handlers than missing move/effect table entries for this tranche.
- Next diffs should focus on per-handler semantics (call order + wait timing + cleanup boundaries), not label/table existence.

## Suggested Next Diff Targets (ASM-first)

1. `engine/battle/animations.asm`:
- `PlaySubanimation`
- `DrawFrameBlock`
- `DoSpecialEffectByAnimationId`
- `PlayApplyingAttackAnimation`

2. `engine/battle/effects.asm` + move_effects includes:
- `Battle_JumpMoveEffect` call-site side effects and pre/postconditions per effect.

3. C counterparts:
- `src/game/battle/move_anim.c`
- `src/game/battle/battle_effects.c`
- `src/game/battle/battle_core.c`
