# pokered Timing Conversion Protocol (ASM <-> pokered-c)

## Goal
Convert animation/movement timing from original `pokered-master` ASM into `pokered-c` so on-screen motion matches original speed and cadence.

## 1) Canonical clocks

- `GB_VBLANK_HZ = 59.7275` (original hardware frame rate)
- ASM overworld loop rate:
  - `ASM_OW_HZ = GB_VBLANK_HZ / 2 = 29.86375`
  - Reason: `OverworldLoop` calls `DelayFrame` twice per iteration (`home/overworld.asm:41-44`).
- Current C main-loop rate target:
  - `C_MAIN_HZ ~= 62.5` because `FRAME_MS = 1000 / 60 = 16ms` (`src/main.c:107-109`)
  - Vsync is disabled (`src/platform/display.c:88-91`)
- Current C overworld logic rate:
  - `C_OW_HZ = C_MAIN_HZ / 2` via alternating frame gate (`src/game/game.c:766-779`)

## 2) Normalize ASM timing to VBlank units first

Always convert ASM behavior to **VBlank count** before converting to C.

- `DelayFrames(c)` -> `vb = c` (`home/delay.asm:1-6`)
- Overworld loop counters (for logic that advances once per `OverworldLoop`) -> `vb = 2 * loops`
  - Example: `wWalkCounter = 8` means `16` VBlanks (`home/overworld.asm:264-265`, `1439-1446`)

## 3) Core conversion equations

Given `vb` ASM VBlanks:

- Real time:
  - `seconds = vb / GB_VBLANK_HZ`
- C ticks for per-frame subsystems (battle/text/btransition):
  - `c_ticks_frame = round(vb * C_MAIN_HZ / GB_VBLANK_HZ)`
- C ticks for overworld-gated subsystems:
  - `c_ticks_ow = round(vb * C_OW_HZ / GB_VBLANK_HZ)`

Equivalent from ASM overworld-loop counts `loops`:

- `c_loops_ow = round(loops * C_OW_HZ / ASM_OW_HZ)`
- Since both divide by 2, this is also:
  - `c_loops_ow = round(loops * C_MAIN_HZ / GB_VBLANK_HZ)`

## 4) Current speed ratio in this repo

With current pacing (`C_MAIN_HZ ~= 62.5`):

- `R = C_MAIN_HZ / GB_VBLANK_HZ ~= 1.0464`
- Current engine runs about **4.64% faster** than original unless counters are scaled.

## 5) Verified subsystem mapping

- Overworld cadence:
  - ASM: 2x `DelayFrame` per loop (`home/overworld.asm:41-44`)
  - C: alternating gate + press latch (`src/game/game.c:766-779`)
- Player walk step:
  - ASM: `wWalkCounter = 8` (`home/overworld.asm:264-265`)
  - ASM scroll: `sla` step vectors -> 2 px per overworld iteration (`home/overworld.asm:1603-1610`)
  - C: `WALK_FRAMES = 8` (`src/game/player.c:66`), step starts with `gWalkTimer = WALK_FRAMES` (`src/game/player.c:362`), per active tick scroll/slide update (`src/game/player.c:420-427`)
- NPC movement:
  - ASM random walk counter is 16 (`engine/overworld/movement.asm:296-339`)
  - ASM scripted NPC walk counter is 8 (`engine/overworld/movement.asm:801-815`)
  - C random/scripted mirrors: `16` and `8` (`src/game/npc.c:44-47`)
- Tile animation:
  - ASM updates in VBlank ISR (`home/vblank.asm:28`)
  - C `Anim_UpdateTiles()` runs before overworld gate (`src/game/game.c:755-757`)
- Text pacing:
  - ASM per-char uses `PrintLetterDelay` with frame waits (`home/print_text.asm:4-40`)
  - C default `TEXT_LETTER_DELAY = 3` frames (`src/game/text.c:74`)
- Battle timing:
  - C battle transition/frame budgets are explicit 60Hz design (`src/game/battle_transition.c:20-29`)
  - HP bar anim uses 1 px / 2 frames to match ASM `DelayFrames(2)` style (`src/game/battle/battle_ui.c:2656-2677`)
  - Pokeball shake uses `40 + 16 = 56` frames/shake to mirror ASM (`src/game/battle/battle_ui.c:265-268`, `3501-3577`)

## 6) Practical protocol for new conversions

1. Identify whether ASM timing is:
   - `DelayFrames`-based (already VBlanks), or
   - Overworld-loop-counter-based (multiply by 2 VBlanks).
2. Convert to VBlanks (`vb`).
3. Choose target clock:
   - Overworld-gated logic -> `C_OW_HZ`
   - Battle/text/menu/non-gated -> `C_MAIN_HZ`
4. Apply equation from section 3 and round.
5. For long-running/repeating timers, use fixed-point accumulation (not repeated naive rounding) to avoid drift.

## 7) Recommended alignment strategy

Preferred: make `C_MAIN_HZ` match GB (`59.7275`) so most ASM counters become 1:1 mappings.

If keeping current `~62.5Hz` pacing, scale all imported ASM durations by `R ~= 1.0464` using the equations above.
