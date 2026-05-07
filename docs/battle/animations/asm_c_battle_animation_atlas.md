# ASM <-> C Battle Animation Atlas

Purpose: enforce literal, repeatable translation workflow from `pokered-master` ASM to this C port.
Rule: every implementation/debug turn must start by diffing the mapped ASM source first, then the mapped C destination.

## Workflow Contract (Per Turn)
1. Identify the behavior under test (move anim, cleanup, sprite visibility, timing, SFX, etc.).
2. Open mapped ASM routine(s) and read call path and data tables.
3. Open mapped C routine(s) and compare line-by-line semantics.
4. Implement only parity changes required by ASM.
5. Rebuild and verify.
6. Record any newly discovered mapping in this atlas.

## Top-Level Battle Flow
- ASM source:
  - `pokered-master/engine/battle/core.asm`
  - `pokered-master/engine/battle/animations.asm`
- C destination:
  - `src/game/battle/battle_core.c`
  - `src/game/battle/battle_ui.c`
  - `src/game/battle/move_anim.c`

## Move Animation Command Pipeline
- ASM source:
  - `pokered-master/engine/battle/animations.asm` (`PlayAnimation`, `LoadSubanimation`, `PlaySubanimation`, `DrawFrameBlock`, `AnimationCleanOAM`)
  - `pokered-master/data/moves/animations.asm` (per-move scripts)
  - `pokered-master/data/battle_anims/subanimations.asm` (subanim entries)
- C destination:
  - `src/game/battle/move_anim.c`
  - `src/data/move_anim_scripts.c`
  - `src/data/move_anim_subanims.c`
  - `src/data/move_anim_frameblocks.c`
  - `src/data/move_anim_basecoords.c`
  - `src/data/move_anim_tiles.c`

## Timing / Delay Semantics
- ASM source:
  - `pokered-master/engine/battle/animations.asm` (`DelayFrames` call sites in frameblock + SE paths)
- C destination:
  - `src/game/battle/move_anim.c` (`MoveAnim_DelayFramesAsm`, `MoveAnim_ConvertAsmFramesToEngineTicks`, tick scheduler)
- Protocol:
  - `docs/timing_conversion_protocol.md`

## Sprite/OAM Cleanup Ownership
- ASM source:
  - `pokered-master/engine/battle/animations.asm` (`AnimationCleanOAM`, `ClearSprites`)
  - `pokered-master/engine/battle/core.asm` (battle scene init: `CopyUncompressedPicToTilemap`, `LoadMonBackPic`, `LoadMonFrontSprite` usage)
- C destination:
  - `src/game/battle/move_anim.c` (`MoveAnim_ClearAnimationOAM`, enemy/player visibility helpers)
  - `src/game/battle/battle_ui.c` (steady-state battle sprite rendering and OAM usage)
  - `src/platform/display.c` (render path backing behavior)

## Battle Scene Init / Intro Ownership
- ASM source:
  - `pokered-master/engine/battle/core.asm` (`InitBattleCommon`, `_InitBattleCommon`, sendout flow)
- C destination:
  - `src/game/battle/battle_ui.c` (`BattleUI_Enter`, `BUI_SLIDE_IN`, `BUI_SEND_OUT`, `BUI_ENEMY_SEND_OUT`, `BUI_POKEMON_APPEAR`)

## Move SFX Mapping
- ASM source:
  - `pokered-master/engine/battle/animations.asm` (`GetMoveSound`, sound trigger timing)
  - `pokered-master/audio/sfx/*.asm` (concrete effect definitions)
  - move script call sites under `data/moves/animations.asm`
- C destination:
  - `src/data/move_sfx_data.c`
  - `src/data/move_sfx_structs.c`
  - `src/game/battle/move_anim.c`
  - `src/platform/audio.c`

## Data Importers (Authoritative Conversion Path)
- Scripts:
  - `tools/extract_move_sfx.py`
  - `tools/extract_move_sfx_structs.py`
  - `tools/extract_move_anim_tiles.py`
- Generated/target data:
  - `src/data/move_sfx_data.c`
  - `src/data/move_sfx_structs.c`
  - `src/data/move_anim_*.c`

## Constrict / Bind Reference Group
- ASM source:
  - `pokered-master/data/moves/animations.asm` (`ConstrictAnim`, `BindAnim`)
  - `pokered-master/data/battle_anims/subanimations.asm` (`Subanim_0Bind`)
  - `pokered-master/engine/battle/animations.asm` (`DrawFrameBlock` mode behavior + cleanup cadence)
- C destination:
  - `src/data/move_anim_scripts.c` (`ConstrictAnim`, `BindAnim` entries)
  - `src/data/move_anim_subanims.c` (`sSubanim_0BindDef`)
  - `src/game/battle/move_anim.c`

## Function-Level Mapping Seeds
- ASM `DrawFrameBlock` -> C `MoveAnim_DrawFrameBlock`
- ASM `AnimationCleanOAM` -> C `MoveAnim_AnimationCleanOAM` / `MoveAnim_ClearAnimationOAM`
- ASM `PlayAnimation` -> C `MoveAnim_Begin` + `MoveAnim_Tick` main command loop
- ASM `LoadMoveAnimationTiles` -> C `MoveAnim_LoadTileset`
- ASM `PlayMoveAnimation` caller chain -> C `bui_run_move_anim_runtime` + `BUI_MOVE_ANIM` state

## Required Diff Checklist (Before Any Edit)
- [ ] Open ASM routine(s) for behavior.
- [ ] Open associated ASM data table(s).
- [ ] Open mapped C function(s).
- [ ] Verify call order parity.
- [ ] Verify timing parity (`DelayFrames`/wait scheduling).
- [ ] Verify cleanup/visibility semantics parity.
- [ ] Verify SFX trigger and channel behavior parity.

## Scope Note
This atlas is specifically for battle animation + battle renderer parity work. Expand with additional modules as needed, but keep all mappings source-of-truth first (ASM -> C).
## Quick Diff Commands (Run First Every Turn)

### 1) Move animation runtime core
```powershell
# ASM
Get-Content C:\Users\Anthony\pokered\pokered-master\engine\battle\animations.asm | Select-String -Pattern 'PlayAnimation:|DrawFrameBlock:|AnimationCleanOAM:' -Context 0,140

# C
Get-Content C:\Users\Anthony\pokered\src\game\battle\move_anim.c | Select-String -Pattern 'MoveAnim_Tick\(|MoveAnim_DrawFrameBlock\(|MoveAnim_AnimationCleanOAM\(|MoveAnim_ClearAnimationOAM\(' -Context 0,120
```

### 2) Per-move script + subanim data
```powershell
# ASM move script
Get-Content C:\Users\Anthony\pokered\pokered-master\data\moves\animations.asm | Select-String -Pattern 'ConstrictAnim|BindAnim|PsychicAnim|SwiftAnim|AcidArmorAnim' -Context 0,8

# ASM subanim definitions
Get-Content C:\Users\Anthony\pokered\pokered-master\data\battle_anims\subanimations.asm | Select-String -Pattern 'Subanim_0Bind|Subanim_.*Psychic|Subanim_.*Swift' -Context 0,16

# C generated tables
Get-Content C:\Users\Anthony\pokered\src\data\move_anim_scripts.c | Select-String -Pattern 'ConstrictAnim|BindAnim|PsychicAnim|SwiftAnim|AcidArmorAnim' -Context 0,4
Get-Content C:\Users\Anthony\pokered\src\data\move_anim_subanims.c | Select-String -Pattern 'sSubanim_0BindDef|sSubanim_.*Psychic|sSubanim_.*Swift' -Context 0,10
```

### 3) Battle scene ownership / sprite model
```powershell
# ASM init + tilemap ownership
Get-Content C:\Users\Anthony\pokered\pokered-master\engine\battle\core.asm | Select-String -Pattern 'InitBattleCommon:|_InitBattleCommon:|CopyUncompressedPicToTilemap:|LoadMonBackPic:' -Context 0,120

# C scene render ownership
Get-Content C:\Users\Anthony\pokered\src\game\battle\battle_ui.c | Select-String -Pattern 'bui_load_sprites\(|bui_set_enemy_oam_visible\(|BUI_MOVE_ANIM|BUI_SLIDE_IN|BUI_ENEMY_SEND_OUT' -Context 0,80
```

### 4) Move SFX mapping and trigger timing
```powershell
# ASM (move anim command timing -> sound)
Get-Content C:\Users\Anthony\pokered\pokered-master\engine\battle\animations.asm | Select-String -Pattern 'GetMoveSound|PlaySound|wAnimSoundID' -Context 0,60
Get-Content C:\Users\Anthony\pokered\pokered-master\data\moves\animations.asm | Select-String -Pattern 'BubbleAnim|BlizzardAnim|BoneClubAnim|BonemerangAnim|AuroraBeamAnim' -Context 0,8

# C
Get-Content C:\Users\Anthony\pokered\src\data\move_sfx_data.c | Select-String -Pattern 'BUBBLE|BLIZZARD|BONE CLUB|BONEMERANG|AURORA BEAM' -Context 0,2
Get-Content C:\Users\Anthony\pokered\src\game\battle\move_anim.c | Select-String -Pattern 'MoveAnim_PlayCommandSound\(|Audio_PlaySFX' -Context 0,50
```

## Routine Atlas (Function-Level)

### ASM `engine/battle/animations.asm`
- `PlayAnimation` -> C `MoveAnim_Begin` + `MoveAnim_Tick` command loop
- `LoadSubanimation` -> C `MoveAnim_LoadSubanimation`
- `PlaySubanimation` -> C `MoveAnim_PlaySubanimationStep`
- `DrawFrameBlock` -> C `MoveAnim_DrawFrameBlock`
- `AnimationCleanOAM` -> C `MoveAnim_AnimationCleanOAM` / `MoveAnim_ClearAnimationOAM`
- `AnimationBlinkMon` (6x hide/show + `DelayFrames 5`) -> C `MoveAnim_AnimationBlinkMon`
- `AnimationShowMonPic` (`Delay3`) -> C `MoveAnim_AnimationShowMonPic`
- `_AnimationSlideMonOff` (8 tile-column slide loop) -> C `MoveAnim_AnimationSlideMonOff`

### ASM `engine/battle/core.asm`
- `PlayMoveAnimation` call chain -> C `bui_run_move_anim_runtime` + `BUI_MOVE_ANIM`
- `InitBattleCommon` / `_InitBattleCommon` scene setup -> C `BattleUI_Enter` + early BUI states
- `CopyUncompressedPicToTilemap` semantics -> C tilemap-backed sprite placement paths
- `LoadMonBackPic` semantics -> C `bui_place_player_sprite` + player BG tile loads

### ASM data tables
- `data/moves/animations.asm` -> C `src/data/move_anim_scripts.c`
- `data/battle_anims/subanimations.asm` -> C `src/data/move_anim_subanims.c`
- frameblocks/basecoords/tile sets -> C `move_anim_frameblocks/basecoords/tiles`

## Turn Log Template

Copy into your working notes each turn:

- Behavior under test:
- ASM files/routines diffed:
- C files/functions diffed:
- Semantic mismatch found:
- Parity change made:
- Build + test outcome:
- Atlas update needed: yes/no
