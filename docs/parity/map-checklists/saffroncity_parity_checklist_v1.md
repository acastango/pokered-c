# SaffronCity Parity Checklist (ASM vs C)

Date: 2026-05-05  
Scope: `scripts/SaffronCity.asm` vs C data/runtime wiring.

## Verdict

`SaffronCity` appears **largely data-parity aligned** for its own map script surface.  
Its prior `partial` classification was structural (no dedicated `saffron_city_scripts.c`), not strong evidence of missing core city-script behavior.

## ASM Behavior Surface

From [`pokered-master/scripts/SaffronCity.asm`](/C:/Users/Anthony/pokered/pokered-master/scripts/SaffronCity.asm):

- Script entry:
  - `SaffronCity_Script: jp EnableAutoTextBoxDrawing`
- No map script state machine, no trainer header table, no door callback logic in this file.
- Main content is text/sign pointer table:
  - 15 NPC text entries (Rockets, Scientist, Silph workers, Gentleman, Pidgeot, Rocker, etc.)
  - sign entries (city sign, dojo, gym, trainer tips, Silph sign, etc.)
- One notable text behavior:
  - Pidgeot text includes cry command (`sound_cry_pidgeot`).

## C Implementation Evidence

From [`src/data/event_data.c`](/C:/Users/Anthony/pokered/src/data/event_data.c):

- `kWarps_SaffronCity` present.
- `kNpcs_SaffronCity` present with 15 NPCs.
- `kSigns_SaffronCity` present with 10 sign entries.
- Map rows for both Saffron map IDs:
  - `[0x0a]` and `[0x0b]` use `kWarps_SaffronCity`, `kNpcs_SaffronCity`, `kSigns_SaffronCity`.

From runtime search:

- No dedicated `saffron_city_scripts.c` file found.
- No SaffronCity-specific dynamic script function references found for this city map in `src/game/*.c`.
- Gate behavior for Saffron access is implemented in [`src/game/gate_scripts.c`](/C:/Users/Anthony/pokered/src/game/gate_scripts.c) via `EVENT_GAVE_SAFFRON_GUARDS_DRINK`, which belongs to route gate maps rather than `SaffronCity.asm` itself.

## Gap Checklist

## 1) Dedicated City Module Absence (Low)

- Current:
  - No `saffron_city_scripts.c`.
- ASM expectation:
  - Not necessarily required; ASM file itself is mostly text pointers + auto textbox drawing.
- Impact:
  - Low by itself.

## 2) Pidgeot Cry Text Behavior Verification (Medium)

- ASM:
  - Pidgeot text explicitly triggers cry.
- C:
  - NPC text present (`"PIDGEOT: Bi bibii!@"`) in `event_data.c`.
  - Need explicit verification that this specific NPC interaction triggers cry equivalent (city-level handler may not attach per-text cry actions).
- Impact:
  - Medium (localized fidelity detail).

## 3) Rocket/Silph Post-Event Presence Logic (Medium, cross-map)

- City NPC set includes both Rocket and post-liberation-flavored lines.
- Whether NPC visibility/text swaps match event progression can depend on broader Silph takeover flow, not solely `SaffronCity.asm`.
- Impact:
  - Medium, but requires cross-map audit (Silph Co progression), not city script file alone.

## Reclassification Note

For `SaffronCity.asm`, the matrix `partial` status should be interpreted as:
- "Grouped/data-owned, no dedicated module", **not** automatically "major missing behavior".

## Immediate Next Checks (If We Continue Here)

1. Verify if city NPC text dispatch supports per-text side effects needed for Pidgeot cry parity.  
2. Verify event-driven NPC visibility/state transitions tied to Silph liberation timeline for Saffron city map IDs `0x0a/0x0b`.
