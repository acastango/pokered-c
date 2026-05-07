# VictoryRoad2F Parity Checklist (ASM vs C)

Date: 2026-05-05  
Scope: `scripts/VictoryRoad2F.asm` vs C data/runtime wiring.

## Verdict

`VictoryRoad2F` is **partial** with major map-logic and encounter-state gaps.

## ASM Behavior Surface

From [`pokered-master/scripts/VictoryRoad2F.asm`](/C:/Users/Anthony/pokered/pokered-master/scripts/VictoryRoad2F.asm):

- Map script includes dynamic boulder-switch logic:
  - reset/check flow for boulder-on-switch events
  - tile replacement side effects based on switch state
- Script-pointer trainer flow for 5 trainers.
- Moltres encounter uses trainer-header/event flow (`EVENT_BEAT_MOLTRES`) plus cry side effect.
- Text table includes boulder text handlers and item pickups.

## C Implementation Evidence

From [`src/data/event_data.c`](/C:/Users/Anthony/pokered/src/data/event_data.c):

- `kWarps_VictoryRoad2F`, `kNpcs_VictoryRoad2F`, and `kItems_VictoryRoad2F` are present.
- NPC set includes trainers, Moltres NPC, and boulders as static entries.
- Map row `[0xc2]` wires warps/npcs/items but no map trainer array.

From C symbol search:

- No runtime references found for:
  - `EVENT_VICTORY_ROAD_1_BOULDER_ON_SWITCH`
  - `EVENT_VICTORY_ROAD_2_BOULDER_ON_SWITCH1`
  - `EVENT_VICTORY_ROAD_2_BOULDER_ON_SWITCH2`
  - `EVENT_BEAT_VICTORY_ROAD_2_TRAINER_*`
  - `EVENT_BEAT_MOLTRES`
  - `wVictoryRoad2FCurScript`

## Gap Checklist

## 1) Boulder Switch / Tile Mutation Map Logic Missing (High)

- ASM drives dynamic tile changes from switch-linked boulder state.
- C currently provides static boulder NPCs with no map script logic hooks shown.
- Impact:
  - Core puzzle progression/state behavior risk.

## 2) Trainer Script Flow Not Explicitly Mapped (High)

- ASM has trainer-header and script-pointer battle flow for 5 trainers.
- C map row for Victory Road 2F does not show dedicated trainer table wiring.
- Impact:
  - Encounter triggering/state persistence can diverge.

## 3) Moltres Event + Cry Script Flow Missing (High)

- ASM uses event-flag trainer flow + cry behavior for Moltres.
- C Moltres NPC has no explicit script callback in map data.
- Impact:
  - Legendary encounter fidelity/progression risk.

## Priority

1. Implement Victory Road 2F map-script hook for boulder switch tile mutation parity.  
2. Add explicit trainer encounter wiring for the 5 trainer NPCs.  
3. Add Moltres encounter callback/event flow including cry behavior parity.
