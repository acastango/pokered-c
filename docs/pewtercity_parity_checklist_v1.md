# PewterCity Parity Checklist (ASM vs C)

Date: 2026-05-05  
Scope: `scripts/PewterCity.asm` vs C data/runtime wiring.

## Verdict

`PewterCity` is **partial**: the east-exit youngster escort is ported, but other PewterCity ASM script-state behavior is not fully represented.

## ASM Behavior Surface

From [`pokered-master/scripts/PewterCity.asm`](/C:/Users/Anthony/pokered/pokered-master/scripts/PewterCity.asm):

- Script-pointer state machine (`wPewterCityCurScript`) with 7 script states.
- `PewterCityDefaultScript`:
  - resets `wMuseum1FCurScript`
  - clears `EVENT_BOUGHT_MUSEUM_TICKET`
  - runs east-exit Brock pre-gym trigger check.
- Youngster escort flow:
  - trigger tiles near east exit
  - forced follow-to-gym sequence
  - hide/reset youngster object.
- Super Nerd 1 museum flow:
  - yes/no branch on museum question
  - if "no", starts scripted escort/show sequence
  - hide/reset museum guide object.
- Super Nerd 2 text has yes/no branch.

## C Implementation Evidence

From [`src/game/gate_scripts.c`](/C:/Users/Anthony/pokered/src/game/gate_scripts.c):

- Explicit grouped port exists for east-exit youngster escort:
  - `Gate_PewterEastCheck`
  - `Gate_PewterTick`
  - `Gate_PewterIsActive`
- Comments explicitly reference ASM sources (`scripts/PewterCity.asm`, `engine/events/pewter_guys.asm`).
- Includes trigger coords, movement sequence, hide/show/reset, and music behavior.

From [`src/data/event_data.c`](/C:/Users/Anthony/pokered/src/data/event_data.c):

- Pewter map is fully present in map dispatch data (`kWarps_PewterCity`, `kNpcs_PewterCity`, `kSigns_PewterCity`).
- Super Nerd 1/2 and youngster entries are present, but city NPCs are static-text entries (`script=NULL`).

From symbol search:

- `EVENT_BOUGHT_MUSEUM_TICKET` appears in constants only; no runtime logic use found.
- No C-side equivalent found for PewterCity script-pointer/state-machine handling (`wPewterCityCurScript`, museum-guide escort chain, Super Nerd 2 yes/no branch logic).

## Gap Checklist

## 1) Museum Guide (Super Nerd 1) Scripted Escort Chain Missing (High)

- ASM has a full branch + scripted movement/hide/reset flow when player says they did not visit museum.
- C currently provides only static opening text for this NPC at city level.
- Impact:
  - Lost interactive branch and scripted city behavior.

## 2) `PewterCityDefaultScript` Side Effects Not Evidenced (Medium)

- ASM resets museum script state and clears `EVENT_BOUGHT_MUSEUM_TICKET` on default city script path.
- No matching C runtime usage of that event/state behavior found in the audited files.
- Impact:
  - Potential cross-map/state consistency mismatch around museum flow.

## 3) Super Nerd 2 Yes/No Response Branch Missing (Low-Medium)

- ASM text is conditional (`DoYouKnowWhatImDoing` -> yes/no responses).
- C city entry is static single text string.
- Impact:
  - Dialogue parity gap, lower gameplay criticality than escort/event logic.

## Priority

1. Implement Super Nerd 1 city callback with yes/no and museum escort/hide/reset behavior.
2. Add/verify default Pewter city script-equivalent side effects (`EVENT_BOUGHT_MUSEUM_TICKET` + museum script reset semantics).
3. Convert Super Nerd 2 to callback-driven yes/no branch text.
