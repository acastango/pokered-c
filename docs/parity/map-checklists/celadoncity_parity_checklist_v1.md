# CeladonCity Parity Checklist (ASM vs C)

Date: 2026-05-05  
Scope: `scripts/CeladonCity.asm` vs C data/runtime wiring.

## Verdict

`CeladonCity` is **partial** with concrete behavioral gaps, not just structural grouping differences.

## ASM Behavior Surface

From [`pokered-master/scripts/CeladonCity.asm`](/C:/Users/Anthony/pokered/pokered-master/scripts/CeladonCity.asm):

- `CeladonCity_Script`:
  - calls `EnableAutoTextBoxDrawing`
  - resets `EVENT_1B8..EVENT_1BF`
  - resets `EVENT_67F`
- `CeladonCityGramps3Text` is `text_asm`:
  - checks `EVENT_GOT_TM41`
  - gives `TM_SOFTBOILED`
  - handles no-room branch
  - sets `EVENT_GOT_TM41` on success
  - shows explanation text after already obtained
- `CeladonCityPoliwrathText`:
  - runs cry behavior (`PlayCry(POLIWRATH)`) after text.

## C Implementation Evidence

From [`src/data/event_data.c`](/C:/Users/Anthony/pokered/src/data/event_data.c):

- `kWarps_CeladonCity`, `kNpcs_CeladonCity`, and `kSigns_CeladonCity` are present.
- Map dispatch row `[0x06]` uses those arrays.
- `TEXT_CELADONCITY_GRAMPS3` entry is static text with `script=NULL`.
- `TEXT_CELADONCITY_POLIWRATH` entry is static text with `script=NULL`.

From symbol usage search:

- `EVENT_GOT_TM41` exists as a constant, but no Celadon callback implementation was found.
- `EVENT_1B8`, `EVENT_1BF`, and `EVENT_67F` appear defined/named only; no runtime reset usage found.
- No Celadon-specific `Audio_PlayCry(SPECIES_POLIWRATH)` callback found.

## Gap Checklist

## 1) TM41 Gift Flow Missing (High)

- ASM has a full conditional item-give script for Gramps3 (`TM_SOFTBOILED`, bag-full branch, post-gift explanation).
- C currently stores only pre-gift text with no callback.
- Impact:
  - Player can miss intended TM reward logic and progression semantics for this interaction.

## 2) Poliwrath Cry Side Effect Missing (Medium)

- ASM explicitly plays Poliwrath cry on interaction.
- C currently uses plain text only with no script callback.
- Impact:
  - Localized fidelity difference; lower gameplay impact than item logic but still canonical behavior loss.

## 3) Celadon Script Event Resets Not Mapped (Medium)

- ASM reset calls (`EVENT_1B8..EVENT_1BF`, `EVENT_67F`) are part of map script entry behavior.
- No direct C equivalent found in current runtime wiring for Celadon city script entry.
- Impact:
  - Potential event-state drift depending on where these flags are consumed.

## Priority

1. Implement Gramps3 TM41 callback and wire in `kNpcs_CeladonCity`.  
2. Implement Poliwrath cry callback and wire in `kNpcs_CeladonCity`.  
3. Trace consumers of `EVENT_1B8..EVENT_1BF` and `EVENT_67F` and add equivalent reset hook if required.
