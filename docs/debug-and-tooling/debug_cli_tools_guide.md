# pokered Debug CLI & Tools Guide

This guide documents the current debug control workflow used by this repo:

- Runtime command processor: [src/game/debug_cli.c](C:/Users/Anthony/pokered/src/game/debug_cli.c)
- Public interface header: [src/game/debug_cli.h](C:/Users/Anthony/pokered/src/game/debug_cli.h)
- Python client wrapper: [tools/game_cli.py](C:/Users/Anthony/pokered/tools/game_cli.py)
- Basic command reference helper: [tools/game_cli_commands.md](C:/Users/Anthony/pokered/tools/game_cli_commands.md)

## Runtime Modes

### Standard mode

- Default window/render path (scaled GB viewport)

### Debug render mode

- Launch with: `pokered.exe --debug-render`
- Fixed window: `512x288` (logical `256x144`, 2x scale)
- Left pane: game framebuffer (`160x144`)
- Right pane: in-window CLI history panel (`96x144`)
- `~` / `Shift+~` toggles CLI typing mode
- While typing mode is active, gameplay input is blocked and Enter submits the command
- While typing mode is inactive, gameplay input does not feed CLI text input
- Intended for tighter debug workflows where command history should remain visible while playing

## Overview

The debug CLI is file-driven. External scripts write a command to `bugs/cli_cmd.txt`, the game consumes it, then writes state output to `bugs/cli_state.txt`.

- Input file: `bugs/cli_cmd.txt`
- Output file: `bugs/cli_state.txt`
- Poll cadence: every ~30 game ticks when idle
- Command consumption: input file is deleted by the game once processed

The same command path is used by:

- CLI scripts (`tools/game_cli.py`)
- PowerShell helpers (for targeted teleports)
- In-game tilde/backquote console bridge

## State Output Modes

`cli_state.txt` is context-aware:

- `=== TEXT ===` when dialogue is open
- `=== POKECENTER ===` during Pokecenter Yes/No prompt
- `=== OVERWORLD ===` in normal field scene
- `=== BATTLE (...) ===` in battle/transition scenes

Overworld output includes:

- Map name/id, player position/facing
- ASCII viewport map with legend (`@`, `#`, `.`, `N`, `S`, `I`, `H`, ledges, etc.)
- Party summary
- Money and badge count

Battle output includes:

- Battle UI state label
- Hittrace status and last sample
- Enemy/player HP + status
- Move list and PP
- Prompt-style “what to do next” hints

## Command Families

The following verbs are currently implemented in `process_cmd(...)`:

- Movement/input: `up`, `down`, `left`, `right`, `a`, `interact`, `b`, `back`, `start`, `menu`, `select`, `wait`, `state`
- Map/debug info: `tile_info`, `tileinfo`, `teleport`, `tele`
- Battle control: `fight`, `run`, `pkmn`, `pokemon`, `bag`, `item`
- Battle diagnostics: `animlab`, `hittrace`
- Flags/progression: `setflag`, `clearflag`, `givebadge`, `checkpoint`
- Inventory/party cheats: `giveitem`, `givetm`, `givehm`, `givemon`, `bulba15`, `givemon_bulba15`, `squirtle15`, `givemon_squirtle15`, `teachmove`, `teach`, `movetestteam1`, `movetestteam2`, `sethealth`, `setlevel`, `healparty`, `setmoney`, `listbag`, `giveteam`, `addexp`, `exprate`
- Save states: `quicksave`, `quickload`, `csave`
- Deterministic replay: `replay`

## Save State System

The CLI save commands use engine-level full snapshots (`Save_StateWrite` / `Save_StateLoad`) from [src/platform/save.c](C:/Users/Anthony/pokered/src/platform/save.c), not normal `.sav` progress saves.

These state snapshots include:

- Event flags
- Map/player coordinates and runtime movement fields
- Party, inventory, money, badges
- Battle/runtime state (including many in-battle globals)
- RNG/frame state

After loading, the CLI reload path re-runs map/NPC/script on-load callbacks so the state is immediately playable.

### Slot saves

- Save: `quicksave 1`
- Load: `quickload 1`

Slot file pattern:

- `bugs/qs_slot_<n>.state`

### Named saves

- Create: `csave create "before_giovanni"`
- Load: `csave load "before_giovanni"`
- Also supported: `quickload before_giovanni`

Named file pattern:

- `bugs/qs_<name>.state`

Names are sanitized to safe characters (`A-Z`, `a-z`, `0-9`, `_`, `-`; spaces become `_`).

### Automatic backup history on overwrite

On every overwrite save (`quicksave` or `csave create`) the previous file is copied to a per-save backup directory before writing the new state.

Examples:

- Primary: `bugs/qs_slot_1.state`
- Backup dir: `bugs/qs_slot_1_backup`
- Backup files: `qs_slot_1_001.state`, `qs_slot_1_002.state`, ...

Named example:

- Primary: `bugs/qs_before_giovanni.state`
- Backup dir: `bugs/qs_before_giovanni_backup`
- Backup files: `qs_before_giovanni_001.state`, `qs_before_giovanni_002.state`, ...

Version index is monotonic per save name/slot: next index = current max + 1.

## Deterministic Replay System

Replay files capture:

- Full starting runtime snapshot (same core used by `quicksave`)
- One raw input byte per frame (`hJoyInput`)

Replay commands:

- `replay record <name>`: starts recording from current exact state
- `replay stop`: finalizes recording to replay file (or stops active playback)
- `replay play <name>`: loads recorded start state, then injects recorded inputs
- `replay status`: prints recording/playback state and frame progress

Replay file location:

- `bugs/replays/<name>.rpl`

Replay name sanitization matches save names (`A-Z`, `a-z`, `0-9`, `_`, `-`, spaces -> `_`).

Overwrite behavior:

- If a replay file already exists, it is backed up to `bugs/<replay>_backup/` before replacement.

Notes:

- Playback is deterministic only when code/assets/runtime behavior are unchanged.
- During replay playback, recorded input is injected frame-by-frame and live keyboard input is ignored.

## Rewind (Frame-Accurate v2)

Rewind is now an in-memory frame ring (not file-based), designed to feel emulator-like.

Hotkeys:

- Hold `Ctrl+Z`: scrub backward frame-by-frame
- Hold `Ctrl+Y`: scrub forward frame-by-frame (redo)

Key properties:

- Capture cadence: every frame (`60fps`)
- Ring depth: 1800 frames (~30 seconds)
- Includes world runtime state, not just player:
  - Camera/scroll offsets
  - NPC runtime state (positions, facing, walk timers/offsets, hidden flags)
  - Battle runtime state (via full save-state snapshot)

Behavior notes:

- If you rewind and then resume normal play from an older point, forward redo history is truncated (timeline fork).
- The rewind ring is ephemeral runtime memory and is not persisted across app restarts.

## Teleport Workflows

You can teleport by numeric map id or by named aliases.

- Numeric: `teleport 0xCA 11 3`
- Named: `teleport saffron_city`
- Alias: `tele route_24`

There is also a focused helper script:

- [tools/teleport_lift_key_grunt.ps1](C:/Users/Anthony/pokered/tools/teleport_lift_key_grunt.ps1)

It writes a teleport command directly into `bugs/cli_cmd.txt` for fast Rocket Hideout Lift Key grunt testing.

## Checkpoint Workflows

`checkpoint <name>` applies curated event flag bundles + map placement for quick scenario setup.

Current implemented names:

- `parcel_ready`
- `pokedex_ready`
- `mt_moon`
- `cerulean`
- `route22`
- `brock`
- `misty`
- `cerulean_rocket`
- `ss_anne_hm`
- `liftkey_reset`
- `giovanni_reset`
- `giovanni_ready`
- `surge`

These are intended for deterministic script/battle regression testing and parity passes.

## Battle Test Helpers

### `animlab`

- `animlab start [level]`
- `animlab stop`
- `animlab status`

Purpose:

- Auto-drives battle menus to repeatedly fire move animations in sequence
- Keeps battle stabilized for visual/effect validation

### `hittrace`

- `hittrace on`
- `hittrace off`
- `hittrace reset`
- `hittrace status`

Purpose:

- Exposes move hit-test internals (accuracy/effect/miss reason) in logs/state

### Move team presets

- `movetestteam1`
- `movetestteam2`

Purpose:

- Populate party with curated move sets for animation/effect sweeps

## tools/ Directory Sweep

`tools/` currently includes three broad groups:

- Runtime CLI helpers:
  - `game_cli.py` (interactive/one-shot/scripted CLI client)
  - `game_cli_commands.md` (quick reference)
  - `teleport_lift_key_grunt.ps1` (targeted teleport helper)
- Data extraction/generation pipeline:
  - `extract_*.py` scripts for maps/audio/sprites/moves/trainers/etc.
  - `gen_*.py` and `gen_*.js` data generation scripts
  - `import_moves_data.py`, `patch_tmhm.py`, related transformers
- Utility scripts:
  - `bug_report.py`, `omni_healer.py`, `transform_coords.py`, `launch_game.sh`

For parity/debug sessions, the most relevant are `game_cli.py` and the save-state commands above.

## Example Session

```text
checkpoint giovanni_ready
quicksave before_giovanni

fight 1
wait 120

quickload before_giovanni
csave create "before_giovanni_after_fix"
```

## Rewind + Replay Workflow Example

```text
replay record tower_bug_01
# play until bug
replay stop

replay play tower_bug_01
# while replaying or after, hold Ctrl+Z / Ctrl+Y to inspect frame-level world state
```

## Maintenance Notes

- Source of truth for command behavior is `process_cmd(...)` in [debug_cli.c](C:/Users/Anthony/pokered/src/game/debug_cli.c).
- If a command is added/renamed in code, update this guide and `tools/game_cli_commands.md` together.
- Save-state format/version logic lives in [save.c](C:/Users/Anthony/pokered/src/platform/save.c). If state structs change, keep backward/forward compatibility in mind for debug workflows.

## Session Logging (Narrative + JSONL)

Session logging now writes two files under `bugs/`:

- Narrative: `bugs/session_narrative.log`
- Structured JSONL: `bugs/session_events.jsonl`

Current logged data includes:

- Generic debug events (`event`)
- NPC interactions / item pickups / item acquisition / captures
- Event flag transitions (`event_flag_change`) from `SetEvent` / `ClearEvent`
- Battle lifecycle summaries:
  - `battle_start` (trainer class/no or wild species/level)
  - `battle_end` (resolved outcome + key state fields)

Primary hook points:

- Logger implementation: [src/game/session_log.c](C:/Users/Anthony/pokered/src/game/session_log.c)
- Event flag bridge: [src/game/wram.c](C:/Users/Anthony/pokered/src/game/wram.c) (`Debug_LogEventFlagChange`)
- Battle start/end hooks: [src/game/game.c](C:/Users/Anthony/pokered/src/game/game.c)
