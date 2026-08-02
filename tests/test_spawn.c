#include "spawn.h"
#include "captive_data.h"
#include <stdio.h>
#include <string.h>

static int failures = 0;
#define ASSERT(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); failures++; } \
} while(0)

static void test_category_table(void) {
    ASSERT(spawn_categories[0].types[0] == 13, "cat 0 first type");
    ASSERT(spawn_categories[1].types[0] == 0, "cat 1 first type");
    ASSERT(spawn_categories[7].types[2] == 7, "cat 7 last type");
}

static void test_subcell_tables(void) {
    ASSERT(spawn_subcell_table1[0] == 0x00, "table1[0]");
    ASSERT(spawn_subcell_table1[1] == 0x06, "table1[1]");
    ASSERT(spawn_subcell_table2[0] == 0x01, "table2[0]");
    ASSERT(spawn_subcell_table2[3] == 0x05, "table2[3]");
}

static void test_subcell_from_direction(void) {
    uint8_t s = spawn_subcell_from_direction(0, 0);
    ASSERT(s == 0x00, "dir 0 pos 0 -> subcell 0");
    s = spawn_subcell_from_direction(0, 1);
    ASSERT(s == 0x06, "dir 0 pos 1 -> subcell 6");
    s = spawn_subcell_from_direction(1, 0);
    ASSERT(s == 0x02, "dir 1 pos 0 -> subcell 2");
    s = spawn_subcell_from_direction(2, 0);
    ASSERT(s == 0x01, "dir 2 pos 0 -> subcell from table2");
}

static void test_hp_computation(void) {
    uint16_t hp = spawn_compute_hp(1, 4, 60);
    ASSERT(hp >= 6, "HP at least 6");
    ASSERT(hp <= 255, "HP at most 255");

    uint16_t hp_min_diff = spawn_compute_hp(1, 0, 60);
    uint16_t hp_max_diff = spawn_compute_hp(1, 8, 60);
    ASSERT(hp_max_diff >= hp_min_diff, "higher difficulty = higher HP");

    ASSERT(spawn_compute_hp(0, 4, 60) == 6, "type 0 returns 6");
    ASSERT(spawn_compute_hp(25, 4, 60) == 6, "type 25 out of range");
}

static void test_spawn_single(void) {
    uint32_t seed = 0xACE1;
    SpawnResult r = spawn_creatures(1, 3, DIR_NORTH, 0, &seed);
    ASSERT(r.count >= 1, "at least one creature spawned");
    ASSERT(r.entries[0].hp >= 6, "creature has valid HP");
}

static void test_spawn_type_0x0F_directional(void) {
    uint32_t seed = 0x1234;
    for (int i = 0; i < 100; i++) {
        SpawnResult r = spawn_creatures(3, 5, DIR_EAST, 2, &seed);
        if (r.count > 0 && r.entries[0].creature_type == 0x0F) {
            ASSERT(r.count == 3, "type 0x0F spawns 3 creatures");
            return;
        }
    }
}

static void test_spawn_count_bounds(void) {
    uint32_t seed = 0x5678;
    for (int trial = 0; trial < 50; trial++) {
        SpawnResult r = spawn_creatures(trial % SPAWN_CATEGORY_COUNT, 8,
                                        trial % 4, trial % 16, &seed);
        ASSERT(r.count <= MAX_SPAWN_ENTRIES, "within max entries");
    }
}

int main(void) {
    test_category_table();
    test_subcell_tables();
    test_subcell_from_direction();
    test_hp_computation();
    test_spawn_single();
    test_spawn_type_0x0F_directional();
    test_spawn_count_bounds();
    if (failures) {
        printf("%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("All spawn tests passed\n");
    return 0;
}
