# Effect Audit: ChargeEffect / Fly (ASM -> C)

Date: 2026-05-03  
ASM source: `pokered-master/engine/battle/effects.asm:998-1031`  
C source: `src/game/battle/battle_effects.c:640-651`

## Verdict

Core battle-state semantics are aligned.  
No battle-state patch required in this pass.

## ASM vs C mapping

1. Choose side pointers by `hWhoseTurn`.
- ASM selects `wPlayerBattleStatus1`/`wEnemyBattleStatus1`, and `w*MoveEffect`.
- C selects equivalent `bstat1`, `effect`, `move_num`.

2. Set charging flag.
- ASM: `set CHARGING_UP, [hl]`
- C: `SET_BIT(*bstat1, BSTAT1_CHARGING_UP)`

3. Set invulnerable for Fly/Dig charge turn.
- ASM:
  - if move effect is `FLY_EFFECT`, set `INVULNERABLE`
  - if move number is `DIG`, set `INVULNERABLE`
- C: `if (effect == EFFECT_FLY || move_num == MOVE_DIG) SET_BIT(...INVULNERABLE)`

4. Store charged move number for follow-up text/logic.
- ASM: `ld [wChargeMoveNum], a`
- C: `wChargeMoveNum = move_num`

## Intentional omissions currently in C

ASM performs immediate UI actions in `ChargeEffect`:

- sets `wAnimationType = 0`
- calls `PlayBattleAnimation` with charge-specific preview anim:
  - default: `XSTATITEM_ANIM` / duplicate variant
  - Fly: `TELEPORT`
  - Dig: `SLIDE_DOWN_ANIM`
- prints `ChargeMoveEffectText` (“flew up high”, “dug a hole”, etc.)

In this C port, these are intentionally omitted from effect logic and battle core currently notes animation omissions on these paths. This does not change charge/invulnerability mechanics, but it does affect presentation parity.

## Risk check

- Mechanics risk: low (state flags match ASM behavior).
- Visual/text parity risk: medium (charge-turn feedback text/preview anim not yet wired 1:1 here).
