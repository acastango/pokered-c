# Move Effect Handler Audit Matrix (ASM -> C)

Date: 2026-05-03  
Source of truth: `pokered-master/data/moves/effects_pointers.asm`  
C dispatch anchor: `src/game/battle/battle_effects.c` (`Battle_JumpMoveEffect`)

## How to read this

- `ASM target`: handler pointer in `MoveEffectPointerTable`.
- `C target`: `Battle_JumpMoveEffect` route.
- `NULL in ASM`: effect pointer intentionally null in ASM table.
- `Audit`: current verification level.
  - `Mapped`: pointer/dispatch parity verified.
  - `Deep Pending`: per-handler semantic step-by-step parity still to audit.

## Matrix

| Effect family / IDs | ASM target | C target | NULL in ASM | Audit |
|---|---|---|---|---|
| `EFFECT_01`, `EFFECT_SLEEP` | `SleepEffect` | `Effect_Sleep` | No | Mapped |
| `POISON_SIDE1`, `POISON_SIDE2`, `POISON` | `PoisonEffect` | `Effect_Poison` | No | Mapped |
| `DRAIN_HP`, `DREAM_EATER` | `DrainHPEffect` | `Effect_DrainHP` | No | Mapped |
| `BURN_SIDE1/2`, `FREEZE_SIDE1/2`, `PARALYZE_SIDE1/2` | `FreezeBurnParalyzeEffect` | `Effect_FreezeBurnParalyze` | No | Mapped |
| `EXPLODE` | `ExplodeEffect` | `Effect_Explode` | No | Mapped |
| `MIRROR_MOVE` | `NULL` | no-op (commented as core-handled path) | Yes | Mapped |
| `ATTACK_UP1..EVASION_UP1`, `ATTACK_UP2..EVASION_UP2` | `StatModifierUpEffect` | `Effect_StatModifierUp` | No | Mapped |
| `PAY_DAY` | `PayDayEffect` | `Effect_PayDay` | No | Mapped |
| `SWIFT` | `NULL` | no-op (always-hit handled elsewhere) | Yes | Mapped |
| `ATTACK_DOWN1..EVASION_DOWN1`, `ATTACK_DOWN2..EVASION_DOWN2`, `*_DOWN_SIDE` | `StatModifierDownEffect` | `Effect_StatModifierDown` | No | Mapped |
| `CONVERSION` | `ConversionEffect` | `Effect_Conversion` | No | Mapped |
| `HAZE` | `HazeEffect` | `Effect_Haze` | No | Mapped |
| `BIDE` | `BideEffect` | `Effect_Bide` | No | Mapped |
| `THRASH_PETAL_DANCE` | `ThrashPetalDanceEffect` | `Effect_ThrashPetalDance` | No | Mapped |
| `SWITCH_AND_TELEPORT` | `SwitchAndTeleportEffect` | `Effect_SwitchAndTeleport` | No | Mapped |
| `TWO_TO_FIVE_ATTACKS`, `EFFECT_1E`, `ATTACK_TWICE`, `TWINEEDLE` | `TwoToFiveAttacksEffect` | `Effect_TwoToFiveAttacks` | No | Mapped |
| `FLINCH_SIDE1/2` | `FlinchSideEffect` | `Effect_FlinchSide` | No | Mapped |
| `OHKO` | `OneHitKOEffect` | `Effect_OHKO` | No | Mapped |
| `CHARGE`, `FLY` | `ChargeEffect` | `Effect_Charge` | No | Mapped |
| `SUPER_FANG` | `NULL` | no-op (core-handled path) | Yes | Mapped |
| `SPECIAL_DAMAGE` | `NULL` | no-op (core-handled path) | Yes | Mapped |
| `TRAPPING` | `TrappingEffect` | `Effect_Trapping` | No | Mapped |
| `JUMP_KICK` | `NULL` | no-op (core-handled path) | Yes | Mapped |
| `MIST` | `MistEffect` | `Effect_Mist` | No | Mapped |
| `FOCUS_ENERGY` | `FocusEnergyEffect` | `Effect_FocusEnergy` | No | Mapped |
| `RECOIL` | `RecoilEffect` | `Effect_Recoil` | No | Mapped |
| `CONFUSION` | `ConfusionEffect` | `Effect_Confusion` | No | Mapped |
| `HEAL` | `HealEffect` | `Effect_Heal` | No | Mapped |
| `TRANSFORM` | `TransformEffect` | `Effect_Transform` | No | Mapped |
| `LIGHT_SCREEN`, `REFLECT` | `ReflectLightScreenEffect` | `Effect_ReflectLightScreen` | No | Mapped |
| `PARALYZE` | `ParalyzeEffect` | `Effect_Paralyze` | No | Mapped |
| `CONFUSION_SIDE` | `ConfusionSideEffect` | `Effect_ConfusionSide` | No | Mapped |
| `SUBSTITUTE` | `SubstituteEffect` | `Effect_Substitute` | No | Mapped |
| `HYPER_BEAM` | `HyperBeamEffect` | `Effect_HyperBeam` | No | Mapped |
| `RAGE` | `RageEffect` | `Effect_Rage` | No | Mapped |
| `MIMIC` | `MimicEffect` | `Effect_Mimic` | No | Mapped |
| `METRONOME` | `NULL` | no-op (pre-table core path) | Yes | Mapped |
| `LEECH_SEED` | `LeechSeedEffect` | `Effect_LeechSeed` | No | Mapped |
| `SPLASH` | `SplashEffect` | `Effect_Splash` | No | Mapped |
| `DISABLE` | `DisableEffect` | `Effect_Disable` | No | Mapped |

## Deep parity checklist (next pass)

For each non-NULL handler above:

1. Verify call-site gating parity (accuracy test location, substitute checks, invulnerability checks).
2. Verify exact state mutation order (status bits, counters, stat mods, flags).
3. Verify timing coupling points to animation core (`wAnimationType`, hit/miss state, HP update boundaries).
4. Verify all known Gen1 bugs are preserved where documented.

## Immediate high-risk families to deep-audit first

1. `TwoToFiveAttacksEffect` (multi-hit flow; historically fragile)
2. `TrappingEffect` (turn lock + residual behavior)
3. `StatModifierDownEffect` (side-effect probability + miss semantics)
4. `ChargeEffect` / `Fly` (invulnerable turn state)
5. `HyperBeamEffect` (recharge flag transitions)
