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
  - Loads and runs `debug/scenes/<name>.scene`
- `scene_stop`
  - Stops active scene runner and restores input (`wJoyIgnore = 0`)
- `scene_trigger set <scene> trigger_point <x_expr> <y_expr> [map]`
  - Registers tile-based auto-trigger for scene
- `scene_trigger set <scene> trigger_point <x_expr> <y_expr> when event_set|event_clear <event> [map]`
  - Registers a trigger with an event-flag gate
- `scene_trigger list`
  - Lists active trigger points
- `scene_trigger clear [scene]`
  - Clears all trigger points, or only one scene name
- `scene_npc add <name> <scene> <sprite> <x_expr> <y_expr>`
  - Spawns/binds an interactable NPC to a scene on current map
- `scene_npc list`
  - Lists current scene NPC bindings
- `scene_npc clear [name]`
  - Clears one or all scene NPC bindings
- `dsl_bank on|off|status|save|load|clear`
  - Opt-in persistence bank for scene NPC bindings

Current canonical scene path:

- `C:\Users\Anthony\pokered\debug\scenes\<name>.scene`

---

## Trigger Points (Outer-Layer Wrapper)

Trigger points are configured from debug CLI and run scenes when the player reaches a tile.

Example:

```txt
scene_trigger set duo trigger_point player.x-1 player.y
scene_trigger set duo trigger_point player.x-1 player.y when event_set EVENT_GOT_POKEDEX
```

Coordinate expressions:

- absolute integer tile coords: `12`, `5`
- player-relative x: `player.x`, `player.x+2`, `player.x-1`
- player-relative y: `player.y`, `player.y+1`, `player.y-3`

Behavior:

- fires only in overworld when no scene is already active
- requires player idle and no open text box
- one-shot while standing on tile; auto-rearms after leaving tile
- optional event gate:
  - `when event_set <event>` only fires when event flag is set
  - `when event_clear <event>` only fires when event flag is clear

Event token formats:

- symbolic: `EVENT_BEAT_BROCK`, `EVENT_GOT_POKEDEX`
- shorthand without prefix: `BEAT_BROCK`, `GOT_POKEDEX`
- numeric: `119`, `0x25`

This keeps map-script C code clean while still enabling location-based scene behavior.

---

## Persistent DSL Bank (Opt-In)

`dsl_bank` is a sidecar persistence layer for DSL-added scene NPC bindings.

- `dsl_bank on`
  - Enables persistence and saves this as default for next startup
- `dsl_bank off`
  - Disables persistence (does not delete saved data)
- `dsl_bank status`
  - Shows enabled state + binding count
- `dsl_bank save`
  - Writes current bindings to sidecar bank file
- `dsl_bank load`
  - Loads bindings and respawns on current map
- `dsl_bank clear`
  - Deletes banked binding data

When enabled, `scene_npc add/clear` auto-save and bindings respawn after restart/map load.

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

Battle-flow policy (enforced):

- Top-level raw `battlestart` / `battlend` are disallowed.
- Battle flow must be authored through reusable defs and called via `use ...`.
- Canonical baseline is `include defs_battle` + `use battle_intro ...`.
- This guarantees the same intro/transition baseline everywhere while staying composable through parameters and follow-up commands.

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

Auto-format behavior:

- Word-wraps to Gen 1 textbox width (18 chars/line)
- Uses max 2 lines per page, then inserts page break
- Avoids splitting words across lines unless a single word is longer than 18 chars
- Appends `@` terminator automatically if omitted

Supported escapes:

- `\n` line break
- `\f` page break
- `\t` tab
- `\\` backslash
- `\"` quote

Tips:

- End text with `@` for explicit terminator parity with existing text usage.

### `ask "<text>"`

Shows prompt text then opens a normal in-game YES/NO box.

Behavior:

- Uses existing `YesNo_Show(...)` flow
- Scene runner blocks until player chooses YES or NO
- Result is printed to CLI history as:
  - `scene ask result: yes`
  - `scene ask result: no`
- Uses the same auto-format rules as `say` (word-wrap/page flow/auto `@`)

### `battlestart ...` / `battlend ...` (definition-only)

These battle primitives are intended for reusable definition bodies (`def ... enddef`), not top-level scene lines.

Enforced behavior:

- If top-level scene lines contain raw `battlestart` or `battlend`, scene load fails with a clear error.
- Use `include defs_battle` and call a reusable flow through `use`.

Why:

- Keeps one canonical intro/transition baseline.
- Maintains composability and parameterization via `use` args.
- Mirrors the project convention that battles always include transition flow.

### `wait <frames>`

Wait N frames before next command.

### `wait_text`

Blocks until text box is fully closed (`Text_IsOpen() == 0`).

### `music <track>`

Force-play a scene-selected music track.

Allowed tracks:

- `wild_battle`
- `trainer_battle`
- `gym_leader` (same bucket used for Elite Four boss battles)
- `champion_battle` (mapped to the Gen1 boss-battle theme bucket)

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
- Legacy fallback paths still include `bugs/scenes/` for backward compatibility

---

## Authoring Workflow

1. Create scene file in:
   - `debug/scenes/<name>.scene`
2. Launch game with debug CLI enabled
3. Run:
   - `scene_run <name>`
4. Stop manually if needed:
   - `scene_stop`

### Canonical battle pattern

Use this as the default structure for battle scenes:

```txt
include defs_battle
spawn rival YOUNGSTER player+1 player+0
face rival player
use battle_intro "Hey! Ready?@" "Great battle!@"
face rival up
move rival up 6
despawn rival
end
```

Notes:

- `battle_intro` parameters (`$1`, `$2`, ...) are defined in `defs_battle.scene`.
- Post-battle overworld behavior (walk off, extra text, despawn) remains fully composable after `use battle_intro ...`.

### Custom team at call time

For custom scripted trainer parties, use `battle_intro_custom` from `defs_battle.scene`.

Call shape (11 args total):

1. pre-text
2. music token (`wild_battle`, `trainer_battle`, `gym_leader`, `elite_4`, `champion_battle`)
3. trainer class token/id (e.g. `BROCK`, `LORELEI`, `LANCE`, `34`)
4. slot1 spec
5. slot2 spec
6. slot3 spec
7. slot4 spec
8. slot5 spec
9. slot6 spec
10. trainer defeat quote (shown by battle UI after enemy's last mon faints)
11. post-text (overworld `battlend` text)

Slot spec formats:

- filled: `"Species,Level,Move1,Move2,Move3,Move4"`
- empty: `"empty"` (also accepts `none`, `-`, `0`)

Example (single line; `use` does not support multiline continuation yet):

```txt
use battle_intro_custom "Hey! Custom team test.@" "gym_leader" "BROCK" "Pikachu,22,Thunderbolt,Quick Attack,Thunder Wave,Growl" "Rattata,18,Hyper Fang,Tail Whip,Quick Attack,Focus Energy" "empty" "empty" "empty" "empty" "No way! My team got smoked!@" "Nice battle. See ya!@"
```

Resolution rules:

- Species: dex number or Pokemon name
- Moves: move id or move name
- Level: `1..100`
- Trainer class: numeric id (`1..47`) or canonical trainer class name (`BROCK`, `MISTY`, `LORELEI`, `BRUNO`, `AGATHA`, `LANCE`, etc.)

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
