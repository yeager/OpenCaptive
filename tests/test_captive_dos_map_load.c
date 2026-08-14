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

int main(void) {
    test_wall_predicate();
    test_convert();
    test_null_safe();
    printf("captive_dos_map_load: all tests passed\n");
    return 0;
}
