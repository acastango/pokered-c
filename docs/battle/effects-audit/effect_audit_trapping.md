# Effect Audit: TrappingEffect (ASM -> C)

Date: 2026-05-03  
ASM source: `pokered-master/engine/battle/effects.asm:1080-1103`  
C source: `src/game/battle/battle_effects.c:653-675`

## Result

Found and fixed one parity bug.

## Verified flow parity

1. Select attacker-side pointers by turn (`w*BattleStatus1`, `w*NumAttacksLeft`).
2. Early return if `USING_TRAPPING_MOVE` already set.
3. Clear target Hyper Beam recharge before hit test path (intentional Gen1 behavior).
4. Set `USING_TRAPPING_MOVE`.
5. Roll weighted value:
- first roll `BattleRandom() & 3`
- if result >= 2, reroll once with same mask

## Fixed mismatch

- Previous C wrote trap counter as `r + 1` (range `1..4`).
- ASM writes `inc a` after weighted roll (range `2..5`).
- C updated to `r + 2`.

## Patch

- File: `src/game/battle/battle_effects.c`
- Change: `*ctr = r + 1u` -> `*ctr = r + 2u`
- Comment updated from `1-4` to `2-5`.
