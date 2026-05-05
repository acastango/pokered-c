# Effect Audit: StatModifierUpEffect (ASM -> C)

Date: 2026-05-03  
ASM source: `pokered-master/engine/battle/effects.asm` (`StatModifierUpEffect`)  
C source: `src/game/battle/battle_effects.c` (`Effect_StatModifierUp`)

## Result

Found and fixed one ASM quirk mismatch.

## Fixed mismatch

- ASM behavior for ATK/DEF/SPD/SPC raise path:
  - after writing raised stage, if current modified stat is already `999`,
    it jumps to `RestoreOriginalStatModifier` and does `dec [hl]` once.
  - net effect:
    - +1 raise at 999: no stage gain
    - +2 raise at 999: only +1 stage gain

- Previous C behavior:
  - stage increase stayed fully applied even when current stat was already capped.

- C fix:
  - added `999` cap quirk check for `idx < 4` (ATK/DEF/SPD/SPC).
  - decrements stage once and returns (no stat recalc path), matching ASM semantics.

## Other verified matching points

1. Effect index mapping (+1/+2 families).
2. Stage cap at +6 (`13`).
3. Recalc scope excludes ACC/EVA.
4. Minimize sets minimized flag.
5. Post-change Gen1 quirk: re-apply burn/paralysis penalties after stat changes.
