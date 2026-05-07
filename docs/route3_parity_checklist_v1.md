# Route3 Parity Checklist (ASM vs C)

Date: 2026-05-05
Scope: `scripts/Route3.asm` vs C data/runtime wiring.

## Verdict

`Route3.asm` appears parity-aligned for its ASM scope.

## ASM Behavior Surface

From `pokered-master/scripts/Route3.asm`:
- Standard trainer-map script pointer flow:
  - `CheckFightingMapTrainers`
  - `DisplayEnemyTrainerTextAndStartBattle`
  - `EndTrainerBattle`
- Text pointers:
  - 1 non-trainer NPC (Super Nerd)
  - 8 trainer NPC text entries
  - 1 route sign
- Trainer headers:
  - 8 trainers with event flags `EVENT_BEAT_ROUTE_3_TRAINER_0..7`

## C Implementation Evidence

From `src/data/event_data.c`:
- `kNpcs_Route3` present with 9 NPC entries (1 non-trainer + 8 trainer NPC slots).
- `kTrainers_Route3` present with 8 trainer mappings and before/after text.
- `kSigns_Route3` present (`ROUTE 3 / MT.MOON AHEAD`).
- Map row:
  - `[0x0e] = { ..., kNpcs_Route3, 9, kSigns_Route3, 1, ..., kTrainers_Route3, 8 }`

Runtime:
- Generic trainer behavior handled by `trainer_sight.c` + `game.c` via `map_trainer_t`.
- Matches standard non-custom script pattern used by Route 3 ASM.

## Confirmed Gaps

No clear structural or procedural gap identified in this pass.
