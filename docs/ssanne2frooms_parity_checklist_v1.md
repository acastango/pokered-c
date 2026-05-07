# SSAnne2FRooms Parity Checklist (ASM vs C)

Date: 2026-05-05  
Scope: `scripts/SSAnne2FRooms.asm` vs C data/runtime wiring.

## Verdict

`SSAnne2FRooms` appears **mostly mapped** with one clear interaction-side-effect gap.

## ASM Behavior Surface

From [`pokered-master/scripts/SSAnne2FRooms.asm`](/C:/Users/Anthony/pokered/pokered-master/scripts/SSAnne2FRooms.asm):

- Script-pointer trainer flow for 4 trainers.
- Item pickups for Max Ether and Rare Candy.
- Most non-trainer NPC text is `text_asm` but equivalent to simple print.
- One special behavior:
  - `SSAnne2FRoomsGentleman3Text` prints text and calls `DisplayPokedex(SNORLAX)`.

## C Implementation Evidence

From [`src/data/event_data.c`](/C:/Users/Anthony/pokered/src/data/event_data.c):

- `kWarps_SSAnne2FRooms`, `kNpcs_SSAnne2FRooms`, `kItems_SSAnne2FRooms` are present.
- `kTrainers_SSAnne2FRooms` exists with 4 trainers and beat-event flags.
- Map row `[0x67]` wires trainers and items for this map.
- Gentleman3 text is static string with `script=NULL`; no callback attached for Pokedex display side effect.

## Gap Checklist

## 1) Gentleman3 SNORLAX Pokedex Display Missing (Low/Medium)

- ASM invokes `DisplayPokedex(SNORLAX)` after the text.
- C currently keeps only dialogue text for that NPC.
- Impact:
  - Localized fidelity gap; no major progression impact.

## Priority

1. Add an SS Anne 2F Rooms callback for Gentleman3 text to invoke Pokedex display for Snorlax after printing text.
