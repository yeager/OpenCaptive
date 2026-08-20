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
    captive_gm_init(&ws);
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
    captive_gm_init(&ws);
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
        captive_gm_init(&ws);
        captive_gm_entry_setup(&ws, cases[i].m, 0u, 0u, 0u);
        captive_gm_seed(&ws);
        assert(w(&ws, 0x3560u) == cases[i].expect);
    }
}

static void test_pass_14c9(void) {
    /* word[0x359A] captured from the real GM.EXE (oracle, at GM 0x3B4). */
    struct { uint16_t m, expect; } cases[] = {
        {1u, 0x0003u}, {2u, 0x0003u}, {5u, 0x0006u},  /* baked table 0x6D16 */
        {10u, 0x0005u}, {20u, 0x0006u},               /* LCG-derived path */
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        CaptiveGmWork ws;
        captive_gm_init(&ws);
        captive_gm_entry_setup(&ws, cases[i].m, 0u, 0u, 0u);
        captive_gm_seed(&ws);
        captive_gm_pass_14c9(&ws);
        assert(w(&ws, 0x359Au) == cases[i].expect);
    }
}

static void test_rng(void) {
    /* First RNG outputs for mission=1, captured from the real GM.EXE (0x1C6E). */
    static const uint16_t EXPECT[12] = {
        0xE860u, 0xF30Au, 0x4F7Bu, 0xD182u, 0xA816u, 0xBD5Fu,
        0x050Bu, 0x99D9u, 0x6F70u, 0x7168u, 0xCC9Eu, 0x53C4u,
    };
    CaptiveGmWork ws;
    captive_gm_init(&ws);
    captive_gm_entry_setup(&ws, 1u, 0u, 0u, 0u);   /* seeds word[0x3074]=1 */
    for (int i = 0; i < 12; ++i)
        assert(captive_gm_rng_next(&ws) == EXPECT[i]);
    /* Determinism: same seed -> same stream. */
    CaptiveGmWork a, b;
    captive_gm_init(&a); captive_gm_entry_setup(&a, 7u, 0u, 0u, 0u);
    captive_gm_init(&b); captive_gm_entry_setup(&b, 7u, 0u, 0u, 0u);
    for (int i = 0; i < 50; ++i)
        assert(captive_gm_rng_next(&a) == captive_gm_rng_next(&b));
}

static void test_rng_pos(void) {
    /* rng_pos packs (y<<8)|x from one RNG draw: x=r&0x3F, y=ror(r,6)&0x1F.  The
     * expected values are the packing of the oracle-verified RNG stream for
     * mission 1 (0xE860, 0xF30A, 0x4F7B): pack -> 0x0120, 0x0C0A, 0x1D3B. */
    static const uint16_t EXPECT[3] = { 0x0120u, 0x0C0Au, 0x1D3Bu };
    CaptiveGmWork ws;
    captive_gm_init(&ws);
    captive_gm_entry_setup(&ws, 1u, 0u, 0u, 0u);
    for (int i = 0; i < 3; ++i) {
        uint16_t p = captive_gm_rng_pos(&ws);
        assert(p == EXPECT[i]);
        assert((p & 0xFFu) <= 0x3Fu);          /* x in range */
        assert(((p >> 8) & 0xFFu) <= 0x1Fu);   /* y in range */
    }
}

static void test_pass_45f(void) {
    /* Room grid + region counts captured from the real GM.EXE (oracle, GM 0x3B7). */
    static const uint8_t GRID_M1[16] = {
        0x01,0x02,0x02,0x02,0x06,0x02,0x02,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,
    };
    static const uint8_t GRID_M2[16] = {
        0x01,0x03,0x03,0x04,0x02,0x02,0x05,0x05,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,
    };
    static const uint8_t GRID_M0[16] = {
        0x01,0x01,0x06,0x06,0x01,0x01,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,
    };
    static const uint8_t GRID_M3[16] = {
        0x01,0x03,0x03,0x02,0x01,0x02,0x02,0x02,0x06,0x02,0x04,0x06,0x06,0x06,0x06,0x06,
    };
    static const uint8_t GRID_M5[16] = {
        0x01,0x01,0x01,0x01,0x01,0x03,0x03,0x06,0x03,0x03,0x03,0x06,0x06,0x02,0x02,0x06,
    };
    struct { uint16_t m; const uint8_t *grid; uint16_t regions; } cases[] = {
        {0u, GRID_M0, 1u}, {1u, GRID_M1, 2u}, {2u, GRID_M2, 5u},
        {3u, GRID_M3, 4u}, {5u, GRID_M5, 3u},
    };
    for (size_t c = 0; c < sizeof(cases)/sizeof(cases[0]); ++c) {
        CaptiveGmWork ws;
        captive_gm_init(&ws);
        captive_gm_entry_setup(&ws, cases[c].m, 0u, 0u, 0u);
        captive_gm_seed(&ws);
        captive_gm_pass_14c9(&ws);
        captive_gm_pass_45f(&ws);
        for (int i = 0; i < 16; ++i)
            assert(ws.b[i] == cases[c].grid[i]);
        assert(w(&ws, 0x002Eu) == cases[c].regions);
        assert(w(&ws, 0x3082u) == cases[c].regions);
        assert(w(&ws, 0x33DAu) == cases[c].regions);
    }
}

static void test_pass_526(void) {
    /* Connection vectors + entry, captured from the real GM.EXE (oracle 0x3BA). */
    struct {
        uint16_t m;
        uint16_t words[8];   /* work[0x10..0x1E] */
        uint16_t w20, w22;
    } cases[] = {
        {1u, {0x0010,0,0,0,0,0,0,0}, 0x000Cu, 0x0001u},
        {2u, {0x0000,0x0010,0x0020,0xFFF0,0x0008,0xFFF8,0x0000,0x0008}, 0x0000u, 0x0001u},
        {3u, {0x0030,0xFFE0,0x0010,0x0000,0x0000,0x0000,0x0010,0x0000}, 0x000Eu, 0x0001u},
    };
    for (size_t c = 0; c < sizeof(cases)/sizeof(cases[0]); ++c) {
        CaptiveGmWork ws;
        captive_gm_init(&ws);
        captive_gm_entry_setup(&ws, cases[c].m, 0u, 0u, 0u);
        captive_gm_seed(&ws);
        captive_gm_pass_14c9(&ws);
        captive_gm_pass_45f(&ws);
        captive_gm_pass_526(&ws);
        for (int i = 0; i < 8; ++i)
            assert(w(&ws, (uint16_t)(0x10u + i*2)) == cases[c].words[i]);
        assert(w(&ws, 0x0020u) == cases[c].w20);
        assert(w(&ws, 0x0022u) == cases[c].w22);
    }
}

static void test_pass_5d4(void) {
    /* Full 2048-word input map (work[0x38..0x1038]) aggregate checks captured from
     * the real GM.EXE (oracle 0x3BD): byte-sum and non-zero count. */
    struct { uint16_t m; unsigned nonzero; uint32_t sum; } cases[] = {
        {1u, 172u, 0x0000AB54u}, {2u, 298u, 0x000128D6u}, {3u, 312u, 0x000136C8u},
    };
    for (size_t c = 0; c < sizeof(cases)/sizeof(cases[0]); ++c) {
        CaptiveGmWork ws;
        captive_gm_init(&ws);
        captive_gm_entry_setup(&ws, cases[c].m, 0u, 0u, 0u);
        captive_gm_seed(&ws);
        captive_gm_pass_14c9(&ws);
        captive_gm_pass_45f(&ws);
        captive_gm_pass_526(&ws);
        captive_gm_pass_5d4(&ws);
        unsigned nz = 0; uint32_t sum = 0;
        for (int i = 0; i < 4096; ++i) {
            uint8_t b = ws.b[0x38 + i];
            if (b) ++nz;
            sum += b;
        }
        assert(nz == cases[c].nonzero);
        assert(sum == cases[c].sum);
    }
}

int main(void) {
    test_entry_pointer_table();
    test_seed_values();
    test_seed_mission_scaling();
    test_pass_14c9();
    test_rng();
    test_rng_pos();
    test_pass_45f();
    test_pass_526();
    test_pass_5d4();
    printf("captive_gm_generator: all tests passed\n");
    return 0;
}
