# SilphCo2F Parity Checklist (ASM vs C)

Date: 2026-05-05  
Scope: `scripts/SilphCo2F.asm` vs current C wiring (`src/data/event_data.c` + `src/game` runtime hooks).  
Rule: Evidence-only; no speculative behavior claims.

## Verdict

`SilphCo2F` is **structurally present but behaviorally partial** in C.

## ASM Behavior Surface (Source of Truth)

From [`pokered-master/scripts/SilphCo2F.asm`](/C:/Users/Anthony/pokered/pokered-master/scripts/SilphCo2F.asm):

- Map script entry and script pointer table:
  - `SilphCo2F_Script`
  - `SilphCo2F_ScriptPointers`:
    - `CheckFightingMapTrainers`
    - `DisplayEnemyTrainerTextAndStartBattle`
    - `EndTrainerBattle`
- Card-key door callback flow:
  - `SilphCo2FGateCallbackScript`
  - `SilphCo2F_SetCardKeyDoorYScript`
  - `SilphCo2F_UnlockedDoorEventScript`
  - Uses:
    - `EVENT_SILPH_CO_2_UNLOCKED_DOOR1`
    - `EVENT_SILPH_CO_2_UNLOCKED_DOOR2`
    - tile replacement at door coords via `ReplaceTileBlock`
- Trainer headers:
  - 4 trainer entries:
    - `EVENT_BEAT_SILPH_CO_2F_TRAINER_0`
    - `EVENT_BEAT_SILPH_CO_2F_TRAINER_1`
    - `EVENT_BEAT_SILPH_CO_2F_TRAINER_2`
    - `EVENT_BEAT_SILPH_CO_2F_TRAINER_3`
- Silph worker TM flow:
  - `SilphCo2FSilphWorkerFText` checks `EVENT_GOT_TM36`, gives `TM_SELFDESTRUCT`, handles bag full branch.

## C Implementation Evidence

From [`src/data/event_data.c`](/C:/Users/Anthony/pokered/src/data/event_data.c):

- Map data exists:
  - `kWarps_SilphCo2F`
  - `kNpcs_SilphCo2F`
  - map bindings at IDs `0xcf`, `0xed`, `0xee`, `0xf1`, `0xf2`, `0xf3`, `0xf4`
- Current NPC wiring:
  - Silph worker text is stub-like: `"{PLAYER} got\n@"`, `script = NULL`
  - 4 trainer-like NPCs (2 scientists, 2 rockets) have `text = NULL`, `script = NULL`
- No SilphCo2F trainer table present:
  - no `kTrainers_SilphCo2F` declaration
  - map row for `0xcf` has trainer pointer/count as `NULL, 0`

From [`src/data/event_constants.h`](/C:/Users/Anthony/pokered/src/data/event_constants.h):

- Related event flags are defined:
  - `EVENT_BEAT_SILPH_CO_2F_TRAINER_0..3`
  - `EVENT_SILPH_CO_2_UNLOCKED_DOOR1/2`
  - indicates constants exist, but definitions alone do not wire behavior.

From code search in `src/game/*.c`, `src/platform/*.c`, `src/data/*.c`:

- No concrete usage found for:
  - `EVENT_GOT_TM36`
  - `EVENT_SILPH_CO_2_UNLOCKED_DOOR1/2`
  - `EVENT_BEAT_SILPH_CO_2F_TRAINER_*`
  - `wCardKeyDoorY`
  - SilphCo2F-specific door tile replacement routine.

## Gap Checklist

## 1) Trainer Battle Wiring (High)

- ASM expectation:
  - 4 map trainers wired through script/trainer headers and defeat flags.
- C current:
  - No `kTrainers_SilphCo2F`; map uses `NULL, 0` trainer table.
  - Trainer-like NPCs exist but have no script/text hooks for trainer engagement.
- Impact:
  - High. Core battle progression on this floor cannot be ASM-equivalent without trainer table or equivalent explicit script routing.

## 2) Card-Key Door State + Tile Replacement (High)

- ASM expectation:
  - On map load, callback checks card-key unlock position/state and opens correct door tiles, sets unlock events.
- C current:
  - No SilphCo2F callback/door handler found.
  - Unlock event constants exist but no floor-specific runtime usage found.
- Impact:
  - High. Door gating/progression and traversal parity risk.

## 3) TM36 (Selfdestruct) NPC Interaction Flow (Medium-High)

- ASM expectation:
  - Conditional give-item sequence with event gating and no-room handling.
- C current:
  - Worker NPC text is placeholder-like and lacks script callback.
  - No `EVENT_GOT_TM36` usage found.
- Impact:
  - Medium-High. Reward/item progression parity risk on this map.

## 4) Script-State Machine Presence (Medium)

- ASM expectation:
  - map script pointer state machine (`default/start battle/end battle`).
- C current:
  - No dedicated `silph_co*_scripts.c` module detected.
  - Behavior appears data-driven only for this floor.
- Impact:
  - Medium. Missing explicit per-map state handling compared to ASM structure.

## Immediate C Touchpoints For Fix Work

1. [`src/data/event_data.c`](/C:/Users/Anthony/pokered/src/data/event_data.c)
- Add `kTrainers_SilphCo2F` equivalent wiring or explicit script callbacks per NPC.
- Replace placeholder Silph worker text path with script callback path.

2. `src/game` grouped script owner module (new or existing grouped Silph handler)
- Implement door callback + TM36 flow + trainer text/battle routing parity.

3. [`src/data/event_constants.h`](/C:/Users/Anthony/pokered/src/data/event_constants.h)
- Reuse existing event constants already defined for 2F trainer/door events.

## Update Back To Master Matrix

For `SilphCo2F.asm`:
- Keep status `partial` until:
  - trainer wiring exists,
  - door unlock callback exists,
  - TM36 item flow exists with event gating.
