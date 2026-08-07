#include "map_gen.h"
#include <stdio.h>
#include <assert.h>
#include <limits.h>

static void test_deterministic(void) {
    DungeonLevel a, b;
    map_generate(&a, 12345, 0);
    map_generate(&b, 12345, 0);

    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            assert(a.cells[y][x].type == b.cells[y][x].type);
}

static void test_generated_level_retains_seed(void) {
    DungeonLevel level;
    map_generate(&level, 0x12345678U, 3);
    assert(level.seed == 0x12345678U);
    assert(level.level == 3);
}

static void test_different_seeds(void) {
    DungeonLevel a, b;
    map_generate(&a, 100, 0);
    map_generate(&b, 200, 0);

    int differences = 0;
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            if (a.cells[y][x].type != b.cells[y][x].type)
                differences++;

    assert(differences > 0);
}

static void test_has_floor(void) {
    DungeonLevel lvl;
    map_generate(&lvl, 42, 0);

    int floor_count = 0;
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            if (lvl.cells[y][x].type != CELL_WALL)
                floor_count++;

    assert(floor_count > 50);
}

static void test_border_walls(void) {
    DungeonLevel lvl;
    map_generate(&lvl, 99, 0);

    for (int x = 0; x < MAP_WIDTH; x++) {
        assert(lvl.cells[0][x].type == CELL_WALL);
        assert(lvl.cells[MAP_HEIGHT-1][x].type == CELL_WALL);
    }
    for (int y = 0; y < MAP_HEIGHT; y++) {
        assert(lvl.cells[y][0].type == CELL_WALL);
        assert(lvl.cells[y][MAP_WIDTH-1].type == CELL_WALL);
    }
}

static void test_level_number_is_bounded(void) {
    DungeonLevel low, high;
    map_generate(&low, 123, -1000000);
    map_generate(&high, 123, 1000000);
    assert(low.level == 0);
    assert(high.level == MAX_LEVELS - 1);
}

static void test_has_generators(void) {
    DungeonLevel lvl;
    map_generate(&lvl, 500, 3);

    int gen_count = 0;
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            if (lvl.cells[y][x].type == CELL_GENERATOR)
                gen_count++;

    assert(gen_count > 0);
}

static void test_mission_seed_formula(void) {
    // seed = ((mission-1)*11) + base
    uint32_t seed = ((17 - 1) * 11) + 3;
    assert(seed == 179);
}

static void test_recovered_original_prng(void) {
    static const uint16_t expected[] = {
        0x89f4, 0x1e39, 0xe65d, 0xf75b, 0x40f0, 0xd82b, 0xaa43, 0xb0a0
    };
    uint16_t actual[sizeof(expected) / sizeof(expected[0])];
    mapgen_original_prng_sequence(179, actual, sizeof(actual) / sizeof(actual[0]));
    for (size_t i = 0; i < sizeof(actual) / sizeof(actual[0]); ++i)
        assert(actual[i] == expected[i]);
}

static void test_first_base_start_matches_architect_special_case(void) {
    DungeonLevel levels[MAX_LEVELS];
    int count = 0;
    map_generate_base(levels, &count, 0);
    assert(count == 1);
    assert(levels[0].cells[0][30].type == CELL_FLOOR);
    for (int x = 0; x < MAP_WIDTH; x++)
        if (x != 30) assert(levels[0].cells[0][x].type == CELL_WALL);
}

static void test_implementation_prng_regression(void) {
    /* This locks current deterministic behaviour only. It is not an original
     * MapGen fixture and therefore cannot establish original-game parity. */
    DungeonLevel base;
    map_generate(&base, 179, 0);
    uint64_t checksum = 0;
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            checksum = checksum * 131u + base.cells[y][x].type;
    assert(checksum == UINT64_C(11480416743716816236));
}

static void test_architect_base_layout(void) {
    DungeonLevel levels[MAX_LEVELS];
    int count = 0;
    map_generate_base(levels, &count, 179);
    assert(count >= 2 && count <= 5);
    int doors = 0;
    for (int level = 0; level < count; level++) {
        int usable = 0;
        int locked_doors = 0;
        for (int y = 0; y < MAP_HEIGHT; y++)
            for (int x = 0; x < MAP_WIDTH; x++)
                if (levels[level].cells[y][x].type != CELL_WALL) {
                    usable++;
                    doors += levels[level].cells[y][x].type == CELL_DOOR ||
                             levels[level].cells[y][x].type == CELL_DOOR_LOCKED;
                    locked_doors += levels[level].cells[y][x].type == CELL_DOOR_LOCKED;
                }
        assert(usable > 20);
        assert(locked_doors > 0);
        if (level + 1 < count) {
            int down = 0, up = 0;
            for (int y = 0; y < MAP_HEIGHT; y++)
                for (int x = 0; x < MAP_WIDTH; x++) {
                    down += levels[level].cells[y][x].type == CELL_STAIRS_DOWN;
                    up += levels[level + 1].cells[y][x].type == CELL_STAIRS_UP;
                }
            assert(down == 1 && up == 1);
        }
    }
    assert(doors > 0);
}

static void test_base_levels_store_their_logical_level(void) {
    DungeonLevel levels[MAX_LEVELS];
    int count = 0;
    map_generate_base(levels, &count, 179);
    assert(count > 1);
    for (int i = 0; i < count; i++)
        assert(levels[i].level == i);
}

static void test_logical_floors_are_connected(void) {
    DungeonLevel levels[MAX_LEVELS];
    int count = 0;
    map_generate_base(levels, &count, 179);
    for (int level = 0; level < count; level++) {
        bool seen[MAP_HEIGHT][MAP_WIDTH] = {{false}};
        int queue[MAP_HEIGHT * MAP_WIDTH];
        int head = 0, tail = 0, total = 0;
        for (int y = 0; y < MAP_HEIGHT; y++)
            for (int x = 0; x < MAP_WIDTH; x++)
                if (levels[level].cells[y][x].type != CELL_WALL) {
                    total++;
                    if (tail == 0) { queue[tail++] = y * MAP_WIDTH + x; seen[y][x] = true; }
                }
        while (head < tail) {
            int pos = queue[head++];
            int x = pos % MAP_WIDTH, y = pos / MAP_WIDTH;
            const int dx[] = {0, 1, 0, -1};
            const int dy[] = {-1, 0, 1, 0};
            for (int d = 0; d < 4; d++) {
                int nx = x + dx[d], ny = y + dy[d];
                if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT &&
                    !seen[ny][nx] && levels[level].cells[ny][nx].type != CELL_WALL) {
                    seen[ny][nx] = true;
                    queue[tail++] = ny * MAP_WIDTH + nx;
                }
            }
        }
        assert(head == total);
    }
}

static void assert_base_is_playable(uint32_t seed) {
    DungeonLevel levels[MAX_LEVELS];
    int count = 0;
    map_generate_base(levels, &count, seed);
    assert(count >= 1 && count <= 5);

    int root_cells = 0;
    for (int x = 0; x < MAP_WIDTH; x++)
        root_cells += levels[0].cells[0][x].type != CELL_WALL;
    assert(root_cells > 0);

    for (int level = 0; level < count; level++) {
        bool seen[MAP_HEIGHT][MAP_WIDTH] = {{false}};
        int queue[MAP_HEIGHT * MAP_WIDTH];
        int head = 0, tail = 0, total = 0, generators = 0;
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                if (levels[level].cells[y][x].type != CELL_WALL) {
                    total++;
                    generators += levels[level].cells[y][x].type == CELL_GENERATOR;
                    if (tail == 0) {
                        queue[tail++] = y * MAP_WIDTH + x;
                        seen[y][x] = true;
                    }
                }
            }
        }
        assert(total > 20);
        assert(generators == 1);
        while (head < tail) {
            int pos = queue[head++];
            int x = pos % MAP_WIDTH, y = pos / MAP_WIDTH;
            const int dx[] = {0, 1, 0, -1};
            const int dy[] = {-1, 0, 1, 0};
            for (int d = 0; d < 4; d++) {
                int nx = x + dx[d], ny = y + dy[d];
                if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT &&
                    !seen[ny][nx] && levels[level].cells[ny][nx].type != CELL_WALL) {
                    seen[ny][nx] = true;
                    queue[tail++] = ny * MAP_WIDTH + nx;
                }
            }
        }
        assert(head == total);

        if (level + 1 < count) {
            int down = 0, up = 0;
            for (int y = 0; y < MAP_HEIGHT; y++)
                for (int x = 0; x < MAP_WIDTH; x++) {
                    down += levels[level].cells[y][x].type == CELL_STAIRS_DOWN;
                    up += levels[level + 1].cells[y][x].type == CELL_STAIRS_UP;
                }
            assert(down == 1 && up == 1);
        }
    }
}

static void test_architect_seed_range(void) {
    /* Cover the documented early-map section masks and a broad selection of
     * later mission/base seeds. */
    /* Include seeds outside the original smoke range; seed 161 previously
     * produced an under-sized but nominally valid root floor. */
    for (uint32_t seed = 0; seed < 10000; seed++)
        assert_base_is_playable(seed);
}

static void test_ca_segments_preserved(void) {
    DungeonLevel lvl;
    map_generate(&lvl, 42, 0);

    int has_segments = 0;
    int partial_walls = 0;
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (lvl.cells[y][x].ca_segments != 0) has_segments++;
            if (lvl.cells[y][x].ca_segments != 0 &&
                lvl.cells[y][x].ca_segments != 0x1F)
                partial_walls++;
        }
    }
    assert(has_segments > 0);
    assert(partial_walls > 0);
}

static void test_mapgen_output_validation(void) {
    for (int seed = 0; seed < 20; seed++) {
        for (int level = 0; level < 5; level++) {
            DungeonLevel lvl;
            map_generate(&lvl, (uint32_t)(seed * 1000 + 1), level);

            int walls = 0, floors = 0, generators = 0;
            for (int y = 0; y < MAP_HEIGHT; y++) {
                for (int x = 0; x < MAP_WIDTH; x++) {
                    switch (lvl.cells[y][x].type) {
                        case CELL_WALL: walls++; break;
                        case CELL_FLOOR: floors++; break;
                        case CELL_GENERATOR: generators++; break;
                        default: break;
                    }
                }
            }
            assert(walls > 0);
            assert(floors > 0);
            (void)generators;

            /* Border must be all walls */
            for (int x = 0; x < MAP_WIDTH; x++) {
                assert(lvl.cells[0][x].type == CELL_WALL);
                assert(lvl.cells[MAP_HEIGHT-1][x].type == CELL_WALL);
            }
            for (int y = 0; y < MAP_HEIGHT; y++) {
                assert(lvl.cells[y][0].type == CELL_WALL);
                assert(lvl.cells[y][MAP_WIDTH-1].type == CELL_WALL);
            }
        }
    }
}

static void test_exterior_generation(void) {
    DungeonLevel ext;
    map_generate_exterior(&ext, 30, 0);

    assert(ext.level == -1);

    /* Entrance cell exists */
    assert(ext.cells[1][30].type == CELL_STAIRS_DOWN);

    /* Landing area has open floor */
    int floor_count = 0;
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            if (ext.cells[y][x].type != CELL_WALL)
                floor_count++;
    assert(floor_count > 100);

    /* Border row 0 is all walls (exterior boundary) */
    for (int x = 0; x < MAP_WIDTH; x++)
        assert(ext.cells[0][x].type == CELL_WALL);
}

static void test_exterior_deterministic(void) {
    DungeonLevel a, b;
    map_generate_exterior(&a, 20, 42);
    map_generate_exterior(&b, 20, 42);
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            assert(a.cells[y][x].type == b.cells[y][x].type);
}

static void test_exterior_different_seeds(void) {
    DungeonLevel a, b;
    map_generate_exterior(&a, 20, 100);
    map_generate_exterior(&b, 20, 200);
    int diffs = 0;
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            if (a.cells[y][x].type != b.cells[y][x].type) diffs++;
    assert(diffs > 0);
}

static void test_exterior_entrance_range(void) {
    /* Exterior must work for any entrance_x within map bounds */
    int positions[] = { 1, 10, 30, 50, MAP_WIDTH - 2 };
    for (int i = 0; i < 5; i++) {
        DungeonLevel ext;
        map_generate_exterior(&ext, positions[i], 42);
        int has_stairs = 0;
        for (int y = 0; y < MAP_HEIGHT; y++)
            for (int x = 0; x < MAP_WIDTH; x++)
                has_stairs += ext.cells[y][x].type == CELL_STAIRS_DOWN;
        assert(has_stairs == 1);
    }
}

static void test_exterior_clamps_invalid_entrance(void) {
    DungeonLevel low, high;
    map_generate_exterior(&low, -1000, 1);
    map_generate_exterior(&high, INT_MAX, 1);
    assert(low.cells[1][1].type == CELL_STAIRS_DOWN);
    assert(high.cells[1][MAP_WIDTH - 2].type == CELL_STAIRS_DOWN);
}

static void test_exterior_rejects_null_destination(void) {
    map_generate_exterior(NULL, 10, 1);
}

static void test_base_generator_clears_count_on_null_levels(void) {
    int count = 99;
    map_generate_base(NULL, &count, 1);
    assert(count == 0);
}

static void test_mapgen_connectivity(void) {
    /* Every generated level's floor cells must be connected */
    for (int seed = 0; seed < 10; seed++) {
        DungeonLevel lvl;
        map_generate(&lvl, (uint32_t)(seed * 777 + 1), 0);

        bool seen[MAP_HEIGHT][MAP_WIDTH] = {{false}};
        int queue[MAP_HEIGHT * MAP_WIDTH];
        int head = 0, tail = 0, total = 0;
        for (int y = 0; y < MAP_HEIGHT; y++)
            for (int x = 0; x < MAP_WIDTH; x++)
                if (lvl.cells[y][x].type != CELL_WALL) {
                    total++;
                    if (tail == 0) { queue[tail++] = y * MAP_WIDTH + x; seen[y][x] = true; }
                }
        while (head < tail) {
            int pos = queue[head++];
            int px = pos % MAP_WIDTH, py = pos / MAP_WIDTH;
            const int ddx[] = {0, 1, 0, -1};
            const int ddy[] = {-1, 0, 1, 0};
            for (int d = 0; d < 4; d++) {
                int nx = px + ddx[d], ny = py + ddy[d];
                if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT &&
                    !seen[ny][nx] && lvl.cells[ny][nx].type != CELL_WALL) {
                    seen[ny][nx] = true;
                    queue[tail++] = ny * MAP_WIDTH + nx;
                }
            }
        }
        assert(head == total);
    }
}

int main(void) {
    test_deterministic();
    test_generated_level_retains_seed();
    test_different_seeds();
    test_has_floor();
    test_border_walls();
    test_level_number_is_bounded();
    test_has_generators();
    test_mission_seed_formula();
    test_recovered_original_prng();
    test_first_base_start_matches_architect_special_case();
    test_implementation_prng_regression();
    test_architect_base_layout();
    test_base_levels_store_their_logical_level();
    test_logical_floors_are_connected();
    test_architect_seed_range();
    test_ca_segments_preserved();
    test_mapgen_output_validation();
    test_exterior_generation();
    test_exterior_deterministic();
    test_exterior_different_seeds();
    test_exterior_entrance_range();
    test_exterior_clamps_invalid_entrance();
    test_exterior_rejects_null_destination();
    test_base_generator_clears_count_on_null_levels();
    test_mapgen_connectivity();
    printf("All map generator tests passed\n");
    return 0;
}
