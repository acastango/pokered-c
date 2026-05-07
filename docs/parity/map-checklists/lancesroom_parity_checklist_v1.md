# LancesRoom Parity Checklist (ASM vs C)

Date: 2026-05-05  
Scope: `scripts/LancesRoom.asm` vs C map/runtime wiring.

## Verdict

`LancesRoom` appears **data-present but logic-partial** in C.

## ASM Behavior Surface

From [`pokered-master/scripts/LancesRoom.asm`](/C:/Users/Anthony/pokered/pokered-master/scripts/LancesRoom.asm):

- Map script entry does more than textbox setup:
  - `LanceShowOrHideEntranceBlocks`
  - script-pointer execution (`wLancesRoomCurScript` state machine)
- Door lock/open tile behavior:
  - uses `EVENT_LANCES_ROOM_LOCK_DOOR`
  - replaces entrance blocks via `ReplaceTileBlock`
- Triggered movement path to Lance (`WalkToLance`):
  - simulated joypad RLE route
  - temporarily ignores player input
- Trainer flow:
  - Lance trainer header uses `EVENT_BEAT_LANCES_ROOM_TRAINER_0`
  - `LancesRoomLanceAfterBattleText` sets `EVENT_BEAT_LANCE`

## C Implementation Evidence

From [`src/data/event_data.c`](/C:/Users/Anthony/pokered/src/data/event_data.c):

- `kWarps_LancesRoom` present.
- `kNpcs_LancesRoom` present with Lance NPC:
  - `{ 6, 1, ..., text=NULL, script=NULL }`
- Multiple map IDs mapped to LancesRoom data (`0x69`..`0x75` set entries shown in table).
- No `kTrainers_LancesRoom` entry found.

From search across `src/game/*.c` and `src/data/*.c`:

- Event constants exist in [`src/data/event_constants.h`](/C:/Users/Anthony/pokered/src/data/event_constants.h):
  - `EVENT_BEAT_LANCES_ROOM_TRAINER_0`
  - `EVENT_BEAT_LANCE`
  - `EVENT_LANCES_ROOM_LOCK_DOOR`
- No concrete runtime references found for those Lance-room events in C game logic search.
- No dedicated `lances_room` script module found.

## Confirmed Gaps

## 1) Lance Room Script-State Machine Missing (High)

- ASM:
  - has explicit script-pointer states for default, battle start/end, forced movement, noop.
- C:
  - no corresponding Lance-room state machine detected.
- Impact:
  - High (room progression and sequencing behavior risk).

## 2) Entrance Block Lock/Open Tile Logic Missing (High)

- ASM:
  - updates entrance blocks based on `EVENT_LANCES_ROOM_LOCK_DOOR` via `ReplaceTileBlock`.
- C:
  - no Lance-room-specific door tile replacement hook found.
- Impact:
  - High (room lock behavior/parity risk).

## 3) Walk-to-Lance Forced Movement Flow Missing (High)

- ASM:
  - uses simulated joypad route (`WalkToLance_RLEList`) and `wJoyIgnore` gating.
- C:
  - no Lance-room-specific equivalent found in current runtime wiring.
- Impact:
  - High (encounter choreography/progression risk).

## 4) Trainer Header Wiring Missing (High)

- ASM:
  - Lance trainer header + explicit event-driven trainer lifecycle.
- C:
  - Lance NPC has `text=NULL`, `script=NULL`, and no `kTrainers_LancesRoom`.
- Impact:
  - High (battle initiation and post-battle event path risk in this room).

## Immediate Touchpoints

1. [`src/data/event_data.c`](/C:/Users/Anthony/pokered/src/data/event_data.c)
- Add Lance-room trainer/script ownership wiring (`script` callbacks or trainer table approach).

2. `src/game` new/owned elite-room script module
- Implement:
  - entrance lock/open tile logic
  - forced movement route to Lance
  - battle state transitions
  - post-battle `EVENT_BEAT_LANCE` progression

3. [`src/data/event_constants.h`](/C:/Users/Anthony/pokered/src/data/event_constants.h)
- Reuse existing Lance-room event constants already defined.
