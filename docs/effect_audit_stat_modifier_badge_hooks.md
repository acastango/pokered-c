# Effect Audit: Stat Modifier Badge Reapply Hooks (ASM -> C)

Date: 2026-05-03  
Scope: side-path parity for stat up/down effects.

## Gap found

ASM calls `ApplyBadgeStatBoosts` in stat modifier handlers at specific turn conditions:

- `StatModifierUpEffect`: call on player turn (`call z, ApplyBadgeStatBoosts`)
- `StatModifierDownEffect`: call on enemy turn (`call nz, ApplyBadgeStatBoosts`)

This was previously missing in C stat-effect handlers.

## Fix implemented

1. Exposed badge boost helper for shared use:
- Added `Battle_ApplyBadgeStatBoosts()` public wrapper.
- Files:
  - [battle_switch.h](/C:/Users/Anthony/pokered/src/game/battle/battle_switch.h)
  - [battle_switch.c](/C:/Users/Anthony/pokered/src/game/battle/battle_switch.c)

2. Wired calls in effect handlers at ASM-equivalent points:
- In `Effect_StatModifierUp`: call when `hWhoseTurn == 0`.
- In `Effect_StatModifierDown`: call when `hWhoseTurn != 0` in both successful side-effect and non-side paths.
- File:
  - [battle_effects.c](/C:/Users/Anthony/pokered/src/game/battle/battle_effects.c)

## Why this matters

This is a high-impact Gen1 quirk path: repeated badge reapplication can further boost player stats in scenarios where enemy lowers stats or player uses self-boosting moves. Without this, battle outcomes can drift from ROM behavior over longer fights.
