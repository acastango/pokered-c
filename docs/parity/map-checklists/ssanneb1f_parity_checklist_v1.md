# SSAnneB1F Parity Checklist (ASM vs C)

Date: 2026-05-05  
Scope: `scripts/SSAnneB1F.asm` vs C implementation/data.

## Verdict

`SSAnneB1F.asm` appears **parity-aligned** for its own scope.

## ASM Behavior Surface

From [`pokered-master/scripts/SSAnneB1F.asm`](/C:/Users/Anthony/pokered/pokered-master/scripts/SSAnneB1F.asm):

- Script is minimal:
  - `SSAnneB1F_Script: jp EnableAutoTextBoxDrawing`
- Text pointer table is effectively unused in this file:
  - `text_end ; unused`
- No trainer headers, no custom event state machine, no special callbacks in this file.

## C Implementation Evidence

From [`src/data/event_data.c`](/C:/Users/Anthony/pokered/src/data/event_data.c):

- `kWarps_SSAnneB1F` present.
- Map row:
  - `[0x62] = { kWarps_SSAnneB1F, 6, NULL, 0, NULL, 0, NULL, 0, ... }`
  - matches expectation that this map has no NPC/script content in `SSAnneB1F.asm`.

## Clarification: SSAnneB1F vs SSAnneB1FRooms

- The trainer/NPC-heavy gameplay is in **SSAnneB1FRooms**, represented separately:
  - `kNpcs_SSAnneB1FRooms`
  - `kItems_SSAnneB1FRooms`
  - `kTrainers_SSAnneB1FRooms`
  - map row `[0x68]`
- This distinction is important to avoid false “partial” classification for `SSAnneB1F.asm` itself.

## Confirmed Gaps

No clear structural or procedural gap identified for `SSAnneB1F.asm` itself in this pass.
