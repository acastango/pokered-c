# SSAnneB1FRooms Parity Checklist (ASM vs C)

Date: 2026-05-05  
Scope: `scripts/SSAnneB1FRooms.asm` vs C data/runtime wiring.

## Verdict

`SSAnneB1FRooms` is **mostly parity-aligned structurally** with one confirmed fidelity gap.

## ASM Behavior Surface

From [`pokered-master/scripts/SSAnneB1FRooms.asm`](/C:/Users/Anthony/pokered/pokered-master/scripts/SSAnneB1FRooms.asm):

- Standard trainer-map script pointer flow:
  - `CheckFightingMapTrainers`
  - `DisplayEnemyTrainerTextAndStartBattle`
  - `EndTrainerBattle`
- Text pointers:
  - 6 trainer-interaction NPCs (5 sailors + fisher)
  - Super Nerd text
  - Machoke text
  - 3 pickup item texts
- Trainer headers:
  - 6 trainers (`EVENT_BEAT_SS_ANNE_10_TRAINER_0..5`)
- Special behavior:
  - `SSAnneB1FRoomsMachokeText` is `text_asm` and calls `PlayCry` for Machoke.

## C Implementation Evidence

From [`src/data/event_data.c`](/C:/Users/Anthony/pokered/src/data/event_data.c):

- `kWarps_SSAnneB1FRooms` present.
- `kNpcs_SSAnneB1FRooms` present with 8 NPCs.
- `kItems_SSAnneB1FRooms` present with 3 items (`ETHER`, `TM_REST`, `MAX_POTION`).
- `kTrainers_SSAnneB1FRooms` present with 6 trainers (mapped to first 6 NPC slots).
- Map binding:
  - `[0x68] = { kWarps_SSAnneB1FRooms, ..., kNpcs_SSAnneB1FRooms, ..., kItems_SSAnneB1FRooms, ..., kTrainers_SSAnneB1FRooms, 6 }`

Generic trainer runtime:
- handled by `trainer_sight.c` + `game.c` using `map_trainer_t` entries.

## Confirmed Gap

## 1) Machoke Cry on Interaction Missing (Confirmed)

- ASM:
  - `SSAnneB1FRoomsMachokeText` executes `PlayCry(MACHOKE)` during text.
- C:
  - Machoke NPC row is plain text with `script = NULL`:
    - `text = "MACHOKE: Gwoh! Goggoh!@"`
    - no callback to invoke cry.
- Search evidence:
  - no SS Anne B1F Rooms Machoke-specific cry call found in `src/game/*.c`.
- Impact:
  - Low-to-medium (localized fidelity issue; not progression-blocking).

## What Appears Aligned

- Warps, NPC population, item pickups, trainer roster/count are all present.
- Core trainer battle loop for the 6 trainer NPCs is structurally represented.

## Recommended Follow-up

- Add a small NPC callback for the Machoke entry in `kNpcs_SSAnneB1FRooms` to play cry before/with text, mirroring ASM `text_asm` behavior.
