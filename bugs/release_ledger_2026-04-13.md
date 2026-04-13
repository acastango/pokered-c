# Release Ledger (2026-04-13)

Git range documented: `e21ac1a..7b279ab`

- Commits included:
1. `3081cce` — SS Anne: rival battle, captain HM01 Cut, 20 trainer battles, music
2. `7b279ab` — ASM fidelity sync: battle/UI/script fixes, PC systems, and tail whip miss fix

- Diff totals:
1. 99 files changed
2. 20,163 insertions
3. 9,730 deletions

## Build / Docs / Notes
- `BUILD.md`
- `CMakeLists.txt`
- `README.md`
- `bugs/oakslab_postbattle_timing_staged.md`
- `bugs/textbox_vertical_offset_staged.md`

## Data Layer (`src/data`)
- `src/data/base_stats.c`
- `src/data/connection_data.c`
- `src/data/dex_data.c`
- `src/data/dex_data.h`
- `src/data/event_constants.h`
- `src/data/event_data.c`
- `src/data/event_data.h`
- `src/data/event_flag_names.h`
- `src/data/map_data.c`
- `src/data/moves_data.c`
- `src/data/music_data_gen.h`
- `src/data/pokedex_tiles.c`
- `src/data/pokedex_tiles.h`
- `src/data/tmhm_data.c`
- `src/data/tmhm_data.h`

## Battle Core / Battle UI (`src/game/battle`)
- `src/game/battle/battle_core.c`
- `src/game/battle/battle_effects.c`
- `src/game/battle/battle_exp.c`
- `src/game/battle/battle_exp.h`
- `src/game/battle/battle_init.c`
- `src/game/battle/battle_items.c`
- `src/game/battle/battle_trainer.c`
- `src/game/battle/battle_ui.c`
- `src/game/battle/battle_ui.h`

## Game Scripts / Systems (`src/game`)
- `src/game/bag_menu.c`
- `src/game/bills_house_scripts.c`
- `src/game/bills_house_scripts.h`
- `src/game/cerulean_scripts.c`
- `src/game/cerulean_scripts.h`
- `src/game/constants.h`
- `src/game/debug_cli.c`
- `src/game/debug_overlay.c`
- `src/game/field_moves.c`
- `src/game/field_moves.h`
- `src/game/game.c`
- `src/game/gate_scripts.c`
- `src/game/gate_scripts.h`
- `src/game/gym_scripts.c`
- `src/game/gym_scripts.h`
- `src/game/inventory.c`
- `src/game/inventory.h`
- `src/game/menu.c`
- `src/game/music.c`
- `src/game/music.h`
- `src/game/npc.c`
- `src/game/npc.h`
- `src/game/oakslab_scripts.c`
- `src/game/overworld.c`
- `src/game/overworld.h`
- `src/game/pallet_scripts.c`
- `src/game/party_menu.c`
- `src/game/party_menu.h`
- `src/game/pc_menu.c`
- `src/game/pc_menu.h`
- `src/game/player.c`
- `src/game/pokecenter.c`
- `src/game/pokedex.c`
- `src/game/pokeflute.c`
- `src/game/pokeflute.h`
- `src/game/pokemon.c`
- `src/game/pokemon.h`
- `src/game/route22_scripts.c`
- `src/game/route22_scripts.h`
- `src/game/route24_scripts.c`
- `src/game/route2gate_scripts.c`
- `src/game/route2gate_scripts.h`
- `src/game/ss_anne_scripts.c`
- `src/game/ss_anne_scripts.h`
- `src/game/text.c`
- `src/game/text.h`
- `src/game/tmhm.c`
- `src/game/tmhm.h`
- `src/game/trainer_sight.c`
- `src/game/vermilion_gym_scripts.c`
- `src/game/vermilion_gym_scripts.h`
- `src/game/warp.c`
- `src/game/wram.c`
- `src/game/yesno.c`
- `src/game/yesno.h`
- `src/main.c`

## Platform Layer (`src/platform`)
- `src/platform/audio.c`
- `src/platform/audio.h`
- `src/platform/display.c`
- `src/platform/display.h`
- `src/platform/hardware.h`
- `src/platform/save.c`

## Tests
- `tests/stubs.c`
- `tests/test_pokemon_storage.c`

## Tools
- `tools/extract_audio.py`
- `tools/game_cli.py`
- `tools/game_cli_commands.md`
- `tools/gen_dex_data.py`
- `tools/gen_tmhm_data.py`
- `tools/patch_tmhm.py`

