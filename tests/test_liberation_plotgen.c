#include "liberation_plotgen.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int tests_run = 0;

#define TEST(name) do { printf("  %s... ", #name); name(); printf("ok\n"); tests_run++; } while(0)

static void test_prng_sequence(void) {
    PlotgenState ps;
    plotgen_init(&ps, 42);
    assert(ps.prng_state == 42);

    uint16_t v1 = plotgen_prng(&ps);
    assert(v1 == (uint16_t)(42 * 0x5E5 + 0x29));

    uint16_t v2 = plotgen_prng(&ps);
    assert(v2 == (uint16_t)(v1 * 0x5E5 + 0x29));
}

static void test_prng_deterministic(void) {
    PlotgenState a, b;
    plotgen_init(&a, 1234);
    plotgen_init(&b, 1234);

    for (int i = 0; i < 100; i++)
        assert(plotgen_prng(&a) == plotgen_prng(&b));
}

static void test_compute_params_seed0(void) {
    PlotgenState ps;
    plotgen_init(&ps, 0);
    plotgen_compute_params(&ps);

    assert(ps.grid_density <= 100);
    assert(ps.num_roads == 5);
    assert(ps.num_cross_roads >= 1 && ps.num_cross_roads <= 5);
    assert(ps.num_columns >= 1 && ps.num_columns <= 15);
}

static void test_compute_params_deterministic(void) {
    PlotgenState a, b;
    plotgen_init(&a, 777);
    plotgen_init(&b, 777);
    plotgen_compute_params(&a);
    plotgen_compute_params(&b);

    assert(a.grid_density == b.grid_density);
    assert(a.num_columns == b.num_columns);
    assert(a.num_roads == b.num_roads);
    assert(a.num_cross_roads == b.num_cross_roads);
    assert(a.num_total == b.num_total);
}

static void test_generate_buildings(void) {
    PlotgenState ps;
    plotgen_init(&ps, 42);
    plotgen_generate_buildings(&ps);

    assert(ps.num_buildings > 0);
    assert(ps.num_buildings <= PLOTGEN_MAX_BUILDINGS);

    int total_types = 0;
    for (int i = 0; i < PLOTGEN_BTYPE_COUNT; i++)
        total_types += ps.type_counts[i];
    assert(total_types == ps.num_buildings);

    for (uint16_t i = 0; i < ps.num_buildings; i++) {
        assert(ps.buildings[i].type < PLOTGEN_BTYPE_COUNT);
        assert(ps.buildings[i].room_count >= 1);
        assert(ps.buildings[i].room_count <= PLOTGEN_MAX_ROOMS_PER_BUILDING);
    }
}

static void test_generate_names(void) {
    PlotgenState ps;
    plotgen_init(&ps, 42);
    plotgen_generate_buildings(&ps);
    plotgen_generate_names(&ps);

    assert(strlen(ps.city_name) > 0);
    assert(strlen(ps.victim_name) > 0);
    assert(strlen(ps.victim_title) > 0);
    assert(strlen(ps.news_source) > 0);
}

static void test_names_deterministic(void) {
    PlotgenState a, b;
    plotgen_init(&a, 999);
    plotgen_generate_buildings(&a);
    plotgen_generate_names(&a);

    plotgen_init(&b, 999);
    plotgen_generate_buildings(&b);
    plotgen_generate_names(&b);

    assert(strcmp(a.city_name, b.city_name) == 0);
    assert(strcmp(a.victim_name, b.victim_name) == 0);
    assert(strcmp(a.victim_title, b.victim_title) == 0);
    assert(strcmp(a.news_source, b.news_source) == 0);
}

static void test_different_seeds(void) {
    PlotgenState a, b;
    plotgen_init(&a, 1);
    plotgen_generate_buildings(&a);
    plotgen_generate_names(&a);

    plotgen_init(&b, 100);
    plotgen_generate_buildings(&b);
    plotgen_generate_names(&b);

    int any_diff = 0;
    if (strcmp(a.city_name, b.city_name) != 0) any_diff++;
    if (strcmp(a.victim_name, b.victim_name) != 0) any_diff++;
    if (a.grid_density != b.grid_density) any_diff++;
    assert(any_diff > 0);
}

static void test_all_seeds_no_crash(void) {
    for (uint16_t seed = 0; seed < 256; seed++) {
        PlotgenState ps;
        plotgen_init(&ps, seed);
        plotgen_generate_buildings(&ps);
        plotgen_generate_names(&ps);
        assert(ps.num_buildings > 0);
        assert(strlen(ps.city_name) > 0);
    }
}

int main(void) {
    printf("liberation_plotgen tests:\n");
    TEST(test_prng_sequence);
    TEST(test_prng_deterministic);
    TEST(test_compute_params_seed0);
    TEST(test_compute_params_deterministic);
    TEST(test_generate_buildings);
    TEST(test_generate_names);
    TEST(test_names_deterministic);
    TEST(test_different_seeds);
    TEST(test_all_seeds_no_crash);
    printf("%d tests passed\n", tests_run);
    return 0;
}
