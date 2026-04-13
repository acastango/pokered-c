cmake_minimum_required(VERSION 3.16)
project(pokered C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# SDL2 — support both modern targets and legacy variable style
set(SDL2_VC_ROOT "${CMAKE_SOURCE_DIR}/deps/SDL2-VC/SDL2-2.32.10")
if(MSVC AND EXISTS "${SDL2_VC_ROOT}/cmake/sdl2-config.cmake")
  list(PREPEND CMAKE_PREFIX_PATH "${SDL2_VC_ROOT}/cmake")
elseif(MINGW)
  list(APPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64")
endif()
find_package(SDL2 REQUIRED)
# Modern SDL2 CMake exports SDL2::SDL2 targets; fallback to legacy variables
if(TARGET SDL2::SDL2)
  # Use SDL2 target only; SDL2main causes WinMain entry issues on MinGW.
  # We use -mconsole to avoid WinMain requirement on Windows.
  set(SDL2_LINK_TARGETS SDL2::SDL2)
else()
  set(SDL2_LINK_TARGETS ${SDL2_LIBRARIES})
  include_directories(${SDL2_INCLUDE_DIRS})
endif()
# Always add include dirs (SDL2::SDL2 carries them transitively, but be safe)
if(SDL2_INCLUDE_DIRS)
  include_directories(${SDL2_INCLUDE_DIRS})
endif()

# Math (libm is implicit on Windows/MinGW, explicit on Linux)
find_library(MATH_LIBRARY m)
if(NOT MATH_LIBRARY)
  set(MATH_LIBRARY "")
endif()

# Sources
set(SOURCES
    src/main.c
    src/game/wram.c
    src/game/overworld.c
    src/game/player.c
    src/game/game.c
    src/platform/input.c
    src/platform/display.c
    src/platform/audio.c
    src/platform/save.c
    src/data/base_stats.c
    src/data/moves_data.c
    src/data/type_chart.c
    src/data/tileset_data.c
    src/data/map_data.c
    src/data/event_data.c
    src/data/wild_data.c
    src/data/connection_data.c
    src/game/warp.c
    src/game/anim.c
    src/game/npc.c
    src/game/text.c
    src/game/menu.c
    src/game/inventory.c
    src/game/pokemon.c
    src/game/bag_menu.c
    src/game/party_menu.c
    src/game/pokecenter.c
    src/game/pokemart.c
    src/game/summary_screen.c
    src/game/battle/battle_damage.c
    src/game/battle/battle_effects.c
    src/game/battle/battle_core.c
    src/game/battle/battle_loop.c
    src/game/battle/battle_init.c
    src/game/battle/battle_exp.c
    src/game/battle/battle_driver.c
    src/game/battle/battle_catch.c
    src/game/battle/battle_items.c
    src/game/battle/battle_switch.c
    src/game/battle/battle_trainer.c
    src/game/battle/battle_ui.c
    src/game/battle_transition.c
    src/game/trainer_sight.c
    src/game/badge.c
    src/game/gate_scripts.c
    src/game/pokedex.c
    src/game/main_menu.c
    src/game/intro.c
    src/game/pallet_scripts.c
    src/game/oakslab_scripts.c
    src/game/gym_scripts.c
    src/game/viridian_mart_scripts.c
    src/game/mtmoon_scripts.c
    src/game/cerulean_scripts.c
    src/game/route22_scripts.c
    src/game/route24_scripts.c
    src/game/bills_house_scripts.c
    src/game/pc_menu.c
    src/game/route2gate_scripts.c
    src/game/ss_anne_scripts.c
    src/game/vermilion_gym_scripts.c
    src/game/yesno.c
    src/game/tmhm.c
    src/game/field_moves.c
    src/game/pokeflute.c
    src/data/tmhm_data.c
    src/data/dex_data.c
    src/game/debug_overlay.c
    src/game/debug_cli.c
    src/data/evos_moves_data.c
    src/data/trainer_data.c
    src/data/trainer_sprites.c
    src/data/sprite_data.c
    src/data/party_icon_data.c
    src/data/font_data.c
    src/data/pokedex_tiles.c
    src/data/player_sprite.c
    src/data/pokemon_sprites.c
    src/data/cry_data.c
    src/game/music.c
)

add_executable(pokered ${SOURCES})

target_include_directories(pokered PRIVATE src)
target_link_libraries(pokered
    ${SDL2_LINK_TARGETS}
    ${MATH_LIBRARY}
)

if(MSVC)
  set(SDL2_VC_DLL "${SDL2_VC_ROOT}/lib/x64/SDL2.dll")
  if(EXISTS "${SDL2_VC_DLL}")
    add_custom_command(TARGET pokered POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "${SDL2_VC_DLL}"
              "$<TARGET_FILE_DIR:pokered>/SDL2.dll"
    )
  endif()
endif()

# On Windows with MinGW, SDL2main uses WinMain; prefer console for dev builds
if(WIN32 AND MINGW)
  target_link_options(pokered PRIVATE -mconsole)
endif()

# Warnings
target_compile_options(pokered PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/W4 /wd4100>
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers>
    $<$<C_COMPILER_ID:GNU>:-include ${CMAKE_SOURCE_DIR}/src/platform/no_avx512.h>
    $<$<CONFIG:Debug>:$<$<C_COMPILER_ID:MSVC>:/Zi /Od>>
    $<$<CONFIG:Debug>:$<$<NOT:$<C_COMPILER_ID:MSVC>>:-g -O0>>
    $<$<CONFIG:Release>:$<$<C_COMPILER_ID:MSVC>:/O2>>
    $<$<CONFIG:Release>:$<$<NOT:$<C_COMPILER_ID:MSVC>>:-O2>>
)

# ---- Test suite (no SDL2 — uses stubs) ----
set(TEST_GAME_SOURCES
    src/game/wram.c
    src/game/overworld.c
    src/game/player.c
    src/game/warp.c
    src/game/anim.c
    src/game/music.c
    src/game/npc.c
    src/game/text.c
    src/game/inventory.c
    src/game/party_menu.c
    src/game/summary_screen.c
    src/data/party_icon_data.c
    src/game/pokecenter.c
    src/game/pokemart.c
    src/data/base_stats.c
    src/data/moves_data.c
    src/data/type_chart.c
    src/data/tileset_data.c
    src/data/map_data.c
    src/data/event_data.c
    src/data/wild_data.c
    src/data/connection_data.c
    src/data/sprite_data.c
    src/data/font_data.c
    src/data/pokedex_tiles.c
    src/data/player_sprite.c
    src/data/pokemon_sprites.c
    src/game/battle/battle_damage.c
    src/game/battle/battle_effects.c
    src/game/battle/battle_core.c
    src/game/battle/battle_loop.c
    src/game/battle/battle_exp.c
    src/game/battle/battle_init.c
    src/game/battle/battle_catch.c
    src/game/battle/battle_items.c
    src/game/battle/battle_switch.c
    src/game/battle/battle_trainer.c
    src/game/trainer_sight.c
    src/game/badge.c
    src/game/gate_scripts.c
    src/game/gym_scripts.c
    src/game/oakslab_scripts.c
    src/game/viridian_mart_scripts.c
    src/game/mtmoon_scripts.c
    src/game/cerulean_scripts.c
    src/game/route22_scripts.c
    src/game/route24_scripts.c
    src/game/bills_house_scripts.c
    src/game/pc_menu.c
    src/game/route2gate_scripts.c
    src/game/ss_anne_scripts.c
    src/game/vermilion_gym_scripts.c
    src/game/yesno.c
    src/game/tmhm.c
    src/game/field_moves.c
    src/game/pokeflute.c
    src/data/tmhm_data.c
    src/game/pokedex.c
    src/game/pallet_scripts.c
    src/game/pokemon.c
    src/data/dex_data.c
    src/data/evos_moves_data.c
    src/data/trainer_data.c
    src/data/trainer_sprites.c
)
set(TEST_SOURCES
    tests/main.c
    tests/test_runner.c
    tests/stubs.c
    tests/test_map.c
    tests/test_warp.c
    tests/test_player.c
    tests/test_text.c
    tests/test_wild.c
    tests/test_battle_damage.c
    tests/test_battle_effects.c
    tests/test_battle_core.c
    tests/test_battle_loop.c
    tests/test_battle_catch.c
    tests/test_battle_items.c
    tests/test_battle_switch.c
    tests/test_battle_trainer.c
    tests/test_battle_init.c
    tests/test_pokemon_storage.c
)
add_executable(pokered_tests ${TEST_GAME_SOURCES} ${TEST_SOURCES})
target_include_directories(pokered_tests PRIVATE src tests)
target_compile_options(pokered_tests PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/W4 /wd4100>
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers>
    $<$<C_COMPILER_ID:GNU>:-include ${CMAKE_SOURCE_DIR}/src/platform/no_avx512.h>
    $<$<CONFIG:Debug>:$<$<C_COMPILER_ID:MSVC>:/Zi /Od>>
    $<$<CONFIG:Debug>:$<$<NOT:$<C_COMPILER_ID:MSVC>>:-g -O0>>
)
# CTest integration
enable_testing()
add_test(NAME pokered_tests COMMAND pokered_tests)
