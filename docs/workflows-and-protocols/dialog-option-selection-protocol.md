# Dialog Option Selection Protocol (Reusable)

Use this protocol for text-driven option menus that must behave like Gen 1 scripting flow.

Current consumers:
- Rocket Hideout Lift floor menu
- Celadon Roof vending machine menu

Implemented in:
- `src/game/elevator_menu.c`

## Goals

- No accidental confirm from the interaction press that opened the dialog.
- Menu opens only after text lifecycle reaches the correct state.
- Input is edge-based and deterministic.
- Cancel/confirm exits cleanly and restores overworld view.

## Protocol

1. Show prompt text first.
- Use text flow to present prompt (example: "Which floor do you want?" / vending intro).
- For these prompts, set `wDoNotWaitForButtonPress = 1` when needed so script sequencing matches ASM-style continuation.

2. Queue menu open; do not open immediately.
- Call a queue function from the interaction callback.
- Open happens later from the main loop only when:
  - no text box is active, and
  - no menu is currently open.

3. On open, install input guards.
- `wait_a_release = 1`: require A to be released first.
- `wait_neutral = 1`: require one full neutral frame (no dpad/A/B held).
- `prev_held = 0xFF` initially, then use edge detection against held-state.

4. Consume opener press.
- Clear `hJoyPressed` on open so the same press cannot confirm the first option.

5. Use edge-triggered navigation only.
- Compute `pressed_local = held & ~prev_held`.
- Move cursor on `pressed_local` up/down.
- Act on `pressed_local` A/B only.

6. Close paths are explicit.
- B or `CANCEL`:
  - play confirm/cancel SFX,
  - close text/menu state,
  - rebuild map/NPC view.
- A on a real option:
  - execute action,
  - close menu state,
  - transition to post-action sequence.

7. Post-action async effects run in their own state.
- Example (vending): delivery SFX loop timing + final "popped out!" text.
- Keep this out of immediate input handler to avoid text/menu overlap bugs.

## Timing Rule (Vending Delivery)

ASM-equivalent cadence:
- play delivery SFX 60 times
- once every 2 frames

At 60 Hz:
- trigger cadence: 30 Hz
- total duration: ~120 frames (~2 seconds)

Use a retrigger-capable SFX call in this loop (`CollisionRetrigger` style) so cadence does not get throttled by "already playing" guards.

## Text Formatting Rule

For vending completion text, follow Gen 1 line behavior:
- line 1: item name
- line 2: `popped out!`
- terminate with `@`

Example:
- `"SODA POP\npopped out!@"`

## Why this protocol exists

This prevents the most common regressions:
- opener press auto-selecting first entry
- menu appearing before text sequence is done
- held-input repeats causing skipped cursor states
- overlapping text/menu render ownership issues
