# Route25 Parity Notes

Date: 2026-05-05

## Summary
- ASM `Route25.asm` is mostly standard trainer-map flow plus one map-load callback (`Route25ToggleBillsScript`).
- C has strong data coverage for Route 25 trainers/items/sign/warps.
- Potential gap: no explicit C hook found for `Route25ToggleBillsScript` behavior that toggles Bill/Nugget-bridge related objects based on Bill quest flags.

## ASM Evidence
- File: `pokered-master/scripts/Route25.asm`
- Notable callback logic:
  - `Route25ToggleBillsScript`
  - checks/sets:
    - `EVENT_LEFT_BILLS_HOUSE_AFTER_HELPING`
    - `EVENT_MET_BILL_2`
    - `EVENT_BILL_SAID_USE_CELL_SEPARATOR`
    - `EVENT_GOT_SS_TICKET`
  - object toggles for Bill/Nugget bridge states.

## C Evidence
- File: `src/data/event_data.c`
  - `kNpcs_Route25` (9 trainer-like NPCs)
  - `kItems_Route25` (TM_SEISMIC_TOSS)
  - `kSigns_Route25` (Bill sign)
  - `kTrainers_Route25` (9 trainer entries)
  - map row `[0x24]` includes trainer table and items.
- Bill quest events are present and used in `bills_house_scripts.c` and `cerulean_scripts.c`.
- No direct Route25-specific callback equivalent to `Route25ToggleBillsScript` found in current search.

## Assessment
- Status: `partial`
- Keyness: `medium`
- Reason: core combat/item route behavior appears present; callback-driven Bill/Nugget object toggle parity needs direct verification/implementation.
