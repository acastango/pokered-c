# VermilionCity Parity Checklist (ASM vs C)

Date: 2026-05-05  
Scope: `scripts/VermilionCity.asm` vs C data/runtime wiring.

## Verdict

`VermilionCity` is **partial** with mixed translation status: key dock-gate step logic is ported, but multiple ASM interaction-side behaviors are still not explicitly wired.

## ASM Behavior Surface

From [`pokered-master/scripts/VermilionCity.asm`](/C:/Users/Anthony/pokered/pokered-master/scripts/VermilionCity.asm):

- Map script controls:
  - SS Anne-left callback/state handling
  - ticket/guard movement gating near dock
  - script-pointer state transitions for forced movement
- `VermilionCitySailor1Text` has conditional branches based on:
  - `EVENT_SS_ANNE_LEFT`
  - player facing/position
  - `S_S_TICKET` possession
- `VermilionCityGambler1Text` changes based on `EVENT_SS_ANNE_LEFT`.
- `VermilionCityMachopText` plays cry (`PlayCry(MACHOP)`) and continues with extra text.

## C Implementation Evidence

From [`src/game/ss_anne_scripts.c`](/C:/Users/Anthony/pokered/src/game/ss_anne_scripts.c):

- `VermilionScripts_StepCheck()` and `VermilionScripts_DoPush()` exist and explicitly port the dock ticket step gate behavior.

From [`src/data/event_data.c`](/C:/Users/Anthony/pokered/src/data/event_data.c):

- `kNpcs_VermilionCity` and `kSigns_VermilionCity` are mapped on row `[0x05]`.
- Vermilion city NPC entries are static text with `script=NULL` (including Sailor1, Gambler1, Machop).

From symbol search:

- No Vermilion city Machop callback (`Audio_PlayCry(SPECIES_MACHOP)`) found.
- No dedicated NPC callback wiring found for Vermilion city Sailor1/Gambler1 conditional text paths.

## Gap Checklist

## 1) Machop Cry + Follow-up Text Side Effect Missing (Medium)

- ASM: Machop interaction plays cry and then prints additional line.
- C: Machop entry is static text only.
- Impact:
  - Interaction fidelity loss, localized but visible.

## 2) Sailor1 Full Text Branching Not Fully NPC-Wired (Medium)

- ASM has rich conditional `text_asm` flow on interaction.
- C currently ports dock gate behavior via step-check logic, but Sailor1 NPC entry itself is static text.
- Impact:
  - Some interaction contexts may not match original branch behavior.

## 3) Gambler1 Post-SS-Anne Branch Not NPC-Wired (Low/Medium)

- ASM switches dialogue after `EVENT_SS_ANNE_LEFT`.
- C gambler line is static text in NPC table.
- Impact:
  - Progress-state dialogue fidelity gap.

## Priority

1. Add Vermilion city NPC callbacks for Machop cry behavior and conditional Sailor1/Gambler1 text branches.  
2. Keep existing step-check gate logic as foundation; unify with NPC callback flow for full ASM parity.
