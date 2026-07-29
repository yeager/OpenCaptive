cmake_minimum_required(VERSION 3.20)
project(OpenCaptive VERSION 1.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED)
find_package(ZLIB REQUIRED)

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
    src/engine/droid_ui.c
    src/engine/terminal.c
    src/engine/liberation.c
    src/audio/midi_player.c
    src/audio/music.c
    src/data/pl5_decoder.c
    src/data/anm_decoder.c
    src/data/rnc_decoder.c
    src/data/mid_loader.c
    src/data/data_vfs.c
    src/data/sha256.c
    src/data/gfx_loader.c
    src/data/texture_atlas.c
    src/data/adf_reader.c
    src/data/iso9660_reader.c
    src/data/liberation_data.c
    src/data/st_disk_reader.c
    src/audio/sound.c
    src/audio/sfx.c
    src/render/renderer.c
    src/render/viewport.c
    src/render/hud.c
    src/render/enhanced_render.c
)

add_executable(opencaptive ${OPENCAPTIVE_SOURCES})

target_include_directories(opencaptive PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(opencaptive PRIVATE SDL3::SDL3 ZLIB::ZLIB)
if(UNIX AND NOT APPLE)
    target_link_libraries(opencaptive PRIVATE m)
endif()

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

add_executable(test_data_vfs
    tests/test_data_vfs.c
    src/data/data_vfs.c
    src/data/sha256.c
)
target_include_directories(test_data_vfs PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_data_vfs PRIVATE ZLIB::ZLIB)
add_test(NAME data_vfs COMMAND test_data_vfs)

add_executable(test_sha256 tests/test_sha256.c src/data/sha256.c)
target_include_directories(test_sha256 PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME sha256 COMMAND test_sha256)

add_executable(test_iso9660_reader tests/test_iso9660_reader.c
    src/data/iso9660_reader.c src/data/sha256.c)
target_include_directories(test_iso9660_reader PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME iso9660_reader COMMAND test_iso9660_reader)

add_executable(test_amiga_hunk tests/test_amiga_hunk.c src/data/amiga_hunk.c)
target_include_directories(test_amiga_hunk PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME amiga_hunk COMMAND test_amiga_hunk)

add_executable(test_liberation tests/test_liberation.c src/engine/liberation.c)
target_include_directories(test_liberation PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME liberation COMMAND test_liberation)

add_executable(test_start_menu tests/test_start_menu.c src/engine/start_menu.c)
target_include_directories(test_start_menu PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_start_menu PRIVATE SDL3::SDL3)
add_test(NAME start_menu COMMAND test_start_menu)

add_executable(test_game_state tests/test_game_state.c
    src/engine/engine.c src/engine/map_gen.c src/engine/combat.c)
target_include_directories(test_game_state PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME game_state COMMAND test_game_state)

add_executable(test_save_load tests/test_save_load.c
    src/engine/save_load.c src/engine/engine.c src/engine/map_gen.c
    src/engine/combat.c src/engine/puzzle.c)
target_include_directories(test_save_load PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME save_load COMMAND test_save_load)

# The tests use assert() for both execution and verification.  Release builds
# define NDEBUG, which would otherwise turn those test expressions into no-ops.
function(opencaptive_enable_test_assertions target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /UNDEBUG)
    else()
        target_compile_options(${target} PRIVATE -UNDEBUG)
    endif()
endfunction()

foreach(test_target IN ITEMS
        test_pl5_decoder test_anm_decoder test_map_gen test_data_vfs
        test_sha256 test_iso9660_reader test_amiga_hunk test_liberation test_start_menu test_game_state test_save_load)
    opencaptive_enable_test_assertions(${test_target})
endforeach()

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

add_executable(iso_list
    tools/iso_list.c
    src/data/iso9660_reader.c
    src/data/sha256.c
)
target_include_directories(iso_list PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(hash_find tools/hash_find.c src/data/data_vfs.c src/data/sha256.c)
target_include_directories(hash_find PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(hash_find PRIVATE ZLIB::ZLIB)

add_executable(hash_extract tools/hash_extract.c src/data/data_vfs.c src/data/sha256.c)
target_include_directories(hash_extract PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(hash_extract PRIVATE ZLIB::ZLIB)

add_executable(verify_liberation
    tools/verify_liberation.c
    src/data/liberation_data.c
    src/data/iso9660_reader.c
    src/data/data_vfs.c
    src/data/sha256.c
)
target_include_directories(verify_liberation PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(verify_liberation PRIVATE ZLIB::ZLIB)

add_executable(liberation_extract
    tools/liberation_extract.c
    src/data/liberation_data.c
    src/data/iso9660_reader.c
    src/data/data_vfs.c
    src/data/sha256.c
)
target_include_directories(liberation_extract PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(liberation_extract PRIVATE ZLIB::ZLIB)

add_executable(st_list tools/st_list.c src/data/st_disk_reader.c src/data/sha256.c)
target_include_directories(st_list PRIVATE ${CMAKE_SOURCE_DIR}/include)
