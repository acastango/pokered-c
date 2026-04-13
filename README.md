# pokered-c

A faithful C port of [Pokémon Red](https://github.com/pret/pokered) for modern platforms, built with SDL2.

The original Game Boy assembly is the spec. Every mechanic is ported from the disassembly rather than approximated.

**Status:** Fully playable from intro through Cerulean City and beyond (Nugget Bridge, Route 24/25, Bill's House). Full Gen 1 battle engine with trainer battles. Story scripts and overworld events implemented for the first chapter of the game.

## What's implemented

### Overworld
- Walking, NPC interaction, map connections, ledge jumping
- Warps (doors, cave exits, underground paths) with fade transition
- Item pickup from overworld
- Wild encounter detection (grass tiles, random rate)
- Trainer sight detection — exclamation emote, walk-up, battle trigger
- Save/load

### Story scripts (Pallet → Cerulean)
- **Intro & main menu** — Game Freak logo, title screen, new game / continue
- **Pallet Town** — Oak intro, player naming
- **Oak's Lab** — starter selection dialogue, rival pick, receive Pokédex, Oak's Parcel delivery chain
- **Viridian City** — Mart locked until Parcel delivered; Old Man tutorial
- **Viridian Forest** — trainer encounters (Bug Catcher parties)
- **Mt. Moon** — trainer encounters, cave exit warps
- **Cerulean City** — rival battle on arrival, Nugget Bridge trainer gauntlet, Route 24/25 trainers, Bill's House transformation sequence (Bill gives SS Ticket)
- **Event flags** — persistent across save/load; completed events stay completed

### Battle engine (Gen 1 faithful)
- Turn structure: move selection, priority, speed order
- Damage formula: types, STAB, critical hits, random factor
- All status conditions (PAR, SLP, PSN, BRN, FRZ) with Gen 1 mechanics
- Move effects via effect ID table (mirrors pokered effect handler dispatch)
- Experience gain, level-up, stat recalculation
- Pokémon switching (voluntary + forced on faint)
- Item use in battle (Potion, Antidote, status heals, Pokéballs)
- Catch mechanic — Gen 1 catch rate formula, shake count
- Trainer battles — trainer sprite slide-in/out, send-out text, win/loss handling, prize money, badge checks
- 8 battle transition animations keyed on trainer/wild/dungeon/stronger-enemy
- Full battle UI: HP bars, status icons, party dots, move menu, bag/switch/run submenus

### UI screens
- **Start menu** — Pokémon party, bag, player card, save
- **Bag** — item list and use
- **Party menu** — party Pokémon with sprite icons
- **Summary screen** — stats, moves, Pokédex entry
- **Pokédex** — species list, entry viewer
- **Pokémart** — buy/sell with Gen 1 item list layout, quantity picker, ¥ pricing
- **Pokémon Center** — nurse dialogue, healing machine animation, party restore

### Audio
- Full music playback from pokered-master score data
- Per-map music: Pallet Town, Viridian City, Pewter City, Cerulean City, Vermilion City, Lavender Town, Celadon City, Mt. Moon, Oak's Lab, Viridian Forest, and more
- Pokémon cries (pitch + tempo modifiers per species)
- SFX: purchase, level-up, healing machine, ball poof, faint, run, ledge hop, key item fanfare, collision, go inside/outside

### Data (extracted from pokered-master)
- All 151 Pokémon front/back sprites
- Party icon tiles for all species
- Level-up movesets and evolution chains
- All trainer parties
- Pokémon cry definitions

## What's missing / in progress

### Evolution
- Pokémon don't evolve yet — data and chains are loaded, trigger not hooked in

### Areas beyond Cerulean chapter
- Route 6, Underground Path, and Vermilion City are accessible but have no story scripts
- Saffron City gate logic (guards blocking until Silph Co. arc) not yet implemented
- Gym leaders implemented through Cerulean (Brock, Misty); remaining gyms not yet implemented

### Trainer AI
- Currently uses basic move selection; Gen 1 AI heuristics not yet ported

## Prerequisites

| Tool         | Notes                                                  |
| ------------ | ------------------------------------------------------ |
| C11 compiler | GCC via MSYS2/MinGW64 on Windows; `gcc` on Linux/macOS |
| CMake ≥ 3.16 |                                                        |
| SDL2         | `pacman -S mingw-w64-x86_64-SDL2` on MSYS2             |
| Python 3     | Only needed to regenerate data files from `tools/`     |

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
