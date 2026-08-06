#include "mapgen_ca.h"
#include <stdio.h>
#include <string.h>

static int failures = 0;
#define ASSERT(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); failures++; } \
} while(0)

static void test_init(void) {
    CAMap map;
    ca_init(&map);
    ASSERT(map.cells[0][0][0] == 0, "init zeroes cells");
    ASSERT(map.cells[CA_HEIGHT-1][CA_WIDTH-1][4] == 0, "init zeroes last cell");
}

static void test_pattern_generation(void) {
    CAMap map;
    ca_init(&map);
    uint16_t state = 0x1234;
    ca_generate_pattern(&map, &state);
    int nonzero = 0;
    for (int y = 0; y < CA_HEIGHT; y++)
        for (int x = 0; x < CA_WIDTH; x++)
            for (int b = 0; b < CA_CELL_SIZE; b++)
                if (map.cells[y][x][b]) nonzero++;
    ASSERT(nonzero > 100, "pattern fills cells");
}

static void test_deterministic(void) {
    CAMap m1, m2;
    uint16_t s1 = 0xACE1, s2 = 0xACE1;
    ca_init(&m1); ca_init(&m2);
    ca_generate_pattern(&m1, &s1);
    ca_generate_pattern(&m2, &s2);
    ASSERT(memcmp(&m1, &m2, sizeof(CAMap)) == 0, "same seed = same pattern");
}

static void test_rules_modify(void) {
    for (int type = 0; type < 4; type++) {
        CAMap map;
        ca_init(&map);
        uint16_t state = 42 + type;
        ca_generate_pattern(&map, &state);
        CAMap before;
        memcpy(&before, &map, sizeof(CAMap));
        ca_apply_rules(&map, (CAMapType)type);
        int diffs = 0;
        for (int y = 0; y < CA_HEIGHT; y++)
            for (int x = 0; x < CA_WIDTH; x++)
                for (int b = 0; b < CA_CELL_SIZE; b++)
                    if (map.cells[y][x][b] != before.cells[y][x][b]) diffs++;
        ASSERT(diffs > 0, "rules modify cells");
    }
}

static void test_second_wall_word_is_written(void) {
    const uint8_t input_bits[] = {1, 2, 1, 1};
    const uint8_t opening_bits[] = {0x10, 0x10, 0x18, 0x18};

    for (int type = 0; type < 4; type++) {
        CAMap map;
        ca_init(&map);
        map.cells[0][0][3] = input_bits[type];
        ca_apply_rules(&map, (CAMapType)type);

        /* The second CA word is stored in c[3]/c[4].  Its low byte must
         * receive the rule output; previously the computed value was dead
         * and c[3] retained only the unprocessed input bit. */
        ASSERT(map.cells[0][0][3] == (uint8_t)(input_bits[type] |
                                               opening_bits[type]),
               "second CA wall word receives rule output");
    }
}

static void test_wall_flags_boundary(void) {
    CAMap map;
    ca_init(&map);
    ASSERT(ca_cell_is_wall(&map, -1, 0) == true, "out of bounds is wall");
    ASSERT(ca_cell_wall_flags(&map, -1, 0) == 0xFF, "out of bounds flags");
    ASSERT(ca_cell_is_wall(NULL, 0, 0) == true, "null map is wall");
    ASSERT(ca_cell_wall_flags(NULL, 0, 0) == 0xFF, "null map flags");
    ca_apply_rules(NULL, CA_TYPE_MAZE);
    ca_to_dungeon_level(NULL, NULL, 0);
}

static void test_level_number_is_bounded(void) {
    CAMap map;
    DungeonLevel level;
    ca_init(&map);
    ca_to_dungeon_level(&map, &level, -1000);
    ASSERT(level.level == 0, "negative level is clamped");
    ca_to_dungeon_level(&map, &level, 1000);
    ASSERT(level.level == MAX_LEVELS - 1, "high level is clamped");
}

static void test_different_types_produce_different_maps(void) {
    CAMap maps[4];
    for (int t = 0; t < 4; t++) {
        ca_init(&maps[t]);
        uint16_t state = 0x5678;
        ca_generate_pattern(&maps[t], &state);
        ca_apply_rules(&maps[t], (CAMapType)t);
    }
    int same = 0;
    for (int t = 1; t < 4; t++)
        if (memcmp(&maps[0], &maps[t], sizeof(CAMap)) == 0) same++;
    ASSERT(same == 0, "different types produce different maps");
}

int main(void) {
    test_init();
    test_pattern_generation();
    test_deterministic();
    test_rules_modify();
    test_second_wall_word_is_written();
    test_wall_flags_boundary();
    test_level_number_is_bounded();
    test_different_types_produce_different_maps();
    if (failures) {
        printf("%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("All mapgen_ca tests passed\n");
    return 0;
}
