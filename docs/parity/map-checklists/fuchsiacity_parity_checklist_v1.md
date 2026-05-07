# FuchsiaCity Parity Checklist (ASM vs C)

Date: 2026-05-05  
Scope: `scripts/FuchsiaCity.asm` vs C data/runtime wiring.

## Verdict

`FuchsiaCity` is **partial** with specific sign-script behavior gaps.

## ASM Behavior Surface

From [`pokered-master/scripts/FuchsiaCity.asm`](/C:/Users/Anthony/pokered/pokered-master/scripts/FuchsiaCity.asm):

- `FuchsiaCity_Script` is simple auto-textbox draw.
- Most NPC/sign entries are plain text.
- Several signs are `text_asm` and call `DisplayPokedex`:
  - Chansey sign
  - Voltorb sign
  - Kangaskhan sign
  - Slowpoke sign
  - Lapras sign
- Fossil sign is dynamic:
  - branches on `EVENT_GOT_DOME_FOSSIL` / `EVENT_GOT_HELIX_FOSSIL`
  - prints species-specific text
  - calls `DisplayPokedex(OMANYTE|KABUTO)` for resolved branch.

## C Implementation Evidence

From [`src/data/event_data.c`](/C:/Users/Anthony/pokered/src/data/event_data.c):

- `kWarps_FuchsiaCity`, `kNpcs_FuchsiaCity`, `kSigns_FuchsiaCity` are present and mapped on row `[0x07]`.
- NPC entries are data-driven and mostly static text.
- Signs for Chansey/Voltorb/Kangaskhan/Slowpoke/Lapras/Fossil are static text entries only.
- No per-sign callback/script hooks are attached in Fuchsia City sign data.

From C symbol search:

- No FuchsiaCity-specific runtime script module reference found for these sign behaviors.

## Gap Checklist

## 1) Pokedex Display Sign Scripts Missing (Medium)

- ASM displays pokedex entries for multiple zoo signs.
- C currently presents text only, with no `DisplayPokedex` side effect for these signs.
- Impact:
  - Canonical interaction fidelity loss; moderate player-facing behavior difference.

## 2) Fossil Sign Branching + Pokedex Display Missing (Medium)

- ASM has event-conditional fossil sign behavior and dynamic species display.
- C fossil sign is static and not event-conditional.
- Impact:
  - Event-state dependent lore/feedback behavior missing.

## Priority

1. Add Fuchsia sign callbacks for pokedex-display signs.  
2. Add fossil sign conditional callback with `EVENT_GOT_DOME_FOSSIL` / `EVENT_GOT_HELIX_FOSSIL` branch parity and species display.
