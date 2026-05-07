# SSAnne1F Parity Checklist (ASM vs C)

Date: 2026-05-05  
Scope: `scripts/SSAnne1F.asm` vs C implementation/data.

## Verdict

`SSAnne1F` appears **parity-aligned** for its actual ASM scope.

## ASM Behavior Surface

From [`pokered-master/scripts/SSAnne1F.asm`](/C:/Users/Anthony/pokered/pokered-master/scripts/SSAnne1F.asm):

- Script function:
  - `SSAnne1F_Script` only does `EnableAutoTextBoxDrawing` and returns.
- Text pointers:
  - `SSAnne1FWaiterText`
  - `SSAnne1FSailorText`
- No trainer headers, no custom state machine, no map-specific scripted movement in this file.

## C Implementation Evidence

From [`src/data/event_data.c`](/C:/Users/Anthony/pokered/src/data/event_data.c):

- `kWarps_SSAnne1F` present.
- `kNpcs_SSAnne1F` present with 2 NPCs:
  - waiter text
  - sailor text
- Map binding:
  - `[0x5f] = { kWarps_SSAnne1F, ..., kNpcs_SSAnne1F, 2, ... }`

From runtime/module context:

- No dedicated `SSAnne1F` script state machine needed for this map based on ASM.
- `ss_anne_scripts.c` correctly focuses on:
  - `SSAnne2F` rival sequence
  - Captain HM01 sequence
  - departure/dock logic
  which are separate script files from `SSAnne1F.asm`.

## Clarification: SSAnne1F vs SSAnne1FRooms

- `SSAnne1FRooms` has trainer/item complexity and is represented separately in C:
  - `kNpcs_SSAnne1FRooms`
  - `kTrainers_SSAnne1FRooms`
  - map row `[0x66]`
- That complexity should not be attributed to `SSAnne1F.asm`.

## Confirmed Gaps

No clear structural or procedural gap identified for `SSAnne1F.asm` in this pass.
