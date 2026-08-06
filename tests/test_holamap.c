#include "holamap.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_init(void) {
    Holamap hm;
    holamap_init(&hm, 12345);
    assert(hm.num_bases >= 4 && hm.num_bases <= 8);
    assert(hm.cursor_x == HOLAMAP_WIDTH / 2);
    assert(hm.cursor_y == HOLAMAP_HEIGHT / 2);
    for (int i = 0; i < hm.num_bases; i++) {
        assert(hm.bases[i].x >= 8 && hm.bases[i].x < HOLAMAP_WIDTH - 8);
        assert(hm.bases[i].y >= 8 && hm.bases[i].y < HOLAMAP_HEIGHT - 8);
        assert(!hm.bases[i].revealed);
    }
}

static void test_deterministic(void) {
    Holamap a, b;
    holamap_init(&a, 42);
    holamap_init(&b, 42);
    assert(a.num_bases == b.num_bases);
    assert(memcmp(a.surface, b.surface, sizeof(a.surface)) == 0);
    for (int i = 0; i < a.num_bases; i++) {
        assert(a.bases[i].x == b.bases[i].x);
        assert(a.bases[i].y == b.bases[i].y);
    }
}

static void test_invalid_base_count_does_not_escape_array(void) {
    Holamap hm = {0};
    uint32_t pixel = 0;
    hm.num_bases = 1000;
    holamap_render(&hm, &pixel, 1, 1);
    holamap_reveal_base(&hm, HOLAMAP_MAX_BASES);
    holamap_render(NULL, &pixel, 1, 1);
    holamap_render(&hm, NULL, 1, 1);
}

static void test_reveal(void) {
    Holamap hm;
    holamap_init(&hm, 99);
    assert(!hm.bases[0].revealed);
    holamap_reveal_base(&hm, 0);
    assert(hm.bases[0].revealed);
    holamap_reveal_base(&hm, -1);
    holamap_reveal_base(&hm, 999);
}

static void test_render(void) {
    Holamap hm;
    holamap_init(&hm, 55);
    holamap_reveal_base(&hm, 0);
    uint32_t fb[320 * 200];
    memset(fb, 0, sizeof(fb));
    holamap_render(&hm, fb, 320, 200);
    int nonzero = 0;
    for (int i = 0; i < 320 * 200; i++)
        if (fb[i] != 0) nonzero++;
    assert(nonzero > 0);
}

static void test_surface_variety(void) {
    Holamap hm;
    holamap_init(&hm, 777);
    int counts[5] = {0};
    for (int y = 0; y < HOLAMAP_HEIGHT; y++)
        for (int x = 0; x < HOLAMAP_WIDTH; x++)
            counts[hm.surface[y][x]]++;
    for (int i = 0; i < 5; i++)
        assert(counts[i] > 0);
}

static void test_base_coordinates_seed_sweep(void) {
    for (uint32_t seed = 0; seed < 512; seed++) {
        Holamap hm;
        holamap_init(&hm, seed);
        assert(hm.num_bases >= 4 && hm.num_bases <= 8);
        for (int i = 0; i < hm.num_bases; i++) {
            assert(hm.bases[i].x >= 8 && hm.bases[i].x < HOLAMAP_WIDTH - 8);
            assert(hm.bases[i].y >= 8 && hm.bases[i].y < HOLAMAP_HEIGHT - 8);
        }
    }
}

int main(void) {
    test_init();
    test_deterministic();
    test_invalid_base_count_does_not_escape_array();
    test_reveal();
    test_render();
    test_surface_variety();
    test_base_coordinates_seed_sweep();
    printf("All holamap tests passed.\n");
    return 0;
}
