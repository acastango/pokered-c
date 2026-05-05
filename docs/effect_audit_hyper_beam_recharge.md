# Effect Audit: Hyper Beam Recharge Semantics (ASM -> C)

Date: 2026-05-03  
ASM sources:
- `pokered-master/engine/battle/effects.asm` (`HyperBeamEffect`, `ClearHyperBeam`)
- `pokered-master/engine/battle/core.asm` (must-recharge turn checks)

C sources:
- `src/game/battle/battle_effects.c`
- `src/game/battle/battle_core.c`

## Verdict

Recharge set/clear semantics are aligned with ASM in the current C port.  
No code patch required in this pass.

## Verified mappings

## 1) HyperBeamEffect (set flag on attacker)

- ASM (`effects.asm`):
  - chooses attacker side (`wPlayerBattleStatus2` or `wEnemyBattleStatus2`)
  - `set NEEDS_TO_RECHARGE`
- C (`battle_effects.c`, `Effect_HyperBeam`):
  - chooses same attacker-side status2
  - `SET_BIT(..., BSTAT2_NEEDS_TO_RECHARGE)`

Status: parity match.

## 2) ClearHyperBeam (clear target’s recharge flag)

- ASM (`effects.asm`):
  - chooses target side opposite `hWhoseTurn`
  - `res NEEDS_TO_RECHARGE`
- C (`battle_effects.c`, `ClearHyperBeam`):
  - same target-side selection and bit clear

Status: parity match.

## 3) Recharge turn consumption (must recharge message + clear)

- ASM (`core.asm`):
  - player/enemy turn status gate checks `NEEDS_TO_RECHARGE`
  - clears bit and consumes turn with “must recharge”
- C (`battle_core.c`):
  - `check_player_turn_status()` and `check_enemy_turn_status()`
  - exact same pattern: check bit -> clear -> status message -> end turn

Status: parity match.

## 4) Call sites that intentionally clear target recharge

Observed parity-relevant clear calls in C:
- sleep/status handling path (mirrors ASM’s recharge-bypass behavior)
- flinch side effect path
- trapping effect path (explicitly before hit test, as in ASM comment)

These all use `ClearHyperBeam()` with target semantics preserved.

## Notes

- No mismatch found in this subsystem.
- If a recharge bug appears at runtime, likely source is upstream move flow/state transitions, not Hyper Beam flag set/clear primitives.
