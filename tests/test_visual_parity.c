#include "liberation.h"
#include "sha256.h"
#include "start_menu.h"
#include "viewport.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SCREEN_PIXELS (CAPTIVE_ORIGINAL_WIDTH * CAPTIVE_ORIGINAL_HEIGHT)
#define VIEWPORT_PIXELS (CAPTIVE_VIEWPORT_WIDTH * CAPTIVE_VIEWPORT_HEIGHT)

static void digest_hex(const uint32_t *pixels, size_t count, char output[65]) {
    static const char hex[] = "0123456789abcdef";
    uint8_t digest[32];
    sha256_digest((const uint8_t *)pixels, count * sizeof(*pixels), digest);
    for (int i = 0; i < 32; ++i) {
        output[i * 2] = hex[digest[i] >> 4];
        output[i * 2 + 1] = hex[digest[i] & 15];
    }
    output[64] = '\0';
}

static void assert_snapshot(const char *name, const uint32_t *pixels,
                            size_t count, const char *expected) {
    char actual[65];
    digest_hex(pixels, count, actual);
    if (strcmp(actual, expected) != 0)
        fprintf(stderr, "%s visual snapshot changed:\n  expected %s\n  actual   %s\n",
                name, expected, actual);
    assert(strcmp(actual, expected) == 0);
}

static void test_start_menu_snapshot(void) {
    uint32_t pixels[SCREEN_PIXELS] = {0};
    StartMenu menu = {0};
    start_menu_init(&menu);
    start_menu_render(&menu, pixels, CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
    assert_snapshot("start menu", pixels, SCREEN_PIXELS,
                    "1e392323bd14e72e1df8d3b6cf962200f8732703813c2c536a8bbe85ba54c549");
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
                    "5a4f29bbed84a3ca11c12f4c07f41c5befbd04a32906b01c9133aecfdcba631d");
}

static void test_liberation_city_snapshot(void) {
    uint32_t pixels[SCREEN_PIXELS] = {0};
    LibState state;
    lib_init(&state, 42);
    lib_render_city(&state, pixels, CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
    assert_snapshot("liberation city", pixels, SCREEN_PIXELS,
                    "1e4e5ee75098ac23f52afc817a2c70274586fb6f185d53f253e18641b161fec7");
}

int main(void) {
    test_start_menu_snapshot();
    test_captive_viewport_snapshot();
    test_liberation_city_snapshot();
    puts("All visual parity snapshots passed");
    return 0;
}
