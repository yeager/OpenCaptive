#include "captive_gm_generator.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/*
 * Verifies the entry-setup + seed transcription against values captured from the
 * real GM.EXE work segment via opencaptive-re/gm_oracle.py (mission param ax=1),
 * read at the post_seed breakpoint (GM 0x3B1).  These are the game's own values,
 * not hand-authored expectations.
 */

static uint16_t w(const CaptiveGmWork *ws, uint16_t off) {
    return captive_gm_wget(ws, off);
}

static void test_entry_pointer_table(void) {
    CaptiveGmWork ws;
    memset(&ws, 0, sizeof(ws));
    captive_gm_entry_setup(&ws, 1u, 0u, 0u, 0u);

    /* Pointer table, GM 0x5C..0xB5. */
    assert(w(&ws, 0x357Eu) == 0x3DE2u);
    assert(w(&ws, 0x3580u) == 0x3DF6u);
    assert(w(&ws, 0x3582u) == 0x3DFEu);
    assert(w(&ws, 0x3584u) == 0x3F8Eu);
    assert(w(&ws, 0x3586u) == 0x3FAEu);
    assert(w(&ws, 0x3588u) == 0x3FE0u);
    assert(w(&ws, 0x358Au) == 0x4FE4u);
    assert(w(&ws, 0x358Cu) == 0x5124u);
    assert(w(&ws, 0x358Eu) == 0x5A28u);
    assert(w(&ws, 0x3590u) == 0x5928u);
    assert(w(&ws, 0x3592u) == 0x59A8u);
    assert(w(&ws, 0x3594u) == 0x5A08u);
    assert(w(&ws, 0x3578u) == 0x5A68u);   /* output map pointer */
    assert(w(&ws, 0x357Au) == 0x6288u);

    /* Mission-param slots. */
    assert(w(&ws, 0x3078u) == 1u);
    assert(w(&ws, 0x3074u) == 1u);
    assert(w(&ws, 0x3076u) == 1u);
}

static void test_seed_values(void) {
    CaptiveGmWork ws;
    memset(&ws, 0, sizeof(ws));
    captive_gm_entry_setup(&ws, 1u, 0u, 0u, 0u);
    int alt = captive_gm_seed(&ws);
    assert(alt == 0); /* mission 1: word[0x33DC]=0, normal path */

    /* Scalars captured from the oracle (post_seed). */
    assert(w(&ws, 0x3560u) == 0xEEF7u);   /* 0xEEFF - (1<<3) */
    assert(w(&ws, 0x353Cu) == 0x00C7u);
    assert(w(&ws, 0x3540u) == 0x0007u);
    assert(w(&ws, 0x3542u) == 0x0004u);
    assert(w(&ws, 0x3398u) == 0x003Fu);

    /* Seed struct at [0x3588] = 0x3FE0. */
    uint16_t p = w(&ws, 0x3588u);
    assert(p == 0x3FE0u);
    assert(w(&ws, (uint16_t)(p + 0)) == 0x1000u);
    assert(w(&ws, (uint16_t)(p + 2)) == 0x0FF8u);
    assert(w(&ws, (uint16_t)(p + 4)) == 0x8882u);
    assert(w(&ws, (uint16_t)(p + 6)) == 0x8881u);

    /* Seed struct at [0x358C] = 0x5124. */
    p = w(&ws, 0x358Cu);
    assert(p == 0x5124u);
    assert(w(&ws, (uint16_t)(p + 0)) == 0x0800u);
    assert(w(&ws, (uint16_t)(p + 2)) == 0x07F8u);
    assert(w(&ws, (uint16_t)(p + 4)) == 0x8882u);
    assert(w(&ws, (uint16_t)(p + 6)) == 0x8881u);
}

static void test_seed_mission_scaling(void) {
    /* word[0x3560] = 0xEEFF - (min(mission,0x7F7)<<3). Spot-check a few. */
    struct { uint16_t m, expect; } cases[] = {
        {1u, 0xEEF7u}, {2u, 0xEEEFu}, {0u, 0xEEFFu},
        {0x7F7u, (uint16_t)(0xEEFFu - (0x7F7u << 3))},
        {0x900u, (uint16_t)(0xEEFFu - (0x7F7u << 3))}, /* clamped */
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        CaptiveGmWork ws;
        memset(&ws, 0, sizeof(ws));
        captive_gm_entry_setup(&ws, cases[i].m, 0u, 0u, 0u);
        captive_gm_seed(&ws);
        assert(w(&ws, 0x3560u) == cases[i].expect);
    }
}

int main(void) {
    test_entry_pointer_table();
    test_seed_values();
    test_seed_mission_scaling();
    printf("captive_gm_generator: all tests passed\n");
    return 0;
}
