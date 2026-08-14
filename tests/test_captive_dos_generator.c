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

static void test_carve(void) {
    uint8_t cells[CAPTIVE_GEN_MAP_SIZE];
    memset(cells, 0, sizeof(cells));

    /* single cell preserves high bit, forces low 7 bits to 0x26 */
    cells[captive_gen_index(10, 5)] = 0x80;
    captive_gen_carve_cell(cells, 10, 5);
    assert(cells[captive_gen_index(10, 5)] == (0x80 | 0x26));

    cells[captive_gen_index(11, 5)] = 0x00;
    captive_gen_carve_cell(cells, 11, 5);
    assert(cells[captive_gen_index(11, 5)] == 0x26);

    /* out of bounds is a no-op, not a wild write */
    captive_gen_carve_cell(cells, -1, 5);
    captive_gen_carve_cell(cells, 64, 5);
    captive_gen_carve_cell(cells, 5, 32);

    /* plus brush: centre + W/E/N/S */
    memset(cells, 0, sizeof(cells));
    captive_gen_carve_plus(cells, 20, 10);
    assert(cells[captive_gen_index(20, 10)] == 0x26); /* centre */
    assert(cells[captive_gen_index(19, 10)] == 0x26); /* W */
    assert(cells[captive_gen_index(21, 10)] == 0x26); /* E */
    assert(cells[captive_gen_index(20, 9)]  == 0x26); /* N */
    assert(cells[captive_gen_index(20, 11)] == 0x26); /* S */
    /* diagonal is untouched */
    assert(cells[captive_gen_index(19, 9)]  == 0x00);

    /* plus brush at a corner clips out-of-bounds arms safely */
    memset(cells, 0, sizeof(cells));
    captive_gen_carve_plus(cells, 0, 0);
    assert(cells[captive_gen_index(0, 0)] == 0x26); /* centre */
    assert(cells[captive_gen_index(1, 0)] == 0x26); /* E */
    assert(cells[captive_gen_index(0, 1)] == 0x26); /* S */
    /* 0x26 is a drawn cell (0x26 > 0x1A): is_wall is true for the stamp. */
    assert(captive_gen_is_wall(cells[captive_gen_index(0, 0)]));
}

static void test_rng(void) {
    /* LCG: state = state*0x5E5 + 0x29 (mod 2^16), from CAPPO 0x44EF. */
    uint16_t s = 0;
    assert(captive_gen_rng_next(&s) == 0x0029);            /* 0*0x5E5+0x29 */
    /* 0x29*0x5E5 = 0xF4C5; +0x29 = 0xF4EE */
    assert(captive_gen_rng_next(&s) == (uint16_t)(0x29u * 0x5E5u + 0x29u));
    /* determinism: same seed -> same stream */
    uint16_t a = 12345, b = 12345;
    for (int i = 0; i < 100; ++i)
        assert(captive_gen_rng_next(&a) == captive_gen_rng_next(&b));

    /* direction is always 0..3 */
    for (uint32_t v = 0; v < 0x10000u; v += 7)
        assert(captive_gen_rng_dir((uint16_t)v) >= 0 &&
               captive_gen_rng_dir((uint16_t)v) <= 3);
    /* spot-check the rol-rol-&3 extraction: low byte 0x40 -> rol2 = 0x01 -> 1 */
    assert(captive_gen_rng_dir(0x0040) == 1);
    /* low byte 0x80 -> rol2 = 0x02 -> 2 */
    assert(captive_gen_rng_dir(0x0080) == 2);
    /* low byte 0xC0 -> rol2 = 0x03 -> 3 */
    assert(captive_gen_rng_dir(0x00C0) == 3);
}

int main(void) {
    test_direction_deltas();
    test_index_and_bounds();
    test_postprocess();
    test_wall_predicate();
    test_carve();
    test_rng();
    printf("captive_dos_generator: all tests passed\n");
    return 0;
}
