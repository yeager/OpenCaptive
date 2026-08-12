cmake_minimum_required(VERSION 3.20)
project(OpenCaptive VERSION 1.1.106 LANGUAGES C)
if(APPLE)
    enable_language(OBJC)
endif()

# GameState contains the full fixed-size dungeon array used by the runtime.
# Several regression tests exercise that object directly; the Windows default
# executable stack is too small for those legitimate fixtures, especially
# when ctest runs them in parallel.  Keep the production and test binaries
# consistent with the runtime's static GameState allocation.
if(MSVC)
    add_link_options(/STACK:8388608)
elseif(WIN32)
    add_link_options(-Wl,--stack,8388608)
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(ANDROID)
    add_compile_options(-Wno-format-security)
endif()

if(UNIX)
  set(MATH_LIBRARY m)
else()
  set(MATH_LIBRARY "")
  add_compile_definitions(_USE_MATH_DEFINES)
endif()

find_package(SDL3 REQUIRED)
find_package(SDL3_ttf QUIET)
if(NOT SDL3_ttf_FOUND)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL3_TTF REQUIRED IMPORTED_TARGET sdl3-ttf)
  set(SDL3_TTF_TARGET PkgConfig::SDL3_TTF)
else()
  set(SDL3_TTF_TARGET SDL3_ttf::SDL3_ttf)
endif()
find_package(Freetype QUIET)
find_package(ZLIB REQUIRED)

# --- Source files ---

set(OPENCAPTIVE_SOURCES
    src/main.c
    src/engine/engine.c
    src/engine/map_gen.c
    src/engine/captive_view_window.c
    src/engine/captive_navigation.c
    src/engine/captive_space_nav.c
    src/engine/start_menu.c
    src/engine/combat.c
    src/engine/droid_damage.c
    src/engine/save_load.c
    src/engine/save_thumbnail.c
    src/engine/inventory.c
    src/engine/shop.c
    src/engine/puzzle.c
    src/engine/droid_ui.c
    src/engine/terminal.c
    src/audio/midi_player.c
    src/audio/music.c
    src/audio/cdda_player.c
    src/audio/speech.c
    src/data/pl5_decoder.c
    src/data/anm_decoder.c
    src/data/rnc_decoder.c
    src/engine/arcd_decoder.c
    src/data/amiga_ofs.c
    src/data/amiga_hunk.c
    src/data/amiga_planar.c
    src/data/captive_amiga_data.c
    src/data/captive_scene_assets.c
    src/data/liberation_anim.c
    src/data/mid_loader.c
    src/data/data_vfs.c
    src/data/sha256.c
    src/data/gfx_loader.c
    src/data/texture_atlas.c
    src/data/dos_vga_reference.c
    src/data/captive_dos_map.c
    src/data/captive_dos_descriptor.c
    src/render/captive_dos_runtime.c
    src/data/captive_cappo_descriptors.c
    src/data/adf_reader.c
    src/data/iso9660_reader.c
    src/data/liberation_data.c
    src/data/liberation_string_tables.c
    src/data/liberation_text_table.c
    src/data/i18n.c
    src/data/stb_image_impl.c
    src/data/amos_sprite.c
    src/data/liberation_fnt.c
    src/data/liberation_img.c
    src/game/liberation_city_nav.c
    src/game/liberation_dialogue.c
    src/game/liberation_shop.c
    src/game/liberation_bar.c
    src/game/liberation_building_interact.c
    src/game/liberation_combat.c
    src/game/liberation_npc_dialogue.c
    src/engine/liberation_plotgen.c
    src/game/liberation_save.c
    src/render/liberation_viewport_3d.c
    src/data/liberation_vgm.c
    src/data/liberation_vgm_hashes.c
    src/data/liberation_x3g.c
    src/data/st_disk_reader.c
    src/audio/sound.c
    src/audio/sfx.c
    src/audio/adlib_data.c
    src/audio/opl2_emu.c
    src/audio/adlib_sfx.c
    src/data/ctv_decoder.c
    src/render/renderer.c
    src/render/hud.c
    src/render/holamap.c
    src/render/creature_sprite.c
    src/render/object_sprite.c
    src/engine/creature_stats.c
    src/engine/xp_level.c
    src/engine/item_price.c
    src/engine/spawn.c
    src/engine/mapgen_ca.c
    src/engine/liberation_citygen.c
    src/engine/liberation_citygen_grid.c
    src/engine/liberation_npc.c
    src/engine/liberation_cutscene.c
    src/render/captive_compositor.c
    src/render/frame_compare.c
    src/render/viewport.c
    src/engine/captive_data.c
    src/custom/custom_features.c
    src/custom/upscale_xbrz.c
    src/custom/minimap.c
    src/custom/debug_hud.c
    src/custom/automap.c
    src/custom/audio_reverb.c
    src/custom/replay.c
    src/custom/lighting.c
    src/custom/cross_save.c
)

list(APPEND OPENCAPTIVE_SOURCES src/platform/captive_emulator.c)

if(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    list(APPEND OPENCAPTIVE_SOURCES src/platform/macos_menu.m)
endif()

if(ANDROID)
    add_library(opencaptive SHARED ${OPENCAPTIVE_SOURCES})
else()
    add_executable(opencaptive ${OPENCAPTIVE_SOURCES})
endif()

target_include_directories(opencaptive PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

if(TARGET SDL3::SDL3main)
    target_link_libraries(opencaptive PRIVATE SDL3::SDL3main)
endif()
target_link_libraries(opencaptive PRIVATE SDL3::SDL3 ${SDL3_TTF_TARGET} ZLIB::ZLIB ${MATH_LIBRARY})
if(Freetype_FOUND)
    target_link_libraries(opencaptive PRIVATE Freetype::Freetype)
endif()
if(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_link_libraries(opencaptive PRIVATE "-framework Cocoa")
endif()

# The launcher artwork is deliberately separate from game data.  Copy it
# beside development binaries as well, so SDL_GetBasePath() resolves the
# same start menu in a local build and in a release package.
if(NOT ANDROID)
    add_custom_command(TARGET opencaptive POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:opencaptive>/assets/menu"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/assets/menu/captive-cover-v1.png"
            "$<TARGET_FILE_DIR:opencaptive>/assets/menu/captive-cover-v1.png"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/assets/menu/liberation-cover-v1.png"
            "$<TARGET_FILE_DIR:opencaptive>/assets/menu/liberation-cover-v1.png"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:opencaptive>/assets/captive"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/assets/captive/holamap-initial.png"
            "$<TARGET_FILE_DIR:opencaptive>/assets/captive/holamap-initial.png"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/assets/captive/holamap-zoom-out.png"
            "$<TARGET_FILE_DIR:opencaptive>/assets/captive/holamap-zoom-out.png"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/assets/captive/holamap-zoom-in.png"
            "$<TARGET_FILE_DIR:opencaptive>/assets/captive/holamap-zoom-in.png"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/assets/captive/holamap-target.png"
            "$<TARGET_FILE_DIR:opencaptive>/assets/captive/holamap-target.png"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/assets/captive/orbit-reference.png"
            "$<TARGET_FILE_DIR:opencaptive>/assets/captive/orbit-reference.png"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/assets/captive/landing-transition-reference.png"
            "$<TARGET_FILE_DIR:opencaptive>/assets/captive/landing-transition-reference.png"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/assets/captive/landed-dungeon-reference.png"
            "$<TARGET_FILE_DIR:opencaptive>/assets/captive/landed-dungeon-reference.png"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:opencaptive>/tools"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/tools/dosbox-x-captive.conf"
            "$<TARGET_FILE_DIR:opencaptive>/tools/dosbox-x-captive.conf"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/tools/dosbox-x-captive.conf"
            "$<TARGET_FILE_DIR:opencaptive>/assets/captive/dosbox-x-captive.conf")
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

add_executable(test_arcd_decoder
    tests/test_arcd_decoder.c
    src/engine/arcd_decoder.c
)
target_include_directories(test_arcd_decoder PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME arcd_decoder COMMAND test_arcd_decoder)

add_executable(test_map_gen
    tests/test_map_gen.c
    src/engine/map_gen.c
    src/engine/mapgen_ca.c
    src/engine/spawn.c
    src/engine/creature_stats.c
    src/engine/captive_data.c
)
target_include_directories(test_map_gen PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME map_gen COMMAND test_map_gen)

add_executable(test_captive_view_window tests/test_captive_view_window.c
    src/engine/captive_view_window.c)
target_include_directories(test_captive_view_window PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME captive_view_window COMMAND test_captive_view_window)

add_executable(test_captive_compositor tests/test_captive_compositor.c
    src/render/captive_compositor.c
    src/data/captive_dos_descriptor.c)
target_include_directories(test_captive_compositor PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME captive_compositor COMMAND test_captive_compositor)

add_executable(test_viewport
    tests/test_viewport.c
    src/render/viewport.c
    src/data/captive_cappo_descriptors.c
    src/engine/captive_view_window.c
    src/engine/creature_stats.c
    src/render/creature_sprite.c)
target_include_directories(test_viewport PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(MSVC)
    target_link_options(test_viewport PRIVATE /STACK:8388608)
elseif(WIN32)
    target_link_options(test_viewport PRIVATE -Wl,--stack,8388608)
endif()
add_test(NAME viewport COMMAND test_viewport)

add_executable(test_dos_vga_reference tests/test_dos_vga_reference.c
    src/data/dos_vga_reference.c src/data/pl5_decoder.c)
target_include_directories(test_dos_vga_reference PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME dos_vga_reference COMMAND test_dos_vga_reference)

add_executable(test_captive_dos_descriptor tests/test_captive_dos_descriptor.c
    src/data/captive_dos_descriptor.c)
target_include_directories(test_captive_dos_descriptor PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME captive_dos_descriptor COMMAND test_captive_dos_descriptor)

add_executable(test_captive_dos_map tests/test_captive_dos_map.c
    src/data/captive_dos_map.c)
target_include_directories(test_captive_dos_map PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME captive_dos_map COMMAND test_captive_dos_map)

add_executable(test_frame_compare tests/test_frame_compare.c
    src/render/frame_compare.c)
target_include_directories(test_frame_compare PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME frame_compare COMMAND test_frame_compare)

add_executable(test_data_vfs
    tests/test_data_vfs.c
    src/data/data_vfs.c
    src/data/amiga_ofs.c
    src/data/sha256.c
)
target_include_directories(test_data_vfs PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_data_vfs PRIVATE ZLIB::ZLIB)
add_test(NAME data_vfs COMMAND test_data_vfs)

add_executable(test_sha256 tests/test_sha256.c src/data/sha256.c)
target_include_directories(test_sha256 PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME sha256 COMMAND test_sha256)

add_executable(test_rnc_decoder tests/test_rnc_decoder.c src/data/rnc_decoder.c)
target_include_directories(test_rnc_decoder PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME rnc_decoder COMMAND test_rnc_decoder)

add_executable(test_version_consistency tests/test_version_consistency.c)
target_include_directories(test_version_consistency PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(test_version_consistency PRIVATE
    OPENCAPTIVE_CMAKE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    OPENCAPTIVE_CMAKE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    OPENCAPTIVE_CMAKE_VERSION_PATCH=${PROJECT_VERSION_PATCH})
add_test(NAME version_consistency COMMAND test_version_consistency)

add_executable(test_iso9660_reader tests/test_iso9660_reader.c
    src/data/iso9660_reader.c src/data/sha256.c)
target_include_directories(test_iso9660_reader PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME iso9660_reader COMMAND test_iso9660_reader)

add_executable(test_amiga_hunk tests/test_amiga_hunk.c src/data/amiga_hunk.c)
target_include_directories(test_amiga_hunk PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME amiga_hunk COMMAND test_amiga_hunk)

add_executable(test_amiga_ofs tests/test_amiga_ofs.c src/data/amiga_ofs.c src/data/sha256.c)
target_include_directories(test_amiga_ofs PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME amiga_ofs COMMAND test_amiga_ofs)

add_executable(test_adf_reader tests/test_adf_reader.c src/data/adf_reader.c)
target_include_directories(test_adf_reader PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME adf_reader COMMAND test_adf_reader)

add_executable(test_st_disk_reader tests/test_st_disk_reader.c src/data/st_disk_reader.c)
target_include_directories(test_st_disk_reader PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME st_disk_reader COMMAND test_st_disk_reader)

add_executable(test_amiga_planar tests/test_amiga_planar.c src/data/amiga_planar.c)
target_include_directories(test_amiga_planar PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME amiga_planar COMMAND test_amiga_planar)

add_executable(test_amos_sprite tests/test_amos_sprite.c src/data/amos_sprite.c)
target_include_directories(test_amos_sprite PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME amos_sprite COMMAND test_amos_sprite)

add_executable(test_liberation_vgm tests/test_liberation_vgm.c
    src/data/liberation_vgm.c src/data/amos_sprite.c)
target_include_directories(test_liberation_vgm PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME liberation_vgm COMMAND test_liberation_vgm)

add_executable(test_liberation_img tests/test_liberation_img.c
    src/data/liberation_img.c)
target_include_directories(test_liberation_img PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME liberation_img COMMAND test_liberation_img)

add_executable(test_liberation_fnt tests/test_liberation_fnt.c
    src/data/liberation_fnt.c)
target_include_directories(test_liberation_fnt PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME liberation_fnt COMMAND test_liberation_fnt)

add_executable(test_liberation_viewport_3d tests/test_liberation_viewport_3d.c
    src/render/liberation_viewport_3d.c src/custom/lighting.c)
target_include_directories(test_liberation_viewport_3d PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_liberation_viewport_3d ${MATH_LIBRARY})
add_test(NAME liberation_viewport_3d COMMAND test_liberation_viewport_3d)

add_executable(test_liberation_city_nav tests/test_liberation_city_nav.c
    src/game/liberation_city_nav.c src/render/liberation_viewport_3d.c
    src/custom/lighting.c)
target_include_directories(test_liberation_city_nav PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_liberation_city_nav ${MATH_LIBRARY})
add_test(NAME liberation_city_nav COMMAND test_liberation_city_nav)

add_executable(test_liberation_dialogue_shop tests/test_liberation_dialogue_shop.c
    src/game/liberation_dialogue.c src/game/liberation_shop.c
    src/game/liberation_bar.c
    src/game/liberation_building_interact.c
    src/engine/liberation_citygen.c src/engine/liberation_citygen_grid.c
    src/data/liberation_string_tables.c src/data/i18n.c)
target_include_directories(test_liberation_dialogue_shop PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_liberation_dialogue_shop PRIVATE SDL3::SDL3)
add_test(NAME liberation_dialogue_shop COMMAND test_liberation_dialogue_shop)

add_executable(test_liberation_combat tests/test_liberation_combat.c
    src/game/liberation_combat.c src/engine/xp_level.c)
target_include_directories(test_liberation_combat PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME liberation_combat COMMAND test_liberation_combat)

add_executable(test_liberation_text_table tests/test_liberation_text_table.c
    src/data/liberation_text_table.c src/engine/arcd_decoder.c)
target_include_directories(test_liberation_text_table PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME liberation_text_table COMMAND test_liberation_text_table)

add_executable(test_liberation_npc_dialogue tests/test_liberation_npc_dialogue.c
    src/game/liberation_npc_dialogue.c src/data/liberation_text_table.c
    src/engine/arcd_decoder.c src/game/liberation_dialogue.c src/data/i18n.c)
target_include_directories(test_liberation_npc_dialogue PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_liberation_npc_dialogue PRIVATE SDL3::SDL3)
add_test(NAME liberation_npc_dialogue COMMAND test_liberation_npc_dialogue)

add_executable(test_i18n tests/test_i18n.c src/data/i18n.c)
target_include_directories(test_i18n PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_i18n PRIVATE SDL3::SDL3)
add_test(NAME i18n COMMAND test_i18n)

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME po_catalogs
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/validate_po.py)
endif()

add_executable(test_liberation_plotgen tests/test_liberation_plotgen.c
    src/engine/liberation_plotgen.c src/data/liberation_string_tables.c)
target_include_directories(test_liberation_plotgen PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME liberation_plotgen COMMAND test_liberation_plotgen)

add_executable(test_liberation_save tests/test_liberation_save.c
    src/game/liberation_save.c src/game/liberation_city_nav.c
    src/render/liberation_viewport_3d.c src/custom/lighting.c)
target_include_directories(test_liberation_save PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_liberation_save ${MATH_LIBRARY})
add_test(NAME liberation_save COMMAND test_liberation_save)

add_executable(test_liberation_x3g tests/test_liberation_x3g.c
    src/data/liberation_x3g.c)
target_include_directories(test_liberation_x3g PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME liberation_x3g COMMAND test_liberation_x3g)

add_executable(test_liberation_anim tests/test_liberation_anim.c
    src/data/liberation_anim.c)
target_include_directories(test_liberation_anim PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME liberation_anim COMMAND test_liberation_anim)

add_executable(test_custom_features tests/test_custom_features.c
    src/custom/custom_features.c src/custom/upscale_xbrz.c
    src/custom/minimap.c src/custom/debug_hud.c src/custom/automap.c
    src/custom/audio_reverb.c src/custom/replay.c src/custom/lighting.c
    src/custom/cross_save.c src/engine/inventory.c)
target_include_directories(test_custom_features PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME custom_features COMMAND test_custom_features)

add_executable(test_start_menu tests/test_start_menu.c src/engine/start_menu.c src/engine/save_thumbnail.c src/data/i18n.c src/data/stb_image_impl.c src/data/data_vfs.c src/data/sha256.c src/data/amiga_ofs.c src/data/captive_amiga_data.c src/data/captive_scene_assets.c src/data/amiga_hunk.c src/data/amiga_planar.c src/data/liberation_data.c src/data/liberation_string_tables.c src/data/rnc_decoder.c src/data/iso9660_reader.c src/data/liberation_anim.c)
target_include_directories(test_start_menu PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_start_menu PRIVATE SDL3::SDL3 ${SDL3_TTF_TARGET} ${MATH_LIBRARY} ZLIB::ZLIB)
add_test(NAME start_menu COMMAND test_start_menu)

add_executable(test_game_state tests/test_game_state.c
    src/engine/engine.c src/engine/map_gen.c src/engine/combat.c
    src/engine/captive_data.c src/engine/xp_level.c src/engine/inventory.c
    src/engine/mapgen_ca.c src/engine/spawn.c src/engine/creature_stats.c
    src/engine/puzzle.c src/engine/droid_damage.c src/data/i18n.c)
target_include_directories(test_game_state PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_game_state PRIVATE SDL3::SDL3)
add_test(NAME game_state COMMAND test_game_state)

add_executable(test_ctv_decoder tests/test_ctv_decoder.c src/data/ctv_decoder.c)
target_include_directories(test_ctv_decoder PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME ctv_decoder COMMAND test_ctv_decoder)

add_executable(test_save_load tests/test_save_load.c
    src/engine/save_load.c src/engine/save_thumbnail.c src/engine/engine.c src/engine/map_gen.c
    src/engine/combat.c src/engine/puzzle.c src/engine/captive_data.c
    src/engine/droid_damage.c src/engine/xp_level.c src/engine/inventory.c
    src/engine/mapgen_ca.c src/engine/spawn.c src/engine/creature_stats.c
    src/data/i18n.c)
target_include_directories(test_save_load PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_save_load PRIVATE SDL3::SDL3)
add_test(NAME save_load COMMAND test_save_load)

add_executable(test_holamap tests/test_holamap.c
    src/render/holamap.c src/engine/captive_data.c)
target_include_directories(test_holamap PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME holamap COMMAND test_holamap)

add_executable(test_space_nav tests/test_space_nav.c
    src/engine/captive_space_nav.c src/engine/captive_data.c
    src/render/holamap.c)
target_include_directories(test_space_nav PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(NOT WIN32)
  target_link_libraries(test_space_nav PRIVATE m)
endif()
add_test(NAME space_nav COMMAND test_space_nav)

add_executable(test_captive_navigation
    tests/test_captive_navigation.c
    src/engine/captive_navigation.c)
target_include_directories(test_captive_navigation PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME captive_navigation COMMAND test_captive_navigation)

add_executable(test_creature_sprite tests/test_creature_sprite.c
    src/render/creature_sprite.c)
target_include_directories(test_creature_sprite PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME creature_sprite COMMAND test_creature_sprite)

add_executable(test_object_sprite tests/test_object_sprite.c
    src/render/object_sprite.c)
target_include_directories(test_object_sprite PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME object_sprite COMMAND test_object_sprite)

add_executable(test_adlib_data tests/test_adlib_data.c src/audio/adlib_data.c)
target_include_directories(test_adlib_data PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME adlib_data COMMAND test_adlib_data)

add_executable(test_opl2_emu tests/test_opl2_emu.c src/audio/opl2_emu.c)
target_include_directories(test_opl2_emu PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(UNIX)
    target_link_libraries(test_opl2_emu PRIVATE ${MATH_LIBRARY})
endif()
add_test(NAME opl2_emu COMMAND test_opl2_emu)

add_executable(test_midi_player tests/test_midi_player.c
    src/audio/midi_player.c src/audio/opl2_emu.c src/audio/adlib_data.c)
target_include_directories(test_midi_player PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(UNIX)
    target_link_libraries(test_midi_player PRIVATE ${MATH_LIBRARY})
endif()
add_test(NAME midi_player COMMAND test_midi_player)

add_executable(test_sound tests/test_sound.c src/audio/sound.c
    src/custom/audio_reverb.c)
target_include_directories(test_sound PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_sound PRIVATE SDL3::SDL3 ${MATH_LIBRARY})
add_test(NAME sound COMMAND test_sound)

add_executable(test_creature_stats tests/test_creature_stats.c
    src/engine/creature_stats.c)
target_include_directories(test_creature_stats PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME creature_stats COMMAND test_creature_stats)

add_executable(test_xp_level tests/test_xp_level.c
    src/engine/xp_level.c)
target_include_directories(test_xp_level PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME xp_level COMMAND test_xp_level)

add_executable(test_item_price tests/test_item_price.c
    src/engine/item_price.c)
target_include_directories(test_item_price PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME item_price COMMAND test_item_price)

add_executable(test_droid_ui tests/test_droid_ui.c
    src/engine/droid_ui.c src/engine/inventory.c src/engine/captive_data.c
    src/engine/xp_level.c tests/test_i18n_stub.c)
target_include_directories(test_droid_ui PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME droid_ui COMMAND test_droid_ui)

add_executable(test_terminal tests/test_terminal.c src/engine/terminal.c
    src/data/i18n.c)
target_include_directories(test_terminal PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_terminal PRIVATE SDL3::SDL3)
add_test(NAME terminal COMMAND test_terminal)

add_executable(test_shop tests/test_shop.c
    src/engine/shop.c src/engine/inventory.c src/engine/captive_data.c)
target_include_directories(test_shop PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME shop COMMAND test_shop)

add_executable(test_droid_damage tests/test_droid_damage.c
    src/engine/droid_damage.c)
target_include_directories(test_droid_damage PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME droid_damage COMMAND test_droid_damage)

add_executable(test_mapgen_ca tests/test_mapgen_ca.c
    src/engine/mapgen_ca.c)
target_include_directories(test_mapgen_ca PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME mapgen_ca COMMAND test_mapgen_ca)

add_executable(test_spawn tests/test_spawn.c
    src/engine/spawn.c src/engine/creature_stats.c src/engine/captive_data.c)
target_include_directories(test_spawn PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME spawn COMMAND test_spawn)

add_executable(test_liberation_citygen tests/test_liberation_citygen.c
    src/engine/liberation_citygen.c src/data/liberation_string_tables.c
    src/data/i18n.c)
target_include_directories(test_liberation_citygen PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_liberation_citygen PRIVATE SDL3::SDL3)
add_test(NAME liberation_citygen COMMAND test_liberation_citygen)

add_executable(test_liberation_citygen_grid tests/test_liberation_citygen_grid.c
    src/engine/liberation_citygen_grid.c)
target_include_directories(test_liberation_citygen_grid PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME liberation_citygen_grid COMMAND test_liberation_citygen_grid)

add_executable(test_adlib_sfx tests/test_adlib_sfx.c
    src/audio/adlib_sfx.c src/audio/opl2_emu.c src/audio/adlib_data.c)
target_include_directories(test_adlib_sfx PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(UNIX)
    target_link_libraries(test_adlib_sfx PRIVATE ${MATH_LIBRARY})
endif()
add_test(NAME adlib_sfx COMMAND test_adlib_sfx)

add_executable(test_renderer_map tests/test_renderer_map.c src/render/renderer.c
    src/custom/upscale_xbrz.c)
target_include_directories(test_renderer_map PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_renderer_map PRIVATE SDL3::SDL3 ${MATH_LIBRARY})
add_test(NAME renderer_map COMMAND test_renderer_map)

# The tests use assert() for both execution and verification.  Release builds
# define NDEBUG, which would otherwise turn those test expressions into no-ops.
function(opencaptive_enable_test_assertions target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /UNDEBUG)
    else()
        target_compile_options(${target} PRIVATE -UNDEBUG)
    endif()
endfunction()

get_property(opencaptive_directory_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(test_target IN LISTS opencaptive_directory_targets)
    if(test_target MATCHES "^test_")
        opencaptive_enable_test_assertions(${test_target})
    endif()
endforeach()

# --- Tools ---

add_executable(pl5_to_bmp
    tools/pl5_to_bmp.c
    src/data/pl5_decoder.c
)
target_include_directories(pl5_to_bmp PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(pl5_analyze
    tools/pl5_analyze.c
    src/data/pl5_decoder.c
)
target_include_directories(pl5_analyze PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(anm_extract
    tools/anm_extract.c
    src/data/anm_decoder.c
)
target_include_directories(anm_extract PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(rnc_decode_tool
    tools/rnc_decode.c
    src/data/rnc_decoder.c
)
target_include_directories(rnc_decode_tool PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(iso_list
    tools/iso_list.c
    src/data/iso9660_reader.c
    src/data/sha256.c
)
target_include_directories(iso_list PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(hash_find tools/hash_find.c src/data/data_vfs.c src/data/amiga_ofs.c src/data/sha256.c)
target_include_directories(hash_find PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(hash_find PRIVATE ZLIB::ZLIB)


add_executable(hash_extract tools/hash_extract.c src/data/data_vfs.c src/data/amiga_ofs.c src/data/sha256.c)
target_include_directories(hash_extract PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(hash_extract PRIVATE ZLIB::ZLIB)

add_executable(liberation_cd32_cue tools/liberation_cd32_cue.c
    src/data/data_vfs.c src/data/amiga_ofs.c src/data/sha256.c)
target_include_directories(liberation_cd32_cue PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(liberation_cd32_cue PRIVATE ZLIB::ZLIB)

add_executable(amiga_ofs_extract tools/amiga_ofs_extract.c src/data/amiga_ofs.c
    src/data/data_vfs.c src/data/sha256.c)
target_include_directories(amiga_ofs_extract PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(amiga_ofs_extract PRIVATE ZLIB::ZLIB)

add_executable(amiga_ofs_inventory tools/amiga_ofs_inventory.c src/data/amiga_ofs.c
    src/data/data_vfs.c src/data/sha256.c)
target_include_directories(amiga_ofs_inventory PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(amiga_ofs_inventory PRIVATE ZLIB::ZLIB)

add_executable(verify_liberation
    tools/verify_liberation.c
    src/data/liberation_data.c src/data/liberation_string_tables.c
    src/data/liberation_anim.c
    src/data/rnc_decoder.c
    src/data/iso9660_reader.c
    src/data/data_vfs.c
    src/data/amiga_ofs.c
    src/data/sha256.c
)
target_include_directories(verify_liberation PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(verify_liberation PRIVATE ZLIB::ZLIB)

add_executable(liberation_extract
    tools/liberation_extract.c
    src/data/liberation_data.c src/data/liberation_string_tables.c
    src/data/liberation_anim.c
    src/data/rnc_decoder.c
    src/data/iso9660_reader.c
    src/data/data_vfs.c
    src/data/amiga_ofs.c
    src/data/sha256.c
)
target_include_directories(liberation_extract PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(liberation_extract PRIVATE ZLIB::ZLIB)

add_executable(liberation_hunk_info
    tools/liberation_hunk_info.c
    src/data/amiga_hunk.c
    src/data/liberation_data.c src/data/liberation_string_tables.c
    src/data/liberation_anim.c
    src/data/rnc_decoder.c
    src/data/iso9660_reader.c
    src/data/data_vfs.c
    src/data/amiga_ofs.c
    src/data/sha256.c
)
target_include_directories(liberation_hunk_info PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(liberation_hunk_info PRIVATE ZLIB::ZLIB)

add_executable(liberation_inventory
    tools/liberation_inventory.c
    src/data/data_vfs.c
    src/data/amiga_ofs.c
    src/data/iso9660_reader.c
    src/data/liberation_data.c src/data/liberation_string_tables.c
    src/data/liberation_anim.c
    src/data/rnc_decoder.c
    src/data/sha256.c)
target_include_directories(liberation_inventory PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(liberation_inventory PRIVATE ZLIB::ZLIB)

add_executable(liberation_form_inventory
    tools/liberation_form_inventory.c
    src/data/data_vfs.c
    src/data/amiga_ofs.c
    src/data/iso9660_reader.c
    src/data/liberation_data.c src/data/liberation_string_tables.c
    src/data/liberation_anim.c
    src/data/rnc_decoder.c
    src/data/sha256.c)
target_include_directories(liberation_form_inventory PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(liberation_form_inventory PRIVATE ZLIB::ZLIB)

add_executable(liberation_presentation_inventory
    tools/liberation_presentation_inventory.c
    src/data/liberation_data.c src/data/liberation_string_tables.c
    src/data/liberation_anim.c
    src/data/rnc_decoder.c
    src/data/iso9660_reader.c
    src/data/data_vfs.c
    src/data/amiga_ofs.c
    src/data/sha256.c
)
target_include_directories(liberation_presentation_inventory PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(liberation_presentation_inventory PRIVATE ZLIB::ZLIB)

add_executable(liberation_presentation_capture
    tools/liberation_presentation_capture.c
    src/data/liberation_data.c src/data/liberation_string_tables.c
    src/data/liberation_anim.c
    src/data/rnc_decoder.c
    src/data/iso9660_reader.c
    src/data/data_vfs.c
    src/data/amiga_ofs.c
    src/data/sha256.c
)
target_include_directories(liberation_presentation_capture PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(liberation_presentation_capture PRIVATE ZLIB::ZLIB)

add_executable(liberation_vgm_verify
    tools/liberation_vgm_verify.c
    src/data/liberation_data.c src/data/liberation_string_tables.c
    src/data/liberation_anim.c src/data/liberation_vgm.c
    src/data/amos_sprite.c
    src/data/rnc_decoder.c
    src/data/iso9660_reader.c
    src/data/data_vfs.c
    src/data/amiga_ofs.c
    src/data/sha256.c
)
target_include_directories(liberation_vgm_verify PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(liberation_vgm_verify PRIVATE ZLIB::ZLIB)

add_executable(amos_sprite_dump tools/amos_sprite_dump.c src/data/amos_sprite.c
    src/data/data_vfs.c src/data/amiga_ofs.c src/data/iso9660_reader.c src/data/liberation_data.c src/data/liberation_string_tables.c
    src/data/liberation_anim.c src/data/rnc_decoder.c src/data/sha256.c)
target_include_directories(amos_sprite_dump PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(amos_sprite_dump PRIVATE ZLIB::ZLIB)

add_executable(captive_panel_match tools/captive_panel_match.c
    src/data/captive_scene_assets.c src/data/pl5_decoder.c src/data/data_vfs.c src/data/amiga_ofs.c src/data/sha256.c)
target_include_directories(captive_panel_match PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(captive_panel_match PRIVATE ZLIB::ZLIB)

# Correlates fixed Captive DOS renderer descriptors with the hash-verified
# renderer memory fixture and its exact VGA frame.  It is an analysis tool;
# it neither accepts nor identifies game data by filename.
add_executable(captive_descriptor_match tools/captive_descriptor_match.c
    src/data/captive_dos_descriptor.c src/data/dos_vga_reference.c
    src/data/pl5_decoder.c src/data/sha256.c)
target_include_directories(captive_descriptor_match PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(captive_map_dump tools/captive_map_dump.c
    src/data/captive_dos_map.c src/data/captive_dos_descriptor.c)
target_include_directories(captive_map_dump PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(captive_runtime_render tools/captive_runtime_render.c
    src/render/captive_dos_runtime.c
    src/data/captive_dos_map.c src/data/captive_dos_descriptor.c
    src/render/captive_compositor.c src/data/dos_vga_reference.c
    src/data/pl5_decoder.c)
target_include_directories(captive_runtime_render PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(dump_pge tools/dump_pge.c
    src/data/liberation_data.c src/data/liberation_string_tables.c
    src/data/liberation_anim.c src/data/rnc_decoder.c
    src/data/iso9660_reader.c src/data/data_vfs.c
    src/data/amiga_ofs.c src/data/sha256.c
    src/engine/arcd_decoder.c)
target_include_directories(dump_pge PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(dump_pge PRIVATE ZLIB::ZLIB)

add_executable(dump_plotgen tools/dump_plotgen.c
    src/data/liberation_data.c src/data/liberation_string_tables.c
    src/data/liberation_anim.c src/data/rnc_decoder.c
    src/data/iso9660_reader.c src/data/data_vfs.c
    src/data/amiga_ofs.c src/data/sha256.c)
target_include_directories(dump_plotgen PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(dump_plotgen PRIVATE ZLIB::ZLIB)

add_executable(st_list tools/st_list.c src/data/st_disk_reader.c src/data/sha256.c)
target_include_directories(st_list PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(visual_compare tools/visual_compare.c)
