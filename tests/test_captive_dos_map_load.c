#include "captive_dos_map_load.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Verifies the captured-map -> DungeonLevel converter (the piece that renders a
 * real, savestate-captured Captive dungeon).  The input bytes here are a small
 * hand-built fixture in CAPPO's exact 64x32 layout; per the project rule this
 * synthetic input is allowed solely because its purpose is to exercise the path
 * that renders REAL captured data. */

static void test_wall_predicate(void) {
    assert(!captive_dos_cell_is_wall(0x00));
    assert(!captive_dos_cell_is_wall(0x1A));
    assert(captive_dos_cell_is_wall(0x1B));
    assert(captive_dos_cell_is_wall(0x26)); /* walk stamp */
    assert(captive_dos_cell_is_wall(0x1E)); /* digger stamp */
    assert(captive_dos_cell_is_wall(0x80 | 0x40)); /* marker bit masked off */
    assert(!captive_dos_cell_is_wall(0x80 | 0x00));
}

static void test_convert(void) {
    uint8_t raw[CAPTIVE_DOS_RAW_MAP_SIZE];
    memset(raw, 0, sizeof(raw));

    /* A little room: border of walls (0x26) around an open interior. */
    for (int x = 0; x < MAP_WIDTH; ++x) {
        raw[0 * MAP_WIDTH + x] = 0x26;               /* top row wall */
        raw[(MAP_HEIGHT - 1) * MAP_WIDTH + x] = 0x26; /* bottom row wall */
    }
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        raw[y * MAP_WIDTH + 0] = 0x26;                /* left col wall */
        raw[y * MAP_WIDTH + (MAP_WIDTH - 1)] = 0x26;  /* right col wall */
    }
    raw[5 * MAP_WIDTH + 10] = 0x80 | 0x26;            /* wall + marker bit */
    raw[6 * MAP_WIDTH + 10] = 0x0A;                   /* open floor (<=0x1a) */

    DungeonLevel level;
    captive_dos_map_to_level(&level, raw);

    /* borders are walls */
    assert(level.cells[0][0].type == CELL_WALL);
    assert(level.cells[0][MAP_WIDTH - 1].type == CELL_WALL);
    assert(level.cells[MAP_HEIGHT - 1][5].type == CELL_WALL);
    /* interior default is floor */
    assert(level.cells[10][20].type == CELL_FLOOR);
    /* marker bit preserved in flags, still a wall */
    assert(level.cells[5][10].type == CELL_WALL);
    assert(level.cells[5][10].flags == 0x80);
    /* low value is floor */
    assert(level.cells[6][10].type == CELL_FLOOR);
}

static void test_null_safe(void) {
    DungeonLevel level;
    captive_dos_map_to_level(&level, NULL); /* must not crash */
    captive_dos_map_to_level(NULL, NULL);   /* must not crash */
    (void)level;
}

static void test_build_level_from_generator(void) {
    /* End-to-end: the native GM.EXE generator feeds the DungeonLevel converter.  The
     * result must be a plausible dungeon (a mix of walls and floors) and deterministic
     * per mission.  The floor counts are those of the byte-exact GM output map for
     * missions 1/2/3 (2048 - non-wall cells of the finished level). */
    struct { uint16_t m; int walls; } cases[] = { {1u, 437}, {2u, 464}, {3u, 428} };
    for (size_t c = 0; c < sizeof(cases)/sizeof(cases[0]); ++c) {
        DungeonLevel level;
        assert(captive_gm_build_level(&level, cases[c].m) == 0);
        int walls = 0, floors = 0;
        for (int y = 0; y < MAP_HEIGHT; ++y)
            for (int x = 0; x < MAP_WIDTH; ++x) {
                if (level.cells[y][x].type == CELL_WALL) ++walls; else ++floors;
            }
        assert(walls == cases[c].walls);
        assert(floors == MAP_WIDTH * MAP_HEIGHT - cases[c].walls);
        assert(walls > 0 && floors > 0);              /* a real dungeon, not all-solid */
        /* Determinism: a second run must match byte-for-byte. */
        DungeonLevel again;
        assert(captive_gm_build_level(&again, cases[c].m) == 0);
        assert(memcmp(&level, &again, sizeof(level)) == 0);
    }
}

int main(void) {
    test_wall_predicate();
    test_convert();
    test_null_safe();
    test_build_level_from_generator();
    printf("captive_dos_map_load: all tests passed\n");
    return 0;
}
