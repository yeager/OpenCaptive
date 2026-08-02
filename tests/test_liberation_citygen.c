#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "liberation_citygen.h"

static void test_prng(void) {
    uint16_t state = 42;
    uint16_t r1 = citygen_prng(&state);
    uint16_t r2 = citygen_prng(&state);
    assert(r1 != r2);

    /* Deterministic: same seed → same sequence */
    uint16_t s1 = 42, s2 = 42;
    assert(citygen_prng(&s1) == citygen_prng(&s2));
    assert(citygen_prng(&s1) == citygen_prng(&s2));

    /* Matches formula: state = state * 0x5E5 + 0x29 */
    uint16_t s = 100;
    uint16_t expected = (uint16_t)(100u * 0x5E5u + 0x29u);
    assert(citygen_prng(&s) == expected);
    assert(s == expected);

    printf("  PRNG: OK\n");
}

static void test_compute_params(void) {
    CityGrid grid;
    citygen_init(&grid, 1000, 0);
    citygen_compute_params(&grid);

    assert(grid.density >= 15 && grid.density <= 100);
    assert(grid.num_columns >= 4 && grid.num_columns <= 15);
    assert(grid.num_roads >= 1);
    assert(grid.num_cross_roads >= 1 && grid.num_cross_roads <= 5);
    assert(grid.total_buildings >= 3);
    assert(grid.total_buildings <= CITYGEN_MAX_BUILDINGS);

    /* level 0 forces num_roads = 5 */
    assert(grid.num_roads == 5);

    printf("  compute_params: OK (density=%u cols=%u roads=%u cross=%u total=%u)\n",
           grid.density, grid.num_columns, grid.num_roads,
           grid.num_cross_roads, grid.total_buildings);
}

static void test_determinism(void) {
    CityGrid g1, g2;
    citygen_generate(&g1, 12345, 3);
    citygen_generate(&g2, 12345, 3);

    assert(g1.total_buildings == g2.total_buildings);
    assert(strcmp(g1.city_name, g2.city_name) == 0);
    for (int i = 0; i < (int)g1.total_buildings; i++) {
        assert(g1.buildings[i].type == g2.buildings[i].type);
        assert(strcmp(g1.buildings[i].name, g2.buildings[i].name) == 0);
    }

    printf("  determinism: OK\n");
}

static void test_different_seeds(void) {
    CityGrid g1, g2;
    citygen_generate(&g1, 100, 5);
    citygen_generate(&g2, 200, 5);

    /* Different seeds should produce different cities */
    int diffs = 0;
    int min_total = g1.total_buildings < g2.total_buildings ?
                    g1.total_buildings : g2.total_buildings;
    for (int i = 0; i < min_total; i++) {
        if (g1.buildings[i].type != g2.buildings[i].type) diffs++;
    }
    assert(diffs > 0);
    assert(strcmp(g1.city_name, g2.city_name) != 0);

    printf("  different_seeds: OK\n");
}

static void test_building_types(void) {
    CityGrid grid;
    citygen_generate(&grid, 42, 10);

    int type_counts[CITYGEN_BUILDING_TYPES] = {0};
    for (int i = 0; i < (int)grid.total_buildings; i++) {
        assert(grid.buildings[i].type < CITYGEN_BUILDING_TYPES);
        type_counts[grid.buildings[i].type]++;
    }

    /* At least some variety */
    int types_used = 0;
    for (int t = 0; t < CITYGEN_BUILDING_TYPES; t++) {
        if (type_counts[t] > 0) types_used++;
    }
    assert(types_used >= 3);

    printf("  building_types: OK (%d types used in %u buildings)\n",
           types_used, grid.total_buildings);
}

static void test_building_names(void) {
    CityGrid grid;
    citygen_generate(&grid, 777, 7);

    for (int i = 0; i < (int)grid.total_buildings; i++) {
        assert(strlen(grid.buildings[i].name) > 0);
    }

    /* Fixed types always have their canonical name */
    for (int i = 0; i < (int)grid.total_buildings; i++) {
        if (grid.buildings[i].type == BUILDING_LIBRARY)
            assert(strcmp(grid.buildings[i].name, "Library") == 0);
        if (grid.buildings[i].type == BUILDING_POLICE)
            assert(strcmp(grid.buildings[i].name, "Police Station") == 0);
        if (grid.buildings[i].type == BUILDING_RECORDS)
            assert(strcmp(grid.buildings[i].name, "City Records Office") == 0);
        if (grid.buildings[i].type == BUILDING_RESIDENCE)
            assert(strcmp(grid.buildings[i].name, "Private Residence") == 0);
    }

    printf("  building_names: OK\n");
}

static void test_connections(void) {
    CityGrid grid;
    citygen_generate(&grid, 555, 2);

    int connected = 0;
    for (int i = 0; i < (int)grid.total_buildings; i++) {
        int cc = grid.buildings[i].flags & 3;
        if (cc > 0) connected++;
        assert(cc <= CITYGEN_MAX_CONNECTIONS);
    }
    /* Most buildings should be connected */
    assert(connected > (int)grid.total_buildings / 2);

    printf("  connections: OK (%d/%u connected)\n", connected, grid.total_buildings);
}

static void test_city_name(void) {
    CityGrid grid;
    citygen_generate(&grid, 12345, 0);
    assert(strlen(grid.city_name) > 0);

    /* Should contain a Greek letter suffix */
    assert(strstr(grid.city_name, "Alpha") != NULL);

    citygen_generate(&grid, 12345, 3);
    assert(strstr(grid.city_name, "Delta") != NULL);

    printf("  city_name: OK (\"%s\")\n", grid.city_name);
}

static void test_level_scaling(void) {
    CityGrid g_low, g_high;
    citygen_generate(&g_low, 500, 0);
    citygen_generate(&g_high, 500, 15);

    /* Higher levels should have higher density */
    assert(g_high.density > g_low.density);

    printf("  level_scaling: OK (lvl0=%u lvl15=%u density)\n",
           g_low.density, g_high.density);
}

int main(void) {
    printf("test_liberation_citygen:\n");
    test_prng();
    test_compute_params();
    test_determinism();
    test_different_seeds();
    test_building_types();
    test_building_names();
    test_connections();
    test_city_name();
    test_level_scaling();
    printf("All liberation citygen tests passed.\n");
    return 0;
}
