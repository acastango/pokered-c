# ASM vs C Structure Coverage Ledger

Date: 2026-05-05  
Scope: Structural coverage only (directory/file presence and count evidence), no behavioral assumptions.

## Method

- ASM source root: `C:\Users\Anthony\pokered\pokered-master`
- C source root: `C:\Users\Anthony\pokered\src`
- Status values:
  - `mapped`: clear structural owner exists in C
  - `partial`: some clear owner files exist, but ASM surface is much broader
  - `missing`: no clear structural owner found in C tree

## High-Level Totals

- ASM `*.asm` files: `1922`
- C `*.c` + `*.h` files: `200`
- ASM `*.asm` total lines: `156229`
- C `*.c` + `*.h` total lines: `99207`

Note: file count overstates gap because C consolidates many ASM units.

## System Coverage Matrix

| ASM system | ASM evidence | C owner evidence | Status |
|---|---|---|---|
| `engine/battle` | `C:\Users\Anthony\pokered\pokered-master\engine\battle` exists | `C:\Users\Anthony\pokered\src\game\battle\*.c/*.h` (26 files, including `battle_core`, `battle_effects`, `battle_items`, `battle_loop`, `battle_ui`) | mapped |
| `engine/events` | `C:\Users\Anthony\pokered\pokered-master\engine\events` exists | `C:\Users\Anthony\pokered\src\data\event_data.c/.h`, `event_constants.h`, plus map-specific script modules in `src\game\*_scripts.c` | partial |
| `engine/items` | `C:\Users\Anthony\pokered\pokered-master\engine\items` exists | `src\game\inventory.c/.h`, `bag_menu.c/.h`, `tmhm.c/.h`, `pokemart.c/.h`, `src\game\battle\battle_items.c/.h` | partial |
| `engine/menus` | `C:\Users\Anthony\pokered\pokered-master\engine\menus` exists | `src\game\menu.c/.h`, `main_menu.c/.h`, `party_menu.c/.h`, `pc_menu.c/.h`, `summary_screen.c/.h`, `pokedex.c/.h`, `naming_screen.c/.h` | partial |
| `engine/overworld` | `C:\Users\Anthony\pokered\pokered-master\engine\overworld` exists | `src\game\overworld.c/.h`, `player.c/.h`, `npc.c/.h`, `warp.c/.h`, `trainer_sight.c/.h`, `field_moves.c/.h` | partial |
| `engine/pokemon` | `C:\Users\Anthony\pokered\pokered-master\engine\pokemon` exists | `src\game\pokemon.c/.h`, `src\data\base_stats.*`, `moves_data.*`, `evos_moves_data.*`, `trainer_data.*`, `wild_data.*` | mapped |
| `engine/movie` | `C:\Users\Anthony\pokered\pokered-master\engine\movie` exists | `src\game\intro.c/.h`, `title_screen.c/.h`, `src\data\intro_scene_data.*`, `title_screen_data.*`, `splash_screen_data.*` | partial |
| `engine/debug` | `C:\Users\Anthony\pokered\pokered-master\engine\debug` exists | `src\game\debug_cli.c/.h`, `debug_overlay.c/.h` | mapped |
| `engine/gfx` | `C:\Users\Anthony\pokered\pokered-master\engine\gfx` exists | Rendering/platform split under `src\platform\display.*`, `audio.*`, and content tables under `src\data` | partial |
| `engine/link` | `C:\Users\Anthony\pokered\pokered-master\engine\link` exists | No clear `link`-named module under `src\game` or `src\platform` | missing |
| `engine/math` | `C:\Users\Anthony\pokered\pokered-master\engine\math` exists | No clear standalone math subsystem in `src` | missing |
| `engine/slots` | `C:\Users\Anthony\pokered\pokered-master\engine\slots` exists | No clear `slots`-named module in `src\game` | missing |
| `scripts` (map scripts) | `224` ASM files in `pokered-master\scripts` | `14` C `*_scripts.c` modules in `src\game`; normalized name overlap with ASM script basenames: `7` | partial |
| `maps` (map blocks) | `225` `.blk` files in `pokered-master\maps` | consolidated map blocks in `src\data\map_data.c` (`222` `kBlk_*` arrays) | mapped |

## Script Coverage Snapshot (Name-Based)

- C script modules (`src\game\*_scripts.c`): 14
- Normalized overlaps with ASM script basenames: 7
- Overlap set:
  - `bills_house`
  - `oakslab`
  - `route22`
  - `route24`
  - `route2gate`
  - `vermilion_gym`
  - `viridian_mart`

## Where To Look First (Structural Risk Ranking)

1. `scripts` parity (`224` ASM vs `14` C modules, overlap `7`)
2. `engine/link` (`missing`)
3. `engine/slots` (`missing`)
4. `engine/math` (`missing`)
5. `engine/events` / `engine/items` / `engine/overworld` / `engine/menus` (present but structurally partial)

## Next Artifact

Build a per-module ledger for `pokered-master\scripts\*.asm`:
- columns: `asm_script`, `c_owner`, `status`, `notes`
- status constrained to `mapped` / `partial` / `missing`
- no behavioral claims until trace-level diff is completed.
