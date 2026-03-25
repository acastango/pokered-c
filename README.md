# pokered-c

A faithful C port of [Pokémon Red](https://github.com/pret/pokered) for modern platforms, built with SDL2.

The original Game Boy assembly is the spec. Every mechanic is ported from the disassembly rather than approximated.

**Status:** Overworld tested up to Cerulean City, data loaded for all of Kanto. Full Gen 1 battle engine implemented (Mechanics still to be fully tested for acute accuracy, trainer battles not fully implemented). Major UI screens complete (Some buggy, like PokeMart).

## What's implemented

### Overworld
- Walking, NPC interaction, map connections, ledge jumping
- Warps (doors, cave exits) with fade transition
- Item pickup from overworld
- Wild encounter detection (grass tiles, random rate)
- Save/load

### UI screens
- **Start menu** — Pokémon party, bag, player card, save
- **Bag** — item list and use
- **Party menu** — party Pokémon with sprite icons
- **Summary screen** — stats, moves, Pokédex entry
- **Pokémart** — buy/sell with Gen 1 item list layout, quantity picker, ¥ pricing
- **Pokémon Center** — nurse dialogue, healing machine animation, party restore

### Battle engine (Gen 1 faithful)
- Turn structure: move selection, priority, speed order
- Damage formula: types, STAB, critical hits, random factor
- All status conditions (PAR, SLP, PSN, BRN, FRZ) with Gen 1 mechanics
- Move effects via effect ID table (mirrors pokered effect handler dispatch)
- Experience gain, level-up, stat recalculation
- Pokémon switching (voluntary + forced on faint)
- Item use in battle (Potion, Antidote, status heals, Pokéballs)
- Catch mechanic — Gen 1 catch rate formula, shake count, critical catch
- Trainer battles — send-out text, win/loss handling, prize money
- 8 battle transition animations (wipe patterns, spiral, etc.)
- Full battle UI: HP bars, status icons, party dots, move menu, bag/switch/run submenus

### Audio
- Full music playback from pokered-master score data
- Pokémon cries (pitch + tempo modifiers per species)
- SFX: purchase kaching, level-up jingle, healing machine, ball poof, faint, run, ledge hop

### Data (extracted from pokered-master)
- All 151 Pokémon front/back sprites
- Party icon tiles for all species
- Level-up movesets and evolution chains
- All trainer parties
- Pokémon cry definitions

## What's missing

### Trainer Battles
-Trainer alert and battle start not yet implemented
-Trainer Battle transition screens not yet implemented
-Trainer AI not yet implemented

### Evolution

-Pokemon don't evolve yet
-Data is in the game, not hooked into an event yet

### Scripts
-Story Scripts haven't been attempted yet
-Story Flags not yet set up fully
-After trainer battles, this.

## Prerequisites

| Tool | Notes |
|---|---|
| C11 compiler | GCC via MSYS2/MinGW64 on Windows; `gcc` on Linux/macOS |
| CMake ≥ 3.16 | |
| SDL2 | `pacman -S mingw-w64-x86_64-SDL2` on MSYS2 |
| Python 3 | Only needed to regenerate data files from `tools/` |

> **Windows note:** `CMakeLists.txt` currently hardcodes `C:/msys64/mingw64` as the SDL2 search path. Edit that line if your MSYS2 is installed elsewhere, or remove it on Linux/macOS.

## pokered-master

The data-extraction tools in `tools/` read from the original pokered disassembly. Clone it into the project root:

```sh
git clone https://github.com/pret/pokered pokered-master
```

You only need this if you want to regenerate `src/data/` files. The generated files are already committed, so a normal build does not require it.

## Build

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/pokered
```

On Windows with MSYS2:
```sh
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/pokered.exe
```

## Tests

```sh
cmake --build build --target pokered_tests
./build/pokered_tests
# or via CTest:
cd build && ctest
```

## Controls

| Key | Action |
|---|---|
| Arrow keys | Move / navigate menus |
| Z | A button (confirm, interact) |
| X | B button (cancel, back) |
| Enter | Start |

## Data extraction tools

After cloning `pokered-master`, regenerate `src/data/` files with:

```sh
python tools/extract_maps.py
python tools/extract_events.py
python tools/extract_data.py
python tools/extract_sprites.py
python tools/extract_audio.py
python tools/extract_cries.py
python tools/extract_party_icons.py
```

## Built with

Developed using [Claude Code](https://claude.ai/code) with [monet-code](https://github.com/acastango/monet-code) for persistent project memory across sessions.

## Legal

This project contains no ROM data or proprietary Nintendo assets. It references the [pret/pokered](https://github.com/pret/pokered) open disassembly only for structure and layout information. Pokémon is a trademark of Nintendo / Game Freak.
