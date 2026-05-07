# ViridianForest Parity Checklist (ASM vs C)

Date: 2026-05-05
Scope: `scripts/ViridianForest.asm` vs C data/runtime wiring.

## Verdict

`ViridianForest.asm` appears parity-aligned for its ASM scope.

## ASM Behavior Surface

From `pokered-master/scripts/ViridianForest.asm`:
- Standard trainer-map script pointer flow:
  - `CheckFightingMapTrainers`
  - `DisplayEnemyTrainerTextAndStartBattle`
  - `EndTrainerBattle`
- Text pointers include:
  - 5 youngster NPC texts (some trainer-linked)
  - 3 item pickup entries
  - 6 sign/tips entries
- Trainer headers:
  - 3 Bug Catcher trainers (`EVENT_BEAT_VIRIDIAN_FOREST_TRAINER_0..2`)

## C Implementation Evidence

From `src/data/event_data.c`:
- `kWarps_ViridianForest` present (6 warp exits/entries).
- `kNpcs_ViridianForest` present (5 NPC entries).
- `kSigns_ViridianForest` present (6 signs/tips).
- `kItems_ViridianForest` present (ANTIDOTE, POTION, POKE_BALL).
- `kTrainers_ViridianForest` present with 3 trainer mappings and event flags.
- Map row:
  - `[0x33] = { kWarps_ViridianForest, ..., kNpcs_ViridianForest, 5, kSigns_ViridianForest, 6, kItems_ViridianForest, 3, ..., kTrainers_ViridianForest, 3 }`

Runtime:
- Generic trainer flow handled by `trainer_sight.c` + `game.c` via `map_trainer_t`.
- Matches the standard ASM script structure for this map.

## Confirmed Gaps

No clear structural or procedural gap identified in this pass.
