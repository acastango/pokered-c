# Route16Gate1F Parity Checklist (ASM vs C)

Date: 2026-05-05  
Scope: `scripts/Route16Gate1F.asm` vs C data/runtime wiring.

## Verdict

`Route16Gate1F` is **partial** with meaningful gating/script-control behavior missing.

## ASM Behavior Surface

From [`pokered-master/scripts/Route16Gate1F.asm`](/C:/Users/Anthony/pokered/pokered-master/scripts/Route16Gate1F.asm):

- Map script controls Cycling Road entry behavior:
  - checks if `BICYCLE` is in bag
  - blocks and redirects player movement if not
  - displays guard wait-up text and guard explanation text in sequence
  - manipulates joypad/scripted movement state
- Guard text also conditionally branches on bicycle possession.

## C Implementation Evidence

From [`src/data/event_data.c`](/C:/Users/Anthony/pokered/src/data/event_data.c):

- `kWarps_Route16Gate1F` and `kNpcs_Route16Gate1F` are present.
- Guard and gambler use static text entries only (`script=NULL`).
- Map rows `[0xba]` and `[0xe7]` wire map data only; no map-specific script callback visible.

From C symbol search:

- No Route16Gate1F-specific script module or state-machine symbols found (`wRoute16Gate1FCurScript`, script pointer handlers).

## Gap Checklist

## 1) Bicycle Entry Gate Enforcement Script Missing (High)

- ASM enforces no-pedestrian behavior with coordinate checks, movement redirection, and staged dialogue.
- C currently provides only static guard text.
- Impact:
  - Core gate behavior and player-flow logic divergence.

## 2) Conditional Guard Text Branch on Bicycle Missing (Medium)

- ASM guard text branches to explanation vs no-pedestrian warning based on bicycle possession.
- C guard line is static.
- Impact:
  - Context-sensitive dialogue fidelity gap.

## Priority

1. Implement Route16Gate1F script callback/state machine for bicycle-gate enforcement and forced movement flow.  
2. Add conditional guard dialogue branch matching bicycle possession state.
