#include "liberation_citygen_grid.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static void test_prng(void) {
    CityGridState s;
    memset(&s, 0, sizeof(s));
    s.prng_state = 179;
    uint16_t v1 = citygrid_prng(&s);
    uint16_t v2 = citygrid_prng(&s);
    assert(v1 == (uint16_t)(179u * 0x5E5u + 0x29u));
    assert(v2 == (uint16_t)(v1 * 0x5E5u + 0x29u));
}

static void test_init(void) {
    CityGridState s;
    citygrid_init(&s, 5, 3, 4);
    assert(s.seed_hi == 5);
    assert(s.seed_lo == 3);
    assert(s.difficulty == 4);
    assert(s.seed_combined == (5 << 4) + 3);
    assert(s.prng_state == s.seed_combined);
}

static void test_determinism(void) {
    CityGridState a, b;
    citygrid_init(&a, 7, 2, 3);
    citygrid_init(&b, 7, 2, 3);
    citygrid_generate(&a);
    citygrid_generate(&b);

    assert(memcmp(a.plane0, b.plane0, CITYGRID_CELLS) == 0);
    assert(memcmp(a.plane1, b.plane1, CITYGRID_CELLS) == 0);
    assert(memcmp(a.plane2, b.plane2, CITYGRID_CELLS) == 0);
}

static void test_different_seeds(void) {
    CityGridState a, b;
    citygrid_init(&a, 3, 1, 3);
    citygrid_init(&b, 8, 4, 3);
    citygrid_generate(&a);
    citygrid_generate(&b);

    int diffs = 0;
    for (int i = 0; i < CITYGRID_CELLS; i++)
        if (a.plane0[i] != b.plane0[i]) diffs++;
    assert(diffs > 0);
}

static void test_borders(void) {
    CityGridState s;
    citygrid_init(&s, 5, 2, 3);
    citygrid_generate(&s);

    for (int x = 0; x < 64; x++) {
        assert(s.plane0[x] == 0xFF);
        assert(s.plane0[63 * 64 + x] == 0xFF);
    }
    for (int y = 0; y < 64; y++) {
        assert(s.plane0[y * 64] == 0xFF);
        assert(s.plane0[y * 64 + 63] == 0xFF);
    }
}

static void test_has_road_cells(void) {
    CityGridState s;
    citygrid_init(&s, 5, 2, 3);
    citygrid_generate(&s);

    int non_empty = 0;
    for (int i = 0; i < CITYGRID_CELLS; i++)
        if (s.plane0[i] != 0 && s.plane0[i] != 0xFF) non_empty++;
    assert(non_empty > 10);
}

static void test_meta_has_connections(void) {
    CityGridState s;
    citygrid_init(&s, 5, 2, 3);
    citygrid_generate(&s);

    int connected = 0;
    for (int i = 0; i < CITYGRID_META_SIZE * CITYGRID_META_SIZE; i++)
        if (s.meta[i] & 0x0F) connected++;
    assert(connected > 2);
}

static void test_difficulty_zero(void) {
    CityGridState s;
    citygrid_init(&s, 3, 1, 0);
    citygrid_generate(&s);

    for (int x = 0; x < 64; x++)
        assert(s.plane0[x] == 0xFF);
}

static void test_grid_size(void) {
    assert(CITYGRID_WIDTH == 64);
    assert(CITYGRID_HEIGHT == 64);
    assert(CITYGRID_CELLS == 4096);
    assert(CITYGRID_META_SIZE == 8);
}

int main(void) {
    test_prng();
    test_init();
    test_determinism();
    test_different_seeds();
    test_borders();
    test_has_road_cells();
    test_meta_has_connections();
    test_difficulty_zero();
    test_grid_size();
    printf("All CityGen grid tests passed\n");
    return 0;
}
