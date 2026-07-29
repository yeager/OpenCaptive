cmake_minimum_required(VERSION 3.20)
project(OpenCaptive VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED)

# --- Source files ---

set(OPENCAPTIVE_SOURCES
    src/main.c
    src/engine/engine.c
    src/engine/map_gen.c
    src/engine/start_menu.c
    src/engine/combat.c
    src/engine/save_load.c
    src/engine/inventory.c
    src/engine/shop.c
    src/engine/puzzle.c
    src/audio/midi_player.c
    src/audio/music.c
    src/data/pl5_decoder.c
    src/data/anm_decoder.c
    src/data/rnc_decoder.c
    src/data/mid_loader.c
    src/data/gfx_loader.c
    src/data/texture_atlas.c
    src/data/adf_reader.c
    src/data/iso9660_reader.c
    src/data/st_disk_reader.c
    src/audio/sound.c
    src/render/renderer.c
    src/render/viewport.c
    src/render/hud.c
)

add_executable(opencaptive ${OPENCAPTIVE_SOURCES})

target_include_directories(opencaptive PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(opencaptive PRIVATE SDL3::SDL3)

# --- Tests ---

enable_testing()

add_executable(test_pl5_decoder
    tests/test_pl5_decoder.c
    src/data/pl5_decoder.c
)
target_include_directories(test_pl5_decoder PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME pl5_decoder COMMAND test_pl5_decoder)

add_executable(test_anm_decoder
    tests/test_anm_decoder.c
    src/data/anm_decoder.c
)
target_include_directories(test_anm_decoder PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME anm_decoder COMMAND test_anm_decoder)

add_executable(test_map_gen
    tests/test_map_gen.c
    src/engine/map_gen.c
)
target_include_directories(test_map_gen PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME map_gen COMMAND test_map_gen)

# --- Tools ---

add_executable(pl5_to_bmp
    tools/pl5_to_bmp.c
    src/data/pl5_decoder.c
)
target_include_directories(pl5_to_bmp PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(anm_extract
    tools/anm_extract.c
    src/data/anm_decoder.c
)
target_include_directories(anm_extract PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(rnc_decode_tool
    tools/rnc_decode.c
)
target_include_directories(rnc_decode_tool PRIVATE ${CMAKE_SOURCE_DIR}/include)
