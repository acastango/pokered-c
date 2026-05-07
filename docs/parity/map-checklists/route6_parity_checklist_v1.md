# Route6 Parity Checklist (ASM vs C)

Date: 2026-05-05
Scope: `scripts/Route6.asm` vs C data/runtime wiring.

## Verdict

`Route6.asm` appears parity-aligned for its ASM scope.

## ASM Behavior Surface

From `pokered-master/scripts/Route6.asm`:
- Standard trainer-map script-pointer flow:
  - `CheckFightingMapTrainers`
  - `DisplayEnemyTrainerTextAndStartBattle`
  - `EndTrainerBattle`
- Text pointers:
  - 6 trainer NPC entries
  - 1 underground path sign entry
- Trainer headers:
  - 6 trainers (`EVENT_BEAT_ROUTE_6_TRAINER_0..5`)

## C Implementation Evidence

From `src/data/event_data.c`:
- `kNpcs_Route6` present with 6 trainer-like NPCs.
- `kTrainers_Route6` present with 6 trainer mappings.
- `kSigns_Route6` present with underground path sign text.
- Map row:
  - `[0x11] = { kWarps_Route6, ..., kNpcs_Route6, 6, kSigns_Route6, 1, ..., kTrainers_Route6, 6 }`

Runtime:
- Generic trainer flow handled via `trainer_sight.c` + `game.c` and `map_trainer_t` tables.
- Matches Route 6 ASM structure (no custom per-map state machine indicated).

## Confirmed Gaps

No clear structural/procedural gap identified in this pass.
