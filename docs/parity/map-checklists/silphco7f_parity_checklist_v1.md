# SilphCo7F Parity Checklist (ASM vs C)

Date: 2026-05-05  
Scope: `scripts/SilphCo7F.asm` vs C data/runtime wiring.

## Verdict

`SilphCo7F` is **partial** with major event-script behavior gaps.

## ASM Behavior Surface

From [`pokered-master/scripts/SilphCo7F.asm`](/C:/Users/Anthony/pokered/pokered-master/scripts/SilphCo7F.asm):

- Card-key door callback logic with coordinate checks and tile replacement.
- Script-pointer trainer flow for 4 trainers.
- Full Rival encounter sequence on this floor:
  - trigger coords, movement, battle init, post-battle text, exit/hide.
- Silph worker gift flow for Lapras (`GivePokemon LAPRAS`) and branching text based on progression flags.
- Additional Silph worker dialogue branches based on `EVENT_BEAT_SILPH_CO_GIOVANNI`.

## C Implementation Evidence

From [`src/data/event_data.c`](/C:/Users/Anthony/pokered/src/data/event_data.c):

- `kWarps_SilphCo7F`, `kNpcs_SilphCo7F`, `kItems_SilphCo7F` exist and map row `[0xd4]` is present.
- Trainer-like NPCs, rival NPC, and silph worker NPCs are currently data entries with `script=NULL`.
- Items on floor are present (Calcium, TM Swords Dance), but no dynamic script wiring in this map row.

From symbol search:

- `EVENT_BEAT_SILPH_CO_7F_TRAINER_*`, `EVENT_SILPH_CO_7_UNLOCKED_DOOR*`, `EVENT_BEAT_SILPH_CO_RIVAL`, and `EVENT_BEAT_SILPH_CO_GIOVANNI` exist as constants.
- No SilphCo7F-specific runtime script symbols found (e.g., `wSilphCo7FCurScript` or map-script owner module references).

## Gap Checklist

## 1) Card-Key Door Callback / Tile Mutation Logic Missing (High)

- ASM has explicit floor-door unlock state and tile updates.
- C currently shows no SilphCo7F map callback wiring for door state behavior.
- Impact:
  - Core navigation/progression behavior risk.

## 2) Rival 7F Encounter Sequence Missing (High)

- ASM includes full rival trigger, scripted movement, battle flow, and cleanup.
- C currently has rival as static NPC text in map data.
- Impact:
  - Major story battle/progression behavior gap.

## 3) Lapras Gift + Branching Worker Logic Missing (High)

- ASM handles one-time Lapras gift and multiple event-dependent text branches.
- C worker entries are static text with no callback flow.
- Impact:
  - Major reward and progression-state interaction gap.

## 4) Trainer Event Flow for 4 Floor Trainers Missing (High)

- ASM has trainer headers and script battle flow.
- C map row does not show explicit trainer-table/callback wiring for this floor.
- Impact:
  - Encounter state fidelity risk.

## Priority

1. Implement SilphCo7F map-script owner with card-key door state/tile logic.  
2. Implement Silph 7F rival sequence parity (trigger, movement, battle, exit).  
3. Implement Lapras gift callback and worker branch logic.  
4. Add trainer encounter wiring for the 4 trainers on this floor.
