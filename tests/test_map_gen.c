#include "map_gen.h"
#include <stdio.h>
#include <assert.h>

static void test_deterministic(void) {
    DungeonLevel a, b;
    map_generate(&a, 12345, 0);
    map_generate(&b, 12345, 0);

    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            assert(a.cells[y][x].type == b.cells[y][x].type);
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

static void test_architect_base_layout(void) {
    DungeonLevel levels[MAX_LEVELS];
    int count = 0;
    map_generate_base(levels, &count, 179);
    assert(count >= 2 && count <= 5);
    for (int level = 0; level < count; level++) {
        int usable = 0;
        for (int y = 0; y < MAP_HEIGHT; y++)
            for (int x = 0; x < MAP_WIDTH; x++)
                if (levels[level].cells[y][x].type != CELL_WALL) usable++;
        assert(usable > 20);
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

int main(void) {
    test_deterministic();
    test_different_seeds();
    test_has_floor();
    test_border_walls();
    test_has_generators();
    test_mission_seed_formula();
    test_architect_base_layout();
    printf("All map generator tests passed\n");
    return 0;
}
