# Bonemerang ASM Trace And C Port Notes

## Scope
This document traces `BONEMERANG` from move data to runtime behavior in the original `pokered` ASM, then maps each stage to the C port.

## 1) Move Data In ASM (source of truth)

- Move definition:
  - `pokered-master/data/moves/moves.asm`
  - `move BONEMERANG, ATTACK_TWICE_EFFECT, 50, GROUND, 90, 10`
- Move animation script:
  - `pokered-master/data/moves/animations.asm`
  - `BonemerangAnim: battle_anim BONEMERANG, SUBANIM_0_STAR_THRICE, 0, 6`
- Move SFX table:
  - `pokered-master/data/moves/sfx.asm`
  - `db SFX_BATTLE_2B, $f0, $60 ; BONEMERANG`

## 2) Multi-Hit Setup In ASM

- `ATTACK_TWICE_EFFECT` is dispatched to `TwoToFiveAttacksEffect`:
  - `pokered-master/data/moves/effects_pointers.asm`
  - `ATTACK_TWICE_EFFECT -> TwoToFiveAttacksEffect`
- `TwoToFiveAttacksEffect` behavior:
  - `pokered-master/engine/battle/effects.asm`
  - Sets `ATTACKING_MULTIPLE_TIMES` and sets `NumAttacksLeft = 2` for `ATTACK_TWICE_EFFECT`.

## 3) Per-Hit Runtime Order In ASM

For player side (enemy side mirrors this), the relevant flow is in:

- `pokered-master/engine/battle/core.asm`
  - `PlayPlayerMoveAnimation`:
    1. `PlayMoveAnimation` (scripted move anim + SFX + applying anim)
    2. `DrawPlayerHUDAndHPBar`
  - Then:
    3. `ApplyAttackToEnemyPokemon` (which calls `UpdateHPBar2` to animate HP bar)
    4. Multi-hit check:
       - decrements `wPlayerNumAttacksLeft`
       - if still > 0, jumps back to `GetPlayerAnimationType` (so animation/SFX/HP sequence runs again)
       - after final hit prints hit-count text and clears multi-hit state.

And inside move animation itself:

- `pokered-master/engine/battle/animations.asm` (`MoveAnimation`):
  - `PlayAnimation` (scripted subanimations and command sound)
  - `PlayApplyingAttackAnimation`
  - `WaitForSoundToFinish`

So the effective per-hit order is:

1. Move animation + move SFX
2. Applying attack animation
3. Wait for SFX end
4. HP bar scroll (`UpdateHPBar2`)
5. If hits remain, repeat from step 1

## 4) C Port Mapping (current)

- Move data:
  - `src/data/moves_data.c` (`BONEMERANG` uses effect `0x2C`)
  - `src/data/move_anim_scripts.c` (`sMoveAnimScript_154` for Bonemerang)
  - `src/data/move_sfx_data.c` (`SFX_BATTLE_2B, 0xF0, 0x60`)
- Multi-hit effect setup:
  - `src/game/battle/battle_effects.c` (`Effect_TwoToFiveAttacks`)
- Turn execution:
  - `src/game/battle/battle_core.c`
- UI sequencing:
  - `src/game/battle/battle_ui.c`

## 5) C Implementation Adjustment Made

Problem addressed:

- Core executes all multi-hit damage within one `Battle_Execute*Move()` call.
- Without extra UI handling, this collapses Bonemerang into a single animation-to-final-HP pass.

Adjustment implemented:

- Added per-move hit snapshot accessors in `battle_core`:
  - `Battle_GetLastPlayerHitCount()`
  - `Battle_GetLastPlayerFirstTargetHP()`
  - `Battle_GetLastEnemyHitCount()`
  - `Battle_GetLastEnemyFirstTargetHP()`
- Recorded first-hit target HP in `Battle_ExecutePlayerMove` / `Battle_ExecuteEnemyMove` before second-hit application.
- In `battle_ui`, when hit count is 2:
  - HP scroll is split into phase 1 (old -> after-hit1) and phase 2 (after-hit1 -> final)
  - Between phases, `BUI_MOVE_ANIM` is re-entered and move animation runtime is replayed once.
  - This restores the ASM-like cadence: animation/SFX/wait + HP for each hit.

## 6) Verification Checklist For Bonemerang

1. Uses `SFX_BATTLE_2B` with move modifiers (`$F0,$60`).
2. First impact animates HP drop once.
3. Second impact replays move animation and animates second HP drop.
4. No immediate jump from full HP to final HP in one scroll when two hits land.
