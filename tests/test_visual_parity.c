#include "liberation.h"
#include "start_menu.h"
#include "viewport.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SCREEN_PIXELS (CAPTIVE_ORIGINAL_WIDTH * CAPTIVE_ORIGINAL_HEIGHT)
#define VIEWPORT_PIXELS (CAPTIVE_VIEWPORT_WIDTH * CAPTIVE_VIEWPORT_HEIGHT)

static uint64_t image_hash(const uint32_t *pixels, size_t count) {
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < count; ++i) {
        uint32_t pixel = pixels[i];
        for (int byte = 3; byte >= 0; --byte) {
            hash ^= (pixel >> (byte * 8)) & 0xffu;
            hash *= UINT64_C(1099511628211);
        }
    }
    return hash;
}

static void assert_snapshot(const char *name, const uint32_t *pixels,
                            size_t count, uint64_t expected) {
    uint64_t actual = image_hash(pixels, count);
    if (actual != expected)
        fprintf(stderr, "%s visual snapshot changed:\n  expected %016llx\n  actual   %016llx\n",
                name, (unsigned long long)expected, (unsigned long long)actual);
    assert(actual == expected);
}

static void test_start_menu_snapshot(void) {
    uint32_t pixels[SCREEN_PIXELS] = {0};
    StartMenu menu = {0};
    start_menu_init(&menu);
    start_menu_render(&menu, pixels, CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
    assert_snapshot("start menu", pixels, SCREEN_PIXELS,
                    UINT64_C(0xc1c7344455ff335d));
}

static void test_captive_viewport_snapshot(void) {
    uint32_t pixels[VIEWPORT_PIXELS] = {0};
    GameState game = {0};
    game.current_level = 0;
    game.party_x = 32;
    game.party_y = 16;
    game.party_dir = DIR_NORTH;
    for (int y = 0; y < MAP_HEIGHT; ++y)
        for (int x = 0; x < MAP_WIDTH; ++x)
            game.levels[0].cells[y][x].type = CELL_WALL;
    for (int y = 12; y <= 16; ++y)
        game.levels[0].cells[y][32].type = CELL_FLOOR;
    game.levels[0].cells[13][31].type = CELL_DOOR_LOCKED;
    game.levels[0].cells[14][33].type = CELL_GENERATOR;
    viewport_render(&game, pixels, CAPTIVE_VIEWPORT_WIDTH);
    assert_snapshot("captive viewport", pixels, VIEWPORT_PIXELS,
                    UINT64_C(0xc56601e520b7e7a5));
}

static void test_liberation_city_snapshot(void) {
    uint32_t pixels[SCREEN_PIXELS] = {0};
    LibState state;
    lib_init(&state, 42);
    lib_render_city(&state, pixels, CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
    assert_snapshot("liberation city", pixels, SCREEN_PIXELS,
                    UINT64_C(0x5345fa5dedde3ca5));
}

int main(void) {
    test_start_menu_snapshot();
    test_captive_viewport_snapshot();
    test_liberation_city_snapshot();
    puts("All visual parity snapshots passed");
    return 0;
}
