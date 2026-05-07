# SilphCo5F Parity Checklist (ASM vs C)

Date: 2026-05-05  
Scope: `scripts/SilphCo5F.asm` vs C data/runtime wiring.

## Verdict

`SilphCo5F` is **partial** with major script/state behavior gaps.

## ASM Behavior Surface

From [`pokered-master/scripts/SilphCo5F.asm`](/C:/Users/Anthony/pokered/pokered-master/scripts/SilphCo5F.asm):

- Map callback logic for card-key doors:
  - gate coordinate checks
  - unlocked-door event propagation
  - tile replacement for unlocked doors
- Script-pointer trainer flow (`default/start/end`) for 4 trainers.
- Silph worker text branch based on Silph progression helper script.
- Item pickups include `CARD_KEY`.

## C Implementation Evidence

From [`src/data/event_data.c`](/C:/Users/Anthony/pokered/src/data/event_data.c):

- `kWarps_SilphCo5F`, `kNpcs_SilphCo5F`, `kItems_SilphCo5F` exist and map row `[0xd2]` is present.
- Trainer-like NPC entries are present but with `script=NULL`.
- No map trainer table is attached on row `[0xd2]`.

From symbol search:

- `EVENT_BEAT_SILPH_CO_5F_TRAINER_0..3` and `EVENT_SILPH_CO_5_UNLOCKED_DOOR1..3` are defined in constants.
- No runtime references found for:
  - those trainer/door events
  - `wSilphCo5FCurScript`
  - unlocked-door callback flow symbols.

## Gap Checklist

## 1) Card-Key Door Callback / Tile Mutation Logic Missing (High)

- ASM has explicit per-load door state and tile updates.
- C currently has no SilphCo5F map-script callback wiring for this behavior.
- Impact:
  - Core floor navigation/state progression risk.

## 2) Trainer Battle Script Flow for 4 Trainers Missing (High)

- ASM uses trainer headers and script-pointer battle flow.
- C map wiring does not show explicit trainer table or callbacks for these NPCs.
- Impact:
  - Encounter/state persistence behavior divergence risk.

## 3) Silph Worker Conditional Text Branch Missing (Medium)

- ASM text path changes with Silph progression helper.
- C worker line is static text.
- Impact:
  - Progress-state dialogue fidelity gap.

## Priority

1. Implement SilphCo5F map-script callback for card-key door unlock state and tile replacement.  
2. Add explicit trainer encounter wiring for the 4 SilphCo5F trainer NPCs.  
3. Add worker conditional dialogue branch parity.
