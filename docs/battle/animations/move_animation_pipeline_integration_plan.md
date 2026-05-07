# Move Animation Pipeline Integration Plan (ASM-First)

## Goal
Turn the imported move-animation subsystems into the real runtime pipeline used during battle, with `pokered-master` ASM as the source of truth and no creative behavior changes.

## Scope
- Review sequence (ASM -> current C status) for the move animation pipeline.
- Gap matrix for each subsystem already imported.
- Phased implementation plan with concrete file targets and exit criteria.

## Source Of Truth
- `C:/Users/Anthony/pokered/pokered-master/engine/battle/animations.asm`
- `C:/Users/Anthony/pokered/pokered-master/data/moves/animations.asm`
- `C:/Users/Anthony/pokered/pokered-master/data/battle_anims/special_effect_pointers.asm`
- `C:/Users/Anthony/pokered/pokered-master/data/battle_anims/subanimations.asm`
- `C:/Users/Anthony/pokered/pokered-master/data/battle_anims/frame_blocks.asm`
- `C:/Users/Anthony/pokered/pokered-master/data/battle_anims/base_coords.asm`

---

## 1) Review Sequence (Repeatable)

Run this exact sequence each time we integrate more of the runtime:

1. Verify top-level flow against ASM:
- `MoveAnimation` order and branches (`animations.asm` line ~403).
- `ShareMoveAnimations` behavior (`animations.asm` line ~452).
- `PlayApplyingAttackAnimation` split (`animations.asm` line ~475).

2. Verify command interpreter:
- `PlayAnimation` command loop and terminator `-1` (`animations.asm` line ~164).
- command decode: subanim command vs `SE_*`.

3. Verify subanimation runtime:
- `LoadSubanimation` transform and counter setup (`animations.asm` line ~270).
- `PlaySubanimation` stepping (forward/reverse) (`animations.asm` line ~580).
- `DrawFrameBlock` transform branches and mode semantics (`animations.asm` line ~3).

4. Verify special-effect dispatch and handlers:
- `SpecialEffectPointers` exact ID -> routine mapping.
- `DoSpecialEffectByAnimationId` post-draw hook (`animations.asm` line ~661).

5. Verify move sound path:
- `GetMoveSound` timing and command sound behavior (`animations.asm` line ~2199).

6. Verify battle integration path:
- where battle calls runtime and where applying-attack animation currently lives.
- ensure final order remains `execute -> scripted move anim -> applying anim -> HP anim -> text` per ASM.

---

## 2) Current C Status Snapshot (As Of 2026-04-25)

### Runtime entry and interpreter
- Implemented:
  - `MoveAnim_Run` exists in `src/game/battle/move_anim.c:98`.
  - `PlayAnimation` loop exists in `src/game/battle/move_anim.c:120`.
  - `PlaySubanimation` exists in `src/game/battle/move_anim.c:162`.
  - `LoadSubanimation` exists in `src/game/battle/move_anim.c:204`.
  - `DrawFrameBlock` exists in `src/game/battle/move_anim.c:240`.

### Imported subsystem data
- Present:
  - attack animation scripts: `src/data/move_anim_scripts.*`
  - subanimations: `src/data/move_anim_subanims.*`
  - frame blocks: `src/data/move_anim_frameblocks.*`
  - base coords: `src/data/move_anim_basecoords.*`

### Integration into battle UI
- Runtime is called before applying phase:
  - `bui_run_move_anim_runtime` at `src/game/battle/battle_ui.c:1609`.
  - calls from first/second move execution paths (`battle_ui.c:1742`, `1758`, `1924`, `1933`, `3791`).
- Legacy applying/hit animation still runs in `BUI_MOVE_ANIM`:
  - `battle_ui.c:2618` (blink/shake logic via `bui_setup_anim`).

### Major stubs still in runtime
- `MoveAnim_LoadTileset` is stubbed (`move_anim.c:317`).
- `MoveAnim_DoSpecialEffectByAnimationId` is stubbed (`move_anim.c:322`).
- `MoveAnim_PlayApplyingAttackAnimation` is stubbed (`move_anim.c:326`).
- Most `SE_*` handlers are no-ops via macro block (`move_anim.c:401` to `436`).
- command sound path does not resolve/play real SFX yet (`move_anim.c:473`).

### Test coverage currently present
- Unit tests cover a small subset:
  - `tests/test_move_anim.c:28`, `:44`, `:57`, `:70`, `:104`, `:123`.
- Current tests validate select palette/delay behavior and cleanup; they do not yet validate full `SE_*`, OAM transforms across all subanims, or sound hookup.

---

## 3) Gap Matrix (Subsystem -> Pipeline Readiness)

| Subsystem | Status | Gap Blocking Full Pipeline |
|---|---|---|
| Attack animation script table | Ready | none |
| Subanimation table | Ready | none |
| Frameblock table | Ready | none |
| Basecoord table | Ready | none |
| Command interpreter | Partial | needs full command-sound semantics and edge-case parity |
| Tileset loader | Stub | animation tiles not loaded by runtime path |
| SE dispatcher map | Ready | mapped, but handlers mostly stubbed |
| SE handlers | Partial | must port real ASM behavior effect-by-effect |
| Post-draw animation-id hook | Stub | missing `DoSpecialEffectByAnimationId` behavior |
| Applying-attack phase | Split legacy/runtime | runtime stub + legacy battle_ui path still authoritative |
| Battle integration orchestration | Partial | needs explicit handoff contract so runtime owns all scripted motion/effects |
| Verification harness | Partial | needs broader script-level and visual parity checks |

---

## 4) Implementation Plan (Execution Order)

## Phase 1: Lock the pipeline contract
- Files:
  - `src/game/battle/move_anim.h`
  - `src/game/battle/move_anim.c`
  - `src/game/battle/battle_ui.c`
- Tasks:
  - define explicit ownership boundary:
    - runtime owns scripted command/subanim/SE playback.
    - `battle_ui` owns state transitions, text timing, HP bar progression.
  - preserve existing UX order while migrating internals.
- Exit criteria:
  - one documented call contract in comments near `bui_run_move_anim_runtime`.
  - no duplicated responsibility ambiguity.

## Phase 2: Implement runtime missing hooks
- Files:
  - `src/game/battle/move_anim.c`
- Tasks:
  - implement `MoveAnim_LoadTileset`.
  - implement `MoveAnim_DoSpecialEffectByAnimationId`.
  - implement real `MoveAnim_PlayCommandSound` (`GetMoveSound` equivalent behavior).
  - implement runtime-side `MoveAnim_PlayApplyingAttackAnimation` or explicitly keep it in `battle_ui` with exact parity docs.
- Exit criteria:
  - no stub bodies for these four core functions.
  - unit tests for each hook path.

## Phase 3: Port `SE_*` handlers by ASM priority
- Files:
  - `src/game/battle/move_anim.c`
  - `tests/test_move_anim.c`
- Tasks:
  - replace no-op `SE_*` handlers with ASM-mirrored implementations.
  - start with handlers used by highest-frequency move scripts, then complete all.
  - preserve unused entries from pointer table with explicit comments.
- Exit criteria:
  - no `MOVE_ANIM_SE_NOOP` for active handlers.
  - each implemented handler has at least one test or scripted repro case.

## Phase 4: Eliminate split-brain applying animation logic
- Files:
  - `src/game/battle/battle_ui.c`
  - `src/game/battle/move_anim.c`
- Tasks:
  - remove duplication between runtime and `BUI_MOVE_ANIM` path.
  - either:
    - A) keep `BUI_MOVE_ANIM` as authoritative and keep runtime applying stub removed by design, or
    - B) move applying behavior fully into runtime and simplify `BUI_MOVE_ANIM`.
  - choose one path and codify it with comments referencing ASM ordering.
- Exit criteria:
  - a single authoritative implementation path for applying-attack animation.
  - no double-application risk.

## Phase 5: Verification and parity checkpoints
- Files:
  - `tests/test_move_anim.c`
  - `docs/move_animation_pipeline_integration_plan.md`
- Tasks:
  - script-level parity matrix for representative archetypes:
    - projectile, contact, palette/screen, transform/minimize, heavy FX.
  - verify both turns (player and enemy transform variants).
  - verify OAM cleanup and palette reset invariants.
- Exit criteria:
  - checklist complete for selected representative moves.
  - no known visual regression against ASM reference runs.

---

## 5) Working Checklist

## Phase 1
- [ ] Pipeline ownership comments and call contract finalized.
- [ ] `battle_ui` -> runtime boundary documented with ASM references.

## Phase 2
- [ ] `MoveAnim_LoadTileset` implemented.
- [ ] `MoveAnim_DoSpecialEffectByAnimationId` implemented.
- [ ] `MoveAnim_PlayCommandSound` implemented (real lookup/playback path).
- [ ] `MoveAnim_PlayApplyingAttackAnimation` strategy finalized and implemented.

## Phase 3
- [ ] Replace remaining no-op `SE_*` handlers.
- [ ] Add tests per `SE_*` family (palette, shake, hide/show, slide, etc.).

## Phase 4
- [ ] Remove split-brain applying animation behavior.
- [ ] Keep exact order parity with ASM (`execute -> anim -> applying -> HP/text`).

## Phase 5
- [ ] Build representative move parity matrix.
- [ ] Verify player/enemy transform variants.
- [ ] Verify visual reset invariants after each animation sequence.

