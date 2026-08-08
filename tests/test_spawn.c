#include "spawn.h"
#include "creature_stats.h"
#include "captive_data.h"
#include <stdio.h>
#include <string.h>

static int failures = 0;
#define ASSERT(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); failures++; } \
} while(0)

static void test_category_table(void) {
    ASSERT(spawn_categories[0].types[0] == 1, "cat 0 first type");
    ASSERT(spawn_categories[1].types[0] == 4, "cat 1 first type");
    ASSERT(spawn_categories[7].types[2] == 24, "cat 7 last type");
    for (int category = 0; category < SPAWN_CATEGORY_COUNT; category++) {
        for (int slot = 0; slot < SPAWN_TYPES_PER_CAT; slot++) {
            uint8_t type = spawn_categories[category].types[slot];
            ASSERT(type >= 1 && type <= 24, "category contains valid creature type");
        }
    }
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

    ASSERT(spawn_compute_hp(1, -100, 60) == hp_min_diff,
           "negative difficulty clamps to minimum");
    ASSERT(spawn_compute_hp(1, 100, 60) == hp_max_diff,
           "high difficulty clamps to maximum");

    ASSERT(spawn_compute_hp(0, 4, 60) == 6, "type 0 returns 6");
    ASSERT(spawn_compute_hp(26, 4, 60) == 6, "type 26 out of range");

    /* Type IDs are 1-based; type 1 has a much smaller HP range than type 2. */
    uint16_t type1 = spawn_compute_hp(1, 0, 255);
    uint16_t type2 = spawn_compute_hp(2, 0, 255);
    ASSERT(type1 < type2, "type 1 uses the first creature stat entry");
}

static void test_spawn_difficulty_is_zero_based(void) {
    bool saw_increase = false;
    for (uint32_t seed = 0; seed < 256; seed++) {
        uint32_t low_seed = seed;
        uint32_t high_seed = seed;
        SpawnResult low = spawn_creatures(0, 0, DIR_NORTH, 0, &low_seed);
        SpawnResult high = spawn_creatures(0, 1, DIR_NORTH, 0, &high_seed);
        ASSERT(low.count == high.count, "difficulty does not change spawn shape");
        if (low.count == 0 || high.count == 0) continue;
        ASSERT(low.entries[0].creature_type == high.entries[0].creature_type,
               "same PRNG seed selects same creature at adjacent difficulties");
        ASSERT(high.entries[0].hp >= low.entries[0].hp,
               "higher dungeon difficulty does not reduce creature HP");
        if (high.entries[0].hp > low.entries[0].hp)
            saw_increase = true;
    }
    ASSERT(saw_increase, "level 1 spawn has a higher HP difficulty than level 0");
}

static void test_spawn_single(void) {
    uint32_t seed = 0xACE1;
    SpawnResult r = spawn_creatures(1, 3, DIR_NORTH, 0, &seed);
    ASSERT(r.count >= 1, "at least one creature spawned");
    ASSERT(r.entries[0].hp >= 6, "creature has valid HP");
}

static void test_spawn_rejects_null_prng(void) {
    ASSERT(spawn_select_type(1, 3, NULL) == 0xFF,
           "null PRNG rejected");
    SpawnResult r = spawn_creatures(1, 3, DIR_NORTH, 0, NULL);
    ASSERT(r.count == 0, "null PRNG produces no creatures");
}

static void test_spawn_rejects_invalid_direction(void) {
    uint32_t seed = 0x1234;
    ASSERT(spawn_subcell_from_direction(4, 0) == 0,
           "invalid direction returns neutral subcell");
    SpawnResult r = spawn_creatures(1, 3, 4, 0, &seed);
    ASSERT(r.count == 0, "invalid direction produces no creatures");
}

static void test_spawn_type_0x0F_directional(void) {
    uint32_t seed = 0x1234;
    for (int i = 0; i < 100; i++) {
        SpawnResult r = spawn_creatures(3, 5, DIR_EAST, 2, &seed);
        if (r.count > 0 && r.entries[0].creature_type == 0x0F) {
            ASSERT(r.count == 2, "type 0x0F spawns 2 creatures");
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

static void test_spawn_modifier_table_covers_all_creatures(void) {
    /* CAPPO.EXE's modifier table is indexed by the 1-based creature ID;
     * entries 16-24 used to fall through to entry 0 in OpenCaptive. */
    ASSERT(spawn_modifier[1] == 4, "type 1 modifier");
    ASSERT(spawn_modifier[15] == 0, "type 15 modifier");
    ASSERT(spawn_modifier[16] == 4, "type 16 modifier");
    ASSERT(spawn_modifier[17] == 8, "type 17 modifier");
    ASSERT(spawn_modifier[18] == 20, "type 18 modifier");
    ASSERT(spawn_modifier[22] == 12, "type 22 modifier");
    ASSERT(spawn_modifier[23] == 4, "type 23 modifier");
    ASSERT(spawn_modifier[24] == 6, "type 24 modifier");
}

static void test_creature_sprite_table_matches_non_alien_record(void) {
    /* CAPPO.EXE v1.06, file offset 0x1895E: type 25 is 0x60/0x20.
     * 0x60 is intentionally unresolved to the six ALIEN PL5 sheets, but its
     * original frame byte must not be discarded. */
    ASSERT(creature_sprite_map[25].graphic_id == 0x60,
           "type 25 source graphic");
    ASSERT(creature_sprite_map[25].frame_index == 0x20,
           "type 25 source frame");
    ASSERT(creature_sprite_sheet_index(25) == -1,
           "type 25 remains outside ALIEN sheets");
}

int main(void) {
    test_category_table();
    test_subcell_tables();
    test_subcell_from_direction();
    test_hp_computation();
    test_spawn_difficulty_is_zero_based();
    test_spawn_single();
    test_spawn_rejects_null_prng();
    test_spawn_rejects_invalid_direction();
    test_spawn_type_0x0F_directional();
    test_spawn_count_bounds();
    test_spawn_modifier_table_covers_all_creatures();
    test_creature_sprite_table_matches_non_alien_record();
    if (failures) {
        printf("%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("All spawn tests passed\n");
    return 0;
}
