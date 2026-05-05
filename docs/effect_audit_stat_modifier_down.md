# Effect Audit: StatModifierDownEffect (ASM -> C)

Date: 2026-05-03  
ASM source: `pokered-master/engine/battle/effects.asm` (`StatModifierDownEffect`)  
C source: `src/game/battle/battle_effects.c` (`Effect_StatModifierDown`)

## Result

Found and fixed one concrete parity mismatch.

## Fixed mismatch

- ASM calls `CheckTargetSubstitute` before branching into side-effect vs non-side lowering logic.
- C previously checked substitute only in the non-side branch, allowing side-effect stat drops through substitute.
- C updated to perform substitute check first for both branches.

Patch:
- [battle_effects.c](/C:/Users/Anthony/pokered/src/game/battle/battle_effects.c)

## Verified matching points

1. Turn-based pointer selection and non-link 25% miss quirk on enemy turn.
2. Side-effect proc chance gate (`33%`).
3. Non-side path includes `MoveHitTest` and invulnerable check.
4. Stage decrement and -2 effect handling with floor at minimum stage.
5. Recalc path restricted to ATK/DEF/SPD/SPC; ACC/EVA only stage values.
6. Gen1 quirk: re-apply burn/paralysis penalties after stat changes.

## Remaining note

- ASM routes certain no-effect cases to text routines like `CantLowerAnymore`.
- This pass focused on state/parity semantics, not battle text/UI parity.
