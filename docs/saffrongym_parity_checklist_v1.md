# SaffronGym Parity Checklist (ASM vs C)

Date: 2026-05-05  
Scope: `scripts/SaffronGym.asm` vs C data/runtime wiring.

## Verdict

`SaffronGym` is **partial** with major dynamic script behavior missing from current C wiring.

## ASM Behavior Surface

From [`pokered-master/scripts/SaffronGym.asm`](/C:/Users/Anthony/pokered/pokered-master/scripts/SaffronGym.asm):

- Map script state machine exists (`SaffronGym_ScriptPointers`):
  - default trainer checks
  - start battle
  - end battle
  - Sabrina post-battle processing
- Sabrina flow includes:
  - pre-battle intro + battle engage
  - Marsh Badge award path
  - TM46 (`TM_PSYWAVE`) give flow
  - bag-full branch
  - post-TM advice
- Gym trainer header table exists for 7 trainers with beat-event flags.
- Gym guide text branches on `EVENT_BEAT_SABRINA`.

## C Implementation Evidence

From [`src/data/event_data.c`](/C:/Users/Anthony/pokered/src/data/event_data.c):

- `kWarps_SaffronGym` and `kNpcs_SaffronGym` are present.
- Sabrina and guide NPCs are static text entries with `script=NULL`.
- Trainer NPC entries are present but also `script=NULL`.
- Map row `[0xb2]` maps Saffron Gym via data arrays only.

From C symbol search:

- No runtime references found for:
  - `EVENT_BEAT_SABRINA`
  - `EVENT_GOT_TM46`
  - `EVENT_BEAT_SAFFRON_GYM_TRAINER_*`
  - `TEXT_SAFFRONGYM_*` script handlers
  - `wSaffronGymCurScript`
- No Saffron Gym specific script module found under `src/game/`.

## Gap Checklist

## 1) Sabrina Battle-to-Reward State Machine Missing (High)

- ASM has explicit script-pointer driven state progression.
- C currently exposes only static NPC text with no gym-specific script callback.
- Impact:
  - Core gym completion semantics likely incomplete (battle trigger/post-battle progression).

## 2) TM46 (Psywave) Award Logic Missing (High)

- ASM includes full item-award logic with bag-full handling and `EVENT_GOT_TM46` gating.
- C has no equivalent callback reference for this flow.
- Impact:
  - Important reward/progression behavior gap.

## 3) Trainer Beat-Flag Flow for Gym Trainers Missing (High)

- ASM has per-trainer headers + beat flags and deactivation behavior.
- C has trainer NPC slots but no attached map-script trainer dispatch for Saffron Gym.
- Impact:
  - Trainer encounter/state persistence may diverge from ASM behavior.

## 4) Gym Guide Post-Badge Branch Missing (Medium)

- ASM gym guide text branches on Sabrina defeat event.
- C currently holds only one static guide text entry.
- Impact:
  - Dialogue-state fidelity gap after gym completion.

## Priority

1. Implement Saffron Gym script module and map-script callback wiring (battle state machine + Sabrina post-battle).  
2. Implement Sabrina TM46 reward and bag-full branch parity.  
3. Add trainer-header equivalent wiring for the 7 gym trainers and event flag transitions.  
4. Add gym guide conditional text branch based on Sabrina defeat state.
