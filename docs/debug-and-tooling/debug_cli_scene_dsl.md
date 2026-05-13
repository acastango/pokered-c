# Debug CLI Scene DSL (v0.1)

This document defines the first-pass in-engine scene DSL used by Debug CLI.

Purpose:

- Compose cutscene-like behavior without adding new C code per test
- Keep script intent readable (spawn/face/text/move/despawn)
- Execute deterministically inside the game runtime

Primary implementation:

- [debug_cli.c](C:/Users/Anthony/pokered/src/game/debug_cli.c)
- Technical architecture note: [debug_cli_scene_dsl_architecture.md](C:/Users/Anthony/pokered/docs/debug-and-tooling/debug_cli_scene_dsl_architecture.md)

---

## Runtime Commands

- `scene_run <name>`
  - Loads and runs `bugs/scenes/<name>.scene`
- `scene_stop`
  - Stops active scene runner and restores input (`wJoyIgnore = 0`)
- `scene_trigger set <scene> trigger_point <x_expr> <y_expr> [map]`
  - Registers tile-based auto-trigger for scene
- `scene_trigger list`
  - Lists active trigger points
- `scene_trigger clear [scene]`
  - Clears all trigger points, or only one scene name

Current file lookup path at runtime:

- `C:\Users\Anthony\pokered\build\bugs\scenes\<name>.scene`

---

## Trigger Points (Outer-Layer Wrapper)

Trigger points are configured from debug CLI and run scenes when the player reaches a tile.

Example:

```txt
scene_trigger set duo trigger_point player.x-1 player.y
```

Coordinate expressions:

- absolute integer tile coords: `12`, `5`
- player-relative x: `player.x`, `player.x+2`, `player.x-1`
- player-relative y: `player.y`, `player.y+1`, `player.y-3`

Behavior:

- fires only in overworld when no scene is already active
- requires player idle and no open text box
- one-shot while standing on tile; auto-rearms after leaving tile

This keeps map-script C code clean while still enabling location-based scene behavior.

---

## Scene File Format

- Plain text
- One command per line
- Blank lines ignored
- `#` starts a comment line

Example:

```txt
spawn testnpc YOUNGSTER player+1 player+0
face testnpc player
lock_input on
wait 24
say "Hello! I am a\ndebug_cli NPC\ntest!\fHope this works!@"
wait_text
lock_input off
move testnpc up 12
despawn testnpc
end
```

---

## DSL Commands (v0.1)

### `spawn <id> <sprite> <x> <y>`

Creates a temporary actor and binds it to `<id>`.

- `<id>`: local actor name (e.g. `npc1`)
- `<sprite>`:
  - symbolic: `YOUNGSTER`, `GAMBLER`, `SUPER_NERD`, `GIRL`, `COOLTRAINER_M`, `COOLTRAINER_F`, `ROCKET`, `GUARD`
  - or numeric sprite ID
- `<x>`, `<y>`:
  - absolute integer tile coords
  - or relative form: `player+N` (e.g. `player+1`, `player+0`)

Notes:

- Relative coords are resolved at scene start/command execution time.

### `despawn <id>`

Despawns actor bound to `<id>`.

### `face <id> <dir>`

Sets actor facing direction.

- `<dir>`: `down|up|left|right|player`
- `player` means face to match the player direction at execution time.

### `move <id> <dir> <steps>`

Scripted walk with animation.

- `<dir>`: `down|up|left|right`
- `<steps>`: integer number of grid steps

Execution behavior:

- Scene runner waits for each step animation to complete before issuing next step.

### `say "<text>"`

Shows dialogue using `Text_ShowASCII(...)`.

Supported escapes:

- `\n` line break
- `\f` page break
- `\t` tab
- `\\` backslash
- `\"` quote

Tips:

- End text with `@` for explicit terminator parity with existing text usage.

### `wait <frames>`

Wait N frames before next command.

### `wait_text`

Blocks until text box is fully closed (`Text_IsOpen() == 0`).

### `lock_input on|off`

- `on` → movement locked (`wJoyIgnore = PAD_CTRL_PAD`)
- `off` → input unlocked (`wJoyIgnore = 0`)

Important:

- `on` still allows A/B so dialogue can be advanced normally.

### `end`

Ends scene execution immediately and unlocks input.

---

## Execution Model

- Single active scene runner (v0.1)
- Commands execute in-order (no branching yet)
- Movement commands are asynchronous internally but blocking to script flow
- Text and waits are first-class blocking operations

If scene exits unexpectedly (bad actor index, etc.), behavior is fail-soft:

- runner advances or ends rather than hard-crashing

---

## Known Constraints (v0.1)

- No labels/jumps/conditionals
- No parallel actor actions
- No `until offscreen` movement primitive yet
- No map validation/pathfinding/collision diagnostics
- Scene path is currently tied to build-local `bugs/scenes/`

---

## Authoring Workflow

1. Create scene file in:
   - `build/bugs/scenes/<name>.scene`
2. Launch game with debug CLI enabled
3. Run:
   - `scene_run <name>`
4. Stop manually if needed:
   - `scene_stop`

Recommended instrumentation while iterating:

- `eventdiff snapshot` / `eventdiff show`
- `script_trace on` / `script_trace off`
- `quicksave` / `quickload`

---

## Planned Near-Term Extensions

- `move <id> <dir> until offscreen_top`
- richer coordinate syntax (`player+dx,dy`)
- labels + `goto`
- `if_flag`/`if_badge` guards
- scene file search fallback (repo-root + build-root)
