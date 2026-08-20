#include "captive_gm_generator.h"

#include <string.h>

/*
 * Baked constant tables from GM.EXE's data segment (real game data, transcribed
 * from GM_UNP.EXE at the corresponding work-segment offsets).  Installed into the
 * work buffer by captive_gm_init so the passes read them exactly where GM does.
 */
/* ws:0x6D16 — mission (0..9) -> initial cell/room selector, read by pass 0x14C9. */
static const uint8_t GM_TBL_6D16[10] = { 3, 3, 3, 5, 3, 6, 5, 3, 5, 5 };

void captive_gm_init(CaptiveGmWork *w) {
    memset(w, 0, sizeof(*w));
    memcpy(&w->b[0x6D16u], GM_TBL_6D16, sizeof(GM_TBL_6D16));
}

/* Fill `n` bytes at work offset `off` with `val` (GM's `rep stosb`). */
static void gm_fill(CaptiveGmWork *w, uint16_t off, uint16_t n, uint8_t val) {
    for (uint16_t i = 0; i < n; ++i)
        w->b[(uint16_t)(off + i)] = val;
}

void captive_gm_entry_setup(CaptiveGmWork *w, uint16_t ax, uint16_t bx,
                            uint16_t cx, uint16_t dx) {
    /* GM 0x3A..0x56: mission-parameter storage.  ax=mission, and the entry maps
     * the DOS-EXEC params to these slots (cx->0x307C & masked 0x33DC, bx->0x33E0,
     * dx->0x33DE) exactly as GM does. */
    captive_gm_wset(w, 0x3078u, ax);
    captive_gm_wset(w, 0x3074u, ax);
    captive_gm_wset(w, 0x3076u, ax);
    captive_gm_wset(w, 0x307Cu, cx);
    captive_gm_wset(w, 0x33DCu, (uint16_t)(cx & 0xFFFEu)); /* `and cl,0xFE` */
    captive_gm_wset(w, 0x33E0u, bx);
    captive_gm_wset(w, 0x33DEu, dx);
    captive_gm_wset(w, 0x307Au, 0u);

    /* GM 0x5C..0xB5: buffer pointer table (offsets within the work segment). */
    captive_gm_wset(w, 0x357Eu, 0x3DE2u);
    captive_gm_wset(w, 0x3580u, 0x3DF6u);
    captive_gm_wset(w, 0x3582u, 0x3DFEu);
    captive_gm_wset(w, 0x3584u, 0x3F8Eu);
    captive_gm_wset(w, 0x3586u, 0x3FAEu);
    captive_gm_wset(w, 0x3588u, 0x3FE0u);
    captive_gm_wset(w, 0x358Au, 0x4FE4u);
    captive_gm_wset(w, 0x358Cu, 0x5124u);
    captive_gm_wset(w, 0x358Eu, 0x5A28u);
    captive_gm_wset(w, 0x3590u, 0x5928u);
    captive_gm_wset(w, 0x3592u, 0x59A8u);
    captive_gm_wset(w, 0x3594u, 0x5A08u);
    captive_gm_wset(w, 0x3578u, 0x5A68u);   /* output map pointer */
    captive_gm_wset(w, 0x357Au, 0x6288u);
    captive_gm_wset(w, 0x357Cu, 0x0000u);
}

int captive_gm_seed(CaptiveGmWork *w) {
    /* GM 0x2F2..0x308: word[0x3560] = 0xEEFF - (min(mission,0x7F7) << 3). */
    uint16_t ax = captive_gm_wget(w, 0x3078u);
    if (ax > 0x7F7u)
        ax = 0x7F7u;
    ax = (uint16_t)(ax << 3);
    captive_gm_wset(w, 0x3560u, (uint16_t)(0xEEFFu - ax));

    /* GM 0x30C..0x328: clear working buffers (al=0) addressed by the pointer
     * table entries 0x3582/0x3584/0x3588. */
    gm_fill(w, captive_gm_wget(w, 0x3582u), 0x190u, 0x00u);
    gm_fill(w, captive_gm_wget(w, 0x3584u), 0x020u, 0x00u);
    gm_fill(w, captive_gm_wget(w, 0x3588u), 0x1000u, 0x00u);

    /* GM 0x329..0x35E: al = 0xFF; clear the remaining buffers to 0xFF. */
    gm_fill(w, captive_gm_wget(w, 0x358Au), 0x140u, 0xFFu);
    gm_fill(w, captive_gm_wget(w, 0x357Eu), 0x012u, 0xFFu);
    gm_fill(w, captive_gm_wget(w, 0x3580u), 0x004u, 0xFFu);
    gm_fill(w, captive_gm_wget(w, 0x3590u), 0x080u, 0xFFu);
    gm_fill(w, captive_gm_wget(w, 0x3592u), 0x060u, 0xFFu);
    gm_fill(w, captive_gm_wget(w, 0x3594u), 0x020u, 0xFFu);

    /* GM 0x360..0x376: seed struct at [0x3588]. */
    uint16_t p = captive_gm_wget(w, 0x3588u);
    captive_gm_wset(w, (uint16_t)(p + 0), 0x1000u);
    captive_gm_wset(w, (uint16_t)(p + 2), 0x0FF8u);
    captive_gm_wset(w, (uint16_t)(p + 4), 0x8882u);
    captive_gm_wset(w, (uint16_t)(p + 6), 0x8881u);

    /* GM 0x377..0x38D: seed struct at [0x358C]. */
    p = captive_gm_wget(w, 0x358Cu);
    captive_gm_wset(w, (uint16_t)(p + 0), 0x0800u);
    captive_gm_wset(w, (uint16_t)(p + 2), 0x07F8u);
    captive_gm_wset(w, (uint16_t)(p + 4), 0x8882u);
    captive_gm_wset(w, (uint16_t)(p + 6), 0x8881u);

    /* GM 0x38E..0x3A5: scalar constants. */
    captive_gm_wset(w, 0x353Cu, 0x00C7u);
    captive_gm_wset(w, 0x3540u, 0x0007u);
    captive_gm_wset(w, 0x3542u, 0x0004u);
    captive_gm_wset(w, 0x3398u, 0x003Fu);

    /* GM 0x3A6..0x3B0: `test word[0x33DC],2` selects the alt-path (0x684). */
    return (captive_gm_wget(w, 0x33DCu) & 0x0002u) ? 1 : 0;
}

uint16_t captive_gm_rng_next(CaptiveGmWork *w) {
    /* GM 0x1C6E: state = state*0x5E5 + 0x29; return ror(state,4) ^ 0x800. */
    uint16_t st = captive_gm_wget(w, 0x3074u);
    st = (uint16_t)(st * 0x05E5u + 0x0029u);
    captive_gm_wset(w, 0x3074u, st);
    uint16_t r = (uint16_t)((st >> 4) | (st << 12)); /* ror ax,1 x4 */
    return (uint16_t)(r ^ 0x0800u);
}

void captive_gm_pass_14c9(CaptiveGmWork *w) {
    uint16_t result;

    if (captive_gm_wget(w, 0x307Cu) == 1u) {
        /* GM 0x14CE: word[0x307C]==1 forces the selector to 8. */
        result = 8u;
    } else {
        uint16_t mission = captive_gm_wget(w, 0x3078u);
        if (mission <= 9u) {
            /* GM 0x1507: al = byte[mission + 0x6D16] (baked table). */
            result = w->b[(uint16_t)(0x6D16u + mission)];
        } else {
            /* GM 0x14DD..0x14EF: two LCG steps then the high word of (x*3). */
            uint16_t x = mission;
            x = (uint16_t)(x * 0x05E5u + 0x0029u);
            x = (uint16_t)(x * 0x05E5u + 0x0029u);
            uint16_t hi = (uint16_t)(((uint32_t)x * 3u) >> 16); /* dx of `mul bx` */
            /* GM 0x14F1: 0 -> 3, 1 -> 5, else -> 6. */
            result = (hi == 0u) ? 3u : (hi == 1u) ? 5u : 6u;
        }
    }
    captive_gm_wset(w, 0x359Au, result);
}
