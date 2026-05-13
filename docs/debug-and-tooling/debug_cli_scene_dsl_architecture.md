# Debug CLI Scene DSL Architecture (v0.1)

This note explains how the scene DSL is wired internally for technical contributors.

Primary source:

- [debug_cli.c](C:/Users/Anthony/pokered/src/game/debug_cli.c)

Related runtime APIs:

- [npc.c](C:/Users/Anthony/pokered/src/game/npc.c)
- [npc.h](C:/Users/Anthony/pokered/src/game/npc.h)
- [text.h](C:/Users/Anthony/pokered/src/game/text.h)

---

## High-Level Model

The scene system is an in-engine interpreter embedded in Debug CLI:

1. `scene_run <name>` loads `bugs/scenes/<name>.scene`
2. file lines are parsed into an in-memory command buffer
3. a per-frame interpreter in `DebugCLI_Tick()` advances commands
4. commands mutate live runtime state (NPCs, text, input mask)

No separate VM thread/process exists. It is tick-driven and single-runner.

---

## Core Data Structures

In `debug_cli.c`:

- `scene_cmd_t`:
  - opcode (`scene_op_t`)
  - actor id (string key)
  - small integer operands (`a/b/c`)
  - text payload for `say`
- `scene_actor_t`:
  - actor table entry mapping script actor id -> runtime NPC index
- global runner state:
  - `s_scene_active`, `s_scene_pc`
  - command buffer (`s_scene_cmds`)
  - wait/move substates (`s_scene_wait`, `s_scene_move_steps_left`, etc.)

Capacity limits (v0.1):

- max commands: `SCENE_CMD_MAX`
- max actor bindings: `SCENE_ACTOR_MAX`
- max trigger points: `SCENE_TRIGGER_MAX`

---

## Parse Pipeline

`scene_load_file()` performs a line-by-line parse:

- trims whitespace
- ignores comments/blank lines
- pattern-matches known directives (`spawn`, `move`, `say`, etc.)
- normalizes fields into `scene_cmd_t`

Helpers:

- direction parsing (`scene_parse_dir`)
- sprite parsing (`scene_parse_sprite`)
- text unescape for `say` (`\n`, `\f`, etc.)

Invalid/unrecognized lines are skipped fail-soft in v0.1 (not fatal).

---

## Execution Pipeline (Per Frame)

Interpreter runs inside `DebugCLI_Tick()` after other CLI per-frame state updates.

Execution priority:

1. resolve active wait (`wait N`)
2. resolve active movement block (`move ... steps`)
3. execute next command at `s_scene_pc`

When no scene is active, the trigger-point evaluator runs:

1. scan configured scene triggers for current map/tile match
2. require overworld + player idle + no open text
3. load and start matched scene
4. disarm that trigger until player leaves tile

Blocking semantics:

- `wait`: blocks by frame countdown
- `move`: blocks until each scripted step animation completes
- `wait_text`: blocks until `Text_IsOpen() == 0`

Non-blocking opcodes (`spawn`, `face`, `lock_input`, etc.) execute and increment PC immediately.

---

## Actor Lifecycle

Scene actors are runtime NPCs created via debug-only APIs:

- `NPC_DebugSpawn(...)`
- `NPC_DebugDespawn(...)`

Actor ids in scene files are symbolic; runtime NPC indices are tracked in `s_scene_actors`.

This indirection allows script commands to refer to stable names while underlying NPC slot indices may vary.

---

## Input/Text Integration

Input lock:

- `lock_input on` sets `wJoyIgnore = PAD_CTRL_PAD` (movement only)
- `lock_input off` resets `wJoyIgnore = 0`

Text:

- `say` uses `Text_ShowASCII(...)`
- `wait_text` waits for textbox fully closed

Design intent:

- preserve A/B handling during dialogue while preventing movement drift

---

## Safety and Failure Behavior

v0.1 is designed fail-soft:

- unknown actor ids: command effectively no-op + advance
- stale NPC indices: movement/despawn ops no-op + advance
- out-of-range PC: runner auto-stops and unlocks input
- `scene_stop`: hard stop + input unlock

Current parser behavior does not surface rich parse errors to the user yet.

---

## Extension Points

The current architecture is intentionally small and extensible:

- add new opcodes by:
  1. parser mapping in `scene_load_file()`
  2. runtime case in interpreter switch
  3. doc update in DSL spec

Planned low-risk additions:

- `move ... until offscreen_top`
- labels + `goto`
- conditional guards (`if_flag`, `if_badge`)
- better path resolution for scene files (build-root + repo-root fallback)

Current trigger-point syntax already supports an outer-layer event pattern:

- `scene_trigger set <scene> trigger_point <x_expr> <y_expr> [map]`
- coordinate expressions accept absolute values and `player.x|player.y` offsets

---

## Why This Shape

This implementation favors:

- zero external dependencies
- deterministic, frame-accurate behavior within existing game tick
- low friction for gameplay/script parity testing

It is deliberately not a full scripting language yet; the goal is fast iteration on test/cutscene orchestration with clear upgrade paths.
