# PowerPlant Parity Checklist (ASM vs C)

Date: 2026-05-05  
Scope: `scripts/PowerPlant.asm` vs C data/runtime wiring.

## Verdict

`PowerPlant` is **partial** with high-impact battle scripting gaps still evident.

## ASM Behavior Surface

From [`pokered-master/scripts/PowerPlant.asm`](/C:/Users/Anthony/pokered/pokered-master/scripts/PowerPlant.asm):

- Map script pointer state machine (`default/start battle/end battle`).
- Trainer headers for:
  - 8 Voltorb/Electrode fake-item encounters (`EVENT_BEAT_POWER_PLANT_VOLTORB_0..7`)
  - Zapdos encounter (`EVENT_BEAT_ZAPDOS`)
- Text handlers route each encounter through `TalkToTrainer`/battle init.
- Zapdos text script triggers cry (`PlayCry(ZAPDOS)` + wait).

## C Implementation Evidence

From [`src/data/event_data.c`](/C:/Users/Anthony/pokered/src/data/event_data.c):

- `kNpcs_PowerPlant` contains Zapdos NPC with `text=NULL`, `script=NULL`.
- `kItems_PowerPlant` contains 13 entries; first 8 are species-labeled comments (`VOLTORB`/`ELECTRODE`) with item id `0x00`.
- Map row `[0x53]` wires NPC/item arrays, but no map trainer array is attached for this map.

From C symbol search:

- No runtime references found for `EVENT_BEAT_POWER_PLANT_VOLTORB_*`, `EVENT_BEAT_ZAPDOS`, or `wPowerPlantCurScript`.
- No Power Plant-specific battle script module found.

## Gap Checklist

## 1) Voltorb/Electrode Encounter Script Flow Not Explicitly Mapped (High)

- ASM uses trainer-header/event-flag battle flow for all 8 fake-item encounters.
- C represents these as `item_event_t` rows with id `0x00`, with no explicit map trainer wiring in the map row.
- Impact:
  - Core encounter behavior and persistence semantics are at risk of divergence.

## 2) Zapdos Encounter Script + Cry Behavior Missing in Map Wiring (High)

- ASM has dedicated Zapdos trainer header event flow and cry side effect.
- C Zapdos NPC currently has no attached script callback.
- Impact:
  - Legendary encounter flow/fidelity likely incomplete.

## Priority

1. Implement explicit Power Plant encounter script module for fake-item battles and Zapdos flow parity.  
2. Wire Zapdos callback to include cry side effect parity after encounter text sequence.
