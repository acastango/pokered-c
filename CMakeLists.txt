cmake_minimum_required(VERSION 3.16)
project(pokered C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# SDL2 — support both modern targets and legacy variable style
list(APPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64")
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
    src/game/battle/battle_damage.c
    src/game/battle/battle_effects.c
    src/game/battle/battle_core.c
    src/game/battle/battle_loop.c
    src/game/battle/battle_init.c
    src/game/battle/battle_driver.c
    src/data/sprite_data.c
    src/data/font_data.c
    src/data/player_sprite.c
    src/game/music.c
)

add_executable(pokered ${SOURCES})

target_include_directories(pokered PRIVATE src)
target_link_libraries(pokered
    ${SDL2_LINK_TARGETS}
    ${MATH_LIBRARY}
)

# On Windows with MinGW, SDL2main uses WinMain; prefer console for dev builds
if(WIN32 AND MINGW)
  target_link_options(pokered PRIVATE -mconsole)
endif()

# Warnings
target_compile_options(pokered PRIVATE
    -Wall -Wextra -Wno-unused-parameter
    -include ${CMAKE_SOURCE_DIR}/src/platform/no_avx512.h  # workaround GCC 10 ICE
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O2>
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
    src/data/player_sprite.c
    src/game/battle/battle_damage.c
    src/game/battle/battle_effects.c
    src/game/battle/battle_core.c
    src/game/battle/battle_loop.c
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
)
add_executable(pokered_tests ${TEST_GAME_SOURCES} ${TEST_SOURCES})
target_include_directories(pokered_tests PRIVATE src tests)
target_compile_options(pokered_tests PRIVATE
    -Wall -Wextra -Wno-unused-parameter
    -include ${CMAKE_SOURCE_DIR}/src/platform/no_avx512.h
    $<$<CONFIG:Debug>:-g -O0>
)
# CTest integration
enable_testing()
add_test(NAME pokered_tests COMMAND pokered_tests)
