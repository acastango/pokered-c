# MtMoon1F Parity Checklist (ASM vs C)

Date: 2026-05-05  
Scope: `scripts/MtMoon1F.asm` vs C map/runtime wiring.

## Verdict

`MtMoon1F` appears **largely parity-aligned via data-driven wiring** (not missing).

## ASM Behavior Surface

From [`pokered-master/scripts/MtMoon1F.asm`](/C:/Users/Anthony/pokered/pokered-master/scripts/MtMoon1F.asm):

- Script pointer table is the standard trainer-map flow:
  - `CheckFightingMapTrainers`
  - `DisplayEnemyTrainerTextAndStartBattle`
  - `EndTrainerBattle`
- Text pointers include:
  - 7 trainer NPC text entries
  - item pickup texts
  - one sign text (`Beware! Zubat ...`)
- Trainer headers:
  - 7 trainers
  - events `EVENT_BEAT_MT_MOON_1_TRAINER_0..6`

## C Implementation Evidence

From [`src/data/event_data.c`](/C:/Users/Anthony/pokered/src/data/event_data.c):

- `kWarps_MtMoon1F` present.
- `kNpcs_MtMoon1F` present with 7 trainer-like NPCs.
- `kSigns_MtMoon1F` present with the Zubat warning sign.
- `kItems_MtMoon1F` present with 6 pickups (Potion, Moon Stone, Rare Candy, Escape Rope, Potion, TM Water Gun).
- `kTrainers_MtMoon1F` present with 7 trainer entries, mapped to NPC indices.
- Map row exists and is wired with all of the above:
  - `[0x3b] = { kWarps_MtMoon1F, ..., kNpcs_MtMoon1F, ..., kSigns_MtMoon1F, ..., kItems_MtMoon1F, ..., kTrainers_MtMoon1F, 7 }`

From runtime architecture:

- Generic trainer behavior is handled by `trainer_sight.c` + `game.c` using `map_trainer_t` from `gMapEvents`.
- This matches the standard non-custom map script pattern used by `MtMoon1F.asm`.

## Clarification About `mtmoon_scripts.c`

- [`src/game/mtmoon_scripts.c`](/C:/Users/Anthony/pokered/src/game/mtmoon_scripts.c) is for **MtMoonB2F fossil event**.
- Its existence/absence does not determine MtMoon1F parity.

## Confirmed Gaps

No clear structural gap identified for `MtMoon1F` from this pass.

## Residual Risk

- Text-level exactness (line phrasing/control flow nuances) still needs targeted text diff if we want strict script-text fidelity assurance.

## Reclassification Note

For `MtMoon1F.asm`, current evidence supports:
- owner present (`event_data.c` + generic trainer runtime),
- likely status should be interpreted closer to `mapped` than high-risk `partial`.
