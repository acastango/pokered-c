# Effect Audit: TwoToFiveAttacksEffect (ASM -> C)

Date: 2026-05-03  
ASM source: `pokered-master/engine/battle/effects.asm:925-969`  
C source: `src/game/battle/battle_effects.c:560-596`

## Verdict

`TwoToFiveAttacksEffect` is currently a literal parity match in C for control flow and state writes.

No code change required in this pass.

## Step-by-step parity map

1. Select attacker-side state pointers by `hWhoseTurn`.
- ASM: loads `wPlayer...` or `wEnemy...` into `hl/de/bc`.
- C: selects `bstat1/ctr/num_hits/eff_ptr` based on `hWhoseTurn`.

2. Early return if already multi-hitting.
- ASM: `bit ATTACKING_MULTIPLE_TIMES, [hl]` then `ret nz`.
- C: `if (TST_BIT(*bstat1, BSTAT1_ATTACKING_MULTIPLE)) return;`

3. Set multi-hit active flag.
- ASM: `set ATTACKING_MULTIPLE_TIMES, [hl]`
- C: `SET_BIT(*bstat1, BSTAT1_ATTACKING_MULTIPLE);`

4. Read current move effect pointer for attacker.
- ASM: `hl = wPlayerMoveEffect` or `wEnemyMoveEffect`.
- C: `effect = *eff_ptr;`

5. Special-case Twineedle.
- ASM: if `TWINEEDLE_EFFECT`, set move effect to `POISON_SIDE_EFFECT1`, then save hit count = 2.
- C: exact same mutation and hit count.

6. Special-case Attack Twice.
- ASM: if `ATTACK_TWICE_EFFECT`, set hit count = 2.
- C: same.

7. Standard two-to-five roll (3/8,3/8,1/8,1/8).
- ASM:
  - `r = BattleRandom() & 3`
  - if `r >= 2`, reroll once with same mask
  - final hits = `r + 2`
- C: same algorithm.

8. Save both counters.
- ASM: writes hit count to `w*NumAttacksLeft` and `w*NumHits`.
- C: `*ctr = n; *num_hits = n;`

## Runtime indicators to verify

1. First turn of multi-hit:
- `BSTAT1_ATTACKING_MULTIPLE` set once.
- `w*NumAttacksLeft` and `w*NumHits` initialized to same value.

2. Twineedle only:
- move effect remapped from `TWINEEDLE_EFFECT` to `POISON_SIDE_EFFECT1` during setup.

3. Distribution sanity (`TWO_TO_FIVE_ATTACKS_EFFECT`):
- over many trials, 2/3 hits occur ~75% combined; 4/5 hits ~25% combined.

## Notes

- This audit only covers effect setup semantics.
- If behavior still diverges in battle, next likely mismatch is outside this handler:
  - multi-hit turn loop consumption/decrement path
  - hit text/HP update cadence
  - interaction with miss/accuracy state resets between hits
