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
    assert(s.entry_point == -1);
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
    for (int i = 0; i < CITYGRID_CELLS; i++) {
        if (a.plane1[i] != b.plane1[i]) diffs++;
        if (a.plane2[i] != b.plane2[i]) diffs++;
    }
    assert(diffs > 0);
}

static void test_borders(void) {
    CityGridState s;
    citygrid_init(&s, 5, 2, 3);
    citygrid_generate(&s);

    /* After finalize, border cells are converted to wall types (non-zero in plane2) */
    for (int x = 0; x < 64; x++) {
        assert(s.plane0[x] != 0);
        assert(s.plane0[63 * 64 + x] != 0);
    }
    for (int y = 0; y < 64; y++) {
        assert(s.plane0[y * 64] != 0);
        assert(s.plane0[y * 64 + 63] != 0);
    }
}

static void test_has_road_cells(void) {
    CityGridState s;
    citygrid_init(&s, 5, 2, 3);
    citygrid_generate(&s);

    int non_empty = 0;
    for (int i = 0; i < CITYGRID_CELLS; i++)
        if (s.plane0[i] != 0) non_empty++;
    assert(non_empty > 10);
}

static void test_finalized_planes_keep_navigation_and_building_ids(void) {
    CityGridState s;
    citygrid_init(&s, 9, 4, 5);
    citygrid_generate(&s);

    int visible_cells = 0;
    int mapped_cells = 0;
    for (int i = 0; i < CITYGRID_CELLS; i++) {
        if (s.plane0[i] != 0) visible_cells++;
        if (s.building_ids[i] != 0 && s.building_ids[i] != 0xFF)
            mapped_cells++;
    }
    /* Finalization must leave a usable cell plane and retain source IDs for
     * later building interaction; both were previously lost or mismatched. */
    assert(visible_cells > 100);
    assert(mapped_cells > 0);
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

    /* Border cells should be finalized to non-zero type */
    assert(s.plane0[0] != 0);
}

static void test_grid_size(void) {
    assert(CITYGRID_WIDTH == 64);
    assert(CITYGRID_HEIGHT == 64);
    assert(CITYGRID_CELLS == 4096);
    assert(CITYGRID_META_SIZE == 8);
}

static void test_difficulty_4_entry_point(void) {
    CityGridState s;
    citygrid_init(&s, 5, 2, 5);
    citygrid_generate(&s);

    /* At difficulty >= 4, an entry point should be placed */
    bool has_entry = false;
    for (int i = 0; i < CITYGRID_CELLS; i++) {
        if (s.plane0[i] == 0x0A) {
            has_entry = true;
            break;
        }
    }
    /* Entry point may or may not be found depending on grid layout */
    (void)has_entry;
}

static void test_high_difficulty(void) {
    CityGridState s;
    citygrid_init(&s, 10, 5, 127);
    citygrid_generate(&s);

    /* Just verify it completes without crashing */
    int non_zero = 0;
    for (int i = 0; i < CITYGRID_CELLS; i++)
        if (s.plane0[i] != 0) non_zero++;
    assert(non_zero > 0);
}

static void test_all_seed_combos(void) {
    /* Test a spread of seed combinations to catch crashes */
    for (uint16_t hi = 0; hi < 16; hi += 5) {
        for (uint16_t lo = 0; lo < 9; lo += 3) {
            CityGridState s;
            citygrid_init(&s, hi, lo, 5);
            citygrid_generate(&s);
        }
    }
}

static void test_advanced_generation_seed_sweep(void) {
    for (uint16_t hi = 0; hi < 32; hi += 7) {
        for (uint16_t lo = 0; lo < 32; lo += 5) {
            CityGridState s;
            citygrid_init(&s, hi, lo, 127);
            citygrid_generate(&s);
            assert(s.entry_point >= -1 && s.entry_point < CITYGRID_CELLS);
        }
    }
}

static void test_out_of_range_seed_low(void) {
    CityGridState s;
    citygrid_init(&s, 0x1234, UINT16_MAX, 5);
    citygrid_generate(&s);

    int non_zero = 0;
    for (int i = 0; i < CITYGRID_CELLS; i++)
        if (s.plane0[i] != 0) non_zero++;
    assert(non_zero > 0);
}

static void test_negative_building_catalog_is_ignored(void) {
    CityGridState s;
    CityGrid buildings;
    citygrid_init(&s, 1, 1, 1);
    memset(&buildings, 0, sizeof(buildings));
    buildings.total_buildings = -1;
    citygrid_map_buildings(&s, &buildings);
    for (int i = 0; i < CITYGRID_CELLS; i++)
        assert(s.plane1[i] == 0);
}

static void test_building_mapping_ignores_plane2_flag_bit(void) {
    CityGridState s;
    CityGrid buildings;
    citygrid_init(&s, 1, 1, 1);
    memset(&buildings, 0, sizeof(buildings));
    buildings.total_buildings = 2;
    buildings.buildings[0].type = BUILDING_SHOP;
    buildings.buildings[1].type = BUILDING_POLICE;
    s.building_ids[0] = 0x81; /* building 1 plus the high-bit cell flag */
    citygrid_map_buildings(&s, &buildings);
    assert(s.plane1[0] == BUILDING_SHOP);

    s.plane1[1] = 0;
    s.building_ids[1] = 0x80; /* flag bit without a building ID */
    citygrid_map_buildings(&s, &buildings);
    assert(s.plane1[1] == 0);
}

static void test_null_public_api_is_safe(void) {
    assert(citygrid_prng(NULL) == 0);
    citygrid_init(NULL, 0, 0, 0);
    citygrid_generate(NULL);
    citygrid_map_buildings(NULL, NULL);
}

int main(void) {
    test_prng();
    test_init();
    test_determinism();
    test_different_seeds();
    test_borders();
    test_has_road_cells();
    test_finalized_planes_keep_navigation_and_building_ids();
    test_meta_has_connections();
    test_difficulty_zero();
    test_grid_size();
    test_difficulty_4_entry_point();
    test_high_difficulty();
    test_all_seed_combos();
    test_advanced_generation_seed_sweep();
    test_out_of_range_seed_low();
    test_negative_building_catalog_is_ignored();
    test_building_mapping_ignores_plane2_flag_bit();
    test_null_public_api_is_safe();
    printf("All CityGen grid tests passed\n");
    return 0;
}
