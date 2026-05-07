# CeladonMart3F Parity Checklist (ASM vs C)

Date: 2026-05-05  
Scope: `scripts/CeladonMart3F.asm` vs C data/runtime wiring.

## Verdict

`CeladonMart3F` is **partial** with one key missing reward script flow.

## ASM Behavior Surface

From [`pokered-master/scripts/CeladonMart3F.asm`](/C:/Users/Anthony/pokered/pokered-master/scripts/CeladonMart3F.asm):

- Map script itself is simple (`EnableAutoTextBoxDrawing`).
- Main dynamic behavior is clerk `text_asm`:
  - checks `EVENT_GOT_TM18`
  - gives `TM_COUNTER`
  - handles bag-full branch
  - sets `EVENT_GOT_TM18` on success
  - shows explanation text once obtained
- Other NPC/sign texts are mostly static.

## C Implementation Evidence

From [`src/data/event_data.c`](/C:/Users/Anthony/pokered/src/data/event_data.c):

- `kWarps_CeladonMart3F`, `kNpcs_CeladonMart3F`, `kSigns_CeladonMart3F` are present and mapped on row `[0x7c]`.
- Clerk NPC entry is static text with `script=NULL`.
- No CeladonMart3F-specific callback wiring is present in this map data.

From symbol search:

- No CeladonMart3F/TM18 clerk runtime script implementation reference found.

## Gap Checklist

## 1) TM18 (Counter) Clerk Reward Flow Missing (High)

- ASM has full conditional item-award logic with state and bag-full handling.
- C currently has only static clerk dialogue.
- Impact:
  - Important item reward and progression-state dialogue behavior gap.

## Priority

1. Add CeladonMart3F clerk callback implementing `EVENT_GOT_TM18` gate, TM_COUNTER give attempt, bag-full branch, and post-gift explanation text parity.
