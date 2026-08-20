#include "captive_gm_generator.h"
#include "captive_gm_translate.h"

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

static void test_generate_output(void) {
    /* The 0xEE driver must read the type map (0x1048), gate by the selector map
     * (0x38), and emit translate() into the output map (0x5A68).  Verify the wiring
     * with controlled inputs against the independently-verified translator. */
    CaptiveGmWork ws;
    captive_gm_init(&ws);
    captive_gm_entry_setup(&ws, 1u, 0u, 0u, 0u);
    /* cell 0: type 0x20 (-> wall 0xA0), selector non-zero, aux 0x0500. */
    captive_gm_wset(&ws, 0x1048u, 0x0020u);
    captive_gm_wset(&ws, 0x0038u, 0x0001u);
    captive_gm_wset(&ws, 0x2058u, 0x0500u);
    /* cell 1: type 0x1E, selector 0xFFFF -> translator sees the empty selector. */
    captive_gm_wset(&ws, 0x104Au, 0x001Eu);
    captive_gm_wset(&ws, 0x003Au, 0xFFFFu);
    captive_gm_wset(&ws, 0x205Au, 0x0300u);
    /* cell 2: type 0x22 door with aux high byte selecting the direction. */
    captive_gm_wset(&ws, 0x104Cu, 0x0422u);   /* type_lo=0x22, type_hi=0x04 */
    captive_gm_wset(&ws, 0x003Cu, 0x0007u);
    captive_gm_wset(&ws, 0x205Cu, 0x0000u);

    captive_gm_generate_output(&ws);
    uint16_t out = captive_gm_wget(&ws, 0x3578u);

    /* cell 0: translate(0x20, .., sel=1) = 0xA0 (wall). */
    assert(ws.b[out + 0] == captive_gm_translate_cell(0x20u, 0x00u, 0x0001u));
    assert(ws.b[out + 0] == 0xA0u);
    /* cell 1: translate(0x1E, .., sel=0xFFFF) -> the 0x1E early-match still wins. */
    assert(ws.b[out + 1] == captive_gm_translate_cell(0x1Eu, 0x00u, 0xFFFFu));
    /* cell 2: door translate(0x22, aux_hi=0x04, sel=7). */
    assert(ws.b[out + 2] == captive_gm_translate_cell(0x22u, 0x04u, 0x0007u));

    /* Second map (0x6288): aux_hi expanded (v<<3)|v unless selector empty. */
    uint16_t out2 = captive_gm_wget(&ws, 0x357Au);
    assert(ws.b[out2 + 0] == (uint8_t)((0x05u << 3) | 0x05u)); /* sel!=0/FFFF */
    assert(ws.b[out2 + 1] == 0x03u);                            /* sel==FFFF: unchanged */
}

static void test_map_primitives(void) {
    /* GM 0x248E: (ch*64+cl)*2. */
    assert(captive_gm_map_index(0, 0) == 0u);
    assert(captive_gm_map_index(1, 0) == 2u);
    assert(captive_gm_map_index(0, 1) == 128u);
    assert(captive_gm_map_index(63, 31) == 4094u);

    CaptiveGmWork ws;
    captive_gm_init(&ws);
    /* GM 0x2831: 4x4 room-grid cell covering (cl,ch). */
    for (int i = 0; i < 16; ++i) ws.b[i] = (uint8_t)(0x10 + i);
    assert(captive_gm_grid_cell(&ws, 0, 0) == 0x10u);      /* grid (0,0) */
    assert(captive_gm_grid_cell(&ws, 63, 0) == 0x13u);     /* col 3, row 0 */
    assert(captive_gm_grid_cell(&ws, 0, 8) == 0x14u);      /* col 0, row 1 (ch&0x18=8) */
    assert(captive_gm_grid_cell(&ws, 63, 31) == 0x1Fu);    /* col 3, row 3 */

    /* GM 0x1C1C cell classification against a map based at 0x1048. */
    captive_gm_wset(&ws, (uint16_t)(0x1048u + captive_gm_map_index(5, 5)), 0x0000u);
    captive_gm_wset(&ws, (uint16_t)(0x1048u + captive_gm_map_index(6, 5)), 0x0020u);
    captive_gm_wset(&ws, (uint16_t)(0x1048u + captive_gm_map_index(7, 5)), 0xFFFFu);
    assert(captive_gm_cell_check(&ws, 0x1048u, 5, 5) == CAPTIVE_GM_CELL_EMPTY);
    assert(captive_gm_cell_check(&ws, 0x1048u, 6, 5) == CAPTIVE_GM_CELL_VALID);
    assert(captive_gm_cell_check(&ws, 0x1048u, 7, 5) == CAPTIVE_GM_CELL_BLOCKED);
    assert(captive_gm_cell_check(&ws, 0x1048u, 64, 5) == CAPTIVE_GM_CELL_BLOCKED);
    assert(captive_gm_cell_check(&ws, 0x1048u, 5, 32) == CAPTIVE_GM_CELL_BLOCKED);
}

static void test_pass_1cb5(void) {
    /* Anchor array work[0x3430..] captured from the real GM.EXE (oracle 0x3D0). */
    struct { uint16_t m; uint16_t words[3]; } cases[] = {
        {1u, {0xFFFFu, 0xFFFFu, 0xFFFFu}},       /* mission 1: no anchors */
        {2u, {0x0414u, 0x0434u, 0x0C24u}},
        {3u, {0x0B03u, 0x0B33u, 0xFFFFu}},
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
        captive_gm_pass_1cb5(&ws);
        for (int i = 0; i < 3; ++i)
            assert(w(&ws, (uint16_t)(0x3430u + i*2)) == cases[c].words[i]);
    }

    /* Also verify 0x1CB5's gate-on map writes (second loop) against the real
     * GM.EXE: map1048 (cell type) and map38 (selector) checksums after 0x1CB5. */
    struct { uint16_t m; int t_nz; uint32_t t_ck; int s_nz; uint32_t s_ck; } mc[] = {
        {2u, 9, 0xC9u, 340, 0x14CBBu},
        {3u, 6, 0x86u, 340, 0x14EB6u},
    };
    for (size_t c = 0; c < sizeof(mc)/sizeof(mc[0]); ++c) {
        CaptiveGmWork ws;
        captive_gm_init(&ws);
        captive_gm_entry_setup(&ws, mc[c].m, 0u, 0u, 0u);
        captive_gm_seed(&ws);
        captive_gm_pass_14c9(&ws);
        captive_gm_pass_45f(&ws);
        captive_gm_pass_526(&ws);
        captive_gm_pass_5d4(&ws);
        captive_gm_pass_1cb5(&ws);
        int tnz = 0, snz = 0; uint32_t tck = 0, sck = 0;
        for (int i = 0; i < 4096; ++i) {
            uint8_t t = ws.b[0x1048 + i], s = ws.b[0x38 + i];
            if (t) ++tnz; tck += t;
            if (s) ++snz; sck += s;
        }
        assert(tnz == mc[c].t_nz && tck == mc[c].t_ck);
        assert(snz == mc[c].s_nz && sck == mc[c].s_ck);
    }
}

static void test_pass_1617(void) {
    /* map38 checksums after 0x1617 captured from the real GM.EXE (oracle 0x3D7). */
    struct { uint16_t m; int nz; uint32_t ck; } cases[] = {
        {1u, 172, 0xAB54u},    /* mission 1: 0x1617 draws nothing (unchanged) */
        {2u, 840, 0x33CD3u},
        {3u, 1108, 0x448B6u},
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
        captive_gm_pass_1cb5(&ws);
        captive_gm_pass_1617(&ws);
        int nz = 0; uint32_t ck = 0;
        for (int i = 0; i < 4096; ++i) { uint8_t b = ws.b[0x38 + i]; if (b) ++nz; ck += b; }
        assert(nz == cases[c].nz && ck == cases[c].ck);
    }
}

static void test_pass_d12(void) {
    /* map1048 (cell type) checksums after 0xD12, captured from the real GM.EXE
     * (oracle breakpoint 0x3DA).  The full selector (0x38) and aux (0x2058) maps
     * were verified byte-identical to GM at the same point, and both RNG states
     * (word[0x3074]/word[0x355C]) match, so the placement machine is byte-exact. */
    struct { uint16_t m; int nz; uint32_t ck; } cases[] = {
        {1u, 550, 0x42D6u},
        {2u, 280, 0x17FEu},
        {3u, 191, 0x10DEu},
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
        captive_gm_pass_1cb5(&ws);
        captive_gm_pass_1617(&ws);
        captive_gm_pass_d12(&ws);
        int nz = 0; uint32_t ck = 0;
        for (int i = 0; i < 4096; ++i) { uint8_t b = ws.b[0x1048 + i]; if (b) ++nz; ck += b; }
        assert(nz == cases[c].nz && ck == cases[c].ck);
    }
}

static void test_pass_2589(void) {
    /* selector-map (0x38) checksums after 0xD12 -> 0x2589 (clears the 0xFFFD room
     * outlines 0x1617 wrote); verified byte-identical to the real GM.EXE. */
    struct { uint16_t m; int nz; uint32_t ck; } cases[] = {
        {1u, 1063, 0x1CE3Fu},
        {2u, 1280, 0x25745u},
        {3u, 1254, 0x20B4Fu},
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
        captive_gm_wset(&ws, 0x3070u, 1u);          /* orchestrator step (GM 0x3BD) */
        captive_gm_pass_1cb5(&ws);
        captive_gm_pass_1617(&ws);
        captive_gm_pass_d12(&ws);
        captive_gm_pass_2589(&ws);
        int nz = 0; uint32_t ck = 0;
        for (int i = 0; i < 4096; ++i) { uint8_t b = ws.b[0x38 + i]; if (b) ++nz; ck += b; }
        assert(nz == cases[c].nz && ck == cases[c].ck);
    }
}

static void test_pass_26be(void) {
    /* cell-type (0x1048) and aux (0x2058) checksums after 0xD12 -> 0x2589 -> 0x26BE;
     * verified byte-identical to the real GM.EXE for missions 1/2/3. */
    struct { uint16_t m; int tnz; uint32_t tck; int anz; uint32_t ack; } cases[] = {
        {1u, 734, 0x4855u, 362, 0x279u},
        {2u, 598, 0x2125u, 427, 0x311u},
        {3u, 535, 0x1ABCu, 357, 0x2EDu},
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
        captive_gm_wset(&ws, 0x3070u, 1u);
        captive_gm_pass_1cb5(&ws);
        captive_gm_pass_1617(&ws);
        captive_gm_pass_d12(&ws);
        captive_gm_pass_2589(&ws);
        captive_gm_pass_26be(&ws);
        int tnz = 0, anz = 0; uint32_t tck = 0, ack = 0;
        for (int i = 0; i < 4096; ++i) {
            uint8_t t = ws.b[0x1048 + i], a = ws.b[0x2058 + i];
            if (t) ++tnz; tck += t;
            if (a) ++anz; ack += a;
        }
        assert(tnz == cases[c].tnz && tck == cases[c].tck);
        assert(anz == cases[c].anz && ack == cases[c].ack);
    }
}

static void test_pass_group_164c(void) {
    /* cell-type (0x1048) and selector (0x38) checksums after the post-0xD12 group
     * 0x28B2 -> 0x29F6 -> 0x28B2 -> 0x2888 -> 0x164C; verified byte-identical to the
     * real GM.EXE for missions 1/2/3 (0x29F6 and 0x2888 draw RNG that stays in sync). */
    struct { uint16_t m; int tnz; uint32_t tck; int snz; uint32_t sck; } cases[] = {
        {1u, 703, 0x4EB2u, 999, 0x1B5F6u},
        {2u, 559, 0x3326u, 1203, 0x23F2Du},
        {3u, 504, 0x2BCAu, 1193, 0x20B9Fu},
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
        captive_gm_wset(&ws, 0x3070u, 1u);
        captive_gm_pass_1cb5(&ws);
        captive_gm_pass_1617(&ws);
        captive_gm_pass_d12(&ws);
        captive_gm_pass_2589(&ws);
        captive_gm_pass_26be(&ws);
        captive_gm_pass_28b2(&ws);
        captive_gm_pass_29f6(&ws);
        captive_gm_pass_28b2(&ws);
        captive_gm_pass_2888(&ws);
        captive_gm_pass_164c(&ws);
        captive_gm_pass_2940(&ws);
        int tnz = 0, snz = 0; uint32_t tck = 0, sck = 0;
        for (int i = 0; i < 4096; ++i) {
            uint8_t t = ws.b[0x1048 + i], s = ws.b[0x38 + i];
            if (t) ++tnz; tck += t;
            if (s) ++snz; sck += s;
        }
        assert(tnz == cases[c].tnz && tck == cases[c].tck);
        assert(snz == cases[c].snz && sck == cases[c].sck);
    }
}

static void test_pass_e12(void) {
    /* After the geometry chain + 0xE12 creature/item spawn: cell-type (0x1048) and
     * selector (0x38) checksums plus the entity-buffer (0x3DE2..0x6288) checksum,
     * verified byte-identical to the real GM.EXE (the full 0..0x6A00 work region and
     * both RNG states match; 0x1314 is skipped for standard missions). */
    struct { uint16_t m; int tnz; uint32_t tck; int snz; uint32_t sck; uint32_t eck; } cases[] = {
        {1u, 703, 0x4C66u, 1000, 0x1BDEEu, 0x280AFu},
        {2u, 559, 0x314Bu, 1203, 0x248BCu, 0x27D7Eu},
        {3u, 504, 0x28D2u, 1193, 0x21B57u, 0x279EEu},
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
        captive_gm_wset(&ws, 0x3070u, 1u);
        captive_gm_pass_1cb5(&ws);
        captive_gm_pass_1617(&ws);
        captive_gm_pass_d12(&ws);
        captive_gm_pass_2589(&ws);
        captive_gm_pass_26be(&ws);
        captive_gm_pass_28b2(&ws);
        captive_gm_pass_29f6(&ws);
        captive_gm_pass_28b2(&ws);
        captive_gm_pass_2888(&ws);
        captive_gm_pass_164c(&ws);
        captive_gm_pass_2940(&ws);
        captive_gm_pass_e12(&ws);
        int tnz = 0, snz = 0; uint32_t tck = 0, sck = 0, eck = 0;
        for (int i = 0; i < 4096; ++i) {
            uint8_t t = ws.b[0x1048 + i], s = ws.b[0x38 + i];
            if (t) ++tnz; tck += t;
            if (s) ++snz; sck += s;
        }
        for (int i = 0x3DE2; i < 0x6288; ++i) eck += ws.b[i];
        assert(tnz == cases[c].tnz && tck == cases[c].tck);
        assert(snz == cases[c].snz && sck == cases[c].sck);
        assert(eck == cases[c].eck);
    }
}

static void test_pass_1736(void) {
    /* cell-type (0x1048) and selector (0x38) checksums after ... 0xE12 -> 0x1736 (the
     * chest/altar placement); verified byte-identical to the real GM.EXE. */
    struct { uint16_t m; int tnz; uint32_t tck; int snz; uint32_t sck; } cases[] = {
        {1u, 703, 0x4BFDu, 1001, 0x1C87Bu},
        {2u, 559, 0x311Eu, 1204, 0x24CC1u},
        {3u, 504, 0x2896u, 1194, 0x2218Du},
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
        captive_gm_wset(&ws, 0x3070u, 1u);
        captive_gm_pass_1cb5(&ws);
        captive_gm_pass_1617(&ws);
        captive_gm_pass_d12(&ws);
        captive_gm_pass_2589(&ws);
        captive_gm_pass_26be(&ws);
        captive_gm_pass_28b2(&ws);
        captive_gm_pass_29f6(&ws);
        captive_gm_pass_28b2(&ws);
        captive_gm_pass_2888(&ws);
        captive_gm_pass_164c(&ws);
        captive_gm_pass_2940(&ws);
        captive_gm_pass_e12(&ws);
        captive_gm_pass_1736(&ws);
        int tnz = 0, snz = 0; uint32_t tck = 0, sck = 0;
        for (int i = 0; i < 4096; ++i) {
            uint8_t t = ws.b[0x1048 + i], s = ws.b[0x38 + i];
            if (t) ++tnz; tck += t;
            if (s) ++snz; sck += s;
        }
        assert(tnz == cases[c].tnz && tck == cases[c].tck);
        assert(snz == cases[c].snz && sck == cases[c].sck);
    }
}

static void test_pass_1806(void) {
    /* cell-type (0x1048), selector (0x38) and entity-buffer (0x3DE2..0x6288) checksums
     * after ... 0x1736 -> 0x1806 (the main creature/item distribution); verified
     * byte-identical to the real GM.EXE for missions 1/2/3. */
    struct { uint16_t m; int tnz; uint32_t tck; int snz; uint32_t sck; uint32_t eck; } cases[] = {
        {1u, 714, 0x4D2Eu, 1009, 0x1CF83u, 0x26943u},
        {2u, 571, 0x32DBu, 1218, 0x2590Fu, 0x25F42u},
        {3u, 523, 0x2B2Bu, 1214, 0x23321u, 0x2552Cu},
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
        captive_gm_wset(&ws, 0x3070u, 1u);
        captive_gm_pass_1cb5(&ws);
        captive_gm_pass_1617(&ws);
        captive_gm_pass_d12(&ws);
        captive_gm_pass_2589(&ws);
        captive_gm_pass_26be(&ws);
        captive_gm_pass_28b2(&ws);
        captive_gm_pass_29f6(&ws);
        captive_gm_pass_28b2(&ws);
        captive_gm_pass_2888(&ws);
        captive_gm_pass_164c(&ws);
        captive_gm_pass_2940(&ws);
        captive_gm_pass_e12(&ws);
        captive_gm_pass_1736(&ws);
        captive_gm_pass_1806(&ws);
        int tnz = 0, snz = 0; uint32_t tck = 0, sck = 0, eck = 0;
        for (int i = 0; i < 4096; ++i) {
            uint8_t t = ws.b[0x1048 + i], s = ws.b[0x38 + i];
            if (t) ++tnz; tck += t;
            if (s) ++snz; sck += s;
        }
        for (int i = 0x3DE2; i < 0x6288; ++i) eck += ws.b[i];
        assert(tnz == cases[c].tnz && tck == cases[c].tck);
        assert(snz == cases[c].snz && sck == cases[c].sck);
        assert(eck == cases[c].eck);
    }
}

static void test_pass_2a9d(void) {
    /* type/selector checksums after ... 0x1806 -> 0x2A9D (chests at dead ends);
     * verified byte-identical to the real GM.EXE for missions 1/2/3. */
    struct { uint16_t m; int tnz; uint32_t tck; int snz; uint32_t sck; } cases[] = {
        {1u, 716, 0x4D83u, 1013, 0x1D307u},
        {2u, 583, 0x34D9u, 1232, 0x2655Du},
        {3u, 529, 0x2C53u, 1222, 0x23A29u},
    };
    for (size_t c = 0; c < sizeof(cases)/sizeof(cases[0]); ++c) {
        CaptiveGmWork ws;
        captive_gm_init(&ws);
        captive_gm_entry_setup(&ws, cases[c].m, 0u, 0u, 0u);
        captive_gm_seed(&ws);
        captive_gm_pass_14c9(&ws); captive_gm_pass_45f(&ws);
        captive_gm_pass_526(&ws); captive_gm_pass_5d4(&ws);
        captive_gm_wset(&ws, 0x3070u, 1u);
        captive_gm_pass_1cb5(&ws); captive_gm_pass_1617(&ws); captive_gm_pass_d12(&ws);
        captive_gm_pass_2589(&ws); captive_gm_pass_26be(&ws); captive_gm_pass_28b2(&ws);
        captive_gm_pass_29f6(&ws); captive_gm_pass_28b2(&ws); captive_gm_pass_2888(&ws);
        captive_gm_pass_164c(&ws); captive_gm_pass_2940(&ws); captive_gm_pass_e12(&ws);
        captive_gm_pass_1736(&ws); captive_gm_pass_1806(&ws); captive_gm_pass_2a9d(&ws);
        int tnz = 0, snz = 0; uint32_t tck = 0, sck = 0;
        for (int i = 0; i < 4096; ++i) {
            uint8_t t = ws.b[0x1048 + i], s = ws.b[0x38 + i];
            if (t) ++tnz; tck += t;
            if (s) ++snz; sck += s;
        }
        assert(tnz == cases[c].tnz && tck == cases[c].tck);
        assert(snz == cases[c].snz && sck == cases[c].sck);
    }
}
static void test_pass_2abc(void) {
    /* type/selector checksums after ... 0x1806 -> 0x2ABC (guard creatures);
     * verified byte-identical to the real GM.EXE for missions 1/2/3. */
    struct { uint16_t m; int tnz; uint32_t tck; int snz; uint32_t sck; uint32_t eck; } cases[] = {
        {1u, 719, 0x5094u, 1023, 0x1DBD1u, 0x260F0u},
        {2u, 592, 0x3C31u, 1256, 0x27A75u, 0x25999u},
        {3u, 536, 0x339Fu, 1246, 0x24F41u, 0x25E85u},
    };
    for (size_t c = 0; c < sizeof(cases)/sizeof(cases[0]); ++c) {
        CaptiveGmWork ws;
        captive_gm_init(&ws);
        captive_gm_entry_setup(&ws, cases[c].m, 0u, 0u, 0u);
        captive_gm_seed(&ws);
        captive_gm_pass_14c9(&ws); captive_gm_pass_45f(&ws);
        captive_gm_pass_526(&ws); captive_gm_pass_5d4(&ws);
        captive_gm_wset(&ws, 0x3070u, 1u);
        captive_gm_pass_1cb5(&ws); captive_gm_pass_1617(&ws); captive_gm_pass_d12(&ws);
        captive_gm_pass_2589(&ws); captive_gm_pass_26be(&ws); captive_gm_pass_28b2(&ws);
        captive_gm_pass_29f6(&ws); captive_gm_pass_28b2(&ws); captive_gm_pass_2888(&ws);
        captive_gm_pass_164c(&ws); captive_gm_pass_2940(&ws); captive_gm_pass_e12(&ws);
        captive_gm_pass_1736(&ws); captive_gm_pass_1806(&ws); captive_gm_pass_2a9d(&ws); captive_gm_pass_2abc(&ws);
        int tnz = 0, snz = 0; uint32_t tck = 0, sck = 0, eck = 0;
        for (int i = 0; i < 4096; ++i) {
            uint8_t t = ws.b[0x1048 + i], s = ws.b[0x38 + i];
            if (t) ++tnz; tck += t;
            if (s) ++snz; sck += s;
        }
        for (int i = 0x3DE2; i < 0x6288; ++i) eck += ws.b[i];
        assert(tnz == cases[c].tnz && tck == cases[c].tck);
        assert(snz == cases[c].snz && sck == cases[c].sck);
        assert(eck == cases[c].eck);
    }
}
static void test_pass_a2a(void) {
    /* type/selector/entity checksums after ... 0x2ABC -> 0xA2A (item + creature-nest
     * distribution); verified byte-identical to the real GM.EXE for missions 1/2/3
     * (full work-segment diff, 0 bytes divergent). */
    struct { uint16_t m; int tnz; uint32_t tck; int snz; uint32_t sck; uint32_t eck; } cases[] = {
        {1u, 727, 0x513Eu, 1023, 0x1DBD1u, 0x2627Bu},
        {2u, 614, 0x3FADu, 1262, 0x27FBBu, 0x2561Bu},
        {3u, 566, 0x3910u, 1256, 0x2580Bu, 0x25B07u},
    };
    for (size_t c = 0; c < sizeof(cases)/sizeof(cases[0]); ++c) {
        CaptiveGmWork ws;
        captive_gm_init(&ws);
        captive_gm_entry_setup(&ws, cases[c].m, 0u, 0u, 0u);
        captive_gm_seed(&ws);
        captive_gm_pass_14c9(&ws); captive_gm_pass_45f(&ws);
        captive_gm_pass_526(&ws); captive_gm_pass_5d4(&ws);
        captive_gm_wset(&ws, 0x3070u, 1u);
        captive_gm_pass_1cb5(&ws); captive_gm_pass_1617(&ws); captive_gm_pass_d12(&ws);
        captive_gm_pass_2589(&ws); captive_gm_pass_26be(&ws); captive_gm_pass_28b2(&ws);
        captive_gm_pass_29f6(&ws); captive_gm_pass_28b2(&ws); captive_gm_pass_2888(&ws);
        captive_gm_pass_164c(&ws); captive_gm_pass_2940(&ws); captive_gm_pass_e12(&ws);
        captive_gm_pass_1736(&ws); captive_gm_pass_1806(&ws); captive_gm_pass_2a9d(&ws);
        captive_gm_pass_2abc(&ws); captive_gm_pass_a2a(&ws);
        int tnz = 0, snz = 0; uint32_t tck = 0, sck = 0, eck = 0;
        for (int i = 0; i < 4096; ++i) {
            uint8_t t = ws.b[0x1048 + i], s = ws.b[0x38 + i];
            if (t) ++tnz; tck += t;
            if (s) ++snz; sck += s;
        }
        for (int i = 0x3DE2; i < 0x6288; ++i) eck += ws.b[i];
        assert(tnz == cases[c].tnz && tck == cases[c].tck);
        assert(snz == cases[c].snz && sck == cases[c].sck);
        assert(eck == cases[c].eck);
    }
}

int main(void) {
    test_entry_pointer_table();
    test_map_primitives();
    test_seed_values();
    test_seed_mission_scaling();
    test_pass_14c9();
    test_rng();
    test_rng_pos();
    test_pass_45f();
    test_pass_526();
    test_pass_5d4();
    test_pass_1cb5();
    test_pass_1617();
    test_pass_d12();
    test_pass_2589();
    test_pass_26be();
    test_pass_group_164c();
    test_pass_e12();
    test_pass_1736();
    test_pass_1806();
    test_pass_2a9d();
    test_pass_2abc();
    test_pass_a2a();
    test_generate_output();
    printf("captive_gm_generator: all tests passed\n");
    return 0;
}
