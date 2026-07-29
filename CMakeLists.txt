cmake_minimum_required(VERSION 3.20)
project(OpenCaptive VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED)

# --- Source files ---

set(OPENCAPTIVE_SOURCES
    src/main.c
    src/engine/engine.c
    src/data/pl5_decoder.c
    src/data/anm_decoder.c
    src/data/mid_loader.c
    src/render/renderer.c
    src/render/viewport.c
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

# --- Tools ---

add_executable(pl5_to_bmp
    tools/pl5_to_bmp.c
    src/data/pl5_decoder.c
)
target_include_directories(pl5_to_bmp PRIVATE ${CMAKE_SOURCE_DIR}/include)
