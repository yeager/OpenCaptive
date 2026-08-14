#include "captive_dos_generator.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Verifies the pieces of CAPPO's dungeon generator recovered by static
 * analysis are transcribed exactly (docs/CAPTIVE_DOS_DUNGEON_RE.md). */

static void test_direction_deltas(void) {
    /* Cardinal steps from DS:0x5E18 / DS:0x5E20. */
    assert(captive_gen_dx[CAPTIVE_GEN_DIR_NORTH] == 0);
    assert(captive_gen_dy[CAPTIVE_GEN_DIR_NORTH] == -1);
    assert(captive_gen_dx[CAPTIVE_GEN_DIR_WEST] == -1);
    assert(captive_gen_dy[CAPTIVE_GEN_DIR_WEST] == 0);
    assert(captive_gen_dx[CAPTIVE_GEN_DIR_SOUTH] == 0);
    assert(captive_gen_dy[CAPTIVE_GEN_DIR_SOUTH] == 1);
    assert(captive_gen_dx[CAPTIVE_GEN_DIR_EAST] == 1);
    assert(captive_gen_dy[CAPTIVE_GEN_DIR_EAST] == 0);
}

static void test_index_and_bounds(void) {
    assert(captive_gen_index(0, 0) == 0);
    assert(captive_gen_index(1, 0) == 1);
    assert(captive_gen_index(0, 1) == 64);   /* (y<<6)+x */
    assert(captive_gen_index(63, 31) == CAPTIVE_GEN_MAP_SIZE - 1);
    assert(captive_gen_in_bounds(0, 0));
    assert(captive_gen_in_bounds(63, 31));
    assert(!captive_gen_in_bounds(64, 0));
    assert(!captive_gen_in_bounds(0, 32));
    assert(!captive_gen_in_bounds(-1, 0));
}

static void test_postprocess(void) {
    uint8_t cells[CAPTIVE_GEN_MAP_SIZE];
    memset(cells, 0, sizeof(cells));

    cells[0] = 0x1B;         /* -> +5 = 0x20 */
    cells[1] = 0x40;         /* -> |0x10 = 0x50 */
    cells[2] = 0x43;         /* -> |0x10 = 0x53 */
    cells[3] = 0x48;         /* -> |0x10 = 0x58 */
    cells[4] = 0x4B;         /* -> |0x10 = 0x5B */
    cells[5] = 0x80 | 0x1B;  /* high bit set: (al=0x1B) still -> +5 = 0x9B+5? */
    cells[6] = 0x10;         /* unchanged (al=0x10) */
    cells[7] = 0x1A;         /* unchanged */

    size_t changed = captive_gen_postprocess(cells);

    assert(cells[0] == 0x20);
    assert(cells[1] == 0x50);
    assert(cells[2] == 0x53);
    assert(cells[3] == 0x58);
    assert(cells[4] == 0x5B);
    /* 0x9B & 0x7F == 0x1B -> add 5 to the WHOLE byte: 0x9B + 5 = 0xA0. */
    assert(cells[5] == 0xA0);
    assert(cells[6] == 0x10);
    assert(cells[7] == 0x1A);
    assert(changed == 6);
}

static void test_wall_predicate(void) {
    assert(!captive_gen_is_wall(0x00));
    assert(!captive_gen_is_wall(0x1A));
    assert(captive_gen_is_wall(0x1B));
    assert(captive_gen_is_wall(0x40));
    /* high bit is masked before comparison */
    assert(captive_gen_is_wall(0x80 | 0x40));
    assert(!captive_gen_is_wall(0x80 | 0x00));
}

int main(void) {
    test_direction_deltas();
    test_index_and_bounds();
    test_postprocess();
    test_wall_predicate();
    printf("captive_dos_generator: all tests passed\n");
    return 0;
}
