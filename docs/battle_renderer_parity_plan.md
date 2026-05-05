# Battle Renderer Parity Plan (ASM-First)

Source of truth: `pokered-master` ASM only.
Execution rule: each phase begins with the atlas diff checklist.

## Goal
Achieve battle scene/render semantics parity so move animations (including Constrict-style blink cadence) behave correctly by design, without per-move hacks.

## Phase 1: Ownership Split (Foundational)

### Objective
Separate persistent battle scene rendering ownership from transient move-animation OAM ownership.

### ASM references
- `engine/battle/core.asm`: `InitBattleCommon`, `_InitBattleCommon`, `CopyUncompressedPicToTilemap`, `LoadMonBackPic`
- `engine/battle/animations.asm`: `AnimationCleanOAM`, `DrawFrameBlock`

### C targets
- `src/game/battle/battle_ui.c`
- `src/game/battle/move_anim.c`
- `src/game/battle/battle_ui.h` (if API bridge needed)

### Deliverables
- Introduce explicit battle-scene sprite ownership helpers in `battle_ui.c`.
- Ensure move animation cleanup can clear animation OAM without corrupting persistent scene state.
- Remove implicit dependency on preserving arbitrary enemy OAM ranges in move-anim cleanup.

### Exit criteria
- No persistent sprite disappearance after move animations.
- No background/OAM coupling artifacts during animation cleanup.

## Phase 2: Move Animation Tick/Cleanup Cadence Parity

### Objective
Match `DrawFrameBlock` + `AnimationCleanOAM` timing/cadence exactly.

### ASM references
- `engine/battle/animations.asm`: `DrawFrameBlock`, `PlaySubanimation`, `AnimationCleanOAM`

### C targets
- `src/game/battle/move_anim.c`

### Deliverables
- Verify one DelayFrame/cleanup cadence at the right points in tick progression.
- Ensure clear-state is presented for the intended frame windows.

### Exit criteria
- Constrict/Bind visible multi-rotation blink cadence matches reference behavior.

## Phase 3: Enemy/Player Visibility and Offset Effects Parity

### Objective
Map hide/show/slide/bink effects to the corrected ownership model.

### ASM references
- `engine/battle/animations.asm`: mon pic hide/show/slide effects

### C targets
- `src/game/battle/move_anim.c`
- `src/game/battle/battle_ui.c`

### Deliverables
- Rebind enemy visibility/offset helpers to correct scene layer model.
- Ensure no delayed sprite return artifacts.

### Exit criteria
- Acid Armor and related hide/show effects restore at correct frame boundaries.

## Phase 4: Regression Sweep by Move Group

### Objective
Validate known-problematic groups after architecture changes.

### Move groups
- Bind/Constrict
- Psychic/Confusion
- Swift/projectile class
- Bone Club/Bonemerang + SFX/HP timing coupling

### Deliverables
- Per-group asm-vs-c verification notes.
- Fix only parity deltas confirmed by asm diff.

## Safety Rules
- No behavioral changes without mapped asm reference.
- No timing "feels right" tweaks without protocol and asm call-site evidence.
- Update `docs/asm_c_battle_animation_atlas.md` whenever new mapping is discovered.
