#include "captive_gm_generator.h"
#include "captive_gm_translate.h"

#include <string.h>

/*
 * Baked constant tables from GM.EXE's data segment (real game data, transcribed
 * from GM_UNP.EXE at the corresponding work-segment offsets).  Installed into the
 * work buffer by captive_gm_init so the passes read them exactly where GM does.
 */
/* ws:0x6D16 — mission (0..9) -> initial cell/room selector, read by pass 0x14C9. */
static const uint8_t GM_TBL_6D16[10] = { 3, 3, 3, 5, 3, 6, 5, 3, 5, 5 };
/* ws:0x6D20 — mission (0..6) -> 16-bit room seed pattern, read by pass 0x45F. */
static const uint8_t GM_TBL_6D20[14] = {
    0x00, 0xCC, 0x00, 0xF6, 0x00, 0xFF, 0x60, 0xFF,
    0xF0, 0xFF, 0xF6, 0xFF, 0xFF, 0xFF,
};
/* ws:0x6D2E / 0x6D36 — random-step delta tables (indexed 0,2,4,6), pass 0x45F. */
static const uint8_t GM_TBL_6D2E[8] = { 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF };
static const uint8_t GM_TBL_6D36[8] = { 0x04, 0x00, 0x00, 0x00, 0xFC, 0xFF, 0x00, 0x00 };
/* ws:0x6D3E — 8-word default cell table copied to work[0..0xF] by pass 0x526. */
static const uint8_t GM_TBL_6D3E[16] = {
    0x06, 0x01, 0x06, 0x06, 0x06, 0x01, 0x06, 0x06,
    0x06, 0x01, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
};

/* ws:0x6D4E / 0x6D56 — 0x2055 step-delta tables. */
static const uint8_t GM_TBL_6D4E[8] = { 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF };
static const uint8_t GM_TBL_6D56[8] = { 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 };

/* ws:0x6D5E — 0x2055 room skip-pattern table (8 groups of 8 bytes, indexed by
 * (word[0x308A]&0x38) + row).  word[0x6DDC] walks it; each byte becomes word[0x33D0]
 * whose bit pattern decides which cells in a room row are carved vs left empty. */
static const uint8_t GM_TBL_6D5E[64] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x22, 0x09, 0x04, 0x51, 0x08, 0x45, 0x22,
    0x00, 0x24, 0x00, 0x12, 0x00, 0x49, 0x00, 0x24,
    0x00, 0x04, 0x00, 0x20, 0x00, 0x04, 0x00, 0x20,
    0x00, 0x2A, 0x00, 0x2A, 0x00, 0x2A, 0x00, 0x2A,
    0x00, 0x22, 0x14, 0x00, 0x14, 0x22, 0x41, 0x00,
    0x00, 0x0E, 0x00, 0x70, 0x00, 0x0E, 0x00, 0x70,
    0x00, 0x04, 0x00, 0x44, 0x00, 0x04, 0x00, 0x44,
};

/* ws:0x6AB2 — a block of baked constant lookup tables the post-0xD12 passes read
 * (mission-invariant in GM_UNP.EXE's image).  Notably the 4-direction step deltas at
 * 0x6AE4 (dx: N=0,W=-1,S=0,E=+1) and 0x6AEC (dy: N=-1,W=0,S=+1,E=0) used by 0x2A59. */
static const uint8_t GM_TBL_6AB2[0x4E] = {
    0x02,0x00,0x80,0xFF,0xFE,0xFF,0x80,0x00,0x01,0x00,0x00,0x00,0xFF,0xFF,0x00,0x00,
    0x00,0x00,0xFF,0xFF,0x00,0x00,0x01,0x00,0x02,0x00,0x00,0x00,0x06,0x00,0x06,0x00,
    0x04,0x00,0x04,0x00,0x02,0x00,0x02,0x00,0x00,0x01,0x01,0xFF,0xFF,0x00,0xFF,0x00,
    0x01,0xFF,0x00,0x00,0xFF,0xFF,0x00,0x00,0x01,0x00,0xFF,0xFF,0x00,0x00,0x01,0x00,
    0x00,0x00,0x00,0x02,0x03,0x05,0x00,0x07,0x03,0x05,0x04,0x02,0x01,0x08,
};

/* ws:0x6B00 — the creature/item spawn database (64 8-byte records at 0x6B16 read by
 * 0x2055's spawn engine 0xFCB/0xFE2, plus the small lookup tables at 0x6B00..0x6B15
 * used by 0x11EE).  Mission-invariant baked constant data from GM_UNP.EXE's image. */
static const uint8_t GM_TBL_6B00[0x216] = {
    0x00,0x08,0x06,0x02,0x07,0x04,0x01,0x03,0x05,0x00,0x06,0x04,0x08,0x02,0x00,0x06,
    0x08,0x02,0x13,0x15,0x14,0x00,0x01,0xA0,0x64,0x00,0x14,0x50,0x24,0x00,0x04,0x20,
    0x0A,0x00,0x0A,0x17,0x21,0x00,0x09,0x10,0x03,0x00,0x08,0x09,0x20,0x00,0x44,0x01,
    0x55,0x00,0x01,0x1B,0x20,0x01,0x01,0xE0,0x2C,0x01,0x14,0xC8,0x29,0x00,0x01,0xA6,
    0x2C,0x01,0x0F,0x64,0x27,0x00,0x81,0x06,0x96,0x00,0x04,0x5C,0x00,0x02,0x20,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x81,0x06,0xC8,0x00,0x02,0x5C,0x00,0x02,0x20,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x30,0x32,0x00,0x01,0x0A,0x42,0x00,0x05,0x60,
    0x64,0x00,0x19,0x32,0x42,0x00,0x04,0x01,0x28,0x00,0x16,0x20,0x40,0x00,0x09,0xA0,
    0x1E,0x00,0x05,0x32,0x44,0x00,0x01,0x01,0x8C,0x00,0x0D,0x14,0x41,0x00,0x04,0x01,
    0x23,0x00,0x08,0x2D,0x45,0x00,0x02,0x90,0x64,0x00,0x02,0x64,0x45,0x00,0x04,0x10,
    0x28,0x00,0x03,0x32,0x42,0x00,0x04,0x01,0x14,0x00,0x08,0x18,0x23,0x00,0x01,0x90,
    0x50,0x00,0x0A,0x46,0x26,0x00,0x01,0x01,0xAA,0x00,0x03,0xA0,0x66,0x01,0x09,0x30,
    0x32,0x00,0x09,0x28,0x63,0x00,0x01,0x75,0x78,0x00,0x0A,0x32,0x65,0x00,0x01,0xA5,
    0x78,0x00,0x07,0x50,0x68,0x00,0x01,0xBA,0x78,0x00,0x04,0x78,0x69,0x00,0x04,0x20,
    0x32,0x00,0x0E,0x5A,0x63,0x00,0x44,0x01,0x28,0x00,0x01,0x28,0x60,0x01,0x01,0xCA,
    0x78,0x00,0x01,0xC8,0x6A,0x00,0x01,0xBA,0xC8,0x00,0x03,0xB4,0x69,0x00,0x01,0xB0,
    0xC8,0x00,0x03,0x64,0x80,0x00,0x01,0xD8,0xC8,0x00,0x07,0xDC,0x80,0x00,0x81,0x01,
    0x01,0x00,0x32,0xF1,0x80,0x01,0x09,0x50,0x16,0x00,0x07,0x64,0x42,0x00,0x09,0x40,
    0x1E,0x00,0x03,0x64,0x46,0x00,0x09,0x60,0x3C,0x00,0x01,0xA0,0x4A,0x00,0x04,0xA0,
    0x3C,0x00,0x02,0xC8,0x80,0x00,0x49,0x60,0x28,0x00,0x01,0x50,0x80,0x01,0x41,0x31,
    0x32,0x00,0x0A,0x32,0xA0,0x01,0x01,0x20,0x28,0x00,0x08,0x3C,0xA0,0x00,0x01,0x01,
    0x0A,0x00,0x02,0x0C,0xA4,0x00,0x01,0x31,0x50,0x00,0x0C,0x50,0xC7,0x00,0x04,0x01,
    0x06,0x00,0x03,0x0D,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,
};

/* ws:0x6DE8 — powers-of-two bitmask table indexed by a 3-bit selector in 0xFE2. */
static const uint8_t GM_TBL_6DE8[8] = { 0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80 };

/* ws:0x6D9E — mission-indexed spawn-count table (10 words) + item-type bitmask table
 * (0x6DB4) read by the main creature/item distribution pass 0x1806.  Mission-invariant
 * baked constant data from GM_UNP.EXE. */
static const uint8_t GM_TBL_6D9E[0x3E] = {
    0x00,0x00,0x27,0x00,0x45,0x00,0x63,0x00,0x81,0x00,0x9F,0x00,0xC7,0x00,0xE5,0x00,
    0x17,0x01,0x3F,0x01,0x3F,0x01,0x01,0x00,0x07,0x00,0xFF,0x00,0xFF,0xFF,0x00,0x01,
    0x02,0x05,0x03,0x06,0x09,0x07,0x0A,0x0D,0x0B,0x0E,0x0F,0x0D,0x09,0x0A,0x0D,0x0B,
    0x0E,0x0F,0x0D,0x09,0x0A,0x0D,0x0B,0x0E,0x0F,0x0D,0x09,0x0E,0x0F,0x0D,
};

void captive_gm_init(CaptiveGmWork *w) {
    memset(w, 0, sizeof(*w));
    memcpy(&w->b[0x6AB2u], GM_TBL_6AB2, sizeof(GM_TBL_6AB2));
    memcpy(&w->b[0x6B00u], GM_TBL_6B00, sizeof(GM_TBL_6B00));
    memcpy(&w->b[0x6DE8u], GM_TBL_6DE8, sizeof(GM_TBL_6DE8));
    memcpy(&w->b[0x6D16u], GM_TBL_6D16, sizeof(GM_TBL_6D16));
    memcpy(&w->b[0x6D20u], GM_TBL_6D20, sizeof(GM_TBL_6D20));
    memcpy(&w->b[0x6D2Eu], GM_TBL_6D2E, sizeof(GM_TBL_6D2E));
    memcpy(&w->b[0x6D36u], GM_TBL_6D36, sizeof(GM_TBL_6D36));
    memcpy(&w->b[0x6D3Eu], GM_TBL_6D3E, sizeof(GM_TBL_6D3E));
    memcpy(&w->b[0x6D4Eu], GM_TBL_6D4E, sizeof(GM_TBL_6D4E));
    memcpy(&w->b[0x6D56u], GM_TBL_6D56, sizeof(GM_TBL_6D56));
    memcpy(&w->b[0x6D5Eu], GM_TBL_6D5E, sizeof(GM_TBL_6D5E));
    memcpy(&w->b[0x6D9Eu], GM_TBL_6D9E, sizeof(GM_TBL_6D9E));
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

uint16_t captive_gm_rng_pos(CaptiveGmWork *w) {
    /* GM 0x1C97: r = rng(); returns cx with cl = r & 0x3F (x in 0..63) and
     * ch = ror(r,6) & 0x1F (y in 0..31) — a random map cell coordinate. */
    uint16_t r = captive_gm_rng_next(w);
    uint16_t x = (uint16_t)(r & 0x3Fu);
    uint16_t ror6 = (uint16_t)((r >> 6) | (r << 10));
    uint16_t y = (uint16_t)(ror6 & 0x1Fu);
    return (uint16_t)((y << 8) | x);
}

/* ror/rol of a 16-bit value (rotation result depends only on count mod 16). */
static uint16_t gm_ror16(uint16_t v, unsigned n) {
    n &= 15u; return (uint16_t)((v >> n) | (v << (16 - n)));
}
static uint16_t gm_rol16(uint16_t v, unsigned n) {
    n &= 15u; return (uint16_t)((v << n) | (v >> (16 - n)));
}

/* GM 0x641: read the room-grid cell at index (cl+ch); returns its value.
 * `*idx` receives cl+ch (the original leaves it in bp for the caller's stamp). */
static uint8_t gm_h641(CaptiveGmWork *w, uint16_t si, uint8_t cl, uint8_t ch,
                       uint8_t *idx) {
    uint8_t bp = (uint8_t)(cl + ch);
    *idx = bp;
    return w->b[(uint16_t)(si + bp)];
}

/* GM 0x651: pick a random room-grid cell -> cl = a&3 (col), ch = a&0xC (row*4),
 * where a = low byte of ror(rng, rng&0xF). */
static void gm_h651(CaptiveGmWork *w, uint8_t *cl, uint8_t *ch) {
    uint16_t r = captive_gm_rng_next(w);
    uint8_t a = (uint8_t)(gm_ror16(r, r & 0xFFu) & 0xFFu);
    *cl = (uint8_t)(a & 3u);
    *ch = (uint8_t)(a & 0xCu);
}

/* GM 0x663: step (cl,ch) by a random cardinal delta (tables 0x6D2E/0x6D36 indexed
 * by ror(rng,rng)&6); returns 1 if still in bounds (cl<4 && ch<0xD), else 0. */
static int gm_h663(CaptiveGmWork *w, uint8_t *cl, uint8_t *ch) {
    uint16_t r = captive_gm_rng_next(w);
    uint16_t idx = (uint16_t)(gm_ror16(r, r & 0xFFu) & 6u);
    *cl = (uint8_t)(*cl + w->b[(uint16_t)(0x6D2Eu + idx)]);
    *ch = (uint8_t)(*ch + w->b[(uint16_t)(0x6D36u + idx)]);
    if (*cl >= 4u) return 0;          /* GM 0x67B: cmp cl,4 / jae -> out */
    return (*ch < 0x0Du) ? 1 : 0;     /* GM 0x680: cmp ch,0xD */
}

void captive_gm_pass_45f(CaptiveGmWork *w) {
    const uint16_t si = 0u;
    uint16_t mission = captive_gm_wget(w, 0x3078u);

    /* GM 0x45F..0x492: lay the initial room grid from the mission seed word:
     * MSB-first, cell = 0 where the bit is 1 (open), 6 where 0 (filled).
     * `count` = number of open cells (popcount). */
    uint16_t m = (mission > 6u) ? 6u : mission;
    uint16_t ax = captive_gm_wget(w, (uint16_t)(0x6D20u + m * 2u));
    int count = 0x10;
    for (int di = 0; di < 0x10; ++di) {
        ax = gm_rol16(ax, 1);
        w->b[di] = 0u;
        if ((ax & 1u) == 0u) { --count; w->b[di] = 6u; }
    }

    /* GM 0x494..0x4AB: region count = (ror(rng,1)&3)+2, or 1 for mission 0. */
    uint16_t rr = captive_gm_rng_next(w);
    uint16_t regions = (uint16_t)((gm_ror16(rr, 1) & 3u) + 2u);
    if (mission == 0u) regions = 1u;
    captive_gm_wset(w, 0x3082u, regions);

    uint8_t dl = 1u, ch = 0u, cl = 0u, dh = 0u, idx = 0u;

region_loop:                                   /* GM 0x4AE */
    do {                                        /* find an empty cell */
        gm_h651(w, &cl, &ch);
        dh = gm_h641(w, si, cl, ch, &idx);
    } while (dh != 0u);
seed_stamp:                                     /* GM 0x4B6 */
    --count;
stamp_only:                                     /* GM 0x4BB */
    w->b[(uint16_t)(si + idx)] = dl;
    if (count == 0) goto after_regions;         /* GM 0x4BE */
    /* GM 0x4C6: grow */
    if (!gm_h663(w, &cl, &ch)) goto next_region; /* out of bounds */
    dh = gm_h641(w, si, cl, ch, &idx);
    if (dh == 0u) goto seed_stamp;              /* empty -> extend region */
    if (dl == dh) goto stamp_only;              /* same region -> restamp */
next_region:                                    /* GM 0x4D4 */
    ++dl;
    if (dl <= (uint8_t)regions) goto region_loop;
    dl = (uint8_t)regions;                       /* GM 0x4DC */
after_regions:                                  /* GM 0x4E0 */
    captive_gm_wset(w, 0x002Eu, dl);
    captive_gm_wset(w, 0x33DAu, dl);

    /* GM 0x4EC..0x524: fill any cells still open by copying a region value from a
     * random in-bounds neighbour (up to 4 tries per empty cell). */
    unsigned guard = 0u;
    while (count != 0 && ++guard < 1000000u) {   /* GM 0x4EC: count==0 -> done */
        do {                                     /* GM 0x4F7: find an empty cell */
            gm_h651(w, &cl, &ch);
            dh = gm_h641(w, si, cl, ch, &idx);
        } while (dh != 0u);
        uint8_t empty_idx = idx;                  /* cl+ch of the empty cell */
        for (int dl2 = 3; dl2 >= 0; --dl2) {      /* GM 0x4FF..0x522: 4 tries */
            uint8_t ncl = cl, nch = ch;           /* push cx (the empty cell) */
            if (!gm_h663(w, &ncl, &nch)) continue;        /* GM 0x505 jae 0x51F */
            uint8_t nidx;
            uint8_t nv = gm_h641(w, si, ncl, nch, &nidx); /* GM 0x507 */
            if (nv == 0u) continue;               /* GM 0x50A je 0x51F: empty */
            /* GM 0x50C..0x518: neighbour holds region nv -> copy it into the
             * empty cell and consume one open cell. */
            w->b[(uint16_t)(si + empty_idx)] = nv;
            --count;
            break;                                /* GM 0x51D jmp 0x4EC */
        }
    }
}

/* GM 0x532/0x557: find a room-grid cell holding region `id`, scanning from a
 * random start index (rng & 0xF) forward mod 16.  Returns the 4-bit index. */
static uint16_t gm_find_region_cell(CaptiveGmWork *w, uint16_t si, uint8_t id) {
    uint16_t idx = (uint16_t)(captive_gm_rng_next(w) & 0x0Fu);
    while (w->b[(uint16_t)(si + idx)] != id)
        idx = (uint16_t)((idx + 1u) & 0x0Fu);
    return idx;
}

void captive_gm_pass_526(CaptiveGmWork *w) {
    const uint16_t si = 0u;
    uint16_t regions = captive_gm_wget(w, 0x002Eu);

    /* GM 0x526..0x598: for each adjacent region pair, store the (col,row) delta
     * between one cell of region dx and one of region dx+1, scaled (col<<4, row<<3)
     * at word[0x10+(dx-1)*2] and word[0x18+(dx-1)*2]. */
    for (uint16_t dx = 1u; dx < regions; ++dx) {
        uint16_t i1 = gm_find_region_cell(w, si, (uint8_t)dx);
        uint16_t col1 = i1 & 3u, row1 = (i1 >> 2) & 3u;
        uint16_t i2 = gm_find_region_cell(w, si, (uint8_t)(dx + 1u));
        uint16_t col2 = i2 & 3u, row2 = (i2 >> 2) & 3u;
        uint16_t ax = (uint16_t)(col2 - col1);      /* GM 0x570: sub (wraps) */
        uint16_t bx = (uint16_t)(row2 - row1);      /* GM 0x574 */
        uint16_t bp = (uint16_t)(0x10u + (dx - 1u) * 2u);
        captive_gm_wset(w, bp, (uint16_t)(ax << 4));
        captive_gm_wset(w, (uint16_t)(bp + 8u), (uint16_t)(bx << 3));
    }

    /* GM 0x59A..0x5D3: pick the entry cell.  Draw rng&0x3F until (v&0xF)!=0xF;
     * for mission 0 the grid is reset to the default table and v forced to 0x1E.
     * word[0x20]=v (entry), word[0x22]=grid cell at (v>>4). */
    uint16_t ax;
    do {
        ax = (uint16_t)(captive_gm_rng_next(w) & 0x3Fu);
    } while ((ax & 0x0Fu) == 0x0Fu);
    if (captive_gm_wget(w, 0x3078u) == 0u) {
        memcpy(&w->b[0], GM_TBL_6D3E, sizeof(GM_TBL_6D3E));
        ax = 0x1Eu;
    }
    captive_gm_wset(w, 0x0020u, ax);
    captive_gm_wset(w, 0x0022u, w->b[(uint16_t)(ax >> 4)]);
}

CaptiveGmCellStatus captive_gm_cell_check(const CaptiveGmWork *w, uint16_t map_off,
                                          uint8_t cl, uint8_t ch) {
    /* GM 0x1C1C. */
    if (cl >= 0x40u || ch >= 0x20u)
        return CAPTIVE_GM_CELL_BLOCKED;             /* GM 0x1C4D: ax=1, ZF=1 */
    uint16_t v = captive_gm_wget(w, (uint16_t)(map_off + captive_gm_map_index(cl, ch)));
    if (v == 0u)
        return CAPTIVE_GM_CELL_EMPTY;               /* GM 0x1C53: ax=1, ZF=0 */
    if (v >= 0xFFCFu)
        return CAPTIVE_GM_CELL_BLOCKED;             /* GM 0x1C48 -> 0x1C4D */
    return CAPTIVE_GM_CELL_VALID;                   /* GM 0x1C4A: ax=0, ZF=1 */
}

/* ---- Pass 0x1CB5: room-outline validator + anchor placement ---- */
#define GM_MAP_TYPE 0x1048u   /* SI: cell-type map */
#define GM_MAP_SEL  0x0038u   /* DI: selector map  */

/* GM 0x1DE5: probe one outline step.  1 = continue (ZF=1), 0 = reject (ZF=0). */
static int gm_h1de5(CaptiveGmWork *w, uint8_t cl, uint8_t ch) {
    if (cl >= 0x40u || ch >= 0x20u) return 1;               /* 0x1E14 accept */
    if ((int8_t)ch <= 0) return 0;                           /* 0x1E17 reject */
    uint16_t bp = captive_gm_map_index(cl, ch);
    if (captive_gm_wget(w, (uint16_t)(GM_MAP_TYPE + bp)) != 0u) return 0;    /* 0x1DF6 */
    if (captive_gm_wget(w, (uint16_t)(GM_MAP_SEL + bp)) == 0xFFFFu) return 1;/* 0x1DFC je: continue */
    if (captive_gm_wget(w, (uint16_t)(GM_MAP_SEL + bp)) != 0u) return 0;     /* 0x1E02 */
    if (captive_gm_wget(w, 0x3506u) == 0u) return 1;         /* 0x1E08 je: continue */
    captive_gm_wset(w, (uint16_t)(GM_MAP_SEL + bp), 0xFFFFu);/* 0x1E0F */
    return 1;
}

/* GM 0x1E1B: trace the room outline; updates *cl,*ch to the final position.
 * Returns 1 accept, 0 reject.  Writes markers only when the gate (0x3506) is set. */
static int gm_h1e1b(CaptiveGmWork *w, uint8_t *pcl, uint8_t *pch) {
    uint8_t xstep = (uint8_t)(captive_gm_wget(w, 0x350Au) & 0xFFu);
    uint16_t gate = captive_gm_wget(w, 0x3506u);
    uint8_t cl = *pcl, ch = *pch;
    int ret = 0;
    if (ch == 0u) goto done;
    uint16_t bp = captive_gm_map_index(cl, ch);
    if (captive_gm_wget(w, (uint16_t)(GM_MAP_SEL + bp)) != 0u) goto done;
    if (gate != 0u) {
        captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + bp), 0x1Cu);
        captive_gm_wset(w, (uint16_t)(GM_MAP_SEL + bp), 2u);
    }
    cl = (uint8_t)(cl - xstep); if (!gm_h1de5(w, cl, ch)) goto done;
    cl = (uint8_t)(cl + xstep); ch = (uint8_t)(ch - 1u);
    if (!gm_h1de5(w, cl, ch)) goto done;
    cl = (uint8_t)(cl + xstep); if (!gm_h1de5(w, cl, ch)) goto done;
    ch = (uint8_t)(ch + 2u); if (!gm_h1de5(w, cl, ch)) goto done;
    cl = (uint8_t)(cl - xstep); if (!gm_h1de5(w, cl, ch)) goto done;
    ch = (uint8_t)(ch - 1u); cl = (uint8_t)(cl + xstep);
    if (cl >= 0x40u) goto done;
    bp = captive_gm_map_index(cl, ch);
    if (captive_gm_wget(w, (uint16_t)(GM_MAP_SEL + bp)) != 0u) goto done;
    captive_gm_wset(w, 0x33ECu, cl);
    captive_gm_wset(w, 0x33EEu, ch);
    if (gate != 0u) {
        captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + bp), 0x18u);
        captive_gm_wset(w, (uint16_t)(GM_MAP_SEL + bp), 1u);
    }
    cl = (uint8_t)(cl + xstep); if (cl >= 0x40u) goto done;
    bp = captive_gm_map_index(cl, ch);
    if (captive_gm_wget(w, (uint16_t)(GM_MAP_SEL + bp)) != 0u) goto done;
    if (gate != 0u) {
        captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + bp), 0x0Fu);
        captive_gm_wset(w, (uint16_t)(GM_MAP_SEL + bp), 0xFFFFu);
    }
    ret = 1;
done:
    *pcl = cl; *pch = ch;
    return ret;
}

/* GM 0x286E: step (cl,ch) by the region-connection vector and advance dh. */
static void gm_h286e(CaptiveGmWork *w, uint8_t *cl, uint8_t *ch, uint8_t *dh) {
    *dh = (uint8_t)(*dh + 1u);
    uint16_t bp = (uint16_t)((((uint16_t)(*dh) - 2u) & 0xFFFFu) << 1);
    *cl = (uint8_t)(*cl + w->b[(uint16_t)(0x0010u + bp)]);
    *ch = (uint8_t)(*ch + w->b[(uint16_t)(0x0018u + bp)]);
}

/* GM 0x1EC8/0x1EEA: connectivity gate.  Steps (cl,ch,dh) to the connected region
 * (0x286E) and returns nonzero to keep looping.  cl/ch/dh advance for the caller. */
static int gm_h1ec8(CaptiveGmWork *w, uint8_t *cl, uint8_t *ch, uint8_t *dh) {
    if (*dh >= captive_gm_wget(w, 0x002Eu)) return 0;   /* 0x1EEA jge: stop */
    gm_h286e(w, cl, ch, dh);                             /* steps cl,ch,dh */
    if (*dh != captive_gm_grid_cell(w, *cl, *ch)) return 0;   /* 0x1BD2: stop */
    /* matched -> GM 0x1ED0 cell check; EMPTY keeps looping. */
    if (*dh > captive_gm_wget(w, 0x002Eu) || *cl >= 0x40u || *ch >= 0x20u) return 0;
    return captive_gm_cell_check(w, GM_MAP_SEL, *cl, *ch) == CAPTIVE_GM_CELL_EMPTY;
}

/* GM 0x1D1A: build the room-outline records in the 0x34C6 scratch, then pack them
 * into the anchor array at `abx` (0x3430).  Returns 1 (accept). */
static int gm_h1d1a(CaptiveGmWork *w, uint16_t abx, uint8_t cl, uint8_t ch, uint8_t dh) {
    captive_gm_wset(w, 0x33EAu, 0u);
    captive_gm_wset(w, 0x350Au, 1u);
    if ((uint8_t)(cl & 0x0Fu) > 7u)
        captive_gm_wset(w, 0x350Au, (uint16_t)(-1));
    uint16_t bx = 0x34C6u;                       /* GM 0x1D34: bx -> scratch */
    for (int k = 0; k < 0x1E; ++k)
        captive_gm_wset(w, (uint16_t)(bx + k * 2), 0xFFFFu);
    captive_gm_wset(w, 0x3506u, 0u);

    while (captive_gm_wget(w, 0x33EAu) <= 5u) {   /* GM 0x1D4B */
        captive_gm_wset(w, bx, cl);
        captive_gm_wset(w, (uint16_t)(bx + 2), ch);
        bx = (uint16_t)(bx + 4);
        uint8_t wcl = cl, wch = ch;
        if (!gm_h1e1b(w, &wcl, &wch)) break;      /* GM 0x1D63 js -> 0x1D8E */
        captive_gm_wset(w, 0x33EAu, (uint16_t)(captive_gm_wget(w, 0x33EAu) + 1u));
        captive_gm_wset(w, bx, wcl);              /* post-walk cl,ch */
        captive_gm_wset(w, (uint16_t)(bx + 2), wch);
        captive_gm_wset(w, (uint16_t)(bx + 4), captive_gm_wget(w, 0x33ECu));
        captive_gm_wset(w, (uint16_t)(bx + 6), captive_gm_wget(w, 0x33EEu));
        bx = (uint16_t)(bx + 8);
        cl = (uint8_t)w->b[(uint16_t)(bx - 0x0Cu)];  /* reload cl0,ch0 for 0x1EC8 */
        ch = (uint8_t)w->b[(uint16_t)(bx - 0x0Au)];
        if (!gm_h1ec8(w, &cl, &ch, &dh)) break;   /* GM 0x1D8C jne; steps cl,ch,dh */
    }

    /* GM 0x1D8F..0x1DDC: pack scratch records into the anchor array. */
    if ((int16_t)captive_gm_wget(w, 0x33EAu) > 1) {
        uint16_t bp = 0x34C6u;
        int iters = (int)captive_gm_wget(w, 0x33EAu) - 1;
        captive_gm_wset(w, 0x33EAu, 0u);
        w->b[0x3507u] = 0xFFu;                    /* word[0x3506] high byte -> gate on */
        for (; iters >= 0; --iters) {
            uint8_t c_lo = w->b[bp];
            if (c_lo & 0x80u) break;              /* GM 0x1DAA js */
            uint8_t c_hi = w->b[(uint16_t)(bp + 2)];
            captive_gm_wset(w, (uint16_t)(abx + 0x32),
                            (uint16_t)((c_hi << 8) | c_lo));
            captive_gm_wset(w, (uint16_t)(abx + 0x64),
                            (uint16_t)((w->b[(uint16_t)(bp + 0xA)] << 8) | w->b[(uint16_t)(bp + 8)]));
            captive_gm_wset(w, abx,
                            (uint16_t)((w->b[(uint16_t)(bp + 6)] << 8) | w->b[(uint16_t)(bp + 4)]));
            abx = (uint16_t)(abx + 2);
            uint8_t wcl = c_lo, wch = c_hi;
            gm_h1e1b(w, &wcl, &wch);
            bp = (uint16_t)(bp + 0xC);
        }
    }
    return 1;
}

/* GM 0x1CD8: place up to min(mission/2-1,4)+1 anchors into work[0x3430]. */
static void gm_h1cd8(CaptiveGmWork *w) {
    uint16_t abx = 0x3430u;
    uint16_t cx = captive_gm_wget(w, 0x3078u);
    if (cx <= 1u) return;
    cx = (uint16_t)((cx >> 1) - 1u);
    if (cx > 4u) cx = 4u;
    for (;;) {
        for (;;) {
            uint16_t pos = captive_gm_rng_pos(w);
            uint8_t cl = (uint8_t)(pos & 0xFFu), ch = (uint8_t)(pos >> 8);
            uint8_t dh = captive_gm_grid_cell(w, cl, ch);
            if (dh > captive_gm_wget(w, 0x002Eu) || cl >= 0x40u || ch >= 0x20u)
                continue;
            if (captive_gm_cell_check(w, GM_MAP_SEL, cl, ch) != CAPTIVE_GM_CELL_EMPTY)
                continue;
            if (captive_gm_wget(w, (uint16_t)(GM_MAP_TYPE + captive_gm_map_index(cl, ch))) != 0u)
                continue;
            gm_h1d1a(w, abx, cl, ch, dh);
            if (captive_gm_wget(w, 0x33EAu) == 1u) continue;
            break;
        }
        abx = (uint16_t)(abx + 0x0Au);
        if (cx == 0u) break;
        --cx;
    }
}

void captive_gm_pass_1cb5(CaptiveGmWork *w) {
    /* The orchestrator wraps 0x1CB5 with push/pop word[0x3074] (GM 0x3C9/0x3D0),
     * so the RNG state is preserved across the pass — its internal RNG use is
     * discarded.  Mirror that here. */
    uint16_t saved_rng = captive_gm_wget(w, 0x3074u);
    for (int k = 0; k < 0x4B; ++k)
        captive_gm_wset(w, (uint16_t)(0x3430u + k * 2), 0xFFFFu);
    gm_h1cd8(w);
    uint16_t dst = captive_gm_wget(w, 0x3586u);
    for (int k = 0; k < 0x19; ++k)
        captive_gm_wset(w, (uint16_t)(dst + k * 2),
                        captive_gm_wget(w, (uint16_t)(0x3494u + k * 2)));
    captive_gm_wset(w, 0x3074u, saved_rng);
}

/* ---- Pass 0x1617 -> 0x2055: room-outline drawing into the selector map ---- */

/* GM 0x2272 (entered with a base bp): step (cl,ch) by the delta tables. */
static void gm_2272(CaptiveGmWork *w, uint8_t *cl, uint8_t *ch, uint16_t bp) {
    bp = (uint16_t)((bp + captive_gm_wget(w, 0x3088u)) & 6u);
    *cl = (uint8_t)(*cl + w->b[(uint16_t)(0x6D4Eu + bp)]);
    *ch = (uint8_t)(*ch + w->b[(uint16_t)(0x6D56u + bp)]);
}
/* GM 0x226E: step using word[0x308C] as the base. */
static void gm_226e(CaptiveGmWork *w, uint8_t *cl, uint8_t *ch) {
    gm_2272(w, cl, ch, captive_gm_wget(w, 0x308Cu));
}

/* GM 0x2681: dl = number of non-VALID (empty/blocked) neighbours of (cl,ch) in the
 * SELECTOR map, or 0 if the centre itself is not empty.  cl,ch are preserved. */
static uint8_t gm_2681(CaptiveGmWork *w, uint8_t cl, uint8_t ch) {
    if (captive_gm_cell_check(w, GM_MAP_SEL, cl, ch) != CAPTIVE_GM_CELL_EMPTY)
        return 0;                                    /* GM 0x2686 je */
    uint8_t dl = 0;
    /* GM visits W(-1,0), E(+1,0), N(0,-1), S(0,+1) relative to (cl,ch). */
    static const int8_t dxs[4] = { -1, +1, 0, 0 }, dys[4] = { 0, 0, -1, +1 };
    for (int i = 0; i < 4; ++i) {
        uint8_t x = (uint8_t)(cl + dxs[i]), y = (uint8_t)(ch + dys[i]);
        dl = (uint8_t)(dl + (captive_gm_cell_check(w, GM_MAP_SEL, x, y)
                             != CAPTIVE_GM_CELL_VALID ? 1 : 0));
    }
    return dl;
}

/* GM 0x165B: scale the draw-count `bx` for pass 0x1617. */
static uint16_t gm_165b(CaptiveGmWork *w, uint16_t bx) {
    uint16_t dx = 0;
    while (bx > 7u) { dx += 8u; bx = (uint16_t)((bx - 8u) >> 1); }
    bx += dx;                                          /* GM 0x1672 */
    uint16_t ax = captive_gm_wget(w, 0x3078u);
    for (;;) {                                         /* GM 0x167A */
        ax = (uint16_t)(ax * 0x05E5u);                 /* mul cx (only the low word is kept) */
        uint32_t s = (uint32_t)ax + 0x0029u;           /* add ax,0x29 */
        ax = (uint16_t)s;
        if (s >> 16) break;                            /* jb: CF from the ADD only (not the mul) */
        if ((ax & 0x20u) == 0u) break;                 /* test al,0x20 / je */
        bx >>= 1;
    }
    return bx;
}

/* Forward decls for 0xD12-section helpers used by gm_2055_full. */
static uint16_t gm_26ae(CaptiveGmWork *w, uint8_t cl, uint8_t ch);
static uint16_t gm_1f7e(CaptiveGmWork *w, uint8_t *cl, uint8_t *ch, uint16_t bp);
static uint16_t gm_25b5(CaptiveGmWork *w, uint8_t *cl, uint8_t *ch, uint8_t *dh,
                        uint16_t entry_bp);

/* GM 0x2055 (both modes): validate a room rectangle sized from the RNG value at
 * word[0x308A]; if valid, draw it — mode 1 (0x1617) writes the outline into the
 * selector map as 0xFFFD; mode 0 (0xD12) writes room codes into the cell-type map
 * and records the room, running the 0x25B5 post-processor.  Returns the walker bp;
 * the cursor (*pcl,*pch,*pdh) advances.  A validation abort still plain-steps. */
static uint16_t gm_2055_full(CaptiveGmWork *w, uint8_t *pcl, uint8_t *pch, uint8_t *pdh) {
    uint8_t cl = *pcl, ch = *pch, dh = *pdh;
    captive_gm_wset(w, 0x6DE6u, (uint16_t)(dh << 8));
    captive_gm_wset(w, 0x355Au, 0u);
    uint16_t a = captive_gm_wget(w, 0x308Au);
    captive_gm_wset(w, 0x308Cu, (a & 0x10u) ? 6u : 2u);
    uint16_t ax = gm_rol16(a, 2);
    ax = (uint16_t)(((ax >> 8) | (ax << 8)) & 0x3Fu);
    if (a > 0x3FF7u) ax &= 0x1Bu;
    captive_gm_wset(w, 0x3558u, 0u);
    uint16_t dx2 = gm_rol16(a, 2);
    if (dx2 > captive_gm_wget(w, 0x3560u)) captive_gm_wset(w, 0x3558u, 0x20u);
    w->b[0x3095u] = (uint8_t)ax;
    w->b[0x3094u] = 0x1Fu;
    if ((dx2 & 0x80u) && captive_gm_wget(w, 0x3078u) > 4u && (ax & 0x24u))
        w->b[0x3094u] = 0x33u;
    uint16_t dxr = (uint16_t)(gm_ror16(ax, 3) & 7u);
    uint16_t axl = (uint16_t)(ax & 7u);
    captive_gm_wset(w, 0x6DDCu, 0x6D5Eu);
    if (axl != 0u && dxr != 0u)
        captive_gm_wset(w, 0x6DDCu, (uint16_t)(0x6D5Eu + (a & 0x38u)));
    uint16_t ext_x = (uint16_t)(axl + 1u);   /* word[0x308E] */
    uint16_t ext_y = (uint16_t)(dxr + 1u);   /* word[0x3090] */
    captive_gm_wset(w, 0x308Eu, ext_x);
    captive_gm_wset(w, 0x3090u, ext_y);
    dh = w->b[0x6DE7u];
    captive_gm_wset(w, 0x3092u, captive_gm_wget(w, 0x3510u));

    /* Validation scan (GM 0x20FA..0x215A).  GM steps the cursor then restores it
     * (pop cx) before the draw, so validate on a copy and leave cl,ch original. */
    uint8_t vcl = cl, vch = ch;
    gm_2272(w, &vcl, &vch, 0);
    uint16_t mode = captive_gm_wget(w, 0x3510u);
    for (int bxa = (int)ext_y; bxa >= 0; --bxa) {
        uint8_t rcl = vcl, rch = vch;
        for (int bxb = (int)ext_x; bxb >= 0; --bxb) {
            if (mode >= 2u) {                             /* GM 0x1B21 mode>1 */
                CaptiveGmCellStatus st = captive_gm_cell_check(w, GM_MAP_SEL, rcl, rch);
                if (st == CAPTIVE_GM_CELL_BLOCKED) goto abort;   /* OOB/>=0xFFCF */
                if (st == CAPTIVE_GM_CELL_VALID)                 /* 0x1B2D */
                    captive_gm_wset(w, 0x355Au, (uint16_t)(captive_gm_wget(w, 0x355Au) + 1u));
                /* EMPTY or VALID -> place (fall through to inc 0x3092) */
            } else {
                if (mode == 1u && rch == 0u) goto abort;  /* 0x2118 */
                uint8_t dl = gm_2681(w, rcl, rch);        /* 0x2133 */
                if (captive_gm_wget(w, 0x3092u) != 0u) dl = (uint8_t)(dl - 1u);
                if (dl != 3u) goto abort;                 /* 0x2144 */
            }
            captive_gm_wset(w, 0x3092u, (uint16_t)(captive_gm_wget(w, 0x3092u) + 1u));
            gm_226e(w, &rcl, &rch);
        }
        gm_2272(w, &vcl, &vch, 0);
    }
    /* GM 0x1B5C: a mode>1 room with no VALID cell found is rejected. */
    if (mode > 1u && captive_gm_wget(w, 0x355Au) == 0u) goto abort;

    /* Draw scan (GM 0x216D..0x221B). */
    gm_2272(w, &cl, &ch, 0);
    uint8_t dcl = cl, dch = ch;
    for (int outer = (int)ext_y; outer >= 0; --outer) {
        /* GM 0x1B7A mov word[0x33CE],bp: the row counter decrements each row, so the
         * final row (word[0x33CE]==0) carves every cell (skip pattern disabled). */
        captive_gm_wset(w, 0x33CEu, (uint16_t)outer);
        uint16_t save_6de2 = captive_gm_wget(w, 0x6DE2u);
        uint16_t cd = captive_gm_wget(w, 0x6DDCu);
        captive_gm_wset(w, 0x6DDCu, (uint16_t)(cd + 1u));
        uint8_t codebyte = w->b[cd];
        captive_gm_wset(w, 0x33D0u, (uint16_t)((codebyte << 8) | codebyte));
        uint8_t rcl = dcl, rch = dch;
        for (int bxb = (int)ext_x; bxb >= 0; --bxb) {
            uint16_t bp = captive_gm_map_index(rcl, rch);
            if (mode == 0u) {                             /* 0x21C7 */
                if (bxb != 0 && captive_gm_wget(w, 0x33CEu) != 0u) {
                    uint16_t v = captive_gm_wget(w, 0x33D0u);
                    int cf = (v & 0x8000u) != 0;
                    captive_gm_wset(w, 0x33D0u, (uint16_t)((v << 1) | (cf ? 1 : 0)));
                    if (cf) {                             /* 0x21D8 */
                        captive_gm_wset(w, 0x6DE2u, (uint16_t)(captive_gm_wget(w, 0x6DE2u) + 8u));
                        captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + bp), captive_gm_wget(w, 0x3558u));
                        goto next_cell;
                    }
                }
                gm_26ae(w, rcl, rch);                     /* 0x21EA */
                captive_gm_wset(w, 0x6DE2u, (uint16_t)(captive_gm_wget(w, 0x6DE2u) + 1u));
                captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + bp), captive_gm_wget(w, 0x3094u));
                captive_gm_wset(w, 0x6DE4u, (uint16_t)((rch << 8) | rcl)); /* 0x21F9 word[0x6DE4]=cx */
            } else if (mode == 1u) {                       /* 0x21BD */
                captive_gm_wset(w, (uint16_t)(GM_MAP_SEL + bp), 0xFFFDu);
            } else {                                       /* 0x21A8 mode>1 */
                if (captive_gm_wget(w, (uint16_t)(GM_MAP_TYPE + bp)) == 0u) {
                    captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + bp), 0x23u);
                    captive_gm_wset(w, (uint16_t)(GM_MAP_SEL + bp), 0xFFC4u);
                }
            }
        next_cell:
            gm_226e(w, &rcl, &rch);
        }
        captive_gm_wset(w, 0x6DE2u, save_6de2);            /* pop word[0x6DE2] */
        captive_gm_wset(w, 0x6DE2u, (uint16_t)(captive_gm_wget(w, 0x6DE2u) + 8u));
        gm_2272(w, &dcl, &dch, 0);
    }

    /* Tail (GM 0x221E..0x2257): mode 0 records the room + runs 0x25B5. */
    if (mode == 0u) {
        captive_gm_wset(w, 0x6DE2u, (uint16_t)(captive_gm_wget(w, 0x6DE2u) + 0x48u));
        /* GM 0x1C2A mov cx,word[0x6DE4]: reload the last placed cell as the cursor. */
        uint16_t cx = captive_gm_wget(w, 0x6DE4u);
        cl = (uint8_t)cx; ch = (uint8_t)(cx >> 8);
        uint16_t bx = captive_gm_wget(w, 0x3398u);
        if (bx != 0u) {
            bx = (uint16_t)(bx - 1u);
            captive_gm_wset(w, 0x3398u, bx);
            uint16_t rec = (uint16_t)(0x3298u + (bx << 2));
            captive_gm_wset(w, rec, cx);
            w->b[(uint16_t)(rec + 2)] = dh;
            w->b[(uint16_t)(rec + 3)] = w->b[0x3095u];
        }
        /* GM 0x1C50 call 0x1C72 with bp still 0xFFFF from the draw loop's
         * word[0x33CE] underflow, so this step uses bp=(0xFFFF+word[0x3088])&6. */
        gm_2272(w, &cl, &ch, 0xFFFFu);
        *pcl = cl; *pch = ch; *pdh = dh;
        return gm_25b5(w, pcl, pch, pdh, captive_gm_wget(w, 0x3088u));
    }
    *pcl = cl; *pch = ch; *pdh = dh;
    return captive_gm_wget(w, 0x3088u);

abort:                                                    /* GM 0x2258: plain-step */
    { uint16_t bp = captive_gm_wget(w, 0x3088u);
      *pcl = cl; *pch = ch; *pdh = dh;
      return gm_1f7e(w, pcl, pch, bp); }
}

/* mode-1 wrapper for pass 0x1617 (kept for its by-value call). */
static void gm_2055(CaptiveGmWork *w, uint8_t cl, uint8_t ch) {
    uint8_t dh = 0u;
    gm_2055_full(w, &cl, &ch, &dh);
}

/* GM 0x1632: draw bx+1 rooms from random cells (current word[0x3510] mode), wrapping
 * the loop with push/pop word[0x3074] so its RNG use is discarded. */
static void gm_1632_loop(CaptiveGmWork *w, uint16_t bx) {
    uint16_t saved_rng = captive_gm_wget(w, 0x3074u);
    for (;;) {
        uint16_t pos = captive_gm_rng_pos(w);          /* GM 0x1C97 */
        captive_gm_wset(w, 0x308Au, captive_gm_wget(w, 0x3074u));
        gm_2055(w, (uint8_t)(pos & 0xFFu), (uint8_t)(pos >> 8));
        if (bx == 0u) break;
        --bx;
    }
    captive_gm_wset(w, 0x3074u, saved_rng);
}

void captive_gm_pass_1617(CaptiveGmWork *w) {
    captive_gm_wset(w, 0x3510u, 1u);
    uint16_t bx = captive_gm_wget(w, 0x3078u);         /* mission */
    if (bx != 0u) {
        if ((bx & 2u) == 0u) return;                   /* GM 0x1625 */
        bx = (uint16_t)(~bx);
        bx = gm_165b(w, bx);
    }
    gm_1632_loop(w, bx);
}

/* GM 0x164C: like 0x1617 but mode 2 — draws 0x2055 "fill" rooms (type 0x23 /
 * selector 0xFFC4) from gm_165b(mission)+1 random cells, RNG discarded. */
void captive_gm_pass_164c(CaptiveGmWork *w) {
    captive_gm_wset(w, 0x3510u, 2u);
    gm_1632_loop(w, gm_165b(w, captive_gm_wget(w, 0x3078u)));
}

/* ==== Pass 0xD12: drunkard's-walk room/corridor placement machine ====
 * Full transcription of GM.EXE 0xD12 + its sub-routine tree, verified byte-exact
 * against the real GM code: after 0xD12 the cell-type map (0x1048), selector map
 * (0x38), and aux map (0x2058) are byte-identical to GM for missions 1/2/3, and
 * both RNG states (word[0x3074]/word[0x355C]) match (map1048 m1 550/0x42D6,
 * m2 280/0x17FE, m3 191/0x10DE — see test_pass_d12). */

static uint16_t gm_1c57(CaptiveGmWork *w) {              /* GM 0x1C57 secondary RNG */
    uint16_t st = captive_gm_wget(w, 0x355Cu);
    st = (uint16_t)(st * 0x05E5u + 0x0029u);
    captive_gm_wset(w, 0x355Cu, st);
    return (uint16_t)((st >> 3) | (st << 13));
}
static void gm_2854(CaptiveGmWork *w, uint8_t *cl, uint8_t *ch, uint8_t *dh) {
    uint16_t bp = (uint16_t)((((uint16_t)*dh - 2u) & 0xFFFFu) << 1);   /* orig dh */
    *cl = (uint8_t)(*cl - w->b[(uint16_t)(0x10u + bp)]);
    *ch = (uint8_t)(*ch - w->b[(uint16_t)(0x18u + bp)]);
    *dh = (uint8_t)(*dh - 1u);
}
static void gm_25a8(CaptiveGmWork *w, uint8_t *cl, uint8_t *ch, uint8_t *dh) {
    *cl = w->b[0x30u]; *ch = w->b[0x32u]; *dh = w->b[0x34u];
}
static int gm_1bd2(CaptiveGmWork *w, uint8_t cl, uint8_t ch, uint8_t dh) {
    return dh == captive_gm_grid_cell(w, cl, ch);
}
static uint16_t gm_26ae(CaptiveGmWork *w, uint8_t cl, uint8_t ch) {  /* GM 0x26AE */
    uint16_t bp = captive_gm_map_index(cl, ch);
    captive_gm_wset(w, (uint16_t)(GM_MAP_SEL + bp), captive_gm_wget(w, 0x6DE2u));
    captive_gm_wset(w, 0x6DE2u, (uint16_t)(captive_gm_wget(w, 0x6DE2u) + 1u));
    return bp;
}

/* Forward decls (defined further down). */
static uint16_t gm_25b5(CaptiveGmWork *w, uint8_t *cl, uint8_t *ch, uint8_t *dh,
                        uint16_t entry_bp);
static uint16_t gm_2055_mode(CaptiveGmWork *w, uint8_t *cl, uint8_t *ch, uint8_t *dh);
static uint16_t gm_24a9(CaptiveGmWork *w, uint8_t *cl, uint8_t *ch, uint8_t *dh);

/* GM 0x2688..0x26A8: count of non-VALID (empty/blocked) selector neighbours of
 * (cl,ch) — the 0x2681 body without the centre-empty gate. */
static uint8_t gm_neighbor_sum(CaptiveGmWork *w, uint8_t cl, uint8_t ch) {
    static const int8_t dxs[4] = { -1, +1, 0, 0 }, dys[4] = { 0, 0, -1, +1 };
    uint8_t dl = 0;
    for (int i = 0; i < 4; ++i) {
        uint8_t x = (uint8_t)(cl + dxs[i]), y = (uint8_t)(ch + dys[i]);
        dl = (uint8_t)(dl + (captive_gm_cell_check(w, GM_MAP_SEL, x, y)
                             != CAPTIVE_GM_CELL_VALID ? 1 : 0));
    }
    return dl;
}

/* GM 0x1FC5..0x2054: carve a cell (or retry via 0x1F9D).  Returns bp (0xFFFF when a
 * cell was carved).  On carve, the cursor is restored to the pre-step position. */
static uint16_t gm_carve(CaptiveGmWork *w, uint8_t *pcl, uint8_t *pch, uint8_t *pdh) {
    uint8_t cl = *pcl, ch = *pch, dh = *pdh;
    if (cl > 0x3Fu || ch > 0x1Fu) goto retry;
    captive_gm_wset(w, 0x307Eu, (uint16_t)(captive_gm_wget(w, 0x307Eu) + 1u));
    captive_gm_wset(w, 0x3080u, (uint16_t)(captive_gm_wget(w, 0x3080u) - 1u));
    if (!gm_1bd2(w, cl, ch, dh)) goto retry;
    if (gm_2681(w, cl, ch) != 4u) goto retry;
    {
        /* Stamp the STEPPED cell (0x19E4..0x1A05). */
        uint16_t bp = gm_26ae(w, cl, ch);
        uint16_t ax = ((captive_gm_wget(w, 0x3084u) & 0xFFu) == 4u) ? 5u : 4u;
        uint8_t cur = w->b[(uint16_t)(GM_MAP_TYPE + bp)];
        if (cur == 0x1Fu || cur == 0x33u) ax = (uint16_t)(ax + 0x31u);
        captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + bp), ax);
        /* GM 0x1A08 push cx,dx (the stepped cursor). */
        uint8_t scl = cl, sch = ch, sdh = dh;
        /* GM 0x1A0A gm_25a8: restore the ORIGINAL cursor for the second stamp. */
        uint8_t rcl, rch, rdh;
        gm_25a8(w, &rcl, &rch, &rdh);
        uint16_t bp2 = captive_gm_map_index(rcl, rch);
        uint8_t al = w->b[(uint16_t)(GM_MAP_TYPE + bp2)];
        uint16_t code;
        if (al == 4u || al == 5u) code = 6u;
        else if (al == 0x35u || al == 0x36u) code = 0x37u;
        else if (al == 0x1Fu || al == 0x33u) code = (uint16_t)(captive_gm_wget(w, 0x3084u) + 0x31u);
        else code = captive_gm_wget(w, 0x3084u);
        captive_gm_wset(w, 0x3084u, code);
        captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + bp2), code);
        /* GM 0x1A48 pop dx,cx: back to the STEPPED cursor. */
        captive_gm_wset(w, 0x3552u, (uint16_t)(captive_gm_wget(w, 0x3552u) + 1u));
        /* GM 0x204E call 0x25B5 with the STEPPED cursor (may relocate it to a
         * junction), then 0x2051 or bp,0xFFFF. */
        gm_25b5(w, &scl, &sch, &sdh, 0u);
        *pcl = scl; *pch = sch; *pdh = sdh;
        return 0xFFFFu;
    }
retry:
    gm_25a8(w, &cl, &ch, &dh);
    {
        uint16_t bp = captive_gm_wget(w, 0x3088u);
        cl = (uint8_t)(cl + w->b[(uint16_t)(0x6D4Eu + bp)]);
        ch = (uint8_t)(ch + w->b[(uint16_t)(0x6D56u + bp)]);
        captive_gm_wset(w, 0x3088u, bp);
        *pcl = cl; *pch = ch; *pdh = dh;
        return bp;
    }
}

/* GM 0x1F00: walker entry (ax&3 -> 0x1F95/0x1F8D/plain step; no carve). */
static uint16_t gm_1f00(CaptiveGmWork *w, uint8_t *pcl, uint8_t *pch, uint8_t *pdh, uint16_t ax) {
    captive_gm_wset(w, 0x30u, *pcl); captive_gm_wset(w, 0x32u, *pch); captive_gm_wset(w, 0x34u, *pdh);
    uint16_t bp = captive_gm_wget(w, 0x3088u);
    uint16_t a = ax & 3u;
    if (a == 0u) bp = (uint16_t)((bp - 2u) & 6u);
    else if (a == 1u) bp = (uint16_t)((bp + 2u) & 6u);
    *pcl = (uint8_t)(*pcl + w->b[(uint16_t)(0x6D4Eu + bp)]);
    *pch = (uint8_t)(*pch + w->b[(uint16_t)(0x6D56u + bp)]);
    captive_gm_wset(w, 0x3088u, bp);
    return bp;
}

/* GM 0x1F29: full walker/dispatcher (ax&7 + thresholds + carve entries). */
static uint16_t gm_1f29(CaptiveGmWork *w, uint8_t *pcl, uint8_t *pch, uint8_t *pdh, uint16_t ax) {
    captive_gm_wset(w, 0x30u, *pcl); captive_gm_wset(w, 0x32u, *pch); captive_gm_wset(w, 0x34u, *pdh);
    uint16_t bp = captive_gm_wget(w, 0x3088u);
    uint16_t a = ax & 7u;
    if (a == 0u) { bp = (uint16_t)((bp - 2u) & 6u); goto plain; }
    if (a == 1u) { bp = (uint16_t)((bp + 2u) & 6u); goto plain; }
    if (captive_gm_wget(w, 0x308Au) > 0xCCB0u && captive_gm_wget(w, 0x6DE2u) > 4u) {
        if (a == 2u) {  /* 0x1FB6 */
            if (*pdh >= (uint8_t)captive_gm_wget(w, 0x002Eu)) goto plain;
            captive_gm_wset(w, 0x3084u, 4u); gm_h286e(w, pcl, pch, pdh);
            return gm_carve(w, pcl, pch, pdh);
        }
        if (a == 3u) {  /* 0x1FA6 */
            if ((int8_t)*pdh <= 1) goto plain;
            captive_gm_wset(w, 0x3084u, 5u); gm_2854(w, pcl, pch, pdh);
            return gm_carve(w, pcl, pch, pdh);
        }
    }
    {   /* threshold 0x1F64 */
        uint16_t v = captive_gm_wget(w, 0x308Au);
        if (v <= 0x07ADu) return gm_24a9(w, pcl, pch, pdh);
        if (v <= 0x8F48u) return gm_2055_mode(w, pcl, pch, pdh);
    }
plain:
    *pcl = (uint8_t)(*pcl + w->b[(uint16_t)(0x6D4Eu + bp)]);
    *pch = (uint8_t)(*pch + w->b[(uint16_t)(0x6D56u + bp)]);
    captive_gm_wset(w, 0x3088u, bp);
    return bp;
}

/* Placeholder bodies for the remaining routines — filled in as each is transcribed
 * and harness-verified.  Until then they no-op so the file compiles; captive_gm_pass_d12
 * is NOT wired into the test suite while these are incomplete. */
/* GM 0x25F6: scan a room record (5 entries at `bx`) for a junction (a cell whose
 * selector-neighbour count is 2); on finding one, stamp it (0xF) and its partners,
 * then re-scan for count-3 cells (junction arms), recording the junction position
 * at work[0x33E6]/[0x33E8] and clearing the abort flag at work[0x33E7]. */
static void gm_25f6(CaptiveGmWork *w, uint16_t bx) {
    captive_gm_wset(w, 0x3502u, bx);
    for (int dhl = 4; dhl >= 0; --dhl) {                 /* first loop 0x25FD */
        bx = (uint16_t)(bx + 2);
        if (w->b[(uint16_t)(bx - 1)] & 0x80u) continue;  /* 0x2600 */
        uint16_t cx = captive_gm_wget(w, (uint16_t)(bx - 2));
        if (gm_neighbor_sum(w, (uint8_t)cx, (uint8_t)(cx >> 8)) != 2u) continue;
        /* 0x2614: junction found. */
        uint16_t bp = gm_26ae(w, (uint8_t)cx, (uint8_t)(cx >> 8));
        captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + bp), 0x0Fu);
        captive_gm_wset(w, (uint16_t)(bx - 2), 0xFFFFu);
        uint16_t cx2 = captive_gm_wget(w, (uint16_t)(bx + 0x62));
        gm_26ae(w, (uint8_t)cx2, (uint8_t)(cx2 >> 8));
        uint16_t cx3 = captive_gm_wget(w, (uint16_t)(bx + 0x30));
        uint16_t bp3 = gm_26ae(w, (uint8_t)cx3, (uint8_t)(cx3 >> 8));
        captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + bp3), 0x1Du);
        /* 0x2632: second loop over the same record for count-3 arms. */
        uint16_t bx2 = captive_gm_wget(w, 0x3502u);
        for (int dhl2 = 4; dhl2 >= 0; --dhl2) {
            bx2 = (uint16_t)(bx2 + 2);
            if (w->b[(uint16_t)(bx2 - 1)] & 0x80u) continue;
            uint16_t cxa = captive_gm_wget(w, (uint16_t)(bx2 - 2));
            if (gm_neighbor_sum(w, (uint8_t)cxa, (uint8_t)(cxa >> 8)) == 3u) {
                uint16_t bpa = gm_26ae(w, (uint8_t)cxa, (uint8_t)(cxa >> 8));
                w->b[0x33E7u] = 0u; w->b[0x33E9u] = 0u;
                w->b[0x33E6u] = (uint8_t)cxa; w->b[0x33E8u] = (uint8_t)(cxa >> 8);
                captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + bpa), 0x0Fu);
            }
            captive_gm_wset(w, (uint16_t)(bx2 - 2), 0xFFFFu);
        }
        return;
    }
}

/* GM 0x25B5: junction post-processor over the 5 room records (0x3430, stride 0xA).
 * If any junction is found (abort flag cleared), relocate the cursor to it and
 * return 0xFFFF; otherwise leave the cursor and return `entry_bp`. */
static uint16_t gm_25b5(CaptiveGmWork *w, uint8_t *pcl, uint8_t *pch, uint8_t *pdh,
                        uint16_t entry_bp) {
    w->b[0x33E7u] = 0xFFu;
    uint16_t bx = 0x3430u;
    for (int k = 4; k >= 0; --k) { gm_25f6(w, bx); bx = (uint16_t)(bx + 0xAu); }
    if (w->b[0x33E7u] & 0x80u) return entry_bp;          /* 0x25F0 abort */
    uint8_t cl = w->b[0x33E6u], ch = w->b[0x33E8u];
    *pcl = cl; *pch = ch; *pdh = captive_gm_grid_cell(w, cl, ch);  /* 0x2831 */
    return 0xFFFFu;
}

static uint16_t gm_2055_mode(CaptiveGmWork *w, uint8_t *cl, uint8_t *ch, uint8_t *dh) {
    return gm_2055_full(w, cl, ch, dh);
}
/* GM 0x1F7E: plain step by `bp`. */
static uint16_t gm_1f7e(CaptiveGmWork *w, uint8_t *cl, uint8_t *ch, uint16_t bp) {
    *cl = (uint8_t)(*cl + w->b[(uint16_t)(0x6D4Eu + bp)]);
    *ch = (uint8_t)(*ch + w->b[(uint16_t)(0x6D56u + bp)]);
    captive_gm_wset(w, 0x3088u, bp);
    return bp;
}

/* GM 0x255D: mark cells whose selector-neighbour count is not 0/4 with 0xFFFE. */
static void gm_255d(CaptiveGmWork *w) {
    for (int cl = 0x3F; cl >= 0; --cl)
        for (int ch = 0x1F; ch >= 0; --ch) {
            uint8_t dl = gm_2681(w, (uint8_t)cl, (uint8_t)ch);
            if (dl == 0u || dl == 4u) continue;
            uint16_t off = (uint16_t)(GM_MAP_SEL + captive_gm_map_index((uint8_t)cl, (uint8_t)ch));
            if (captive_gm_wget(w, off) == 0u)
                captive_gm_wset(w, off, 0xFFFEu);
        }
}

/* GM 0x2590: clear the 0xFFFE markers in the selector map back to 0. */
static void gm_2590(CaptiveGmWork *w) {
    for (int i = 0; i < 0x800; ++i) {
        uint16_t off = (uint16_t)(GM_MAP_SEL + i * 2);
        if (captive_gm_wget(w, off) == 0xFFFEu) captive_gm_wset(w, off, 0u);
    }
}

/* GM 0x2589: clear the 0xFFFD room-outline markers (written by 0x1617 mode 1) in the
 * selector map back to 0.  Shares GM's 0x2595 clear loop with 0x2590 (di=0x38). */
void captive_gm_pass_2589(CaptiveGmWork *w) {
    for (int i = 0; i < 0x800; ++i) {
        uint16_t off = (uint16_t)(GM_MAP_SEL + i * 2);
        if (captive_gm_wget(w, off) == 0xFFFDu) captive_gm_wset(w, off, 0u);
    }
}

/* GM 0x250D: place up to 0x31 satellite feature cells around the current cursor via
 * the 0x1F00 walker. */
static void gm_250d(CaptiveGmWork *w, uint8_t *pcl, uint8_t *pch, uint8_t *pdh) {
    int bx = 0x31;
    uint16_t ax = captive_gm_rng_next(w);               /* 0x2511 */
    for (;;) {
        gm_1f00(w, pcl, pch, pdh, ax);                  /* 0x2514 */
        int placed_or_full;
        if (*pcl >= 0x40u || *pch >= 0x20u) placed_or_full = 0;   /* -> 0x253E retry */
        else {
            uint16_t off = (uint16_t)(GM_MAP_SEL + captive_gm_map_index(*pcl, *pch));
            uint16_t v = captive_gm_wget(w, off);
            if (v >= 0xFFFDu) placed_or_full = 0;        /* 0x2524 jae -> retry */
            else if (v != 0u) placed_or_full = 1;        /* 0x252E jne -> 0x2556 */
            else {                                       /* 0x2530 place */
                uint16_t bp = gm_26ae(w, *pcl, *pch);
                captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + bp), captive_gm_wget(w, 0x355Eu));
                gm_25b5(w, pcl, pch, pdh, 0u);
                placed_or_full = 1;                      /* -> 0x2556 */
            }
        }
        if (!placed_or_full) {                           /* 0x253E */
            gm_25a8(w, pcl, pch, pdh);
            ax = (uint16_t)(gm_ror16(captive_gm_rng_next(w), 5) & 1u);
            if (--bx < 0) break;
            continue;                                    /* jns 0x2514 */
        }
        if (--bx < 0) break;                             /* 0x2556 */
        ax = captive_gm_rng_next(w);                     /* back to 0x2511 */
    }
}

/* GM 0x24A9: feature/door placer (code 0x11 or 0x14). */
static uint16_t gm_24a9(CaptiveGmWork *w, uint8_t *pcl, uint8_t *pch, uint8_t *pdh) {
    uint16_t bp = gm_1f7e(w, pcl, pch, captive_gm_wget(w, 0x3088u));  /* 0x24A9 */
    if (gm_2681(w, *pcl, *pch) != 3u)                    /* 0x24AC */
        return gm_25b5(w, pcl, pch, pdh, bp);            /* 0x24B4 jmp 0x25B5 */
    captive_gm_wset(w, 0x355Eu, 0x11u);                  /* 0x24B7 */
    uint16_t r = captive_gm_rng_next(w);
    r = (uint16_t)((r >> 8) | (r << 8));                 /* xchg ah,al */
    if (r < 0x2711u && captive_gm_wget(w, 0x3078u) != 0u)
        captive_gm_wset(w, 0x355Eu, 0x14u);              /* 0x24CE */
    gm_255d(w);                                          /* 0x24D4 */
    gm_25a8(w, pcl, pch, pdh);
    uint16_t bpm = captive_gm_map_index(*pcl, *pch);
    uint16_t saved = captive_gm_wget(w, (uint16_t)(GM_MAP_SEL + bpm));  /* push */
    captive_gm_wset(w, (uint16_t)(GM_MAP_SEL + bpm), 0xFFFEu);
    uint8_t scl = *pcl, sch = *pch, sdh = *pdh;          /* push cx,dx */
    gm_1f7e(w, pcl, pch, captive_gm_wget(w, 0x3088u));   /* 0x24EB */
    uint16_t bp2 = gm_26ae(w, *pcl, *pch);               /* 0x24EE/0x24F1 */
    captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + bp2), captive_gm_wget(w, 0x355Eu));
    gm_250d(w, pcl, pch, pdh);                           /* 0x24FA */
    *pcl = scl; *pch = sch; *pdh = sdh;                  /* pop cx,dx */
    captive_gm_wset(w, (uint16_t)(GM_MAP_SEL + captive_gm_map_index(*pcl, *pch)), saved); /* pop */
    gm_2590(w);                                          /* 0x2505 */
    return gm_25b5(w, pcl, pch, pdh, 0u);                /* 0x2508 xor bp; jmp 0x25B5 */
}

/* GM 0xD12: the placement-machine driver.  Walks the dispatcher, carving rooms and
 * corridors into the cell-type map, budgeted by word[0x307E]/word[0x3080] and
 * word[0x6DE2], restarting from random cells when a walk stalls. */
void captive_gm_pass_d12(CaptiveGmWork *w) {
    captive_gm_wset(w, 0x3510u, 0u);
    captive_gm_wset(w, 0x355Cu, 0u);
    uint8_t cl = (uint8_t)captive_gm_wget(w, 0x0020u);
    uint8_t ch = 0u;
    uint8_t dh = w->b[0x0022u];
    captive_gm_wset(w, 0x6DE2u, 1u);
    captive_gm_wset(w, 0x307Eu, 0x012Cu);
    captive_gm_wset(w, 0x3080u, 0x0384u);
    captive_gm_wset(w, 0x3088u, (w->b[0x33DCu] & 1u) ? 0u : 2u);
    captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + gm_26ae(w, cl, ch)), 0x0Fu);
    uint16_t ax = 0u, bp = 0u; int bx = 0;

d52:
    gm_26ae(w, cl, ch);
d55:
    bx = 9;
    ax = captive_gm_rng_next(w); captive_gm_wset(w, 0x308Au, ax);
d5e:
    if (captive_gm_wget(w, 0x6DE2u) >= 0x3001u) return;
    bp = gm_1f29(w, &cl, &ch, &dh, ax);
    if (bp == 0xFFFFu) goto d55;                        /* carved -> new batch */
    if (gm_2681(w, cl, ch) == 3u) goto d52;
    gm_25a8(w, &cl, &ch, &dh);
    ax = (uint16_t)(captive_gm_rng_next(w) & 3u);
    if (--bx >= 0) goto d5e;
d85:
    bx = 0x63;
d88:
    { uint16_t pos = captive_gm_rng_pos(w); cl = (uint8_t)pos; ch = (uint8_t)(pos >> 8); }
    if (captive_gm_wget(w, 0x6DE2u) >= 0x3001u) return;
    if (gm_1c57(w) <= 0xA200u) goto df0;
    if (captive_gm_cell_check(w, GM_MAP_SEL, cl, ch) != CAPTIVE_GM_CELL_VALID) goto df0;
    if (captive_gm_wget(w, 0x3080u) & 0x8000u) return;
    {
        uint8_t v = w->b[(uint16_t)(GM_MAP_TYPE + captive_gm_map_index(cl, ch))];
        if (v == 0x1Fu || v == 0x35u || v == 0x36u || v == 4u || v == 5u ||
            v == 0x33u || v == 0x11u) {                 /* 0xDD4 */
            ax = (w->b[0x308Au] & 0x10u) ? 3u : 2u;
            dh = captive_gm_grid_cell(w, cl, ch);
            w->b[0x308Bu] = 0xFFu;
            bx = 0;
            captive_gm_wset(w, 0x307Eu, (uint16_t)(captive_gm_wget(w, 0x307Eu) - 1u));
            goto d5e;
        }
    }
df0:
    {
        if (gm_2681(w, cl, ch) != 3u) goto e05;
        dh = captive_gm_grid_cell(w, cl, ch);
        uint16_t v = (uint16_t)(captive_gm_wget(w, 0x307Eu) - 1u);
        captive_gm_wset(w, 0x307Eu, v);
        if (v & 0x8000u) return;                        /* js */
        goto d52;
    }
e05:
    if (--bx >= 0) goto d88;
    {
        uint16_t v = (uint16_t)(captive_gm_wget(w, 0x307Eu) - 1u);
        captive_gm_wset(w, 0x307Eu, v);
        if (v & 0x8000u) return;
        goto d85;
    }
}

/* ==== Pass 0x26BE: per-cell wall / flow-direction flag computation ====
 * For every cell with a valid selector, encode into the high byte of the aux map
 * (0x2058) the direction (0..5) toward the neighbouring cell with the smallest
 * selector value (the "downhill" flow used by the renderer), and fill still-empty
 * cell-type cells with 7.  Then stamp the entry cell and process the 5 door-record
 * lists at 0x3462 into door codes (0x1A/0x1B). */

#define GM_MAP_AUX 0x2058u

/* GM 0x280C: if (cl,ch) is an in-bounds valid selector cell whose value <= *bx,
 * lower the running minimum *bx to it and report a hit. */
static int gm_280c(CaptiveGmWork *w, uint8_t cl, uint8_t ch, uint16_t *bx) {
    if (cl >= 0x40u || ch >= 0x20u) return 0;
    uint16_t bp = captive_gm_map_index(cl, ch);
    uint16_t ax = captive_gm_wget(w, (uint16_t)(GM_MAP_SEL + bp));
    if (ax == 0u || ax == 0xFFFFu) return 0;
    if (ax > *bx) return 0;
    *bx = ax;
    return 1;
}

/* GM 0x27BB: step forward by the connection vector; hit -> dir 4. */
static void gm_27bb(CaptiveGmWork *w, uint8_t cl, uint8_t ch, uint8_t *dl, uint16_t *bx) {
    uint8_t dh = captive_gm_grid_cell(w, cl, ch);        /* 0x2831 */
    gm_h286e(w, &cl, &ch, &dh);                          /* 0x286E */
    if (gm_280c(w, cl, ch, bx)) *dl = 4u;
}

/* GM 0x27CE: step back by the connection vector; hit -> dir 5. */
static void gm_27ce(CaptiveGmWork *w, uint8_t cl, uint8_t ch, uint8_t *dl, uint16_t *bx) {
    uint8_t dh = captive_gm_grid_cell(w, cl, ch);        /* 0x2831 */
    gm_2854(w, &cl, &ch, &dh);                           /* 0x2854 */
    if (gm_280c(w, cl, ch, bx)) *dl = 5u;
}

/* GM 0x27E1: check the 4 orthogonal neighbours (N,S,E,W); the last one that lowers
 * the running minimum sets the flow direction (N=0, S=2, E=3, W=1). */
static void gm_27e1(CaptiveGmWork *w, uint8_t cl, uint8_t ch, uint8_t *dl, uint16_t *bx) {
    if (gm_280c(w, cl, (uint8_t)(ch - 1u), bx)) *dl = 0u;
    if (gm_280c(w, cl, (uint8_t)(ch + 1u), bx)) *dl = 2u;
    if (gm_280c(w, (uint8_t)(cl + 1u), ch, bx)) *dl = 3u;
    if (gm_280c(w, (uint8_t)(cl - 1u), ch, bx)) *dl = 1u;
}

/* GM 0x2774: process one 5-entry door-record list (each entry a packed cell or a
 * negative sentinel), stamping door codes (0x1A/0x1B) and aux flags. */
static void gm_2774(CaptiveGmWork *w, uint16_t bx) {
    uint8_t dl = 4u, dh = 0x1Au;
    for (int i = 4; i >= 0; --i) {
        uint16_t v = captive_gm_wget(w, bx);
        bx = (uint16_t)(bx + 2u);
        if (v & 0x8000u) continue;                       /* 0x2781 js: skip */
        uint8_t cl = (uint8_t)v, ch = (uint8_t)(v >> 8);
        uint16_t bp = captive_gm_map_index(cl, ch);
        if (w->b[(uint16_t)(GM_MAP_TYPE + bp)] == 0x1Du) {           /* 0x2789 */
            uint8_t al = 3u;
            if (cl != 0u &&
                captive_gm_wget(w, (uint16_t)(GM_MAP_SEL + bp - 2u)) != 0xFFFFu)
                al = 1u;
            w->b[(uint16_t)(GM_MAP_AUX + bp + 1u)] = al;
            dl = 5u; dh = 0x1Bu;
        } else {
            w->b[(uint16_t)(GM_MAP_AUX + bp + 1u)] = dl;
            w->b[(uint16_t)(GM_MAP_TYPE + bp)] = dh;
            w->b[(uint16_t)(GM_MAP_TYPE + bp + 1u)] = 0u;
        }
    }
}

void captive_gm_pass_26be(CaptiveGmWork *w) {
    for (int ch = 0; ch < 0x20; ++ch)
        for (int cl = 0; cl < 0x40; ++cl) {
            uint16_t bp = captive_gm_map_index((uint8_t)cl, (uint8_t)ch);
            uint16_t sel = captive_gm_wget(w, (uint16_t)(GM_MAP_SEL + bp));
            if (sel == 0u || sel == 0xFFFFu) continue;   /* 0x26D0/0x26D5 */
            uint16_t bx = 0x4000u;
            uint8_t dl = 0xFEu;
            uint8_t al = w->b[(uint16_t)(GM_MAP_TYPE + bp)];
            int bb = 0, ce = 0, e1 = 0;
            if (al == 0x1Cu)                     { bb = 1; ce = 1; }
            else if (al == 0x37u || al == 6u)    { bb = 1; ce = 1; e1 = 1; }
            else if (al == 0x1Au)                { bb = 1; }
            else if (al == 0x35u || al == 4u)    { bb = 1; e1 = 1; }
            else if (al == 0x1Bu)                { ce = 1; }
            else if (al == 0x36u || al == 5u)    { ce = 1; e1 = 1; }
            else                                 { e1 = 1; }
            if (bb) gm_27bb(w, (uint8_t)cl, (uint8_t)ch, &dl, &bx);
            if (ce) gm_27ce(w, (uint8_t)cl, (uint8_t)ch, &dl, &bx);
            if (e1) gm_27e1(w, (uint8_t)cl, (uint8_t)ch, &dl, &bx);
            w->b[(uint16_t)(GM_MAP_AUX + bp + 1u)] = dl;              /* 0x272B */
            if (w->b[(uint16_t)(GM_MAP_TYPE + bp)] == 0u)            /* 0x2730 */
                w->b[(uint16_t)(GM_MAP_TYPE + bp)] = 7u;
        }
    /* Entry cell + the 5 door-record lists (GM 0x274B). */
    {
        uint8_t cl = w->b[0x0020u];
        uint16_t bp = captive_gm_map_index(cl, 0u);
        w->b[(uint16_t)(GM_MAP_TYPE + bp + 1u)] = 0x7Eu;            /* 0x2754 */
        captive_gm_wset(w, (uint16_t)(GM_MAP_AUX + bp), 0u);        /* 0x2759 */
    }
    for (uint16_t rec = 0x3462u; rec != (uint16_t)(0x3462u + 5u * 0x0Au); rec += 0x0Au)
        gm_2774(w, rec);
}

/* GM 0x2675: like 0x2681 but gated on the centre being a VALID selector cell (not
 * empty) — dl = number of non-VALID (empty/blocked) selector neighbours, else 0. */
static uint8_t gm_2675(CaptiveGmWork *w, uint8_t cl, uint8_t ch) {
    if (captive_gm_cell_check(w, GM_MAP_SEL, cl, ch) != CAPTIVE_GM_CELL_VALID)
        return 0;
    uint8_t dl = 0;
    static const int8_t dxs[4] = { -1, +1, 0, 0 }, dys[4] = { 0, 0, -1, +1 };
    for (int i = 0; i < 4; ++i) {
        uint8_t x = (uint8_t)(cl + dxs[i]), y = (uint8_t)(ch + dys[i]);
        dl = (uint8_t)(dl + (captive_gm_cell_check(w, GM_MAP_SEL, x, y)
                             != CAPTIVE_GM_CELL_VALID ? 1 : 0));
    }
    return dl;
}

/* ==== Pass 0x28B2: dead-end (3-wall) spur marker ====
 * Scan every cell (ch high->low, cl high->low); a VALID cell with exactly 3
 * non-VALID selector neighbours, that is not the entry cell and whose type is not a
 * special code, is marked 0x7F and appended (as a packed cell) to the list at
 * 0x3098 (count in word[0x3096]), up to 0xFE cells. */
void captive_gm_pass_28b2(CaptiveGmWork *w) {
    for (int i = 0; i < 0x100; ++i) captive_gm_wset(w, (uint16_t)(0x3098u + i * 2), 0u);
    captive_gm_wset(w, 0x3096u, 0u);
    uint16_t bx = 0x3098u;
    int limit = 0xFE;
    for (int ch = 0x1F; ch >= 0; --ch)
        for (int cl = 0x3F; cl >= 0; --cl) {
            if (limit == 0) continue;                        /* 0x28D1 */
            if (gm_2675(w, (uint8_t)cl, (uint8_t)ch) != 3u) continue;   /* 0x28D9 */
            uint16_t bp = captive_gm_map_index((uint8_t)cl, (uint8_t)ch);
            if (ch == 0 && (uint8_t)cl == w->b[0x0020u]) continue;      /* 0x28E4 entry */
            uint8_t al = w->b[(uint16_t)(GM_MAP_TYPE + bp)];
            if (al == 6u || al == 0x37u || al == 0x1Cu || al == 0x1Du ||
                al == 0x1Au || al == 0x1Bu || al == 5u || al == 4u ||
                al == 0x36u || al == 0x35u || al == 0x0Fu || al == 0x21u ||
                al == 0x18u) continue;                       /* 0x28F1..0x2923 */
            captive_gm_wset(w, bx, (uint16_t)(((uint16_t)ch << 8) | (uint16_t)cl));
            bx = (uint16_t)(bx + 2u);
            captive_gm_wset(w, 0x3096u, (uint16_t)(captive_gm_wget(w, 0x3096u) + 1u));
            w->b[(uint16_t)(GM_MAP_TYPE + bp)] = 0x7Fu;
            --limit;
        }
}

/* GM 0x2A59: move (cl,ch) one step along its flow direction (the aux-map high byte
 * set by 0x26BE): 4 = forward connection vector, 5 = back, else the cardinal step
 * from the 0x6AE4/0x6AEC tables. */
/* Returns aux_high<<1 for a cardinal step, or 0xFFFF for a forward/back relocation
 * (GM 0x2A6B shl ax,1 vs 0x2A8F/0x2A99 or ax,0xFFFF). */
static uint16_t gm_2a59(CaptiveGmWork *w, uint8_t *pcl, uint8_t *pch) {
    uint16_t bp = captive_gm_map_index(*pcl, *pch);
    uint8_t al = w->b[(uint16_t)(GM_MAP_AUX + bp + 1u)];
    if (al == 4u) { uint8_t dh = captive_gm_grid_cell(w, *pcl, *pch); gm_h286e(w, pcl, pch, &dh); return 0xFFFFu; }
    if (al == 5u) { uint8_t dh = captive_gm_grid_cell(w, *pcl, *pch); gm_2854(w, pcl, pch, &dh); return 0xFFFFu; }
    uint16_t bx = (uint16_t)(al << 1);
    *pcl = (uint8_t)(*pcl + w->b[(uint16_t)(bx + 0x6AE4u)]);
    *pch = (uint8_t)(*pch + w->b[(uint16_t)(bx + 0x6AECu)]);
    return bx;
}

/* ==== Pass 0x29F6: probabilistic dead-end pruning ====
 * Walk the 0x28B2 dead-end list; for 256 RNG draws, when the draw exceeds a
 * mission-scaled threshold and the cell it points to (following the flow direction)
 * is not a 2/3-junction, clear that dead-end cell (selector + type = 0). */
void captive_gm_pass_29f6(CaptiveGmWork *w) {
    uint16_t bx = 0x3098u;
    uint16_t mission = captive_gm_wget(w, 0x3078u);
    uint16_t thr = (mission <= 0x1Fu)
        ? (uint16_t)(gm_ror16(mission, 5) & 0xF800u)
        : 0xFFFFu;
    captive_gm_wset(w, 0x33E2u, thr);
    for (int limit = 0xFF; limit >= 0; --limit, bx = (uint16_t)(bx + 2u)) {
        uint16_t ax = captive_gm_rng_next(w);            /* 0x2A1F */
        if (ax <= thr) continue;                         /* 0x2A26 jbe */
        uint16_t cell = captive_gm_wget(w, bx);
        if (cell == 0u) continue;                        /* 0x2A28 */
        uint8_t cl = (uint8_t)cell, ch = (uint8_t)(cell >> 8);
        gm_2a59(w, &cl, &ch);                            /* 0x2A2F step to neighbour */
        uint8_t dl = gm_2675(w, cl, ch);                 /* 0x2A32 */
        if (dl == 3u || dl == 2u) continue;              /* 0x2A35/0x2A3A keep */
        uint16_t bp = captive_gm_map_index((uint8_t)cell, (uint8_t)(cell >> 8));
        captive_gm_wset(w, (uint16_t)(GM_MAP_SEL + bp), 0u);   /* 0x2A44 */
        captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + bp), 0u);  /* 0x2A49 */
    }
}

/* ==== Pass 0x2888: shuffle the dead-end list ====
 * 513 random pair swaps over the 0x3098 list (indices masked to even byte offsets
 * 0..0x1FE), with word[0x3074] saved/restored so the RNG use is discarded. */
void captive_gm_pass_2888(CaptiveGmWork *w) {
    uint16_t saved = captive_gm_wget(w, 0x3074u);
    for (int i = 0; i < 0x201; ++i) {
        uint16_t bp = (uint16_t)(captive_gm_rng_next(w) & 0x1FEu);
        uint16_t bx = (uint16_t)(captive_gm_rng_next(w) & 0x1FEu);
        uint16_t a = captive_gm_wget(w, (uint16_t)(0x3098u + bp));
        uint16_t b = captive_gm_wget(w, (uint16_t)(0x3098u + bx));
        captive_gm_wset(w, (uint16_t)(0x3098u + bp), b);
        captive_gm_wset(w, (uint16_t)(0x3098u + bx), a);
    }
    captive_gm_wset(w, 0x3074u, saved);
}

/* GM 0x29CA: 1 iff (cl,ch) is in bounds and its cell type is a room code
 * (0x35/0x36/0x37/0x33/0x1F). */
static int gm_29ca(CaptiveGmWork *w, uint8_t cl, uint8_t ch) {
    if (cl >= 0x40u || ch >= 0x20u) return 0;
    uint8_t dl = w->b[(uint16_t)(GM_MAP_TYPE + captive_gm_map_index(cl, ch))];
    return (dl == 0x35u || dl == 0x36u || dl == 0x37u || dl == 0x33u || dl == 0x1Fu);
}

/* GM 0x2974: stamp a 3x3 block of code `al` around (cl,ch), stepping toward the open
 * side (dh from the east neighbour, bh from the south), skipping wall-type cells. */
static void gm_2974(CaptiveGmWork *w, uint8_t cl, uint8_t ch, uint8_t al) {
    int dh = gm_29ca(w, (uint8_t)(cl + 1u), ch) ? 1 : -1;
    int bh = gm_29ca(w, cl, (uint8_t)(ch + 1u)) ? 1 : -1;
    uint8_t rch = ch;
    for (int oy = 2; oy >= 0; --oy) {
        uint8_t rcl = cl;
        for (int ox = 2; ox >= 0; --ox) {
            uint16_t bp = captive_gm_map_index(rcl, rch);
            uint8_t ah = w->b[(uint16_t)(GM_MAP_TYPE + bp)];
            if (!(ah == 0x35u || ah == 0x36u || ah == 0x37u ||
                  ah == 4u || ah == 5u || ah == 6u))
                w->b[(uint16_t)(GM_MAP_TYPE + bp)] = al;
            rcl = (uint8_t)(rcl + dh);
        }
        rch = (uint8_t)(rch + bh);
    }
}

/* GM 0x16D7: pick a random still-0x7F dead-end from the 0x3098 list; clears stale
 * (non-0x7F) entries as it probes.  Returns 1 (and *pcell) on success. */
static int gm_16d7_off(CaptiveGmWork *w, uint16_t *pcell, uint16_t *poff) {
    uint16_t bx = (uint16_t)(captive_gm_rng_next(w) & 0x1FEu);   /* 0x16DD */
    int dl = 0;
    for (;;) {
        uint16_t v = captive_gm_wget(w, (uint16_t)(0x3098u + bx));
        if (v == 0u) {                                          /* 0x16EA empty slot */
            bx = (uint16_t)((bx + 2u) & 0x1FEu);
            dl = (dl - 1) & 0xFF;
            if (dl == 0) return 0;                              /* probed all -> fail */
            continue;
        }
        uint16_t bp = captive_gm_map_index((uint8_t)v, (uint8_t)(v >> 8));
        if (w->b[(uint16_t)(GM_MAP_TYPE + bp)] == 0x7Fu) {      /* 0x1702 */
            *pcell = v; if (poff) *poff = bx; return 1;
        }
        captive_gm_wset(w, (uint16_t)(0x3098u + bx), 0u);       /* 0x1708 clear stale */
        /* jmp 0x16E6: re-mask the same bx and re-probe */
    }
}
static int gm_16d7(CaptiveGmWork *w, uint16_t *pcell) { return gm_16d7_off(w, pcell, 0); }

/* ==== Pass 0x2940: objective / special-item placement ====
 * Find the first of the 64 room records at 0x3298 whose shape flags have both a low
 * (0x07) and a mid (0x38) bit set, and stamp a 3x3 block of code 9 around its cell.
 * If none qualifies, place a single code-9 cell at a random surviving dead-end. */
void captive_gm_pass_2940(CaptiveGmWork *w) {
    uint16_t bx = 0x3298u;
    for (int cx = 0x40; cx > 0; --cx) {
        bx = (uint16_t)(bx + 4u);
        uint8_t flags = w->b[(uint16_t)(bx - 1u)];
        if ((flags & 0x07u) != 0u && (flags & 0x38u) != 0u) {  /* 0x2949/0x294F */
            uint16_t cell = captive_gm_wget(w, (uint16_t)(bx - 4u));
            gm_2974(w, (uint8_t)cell, (uint8_t)(cell >> 8), 9u);
            return;
        }
    }
    /* 0x2957: no record qualified -> random dead-end. */
    uint16_t cell;
    while (!gm_16d7(w, &cell)) { /* retry */ }
    w->b[(uint16_t)(GM_MAP_TYPE + captive_gm_map_index((uint8_t)cell, (uint8_t)(cell >> 8)))] = 9u;
}

/* GM 0x1BF5: 5-bit adjacency mask of the cells reached by applying the cumulative
 * deltas at 0x6ADA/0x6ADB (S, E, centre, W, N).  `mov dl,bl` runs once before the
 * loop (dl=0); the loop-back target is the `shl dl,1`, so dl accumulates: MSB = first
 * step, a bit set when that cell's selector is non-VALID. */
static uint8_t gm_1bf5(CaptiveGmWork *w, uint8_t cl, uint8_t ch) {
    uint8_t dl = 0;
    for (int i = 0; i < 5; ++i) {
        uint16_t bx = (uint16_t)(i * 2);
        dl = (uint8_t)(dl << 1);
        cl = (uint8_t)(cl + w->b[(uint16_t)(0x6ADAu + bx)]);
        ch = (uint8_t)(ch + w->b[(uint16_t)(0x6ADBu + bx)]);
        if (captive_gm_cell_check(w, GM_MAP_SEL, cl, ch) != CAPTIVE_GM_CELL_VALID)
            dl |= 1u;
    }
    return dl;
}

/* GM 0x13D2: accept a corridor cell iff its adjacency mask has bit2 clear and equals
 * 0x0A or 0x11. */
static int gm_13d2(CaptiveGmWork *w, uint8_t cl, uint8_t ch) {
    uint8_t dl = gm_1bf5(w, cl, ch);
    if (dl & 4u) return 0;
    if (dl == 0x0Au) return 1;
    return (dl == 0x11u);
}

/* ==== Pass 0x1314: longest-dead-end objective placement ====
 * The orchestrator runs this ONLY when word[0x307C]==1 (GM 0x3FB cmp/jne); the
 * standard missions (word[0x307C]==0) skip it, so it is not in the normal pass chain.
 * Walk the 0x3098 dead-end list; for each qualifying dead end (0x13D2), follow the
 * flow direction (0x2A59) counting corridor length to the top edge, tracking the
 * longest in word[0x352A] (best cell in 0x352C/0x352E).  The found branch marks that
 * corridor end and spawns the objective + guardian via 0xAB4/0xB00/0xFCB.  Verified
 * byte-exact against the real GM.EXE for the word[0x307C]==1 case (planet 0x15). */
static int gm_ab4(CaptiveGmWork *w, uint8_t cl, uint8_t ch, uint8_t *pdl);
static int gm_b00(CaptiveGmWork *w, uint8_t al, uint16_t cx, uint16_t bp_rng);
static uint16_t gm_fcb(CaptiveGmWork *w, uint16_t cx, uint16_t bx, uint16_t dx);
void captive_gm_pass_1314(CaptiveGmWork *w) {
    captive_gm_wset(w, 0x352Au, 0u);
    for (int cx = 0x100; cx > 0; --cx) {
        uint16_t bx = (uint16_t)(0x3098u + (0x100 - cx) * 2);
        uint16_t cell = captive_gm_wget(w, bx);
        if (cell == 0u) continue;                         /* 0x1323 */
        uint8_t cl = (uint8_t)cell, ch = (uint8_t)(cell >> 8);
        w->b[0x33A2u] = cl; w->b[0x33B6u] = ch;           /* save this dead end */
        gm_2a59(w, &cl, &ch);                             /* 0x1398 step */
        if (!gm_13d2(w, cl, ch)) continue;                /* 0x139E reject */
        captive_gm_wset(w, 0x33A6u, 0u);                  /* 0x13A0 */
        for (;;) {                                        /* 0x13A6 count to top edge */
            gm_2a59(w, &cl, &ch);
            if (ch == 0xFFu) break;                       /* 0x13A9 */
            uint16_t n = (uint16_t)(captive_gm_wget(w, 0x33A6u) + 1u);
            captive_gm_wset(w, 0x33A6u, n);               /* 0x13AE inc word[0x33A6] */
            if (n & 0x8000u) break;                       /* jns guard */
        }
        uint16_t len = captive_gm_wget(w, 0x33A6u);
        if (captive_gm_wget(w, 0x352Au) <= len) {         /* 0x13B9 keep the longest */
            captive_gm_wset(w, 0x352Au, len);
            captive_gm_wset(w, 0x352Cu, captive_gm_wget(w, 0x33A2u));  /* WORD copy (0x13C5) */
            captive_gm_wset(w, 0x352Eu, captive_gm_wget(w, 0x33B6u));
        }
    }
    if (captive_gm_wget(w, 0x352Au) == 0u) return;        /* 0x1332: nothing found */
    /* GM 0x133C: objective + guardian at the longest dead end. */
    uint8_t cl = w->b[0x352Cu], ch = w->b[0x352Eu];
    gm_2a59(w, &cl, &ch);                                 /* 0x1344 step */
    uint8_t dl; gm_ab4(w, cl, ch, &dl);                   /* 0x1347 classify (only dl used) */
    uint8_t code = (uint8_t)(4u + dl);                    /* 0x134A ax=4; add al,dl */
    gm_b00(w, code, (uint16_t)(((uint16_t)ch << 8) | cl),
           captive_gm_wget(w, 0x3074u));                  /* 0x134F bp=rng; 0x1353 place */
    cl = w->b[0x352Cu]; ch = w->b[0x352Eu];               /* 0x1356 */
    captive_gm_wset(w, 0x36u, 0u);                        /* 0x1364 */
    w->b[0x3564u] |= 0x10u;                               /* 0x136A */
    captive_gm_wset(w, 0x3566u, 0u);                      /* 0x136F */
    gm_fcb(w, (uint16_t)(((uint16_t)ch << 8) | cl), 0x1Eu, 0u);  /* 0x1375 spawn guardian */
    cl = w->b[0x352Cu]; ch = w->b[0x352Eu];               /* 0x1378 */
    uint16_t bp = captive_gm_map_index(cl, ch);
    w->b[(uint16_t)(GM_MAP_TYPE + bp)] |= 0x80u;          /* 0x1383 */
    captive_gm_wset(w, (uint16_t)(GM_MAP_SEL + bp), 0xFFEAu);  /* 0x1387 */
}

/* ==== The creature/item spawn engine (0xFCB/0xFE2) + its placement pass 0xE12 ==== */

/* GM 0x11E1: advance di in steps of 4 to the first slot with word[di+2] negative. */
static uint16_t gm_11e1(CaptiveGmWork *w, uint16_t di) {
    do { di = (uint16_t)(di + 4u); } while (!(captive_gm_wget(w, (uint16_t)(di + 2u)) & 0x8000u));
    return di;
}

/* GM 0x11EE: pick the creature-type table for `dh` and clamp dh by an RNG bound. */
static uint8_t gm_11ee(uint8_t dh, uint16_t ax, uint16_t *psi) {
    uint16_t si;
    if (dh == 4u) si = 0x6B0Eu; else if (dh == 5u) si = 0x6B09u;
    else if (dh == 2u) si = 0x6B12u; else if (dh == 1u) si = 0x6B14u; else si = 0x6B00u;
    *psi = si;
    uint8_t al = (uint8_t)((gm_ror16(ax, 4) & 0x1Fu) + 1u);
    if (dh > al) dh = al;
    return dh;
}

/* GM 0x2468: 1 iff (cl,ch) is a valid spawn cell (selector >= 1 and cell type in the
 * door/junction set 7/0x1F/0x33/0x14/0x11). */
static int gm_2468(CaptiveGmWork *w, uint8_t cl, uint8_t ch) {
    uint16_t bp = captive_gm_map_index(cl, ch);
    if ((int16_t)captive_gm_wget(w, (uint16_t)(GM_MAP_SEL + bp)) < 1) return 0;
    uint8_t t = w->b[(uint16_t)(GM_MAP_TYPE + bp)];
    return (t == 7u || t == 0x1Fu || t == 0x33u || t == 0x14u || t == 0x11u);
}

/* GM 0xFE2: pick a spawn record from the 0x6B16 database (matching category, mission,
 * flags) and write the creature/item entity records (with RNG-scaled HP + stats) into
 * the buffer at word[0x3588].  Draws the secondary RNG once.  Verified byte-for-byte
 * against GM via a captured-state harness. */
static uint16_t gm_fe2(CaptiveGmWork *w, uint16_t cx, uint16_t bx, uint16_t dx) {
    uint8_t dh = (uint8_t)(dx >> 8), dl = (uint8_t)dx;
    captive_gm_wset(w, 0x3516u, cx);
    uint16_t ax = gm_1c57(w);                                  /* 0xFE6 */
    uint16_t idx = bx ? captive_gm_wget(w, 0x3598u) : captive_gm_wget(w, 0x3596u);
    dl = (uint8_t)(((idx & 0xFFu) + 1u) & 0x3Fu);
    ax = gm_ror16(ax, 5);                                      /* 0xFFA */
    captive_gm_wset(w, 0x6DE6u, bx);
    uint16_t si;
    for (;;) {                                                 /* 0x1008 */
        if (captive_gm_wget(w, 0x6DE6u) == 0u) w->b[0x3596u] = dl; else w->b[0x3598u] = dl;
        uint16_t di = captive_gm_wget(w, 0x3588u);
        w->b[0x351Cu] = dl;
        si = (uint16_t)((dl << 3) + 0x6B16u);
        captive_gm_wset(w, 0x6DDEu, si);
        uint16_t cat = (uint16_t)(w->b[si] & 0xF0u);
        if (captive_gm_wget(w, 0x6DE6u) != cat) { dl = (uint8_t)((dl + 1u) & 0x3Fu); continue; }
        dh = (uint8_t)(w->b[si] & 0x0Fu);
        if (dh == 0u) { dl = (uint8_t)((dl + 1u) & 0x3Fu); continue; }
        if ((captive_gm_wget(w, (uint16_t)(si + 6u)) & 0x1Fu) > captive_gm_wget(w, 0x3078u)) {
            dl = (uint8_t)((dl + 1u) & 0x3Fu); continue; }
        uint16_t fb = (uint16_t)((captive_gm_wget(w, (uint16_t)(si + 6u)) >> 5) & 7u);
        if (fb != 0u) {
            uint8_t al = w->b[(uint16_t)((fb - 1u) + 0x6DE8u)];
            if ((w->b[0x359Au] & al) == 0u) { dl = (uint8_t)((dl + 1u) & 0x3Fu); continue; }
        }
        /* accepted */
        dh = gm_11ee(dh, ax, &si);                             /* 0x107A (si -> table ptr) */
        captive_gm_wset(w, 0x6DE0u, si);
        uint16_t sz = (uint16_t)((dh + 1u) << 3);
        if (sz > captive_gm_wget(w, (uint16_t)(di + 2u))) return 0xFFFFu;
        captive_gm_wset(w, (uint16_t)(di + 2u),
                        (uint16_t)(captive_gm_wget(w, (uint16_t)(di + 2u)) - sz));
        uint16_t rec = captive_gm_wget(w, 0x6DDEu);
        di = gm_11e1(w, di);
        captive_gm_wset(w, di, captive_gm_wget(w, 0x3516u)); di += 2;
        captive_gm_wset(w, di, sz); di += 2;
        w->b[di] = 0; di += 1;
        int do_cc = 1;
        if (captive_gm_wget(w, 0x3566u) == 0u) {               /* 0x10A6 */
            if (captive_gm_wget(w, 0x33DAu) != 0u) {
                captive_gm_wset(w, 0x33DAu, (uint16_t)(captive_gm_wget(w, 0x33DAu) - 1u));
                captive_gm_wset(w, 0x3568u, 0x62u);
                uint8_t ab = (uint8_t)(captive_gm_wget(w, 0x33DAu) & 0xFFu);  /* GM 0x10C1 ror al,3 (8-bit) */
                uint8_t al = (uint8_t)((ab >> 3) | (ab << 5));
                captive_gm_wset(w, 0x356Au, al);
            } else do_cc = 0;
        }
        if (do_cc) {                                           /* 0x10CC */
            uint16_t bbx = (uint16_t)((captive_gm_wget(w, 0x3556u) << 2) + captive_gm_wget(w, 0x358Eu));
            uint16_t a1 = captive_gm_wget(w, 0x3568u); a1 = (uint16_t)((a1 >> 8) | (a1 << 8));
            captive_gm_wset(w, bbx, a1);
            uint16_t a2 = captive_gm_wget(w, 0x356Au); a2 = (uint16_t)((a2 >> 8) | (a2 << 8));
            captive_gm_wset(w, (uint16_t)(bbx + 2u), a2);
            w->b[(uint16_t)(di - 1u)] = (uint8_t)(w->b[0x3556u] | 0x80u);
            captive_gm_wset(w, 0x3556u, (uint16_t)(captive_gm_wget(w, 0x3556u) + 1u));
        }
        w->b[di] = w->b[0x3564u]; di += 1;                     /* 0x10F3 */
        if (captive_gm_wget(w, (uint16_t)(captive_gm_wget(w, 0x6DDEu) + 6u)) & 0x100u)
            w->b[(uint16_t)(di - 1u)] |= 0x40u;
        { uint8_t al = (uint8_t)(ax & 0xFu); w->b[di] = al; di += 1;
          al = (uint8_t)((al >> 1) + w->b[(uint16_t)(rec + 4u)]); w->b[di] = al; di += 1; }
        dh = (uint8_t)(dh - 1u);                               /* 0x1111 */
        int32_t b = (int16_t)captive_gm_wget(w, 0x3078u);
        if (b < 0) b = 0x7F; else { b++; if (b > 0x7F) b = 0x7F; }
        uint16_t bb = (uint16_t)(3 * (uint16_t)b - 2u + captive_gm_wget(w, 0x36u));
        do {                                                   /* 0x1132 per-creature */
            w->b[di] = w->b[0x351Cu]; di += 1;
            { uint16_t s3 = captive_gm_wget(w, 0x6DE0u);
              captive_gm_wset(w, 0x6DE0u, (uint16_t)(s3 + 1u)); w->b[di] = w->b[s3]; di += 1; }
            ax = gm_rol16(ax, 3);
            uint16_t sax = ax, rr = captive_gm_wget(w, 0x6DDEu);
            uint16_t cx16 = (uint16_t)(((uint32_t)captive_gm_wget(w, (uint16_t)(rr + 2u)) * bb) & 0xFFFFu);
            cx16 = (uint16_t)(cx16 + (uint16_t)(((uint32_t)cx16 * sax) >> 16));
            uint16_t msc = (captive_gm_wget(w, 0x3078u) > 0x10u)
                ? (uint16_t)(captive_gm_wget(w, 0x3078u) >> 2)
                : (uint16_t)(captive_gm_wget(w, 0x3078u) >> 1);
            uint16_t dxv = (uint16_t)(((uint32_t)sax * msc) >> 16);
            uint32_t sum = (uint32_t)cx16 + dxv; cx16 = (uint16_t)sum; int carry = (int)((sum >> 16) & 1u);
            uint16_t cxf;
            if (carry || cx16 > 0xFFDCu) {
                dxv >>= 1; uint16_t cxn = (uint16_t)(cx16 - dxv); int borrow = (cx16 < dxv);
                cxf = borrow ? 0xFFDCu : (cxn <= 0xFFDCu ? cxn : 0xFFDCu);
            } else cxf = cx16;
            captive_gm_wset(w, di, cxf); di += 2;
            ax = gm_ror16(sax, 5);
            { uint8_t cl = (uint8_t)((ax & 3u) + w->b[(uint16_t)(rr + 1u)]); if (cl > 0xFDu) cl = 0xFDu;
              w->b[di] = cl; di += 1; }
            ax = gm_ror16(ax, 3);
            { uint8_t cl = (uint8_t)((ax & 3u) + w->b[(uint16_t)(rr + 5u)]); w->b[di] = cl; di += 1; }
            captive_gm_wset(w, di, 0u); di += 2;
            dh = (uint8_t)(dh - 1u);
        } while (!((int8_t)dh < 0));
        captive_gm_wset(w, di, 0x8882u); captive_gm_wset(w, (uint16_t)(di + 2u), 0x8881u);
        captive_gm_wset(w, 0x3534u, (uint16_t)(captive_gm_wget(w, 0x3534u) + 1u));
        return 0u;
    }
}

/* GM 0xFCB: set the category index (word[0x3598]) then invoke 0xFE2 with bx=0x80. */
static uint16_t gm_fcb(CaptiveGmWork *w, uint16_t cx, uint16_t bx, uint16_t dx) {
    captive_gm_wset(w, 0x3598u, bx);
    return gm_fe2(w, cx, 0x80u, dx);
}

/* GM 0x14B7: for mission != 0 this is a no-op (returns immediately).  The mission-0
 * variant is not yet transcribed (unused by the standard missions). */
static void gm_14b7(CaptiveGmWork *w, uint8_t cl, uint8_t ch) {
    (void)cl; (void)ch;
    if (captive_gm_wget(w, 0x3078u) == 0u) { /* TODO: mission-0 seeding pass */ }
}

/* GM 0xE61: place `count`+1 entities at random surviving dead ends; where the flow
 * neighbour is a valid spawn cell, mark it 0x8F and spawn a creature there. */
static void gm_e61(CaptiveGmWork *w, uint16_t count, uint8_t dl, uint16_t bufbx) {
    int limit = (int)count;
    uint16_t bx = bufbx;
    for (;;) {
        uint16_t cell;
        if (!gm_16d7(w, &cell)) return;                        /* 0xE65 no dead end left */
        uint16_t bp_de = captive_gm_map_index((uint8_t)cell, (uint8_t)(cell >> 8));
        uint8_t cl = (uint8_t)cell, ch = (uint8_t)(cell >> 8);
        gm_2a59(w, &cl, &ch);                                  /* step to the flow neighbour */
        if (gm_2468(w, cl, ch)) {                              /* valid spawn spot -> 0xE88 */
            uint16_t bp_st = captive_gm_map_index(cl, ch);
            w->b[(uint16_t)(GM_MAP_TYPE + bp_st)] = 0x0Fu;
            if (captive_gm_wget(w, 0x356Cu) != 0u) {           /* 0xE8C spawn a creature */
                captive_gm_wset(w, 0x3568u, 0x64u);
                captive_gm_wset(w, 0x356Au, w->b[0x359Cu]);
                captive_gm_wset(w, 0x36u, 0u);
                w->b[0x3564u] &= 0xEFu;
                captive_gm_wset(w, 0x3566u, 0xFFFFu);
                gm_fcb(w, (uint16_t)(((uint16_t)ch << 8) | cl), 7u, dl);
                gm_14b7(w, cl, ch);
                w->b[(uint16_t)(GM_MAP_TYPE + bp_st)] |= 0x80u;
                captive_gm_wset(w, 0x359Cu, (uint16_t)(captive_gm_wget(w, 0x359Cu) + 1u));
            }
        } else if (captive_gm_wget(w, 0x356Cu) != 0u) {
            continue;                                          /* 0xE81 retry */
        }
        /* 0xED1: mark the dead end and record it */
        w->b[(uint16_t)(GM_MAP_TYPE + bp_de)] = dl;
        captive_gm_wset(w, (uint16_t)(GM_MAP_SEL + bp_de), 0xFFFBu);
        captive_gm_wset(w, bx, cell); bx = (uint16_t)(bx + 2u);
        if (--limit < 0) return;
    }
}

/* ==== Pass 0xE12: place two waves of creatures/items at dead ends ==== */
void captive_gm_pass_e12(CaptiveGmWork *w) {
    uint16_t ax = captive_gm_rng_next(w);
    ax = (uint16_t)(((uint32_t)ax * 7u) >> 16);
    ax = (uint16_t)(ax + 1u);
    if (ax < 5u) ax = 4u;
    captive_gm_wset(w, 0x356Cu, 0u);
    gm_e61(w, ax, 0x12u, captive_gm_wget(w, 0x357Eu));
    ax = (uint16_t)(captive_gm_rng_next(w) & 1u);
    captive_gm_wset(w, 0x356Cu, 0xFFFFu);
    captive_gm_wset(w, 0x359Cu, 0u);
    gm_e61(w, ax, 0x13u, captive_gm_wget(w, 0x3580u));
    uint16_t bx = captive_gm_wget(w, 0x3580u);                 /* 0xE4F list rotate */
    if (captive_gm_wget(w, (uint16_t)(bx + 2u)) & 0x8000u) return;
    uint16_t a = captive_gm_wget(w, bx), b = captive_gm_wget(w, (uint16_t)(bx + 2u));
    captive_gm_wset(w, (uint16_t)(bx + 2u), a);
    captive_gm_wset(w, bx, b);
}

/* GM 0x2A6D: step (cl,ch) by the direction index `ax` via the 0x6AE4/0x6AEC tables. */
static void gm_2a6d(CaptiveGmWork *w, uint8_t *pcl, uint8_t *pch, uint16_t ax) {
    *pcl = (uint8_t)(*pcl + w->b[(uint16_t)(ax + 0x6AE4u)]);
    *pch = (uint8_t)(*pch + w->b[(uint16_t)(ax + 0x6AECu)]);
}

/* GM 0x2A7A: a direction derived from a cell's flow flag = (aux_high << 1) ^ 4. */
static uint16_t gm_2a7a(CaptiveGmWork *w, uint8_t cl, uint8_t ch) {
    uint16_t bp = captive_gm_map_index(cl, ch);
    uint16_t al = w->b[(uint16_t)(GM_MAP_AUX + bp + 1u)];
    return (uint16_t)(((al << 1) ^ 4u) & 0xFFu);
}

/* GM 0x170C: pick a random non-empty room record from 0x3298 (draws one RNG). */
static int gm_170c(CaptiveGmWork *w, uint16_t *paxoff, uint16_t *pcx) {
    uint16_t di = 0x3298u;
    uint16_t bx = (uint16_t)(captive_gm_rng_next(w) & 0xFCu);
    for (int dl = 0x20; dl >= 0; --dl) {
        uint16_t v = captive_gm_wget(w, (uint16_t)(bx + di));
        if (v != 0u) { *pcx = v; *paxoff = bx; return 1; }
        bx = (uint16_t)(bx + 4u);
    }
    *paxoff = bx; return 0;
}

/* GM 0x1755: from a room record, navigate by the room's shape flags to an adjacent
 * empty cell bordering room floor, and place a chest/altar (type 0x21 / selector
 * 0xFFEC) with a 0x0E marker on the floor side. */
static void gm_1755(CaptiveGmWork *w, uint16_t axoff, uint16_t cx) {
    captive_gm_wset(w, 0x339Cu, axoff);
    uint8_t flags = w->b[(uint16_t)(0x3298u + axoff + 3u)];
    uint16_t nsteps = (uint16_t)(((flags & 0x38u) >> 3) + (flags & 7u));
    uint8_t cl = (uint8_t)cx, ch = (uint8_t)(cx >> 8);
    for (int i = (int)nsteps + 1; i >= 0; --i) gm_2a59(w, &cl, &ch);   /* 0x176E */
    uint16_t ax = gm_2a7a(w, cl, ch);                                  /* 0x1774 */
    gm_2a6d(w, &cl, &ch, ax);                                          /* 0x177A */
    gm_2a6d(w, &cl, &ch, ax);                                          /* 0x177D */
    /* 0x1782: current cell — a valid, non-floor selector cell means abort. */
    int valid0 = (captive_gm_cell_check(w, GM_MAP_SEL, cl, ch) == CAPTIVE_GM_CELL_VALID);
    uint8_t t0 = w->b[(uint16_t)(GM_MAP_TYPE + captive_gm_map_index(cl, ch))];
    if (valid0 && t0 != 0x1Fu && t0 != 0x33u) return;                  /* 0x1798 */
    /* 0x179B: step to the opposite cell (the candidate) and require at least one
     * perpendicular neighbour to be empty; otherwise abort.  GM keeps the opposite
     * cell (cl,ch) as the candidate — the perpendicular steps are only probes. */
    ax ^= 4u;
    gm_2a6d(w, &cl, &ch, ax);                                          /* 0x179F candidate */
    uint8_t a1 = cl, b1 = ch; gm_2a6d(w, &a1, &b1, (uint16_t)((ax + 2u) & 6u));  /* 0x17A4 */
    int perp_empty = (captive_gm_cell_check(w, GM_MAP_SEL, a1, b1) != CAPTIVE_GM_CELL_VALID);
    if (!perp_empty) {
        uint8_t a2 = cl, b2 = ch; gm_2a6d(w, &a2, &b2, (uint16_t)((ax - 2u) & 6u));  /* 0x17B8 */
        perp_empty = (captive_gm_cell_check(w, GM_MAP_SEL, a2, b2) != CAPTIVE_GM_CELL_VALID);
    }
    if (!perp_empty) return;                                           /* 0x17C7 */
    /* 0x17CC: the empty candidate must border room floor to receive the chest. */
    uint16_t bp_c = captive_gm_map_index(cl, ch);
    uint8_t tc = w->b[(uint16_t)(GM_MAP_TYPE + bp_c)];
    if (tc != 0x1Fu && tc != 0x33u) return;
    uint8_t rcl = cl, rch = ch;
    gm_2a59(w, &rcl, &rch);                                            /* 0x17DF step to floor */
    uint16_t bp_f = captive_gm_map_index(rcl, rch);
    uint8_t tf = w->b[(uint16_t)(GM_MAP_TYPE + bp_f)];
    if (tf != 0x1Fu && tf != 0x33u) return;                            /* 0x1804 */
    w->b[(uint16_t)(GM_MAP_TYPE + bp_f)] = 0x0Eu;                      /* 0x17F1 */
    w->b[(uint16_t)(GM_MAP_TYPE + bp_c)] = 0x21u;                      /* 0x17F6 */
    captive_gm_wset(w, (uint16_t)(GM_MAP_SEL + bp_c), 0xFFECu);        /* 0x17FA */
    captive_gm_wset(w, 0x33D8u, (uint16_t)(captive_gm_wget(w, 0x33D8u) + 1u));
}

/* ==== Pass 0x1736: place up to 0x3D chests/altars at random rooms ==== */
void captive_gm_pass_1736(CaptiveGmWork *w) {
    captive_gm_wset(w, 0x33D8u, 0u);
    for (int limit = 0x3C; limit >= 0; --limit) {
        uint16_t axoff, cx;
        if (gm_170c(w, &axoff, &cx)) gm_1755(w, axoff, cx);
    }
}

/* ==== Pass 0x1806: main creature/item distribution ==== */

/* GM 0x1B75: count orthogonal neighbours that are empty selector cells of type 0x23. */
static uint8_t gm_1b75(CaptiveGmWork *w, uint8_t cl, uint8_t ch) {
    uint8_t dl = 0;
#define GM_1B75_CHK(x, y) do { \
    if (captive_gm_cell_check(w, GM_MAP_SEL, (x), (y)) == CAPTIVE_GM_CELL_EMPTY && \
        w->b[(uint16_t)(GM_MAP_TYPE + captive_gm_map_index((x), (y)))] == 0x23u) ++dl; \
    } while (0)
    cl = (uint8_t)(cl - 1u); GM_1B75_CHK(cl, ch);
    cl = (uint8_t)(cl + 2u); GM_1B75_CHK(cl, ch);
    cl = (uint8_t)(cl - 1u); ch = (uint8_t)(ch - 1u); GM_1B75_CHK(cl, ch);
    ch = (uint8_t)(ch + 2u); GM_1B75_CHK(cl, ch);
#undef GM_1B75_CHK
    return dl;
}

/* GM 0x1BA6: mission-scaled RNG lookup into 0x6DBC -> (*pax = al&3, *pdl = (al>>2)&3). */
static void gm_1ba6(CaptiveGmWork *w, uint16_t *pax, uint8_t *pdl) {
    uint16_t dx = captive_gm_wget(w, 0x3078u); if (dx > 0x1Fu) dx = 0x1Fu; ++dx;
    uint16_t hi = (uint16_t)(((uint32_t)captive_gm_wget(w, 0x3074u) * dx) >> 16);
    uint8_t al = w->b[(uint16_t)((hi & 0x1Fu) + 0x6DBCu)];
    *pax = (uint16_t)(al & 3u);
    *pdl = (uint8_t)((al >> 2) & 3u);
}

/* GM 0x1A0B: choose the item-type bit for the current group into word[0x3524]. */
static void gm_1a0b(CaptiveGmWork *w) {
    uint16_t bx = (uint16_t)(captive_gm_wget(w, 0x3520u) & 3u);
    if (bx == 0u) w->b[0x3524u] = 0xFFu;
    bx <<= 1;
    uint16_t ax = captive_gm_wget(w, (uint16_t)(bx + 0x6DB4u));
    uint16_t bx2 = captive_gm_wget(w, 0x3524u);
    while ((ax & bx2) == 0u) bx2 = gm_ror16(bx2, 3);
    captive_gm_wset(w, 0x3524u, (uint16_t)(bx2 & ax));
}

/* GM 0x1226: insert a creature type into the sorted spawn list at word[0x358C]. */
static void gm_1226_body(CaptiveGmWork *w, uint16_t cx, uint8_t al, uint8_t dh, uint8_t dl) {
    captive_gm_wset(w, 0x3516u, cx);
    w->b[0x3518u] = al; w->b[0x3519u] = dh; w->b[0x351Au] = dl;
    uint16_t bx = captive_gm_wget(w, 0x358Cu);
    if (captive_gm_wget(w, (uint16_t)(bx + 2u)) <= 8u) return;
    uint16_t ax = gm_1c57(w);
    if (captive_gm_wget(w, 0x356Eu) != 0u) ax = 0u;
    ax = (uint16_t)(gm_ror16(ax, 5) & 3u); ax |= 0x80u; w->b[0x351Bu] = (uint8_t)ax;
    uint16_t si = (uint16_t)(bx + 4u);
    cx = captive_gm_wget(w, 0x3516u);
    for (;;) {
        if (captive_gm_wget(w, si) == cx) {
            uint16_t b2 = (uint16_t)(captive_gm_wget(w, 0x358Cu) + 0x800u);
            do {
                b2 = (uint16_t)(b2 - 4u);
                captive_gm_wset(w, (uint16_t)(b2 + 4u), captive_gm_wget(w, b2));
                captive_gm_wset(w, (uint16_t)(b2 + 6u), captive_gm_wget(w, (uint16_t)(b2 + 2u)));
            } while (b2 > si);
            bx = captive_gm_wget(w, 0x358Cu);
            captive_gm_wset(w, (uint16_t)(si + 2u), (uint16_t)(captive_gm_wget(w, (uint16_t)(si + 2u)) + 4u));
            captive_gm_wset(w, (uint16_t)(si + 4u), captive_gm_wget(w, 0x3518u));
            captive_gm_wset(w, (uint16_t)(si + 6u), captive_gm_wget(w, 0x351Au));
            captive_gm_wset(w, (uint16_t)(bx + 2u), (uint16_t)(captive_gm_wget(w, (uint16_t)(bx + 2u)) - 4u));
            return;
        }
        uint16_t a = captive_gm_wget(w, (uint16_t)(si + 2u));
        if (a & 0x8000u) {
            captive_gm_wset(w, si, cx);
            captive_gm_wset(w, (uint16_t)(si + 2u), 8u);
            captive_gm_wset(w, (uint16_t)(si + 4u), captive_gm_wget(w, 0x3518u));
            captive_gm_wset(w, (uint16_t)(si + 6u), captive_gm_wget(w, 0x351Au));
            captive_gm_wset(w, (uint16_t)(si + 8u), 0x8882u);
            captive_gm_wset(w, (uint16_t)(si + 0xAu), 0x8881u);
            captive_gm_wset(w, (uint16_t)(bx + 2u), (uint16_t)(captive_gm_wget(w, (uint16_t)(bx + 2u)) - 8u));
            return;
        }
        si = (uint16_t)(si + a);
    }
}

/* GM 0x1226: reset the discard flag, then insert. */
static void gm_1226(CaptiveGmWork *w, uint16_t cx, uint8_t al, uint8_t dh, uint8_t dl) {
    captive_gm_wset(w, 0x356Eu, 0u);
    gm_1226_body(w, cx, al, dh, dl);
}

/* GM 0x1A3A: find a corridor spot, record the path cells (byte[0x33A2..0x33CC]) and the
 * group parameters; returns 1 on success (odl/odh receive the residual dl/dh for the
 * follow-on spawn), 0 on failure. */
static int gm_1a3a(CaptiveGmWork *w, uint8_t cl, uint8_t ch, uint8_t *odl, uint8_t *odh) {
    w->b[0x33A2u] = cl; w->b[0x33B6u] = ch;
    uint8_t dl = (uint8_t)(captive_gm_rng_next(w) & 0xFu);
    for (int i = dl; i >= 0; --i) gm_2a59(w, &cl, &ch);
    /* GM 0x1A51 `call 0x2675; jne 0x1A59`: branch on 0x2675's ZF (set iff the CENTRE
     * cell's selector is VALID), not on the neighbour count it returns. */
    if (captive_gm_cell_check(w, GM_MAP_SEL, cl, ch) != CAPTIVE_GM_CELL_VALID) return 0;
    if (w->b[(uint16_t)(GM_MAP_TYPE + captive_gm_map_index(cl, ch))] != 7u) return 0;
    uint8_t d2 = gm_1b75(w, cl, ch);
    uint16_t bx;
    if (d2 == 0u) {
        bx = 7u; uint8_t am = gm_1bf5(w, cl, ch);
        if (am & 4u) return 0;
        w->b[0x3522u] = 6u;
        if (am != 0x0Au) { w->b[0x3522u] = (uint8_t)(w->b[0x3522u] + 8u); if (am != 0x11u) bx = 6u; }
    } else if (d2 == 1u) bx = 6u; else bx = 4u;
    uint16_t ax0; uint8_t dl0; gm_1ba6(w, &ax0, &dl0);
    uint16_t axm = (uint16_t)(ax0 - 1u);
    if ((w->b[(uint16_t)(axm + 0x6DE8u)] & (uint8_t)bx) == 0u) return 0;
    captive_gm_wset(w, 0x351Eu, axm);
    captive_gm_wset(w, 0x3520u, (uint16_t)((axm & 0xFF00u) | dl0));
    w->b[0x33A4u] = cl; w->b[0x33B8u] = ch;
    dl = (uint8_t)(captive_gm_rng_next(w) & 7u);
    for (;;) { if (gm_2a59(w, &cl, &ch) == 0xFFFFu) return 0; if ((int8_t)(--dl) < 0) break; }
    uint8_t lastdh = captive_gm_grid_cell(w, cl, ch);
    uint8_t cnt2675 = gm_2675(w, cl, ch);
    if (cnt2675 == 0u) return 0;
    w->b[0x33A6u] = cl; w->b[0x33BAu] = ch;
    uint16_t axr = gm_2a59(w, &cl, &ch);
    if (axr == 0xFFFFu) return 0;
    uint16_t bxm = (uint16_t)((captive_gm_wget(w, 0x3074u) >> 8) & 3u);
    { uint16_t axx = axr; if (bxm != 2u) axx = (uint16_t)((axx + bxm + bxm) & 3u); captive_gm_wset(w, 0x33CCu, axx); }
    int bxn = (int)bxm, first = 1;
    for (;;) {
        if (!first) { w->b[0x33A6u] = cl; w->b[0x33BAu] = ch; gm_2a6d(w, &cl, &ch, captive_gm_wget(w, 0x33CCu)); }
        first = 0;
        if (cl > 0x3Fu || ch > 0x1Fu) return 0;
        uint16_t sv = captive_gm_wget(w, (uint16_t)(GM_MAP_SEL + captive_gm_map_index(cl, ch)));
        if (sv & 0x8000u) return 0;
        if (sv == 0u) break;
        if (--bxn < 0) return 0;
    }
    w->b[0x33A8u] = cl; w->b[0x33BCu] = ch;
    *odl = cnt2675; *odh = lastdh;
    uint8_t rcl = w->b[0x33A6u], rch = w->b[0x33BAu];
    uint8_t al = w->b[(uint16_t)(GM_MAP_TYPE + captive_gm_map_index(rcl, rch))];
    if (gm_2468(w, rcl, rch)) return 1;
    if (al == 6u || al == 0x37u || al == 4u || al == 5u || al == 0x35u ||
        al == 0x36u || al == 9u || al == 0x0Eu || al == 0x7Fu) return 1;
    return 0;
}

/* GM 0x1840: place one creature/item group at the spot found by 0x1A3A. */
static void gm_1840(CaptiveGmWork *w, uint16_t axoff, uint8_t cl0, uint8_t ch0) {
    captive_gm_wset(w, 0x339Cu, axoff);
    uint8_t g_dl, g_dh;
    if (!gm_1a3a(w, cl0, ch0, &g_dl, &g_dh)) return;
    uint16_t ax = captive_gm_wget(w, 0x3074u); if (ax == 0u) ax = 0x4285u;
    ax = gm_ror16(ax, 1); ax = (uint16_t)((ax << 8) | (ax >> 8));
    captive_gm_wset(w, 0x3524u, ax);
    gm_1a0b(w);
    captive_gm_wset(w, 0x3530u, 0u);
    uint16_t bx = (uint16_t)(captive_gm_wget(w,
        (uint16_t)(GM_MAP_SEL + captive_gm_map_index(w->b[0x33A4u], w->b[0x33B8u]))) + 1u);
    if (captive_gm_wget(w, 0x3520u) > 1u) {                 /* 0x1875 search for a door cell */
        int found = 0; uint8_t dcl = 0, dch = 0;
        for (int cnt = 0xA; cnt >= 0; --cnt) {
            for (;;) {
                uint16_t pos = captive_gm_rng_pos(w);
                uint8_t pcl = (uint8_t)pos, pch = (uint8_t)(pos >> 8);
                int16_t sv = (int16_t)captive_gm_wget(w, (uint16_t)(GM_MAP_SEL + captive_gm_map_index(pcl, pch)));
                if (gm_2468(w, pcl, pch)) {
                    if (sv < 0 || sv <= 1) break;
                    uint16_t axv = (uint16_t)(sv + 1);
                    if (axv == bx) { captive_gm_wset(w, 0x3530u, 0xFFFFu); dcl = pcl; dch = pch; found = 1; break; }
                    --axv; if (axv > bx) break;
                    if (axv != bx) { dcl = pcl; dch = pch; found = 1; break; }
                    break;
                } else { if (sv < 1) continue; break; }
            }
            if (found) break;
        }
        if (!found) return;
        uint16_t bp = captive_gm_map_index(dcl, dch);
        w->b[(uint16_t)(GM_MAP_TYPE + bp)] = (w->b[(uint16_t)(GM_MAP_TYPE + bp)] == 0x14u) ? 0x29u : 0x28u;
        uint16_t creat = (captive_gm_wget(w, 0x3520u) == 2u) ? 0x5Cu : 0x5Du;
        uint16_t dxv = captive_gm_wget(w, 0x3524u);
        gm_1226(w, (uint16_t)(((uint16_t)dch << 8) | dcl), (uint8_t)creat, (uint8_t)(dxv >> 8), (uint8_t)dxv);
    }
    /* 0x18D3 common placement */
    captive_gm_wset(w, (uint16_t)(0x3098u + captive_gm_wget(w, 0x339Cu)), 0u);
    { uint16_t bp = captive_gm_map_index(w->b[0x33A2u], w->b[0x33B6u]);
      w->b[(uint16_t)(GM_MAP_TYPE + bp)] = (w->b[(uint16_t)(GM_MAP_TYPE + bp)] == 0x33u) ? 0x34u : 0x0Du; }
    uint16_t startcx = (uint16_t)(((uint16_t)w->b[0x33B6u] << 8) | w->b[0x33A2u]);
    uint16_t bpstart = captive_gm_map_index(w->b[0x33A2u], w->b[0x33B6u]);
    { uint16_t dxv = (uint16_t)((captive_gm_wget(w, 0x3074u) & 0x3FFu) + 0x80u);
      gm_1226(w, startcx, 0x19u, (uint8_t)(dxv >> 8), (uint8_t)dxv); }
    captive_gm_wset(w, 0x36u, 0u); w->b[0x3564u] &= 0xEFu; captive_gm_wset(w, 0x3566u, 0u);
    { uint16_t r = gm_fe2(w, startcx, 0u, (uint16_t)((g_dh << 8) | g_dl));
      if (!(r & 0x8000u)) w->b[(uint16_t)(GM_MAP_TYPE + bpstart)] |= 0x80u; }
    { uint16_t bp = captive_gm_map_index(w->b[0x33A4u], w->b[0x33B8u]);
      if (captive_gm_wget(w, 0x3530u) != 0u) { w->b[(uint16_t)(GM_MAP_TYPE + bp)] = 0x32u; captive_gm_wset(w, 0x351Eu, 1u); }
      else if (captive_gm_wget(w, 0x351Eu) == 0u) captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + bp), (uint16_t)((w->b[0x3522u] << 8) | 0x15u));
      else if (captive_gm_wget(w, 0x351Eu) == 1u) w->b[(uint16_t)(GM_MAP_TYPE + bp)] = 0u;
      else w->b[(uint16_t)(GM_MAP_TYPE + bp)] = 0x27u; }
    { uint8_t c = w->b[0x33A6u], h = w->b[0x33BAu]; uint16_t bp = captive_gm_map_index(c, h);
      if (gm_2468(w, c, h) || w->b[(uint16_t)(GM_MAP_TYPE + bp)] == 0x7Fu) w->b[(uint16_t)(GM_MAP_TYPE + bp)] = 0x0Cu; }
    { uint16_t bp = captive_gm_map_index(w->b[0x33A8u], w->b[0x33BCu]);
      captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + bp), (uint16_t)((w->b[0x33CCu] << 8) | 0x22u));
      captive_gm_wset(w, (uint16_t)(GM_MAP_SEL + bp), 0xFFC3u); }
    { uint16_t rbx = (uint16_t)(captive_gm_wget(w, 0x358Au) + captive_gm_wget(w, 0x353Eu));
      captive_gm_wset(w, rbx, (uint16_t)(((uint16_t)w->b[0x33BCu] << 8) | w->b[0x33A8u]));
      captive_gm_wset(w, (uint16_t)(rbx + 2u), captive_gm_wget(w, 0x3524u));
      captive_gm_wset(w, (uint16_t)(rbx + 4u), 0u);
      captive_gm_wset(w, (uint16_t)(rbx + 6u), (uint16_t)(((uint16_t)w->b[0x33B8u] << 8) | w->b[0x33A4u]));
      w->b[(uint16_t)(rbx + 8u)] = (uint8_t)captive_gm_wget(w, 0x3520u);
      w->b[(uint16_t)(rbx + 8u)] |= (uint8_t)((w->b[0x351Eu] << 2) & 0x0Cu);
      if (captive_gm_wget(w, 0x3074u) > 0x7000u) w->b[(uint16_t)(rbx + 8u)] |= 0x10u;
      w->b[(uint16_t)(rbx + 9u)] = 0x20u;
      captive_gm_wset(w, 0x353Eu, (uint16_t)(captive_gm_wget(w, 0x353Eu) + 0xAu));
      captive_gm_wset(w, 0x33D6u, (uint16_t)(captive_gm_wget(w, 0x33D6u) + 1u));
      captive_gm_wset(w, 0x3530u, 0u); }
}

void captive_gm_pass_1806(CaptiveGmWork *w) {
    captive_gm_wset(w, 0x33D6u, 0u);
    uint16_t bx = captive_gm_wget(w, 0x3078u); if (bx > 9u) bx = 9u;
    bx = captive_gm_wget(w, (uint16_t)((bx << 1) + 0x6D9Eu));
    captive_gm_wset(w, 0x3526u, bx);
    for (int limit = 0x1E0; limit >= 0; --limit) {
        uint16_t cx, off;
        if (!gm_16d7_off(w, &cx, &off)) return;                 /* 0x182C no dead end left */
        gm_1840(w, off, (uint8_t)cx, (uint8_t)(cx >> 8));
        if (captive_gm_wget(w, 0x3526u) <= captive_gm_wget(w, 0x353Eu)) return;  /* 0x1836 */
    }
}

/* GM 0x168D: find the first empty (type 0 and selector 0) orthogonal neighbour in
 * N,S,W,E order; on success move (cl,ch) there, set *pdh (0/2/1/3), return 1. */
static int gm_168d(CaptiveGmWork *w, uint8_t *pcl, uint8_t *pch, uint8_t *pdh) {
    uint8_t cl = *pcl, ch = *pch;
#define GM_168D_CK(dir, x, y) do { *pdh = (dir); \
    if (!((x) >= 0x40u || (y) >= 0x20u)) { uint16_t bp = captive_gm_map_index((x), (y)); \
        if (w->b[(uint16_t)(GM_MAP_TYPE + bp)] == 0u && captive_gm_wget(w, (uint16_t)(GM_MAP_SEL + bp)) == 0u) { \
            *pcl = (x); *pch = (y); return 1; } } } while (0)
    ch = (uint8_t)(ch - 1u); GM_168D_CK(0u, cl, ch);
    ch = (uint8_t)(ch + 2u); GM_168D_CK(2u, cl, ch);
    ch = (uint8_t)(ch - 1u); cl = (uint8_t)(cl - 1u); GM_168D_CK(1u, cl, ch);
    cl = (uint8_t)(cl + 2u); GM_168D_CK(3u, cl, ch);
#undef GM_168D_CK
    return 0;
}

/* GM 0x2B79: walk a corridor from a dead end and place a chest (type 0x22 / selector
 * 0xFFC3) at an empty cell beside its start, recording the 3-cell path. */
static int gm_2b79(CaptiveGmWork *w, uint8_t cl, uint8_t ch) {
    w->b[0x33A2u] = cl; w->b[0x33B6u] = ch;
    int bx = (int)((captive_gm_wget(w, 0x3074u) & 7u) + 8u);
walk:
    for (;;) {
        gm_2a59(w, &cl, &ch);
        if (gm_2468(w, cl, ch)) { /* valid -> just decrement */ }
        else {
            uint8_t t = w->b[(uint16_t)(GM_MAP_TYPE + captive_gm_map_index(cl, ch))];
            if (t == 6u || t == 0x37u || t == 4u || t == 5u || t == 0x35u || t == 0x36u) ++bx;
            else return 0;
        }
        if (--bx < 0) break;
    }
cell2:
    w->b[0x33A4u] = cl; w->b[0x33B8u] = ch;                        /* path cell 2 */
    gm_2a59(w, &cl, &ch);
    { uint16_t t = captive_gm_wget(w, (uint16_t)(GM_MAP_TYPE + captive_gm_map_index(cl, ch)));
      if (t == 0x1Fu || t == 0x33u || t == 0x11u) { ++bx; if (bx >= 0) goto walk; goto cell2; }  /* GM word compares */
      if (t != 7u) return 0; }
    w->b[0x33A6u] = cl; w->b[0x33BAu] = ch;                        /* path cell 3 */
    cl = w->b[0x33A2u]; ch = w->b[0x33B6u];
    uint8_t dh;
    if (!gm_168d(w, &cl, &ch, &dh)) return 0;
    w->b[0x33A2u] = cl; w->b[0x33B6u] = ch;
    { uint16_t bp = captive_gm_map_index(cl, ch);
      captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + bp), (uint16_t)(((dh << 1) << 8) | 0x22u));
      captive_gm_wset(w, (uint16_t)(GM_MAP_SEL + bp), 0xFFC3u); }
    captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + captive_gm_map_index(w->b[0x33A6u], w->b[0x33BAu])), 0x0Fu);
    captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + captive_gm_map_index(w->b[0x33A4u], w->b[0x33B8u])), 0x25u);
    { uint16_t rbx = (uint16_t)(captive_gm_wget(w, 0x33D4u) * 6u + captive_gm_wget(w, 0x3592u));
      captive_gm_wset(w, rbx, (uint16_t)(((uint16_t)w->b[0x33B8u] << 8) | w->b[0x33A4u]));
      captive_gm_wset(w, (uint16_t)(rbx + 2u), (uint16_t)(((uint16_t)w->b[0x33BAu] << 8) | w->b[0x33A6u]));
      captive_gm_wset(w, (uint16_t)(rbx + 4u), (uint16_t)(((uint16_t)w->b[0x33B6u] << 8) | w->b[0x33A2u])); }
    captive_gm_wset(w, 0x33D4u, (uint16_t)(captive_gm_wget(w, 0x33D4u) + 1u));
    return 1;
}

/* ==== Pass 0x2A9D: place up to 0x10 chests at random dead ends ==== */
void captive_gm_pass_2a9d(CaptiveGmWork *w) {
    if (captive_gm_wget(w, 0x3078u) == 0u) return;
    for (int bx = 0x64; bx >= 0; --bx) {
        uint16_t cell;
        if (!gm_16d7(w, &cell)) return;
        gm_2b79(w, (uint8_t)cell, (uint8_t)(cell >> 8));
        if (captive_gm_wget(w, 0x33D4u) >= 0x10u) return;
    }
}

/* GM 0x12DA: derive creature HP/level scalars (word[0x3568]/word[0x356A]) from the RNG. */
static void gm_12da(CaptiveGmWork *w) {
    uint16_t ax = captive_gm_wget(w, 0x3078u); if (ax > 0x16u) ax = 0x16u; ++ax;
    uint16_t dx = (uint16_t)(((uint32_t)ax * captive_gm_wget(w, 0x3074u)) >> 16);
    dx = (uint16_t)(dx + 0x36u); if (dx > 0x4Cu) dx = 0x4Cu;
    captive_gm_wset(w, 0x3568u, dx);
    captive_gm_wset(w, 0x356Au, (uint16_t)(captive_gm_wget(w, 0x3074u) & 0x3Fu));
    w->b[0x356Bu] |= (uint8_t)(captive_gm_wget(w, 0x3074u) & 0xE0u);
}

/* GM 0x233D (main entry at 0x2344 jmp 0x2366): insert a creature into the spawn list
 * with the discard flag word[0x356E]=0xFF (so 0x1226 uses ax=0, not a fresh RNG). */
static void gm_233d(CaptiveGmWork *w, uint16_t cx) {
    uint16_t dx = captive_gm_wget(w, 0x356Au);
    uint8_t al = w->b[0x3568u];
    captive_gm_wset(w, 0x356Eu, 0xFFu);
    gm_1226_body(w, cx, al, (uint8_t)(dx >> 8), (uint8_t)dx);
}

/* GM 0x2AD4: at a dead end whose 2-step flow reaches a 2-then-1 junction, place a chest
 * (0x22 / 0xFFC3) at an adjacent empty cell and spawn a creature there. */
static void gm_2ad4(CaptiveGmWork *w, uint16_t axoff, uint8_t cl, uint8_t ch) {
    captive_gm_wset(w, 0x339Cu, axoff);
    w->b[0x339Eu] = cl; w->b[0x33A0u] = ch;
    gm_2a59(w, &cl, &ch); if (gm_2675(w, cl, ch) != 2u) return;   /* 0x2AE8 */
    if (!gm_2468(w, cl, ch)) return;                             /* 0x2AF0 */
    gm_2a59(w, &cl, &ch); if ((int8_t)gm_2675(w, cl, ch) < 1) return;  /* 0x2AF8 */
    if (!gm_2468(w, cl, ch)) return;                             /* 0x2B00 */
    uint8_t dh; if (!gm_168d(w, &cl, &ch, &dh)) return;          /* 0x2B05 */
    uint16_t chest_dx = (uint16_t)(((dh << 1) << 8) | 0x22u);
    uint16_t bp = captive_gm_map_index(cl, ch);
    captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + bp), chest_dx);  /* 0x2B0B chest */
    captive_gm_wset(w, (uint16_t)(GM_MAP_SEL + bp), 0xFFC3u);
    w->b[0x33A2u] = cl; w->b[0x33B6u] = ch;
    gm_12da(w);                                                  /* 0x2B1B */
    gm_233d(w, (uint16_t)(((uint16_t)ch << 8) | cl));            /* 0x2B1E */
    cl = w->b[0x339Eu]; ch = w->b[0x33A0u];                      /* dead end */
    captive_gm_wset(w, 0x36u, 2u); w->b[0x3564u] |= 0x10u; captive_gm_wset(w, 0x3566u, 0u);
    { uint16_t r = gm_fe2(w, (uint16_t)(((uint16_t)ch << 8) | cl), 0u, chest_dx);  /* 0x2B41 */
      if (r & 0x8000u) return; }
    w->b[(uint16_t)(GM_MAP_TYPE + captive_gm_map_index(cl, ch))] |= 0x80u;  /* 0x2B48 */
    gm_2a59(w, &cl, &ch);                                        /* 0x2B4F */
    { uint16_t rbx = (uint16_t)(captive_gm_wget(w, 0x33D2u) * 4u + captive_gm_wget(w, 0x3590u));
      captive_gm_wset(w, rbx, (uint16_t)(((uint16_t)w->b[0x33B6u] << 8) | w->b[0x33A2u]));
      captive_gm_wset(w, (uint16_t)(rbx + 2u), (uint16_t)(((uint16_t)ch << 8) | cl));
      captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + captive_gm_map_index(cl, ch)), 0u);  /* 0x2B6F */
      captive_gm_wset(w, 0x33D2u, (uint16_t)(captive_gm_wget(w, 0x33D2u) + 1u)); }
}

/* ==== Pass 0x2ABC: place up to 0x20 guard creatures at 2-1 junctions off dead ends ==== */
void captive_gm_pass_2abc(CaptiveGmWork *w) {
    for (int bx = 0x64; bx >= 0; --bx) {
        uint16_t cell, off;
        if (!gm_16d7_off(w, &cell, &off)) return;
        gm_2ad4(w, off, (uint8_t)cell, (uint8_t)(cell >> 8));
        if (captive_gm_wget(w, 0x33D2u) > 0x1Fu) return;         /* 0x2AC9 */
    }
}

/* ==== Pass 0xA2A: item / creature-nest distribution ====
 * Two loops.  First: 25 records from word[0x3586] each place a plain type-0x15 item
 * (code 9, or 1 when the east-neighbour type is 0x1A..0x1D) via 0xB00.  Second: 300
 * RNG-seeded probe walks; at each cell classify (0xAB4), reject neighbours already
 * holding a 0x15 (0xAD6), then place an item whose code = GM_TBL[0x6AF4+((rng ror5)&7)]
 * + dl through 0xAEE -> 0xB00.  Item code low-3-bits select the placer sub-mode:
 * 0/1/5/7 = plain item, 2 = timed/counted item (0xB35), 3 = a creature nest (0xB73)
 * that walks a trail, writes the nest anchor + a 0x2C marker, and spawns creatures
 * via 0xFCB/0xFE2.  Verified byte-for-byte against GM (m1/m2/m3). */

/* GM 0x15F2: stamp a type-0x2C marker at (cl,ch) and append a 4-byte record
 * {cx, al, dl} to the buffer at word[0x3582] (indexed by word[0x354C]). */
static void gm_15f2(CaptiveGmWork *w, uint8_t cl, uint8_t ch, uint16_t cx,
                    uint8_t al, uint8_t dl, uint8_t dh) {
    uint16_t bp = captive_gm_map_index(cl, ch);
    w->b[(uint16_t)(GM_MAP_TYPE + bp + 1u)] = dh;               /* byte[bp+si+1] = dh */
    w->b[(uint16_t)(GM_MAP_TYPE + bp)] = 0x2Cu;                 /* byte[bp+si]   = 0x2C */
    uint16_t t = (uint16_t)(captive_gm_wget(w, 0x3582u) + (captive_gm_wget(w, 0x354Cu) << 2));
    captive_gm_wset(w, t, cx);
    w->b[(uint16_t)(t + 2u)] = al;
    w->b[(uint16_t)(t + 3u)] = dl;
    captive_gm_wset(w, 0x354Cu, (uint16_t)(captive_gm_wget(w, 0x354Cu) + 1u));
}

/* GM 0xB73 (0xB00 dl==3): place a creature nest.  Returns 1 on success (GM ZF=1),
 * 0 on any early bail (GM `or ax,0xffff`). */
static int gm_b73(CaptiveGmWork *w, uint8_t al_item, uint16_t cx) {
    uint8_t cl = (uint8_t)cx, ch = (uint8_t)(cx >> 8);
    if (captive_gm_wget(w, 0x354Cu) >= 0x64u) return 0;               /* 0xB73 */
    { uint16_t bp = captive_gm_wget(w, 0x3588u);
      if (captive_gm_wget(w, (uint16_t)(bp + 2u)) <= 0x14u) return 0; } /* 0xB81 ja */
    if (captive_gm_wget(w, 0x354Au) >= 8u) return 0;                  /* 0xB8B */
    captive_gm_wset(w, 0x33A8u, al_item);                             /* save item code (ah=0) */
    captive_gm_wset(w, 0x33A2u, cl);                                  /* save orig cl */
    captive_gm_wset(w, 0x33B6u, ch);                                  /* save orig ch */
    gm_2a59(w, &cl, &ch);                                             /* 0xBA4 step */
    captive_gm_wset(w, 0x33AAu, cl);
    captive_gm_wset(w, 0x33BEu, ch);
    if ((int8_t)ch < 0) return 0;                                    /* 0xBB3 */
    if (captive_gm_cell_check(w, GM_MAP_SEL, cl, ch) != CAPTIVE_GM_CELL_VALID) return 0; /* 0xBBA */
    if (!gm_2468(w, cl, ch)) return 0;                              /* 0xBC4 */
    uint8_t dh;
    if (!gm_168d(w, &cl, &ch, &dh)) {                               /* 0xBCC */
        gm_2a59(w, &cl, &ch);                                       /* 0xBD1 second step */
        captive_gm_wset(w, 0x33AAu, cl);
        captive_gm_wset(w, 0x33BEu, ch);
        if ((int8_t)ch < 0) return 0;                              /* 0xBE0 */
        if (captive_gm_cell_check(w, GM_MAP_SEL, cl, ch) != CAPTIVE_GM_CELL_VALID) return 0; /* 0xBE7 */
        if (!gm_2468(w, cl, ch)) return 0;                        /* 0xBF1 */
        if (!gm_168d(w, &cl, &ch, &dh)) return 0;                 /* 0xBF9 */
    }
    /* 0xC01: cl,ch = 168d-found empty cell; dh = its direction */
    captive_gm_wset(w, 0x33BCu, dh);                                /* save dir */
    captive_gm_wset(w, 0x33B8u, ch);                               /* save found ch */
    captive_gm_wset(w, 0x33A4u, cl);                               /* save found cl */
    uint16_t bxcnt = (uint16_t)((captive_gm_wget(w, 0x3074u) & 0xFu) + 3u); /* trail length */
    cl = (uint8_t)captive_gm_wget(w, 0x33A2u);                     /* restore orig */
    ch = (uint8_t)captive_gm_wget(w, 0x33B6u);
    gm_2a59(w, &cl, &ch);                                          /* 0xC24 */
    if ((int8_t)ch < 0) return 0;                                 /* 0xC27 */
    for (;;) {                                                     /* 0xC2E walk */
        gm_2a59(w, &cl, &ch);
        if ((int8_t)ch < 0) return 0;                            /* 0xC31 */
        if (captive_gm_cell_check(w, GM_MAP_SEL, cl, ch) != CAPTIVE_GM_CELL_VALID) continue; /* 0xC38 */
        bxcnt = (uint16_t)(bxcnt - 1u);                          /* 0xC3F dec bx; jns */
        if ((int16_t)bxcnt < 0) break;
    }
    if (!gm_2468(w, cl, ch)) return 0;                            /* 0xC42 */
    w->b[0x33A6u] = cl;                                            /* 0xC4A walk-end */
    w->b[0x33BAu] = ch;
    /* 0xC52: write the nest anchor at the original cell */
    cl = (uint8_t)captive_gm_wget(w, 0x33A2u);
    ch = (uint8_t)captive_gm_wget(w, 0x33B6u);
    { uint16_t bp = captive_gm_map_index(cl, ch);
      uint8_t ah = w->b[0x33A8u];
      captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + bp), (uint16_t)(((uint16_t)ah << 8) | 0x15u)); } /* 0xC63 */
    /* 0xC66: mark the found empty cell in the selector map + a 0x2C record */
    cl = (uint8_t)captive_gm_wget(w, 0x33A4u);
    ch = (uint8_t)captive_gm_wget(w, 0x33B8u);
    { uint16_t bp = captive_gm_map_index(cl, ch);
      captive_gm_wset(w, (uint16_t)(GM_MAP_SEL + bp), 0xFFC3u); }   /* 0xC71 */
    uint8_t dl_lo = (uint8_t)captive_gm_wget(w, 0x354Au);           /* dx = word[0x354A], dh=0 */
    captive_gm_wset(w, 0x3568u, 0x65u);
    captive_gm_wset(w, 0x356Au, dl_lo);
    uint16_t dir = captive_gm_wget(w, 0x33BCu);
    uint8_t dh2 = w->b[(uint16_t)(dir + 0x6AFCu)];                  /* table_6AFC[dir] */
    gm_15f2(w, cl, ch, (uint16_t)(((uint16_t)ch << 8) | cl), 1u, dl_lo, dh2); /* 0xC90 */
    /* 0xC93: spawn creatures at the walk-end */
    cl = w->b[0x33A6u]; ch = w->b[0x33BAu];
    captive_gm_wset(w, 0x36u, 0u);                                  /* 0xCA1 */
    captive_gm_wset(w, 0x3564u, (uint16_t)(captive_gm_wget(w, 0x3564u) & 0xEFu));
    captive_gm_wset(w, 0x3566u, 0xFFu);
    { uint16_t dxv = (uint16_t)(((uint16_t)dh2 << 8) | dl_lo);
      gm_fcb(w, (uint16_t)(((uint16_t)ch << 8) | cl), 5u, dxv); }   /* 0xCB3 */
    /* 0xCB6: high-bit the walk-end type, floor the stepped cell */
    cl = w->b[0x33A6u]; ch = w->b[0x33BAu];
    w->b[(uint16_t)(GM_MAP_TYPE + captive_gm_map_index(cl, ch))] |= 0x80u; /* 0xCC1 */
    cl = (uint8_t)captive_gm_wget(w, 0x33AAu);
    ch = (uint8_t)captive_gm_wget(w, 0x33BEu);
    w->b[(uint16_t)(GM_MAP_TYPE + captive_gm_map_index(cl, ch))] = 0x0Fu;  /* 0xCD0 */
    /* 0xCD4: append a 4-cell record to buffer word[0x3594] */
    { uint16_t bx = (uint16_t)(captive_gm_wget(w, 0x3594u) + (captive_gm_wget(w, 0x354Au) << 2));
      w->b[bx]        = (uint8_t)captive_gm_wget(w, 0x33A4u);
      w->b[(uint16_t)(bx + 1u)] = (uint8_t)captive_gm_wget(w, 0x33B8u);
      w->b[(uint16_t)(bx + 2u)] = (uint8_t)captive_gm_wget(w, 0x33A2u);
      w->b[(uint16_t)(bx + 3u)] = (uint8_t)captive_gm_wget(w, 0x33B6u); }
    captive_gm_wset(w, 0x354Au, (uint16_t)(captive_gm_wget(w, 0x354Au) + 1u));
    return 1;                                                       /* 0xD03 cmp ax,ax */
}

/* GM 0xB35 (0xB00 dl==2, and dl==4 with do_check=0): place a counted item and record
 * a slot in buffer word[0x3584]. */
static int gm_b35(CaptiveGmWork *w, uint8_t al, uint16_t cx, uint16_t bp_rng,
                  int do_check, uint16_t bx) {
    uint8_t dh = w->b[0x3540u];
    if (do_check && dh == 0u) return 1;                            /* 0xB70 no-op success */
    w->b[bx] = 0x15u; w->b[(uint16_t)(bx + 1u)] = al;              /* 0xB3D gm_b18 */
    captive_gm_wset(w, 0x3540u, (uint16_t)(captive_gm_wget(w, 0x3540u) - 1u));
    uint16_t axoff = (uint16_t)((uint8_t)(7u - dh) * 4u);
    uint16_t di = (uint16_t)(captive_gm_wget(w, 0x3584u) + axoff);
    uint16_t axv = (uint16_t)((uint16_t)(((uint32_t)0x18u * bp_rng) >> 16) << 4); /* (0x18*bp).hi << 4 */
    captive_gm_wset(w, di, cx);
    axv = (uint16_t)((axv >> 8) | (axv << 8));                     /* xchg ah,al */
    captive_gm_wset(w, (uint16_t)(di + 2u), axv);
    return 1;
}

/* GM 0xB00: the item placer.  al = item code, cx = packed (cl,ch), bp_rng = the
 * caller's RNG (used by the dl==2/3 sub-modes).  Returns 1 on success (GM ZF=1). */
static int gm_b00(CaptiveGmWork *w, uint8_t al, uint16_t cx, uint16_t bp_rng) {
    uint8_t cl = (uint8_t)cx, ch = (uint8_t)(cx >> 8);
    uint16_t bx = (uint16_t)(GM_MAP_TYPE + captive_gm_map_index(cl, ch));
    uint8_t dl = (uint8_t)(al & 7u);
    if (dl == 2u) return gm_b35(w, al, cx, bp_rng, 1, bx);
    if (dl == 3u) return gm_b73(w, al, cx);
    if (dl == 4u) return gm_b35(w, al, cx, bp_rng, 0, bx);
    captive_gm_wset(w, 0x3536u, (uint16_t)(captive_gm_wget(w, 0x3536u) + 1u)); /* 0xB14 */
    w->b[bx] = 0x15u; w->b[(uint16_t)(bx + 1u)] = al;
    return 1;
}

/* GM 0xAEE: dispatch to 0xB00 for missions != 0 (mission 0 uses a simplified place). */
static int gm_aee(CaptiveGmWork *w, uint8_t al, uint16_t cx, uint16_t bp_rng) {
    if (captive_gm_wget(w, 0x3078u) != 0u) return gm_b00(w, al, cx, bp_rng); /* 0xAF3 */
    if ((al & 7u) != 0u) al = (uint8_t)((al & 0xF8u) | 5u);        /* 0xAF5 */
    uint8_t cl = (uint8_t)cx, ch = (uint8_t)(cx >> 8);
    uint16_t bx = (uint16_t)(GM_MAP_TYPE + captive_gm_map_index(cl, ch));
    captive_gm_wset(w, 0x3536u, (uint16_t)(captive_gm_wget(w, 0x3536u) + 1u));
    w->b[bx] = 0x15u; w->b[(uint16_t)(bx + 1u)] = al;
    return 1;
}

/* GM 0xAB4: classify (cl,ch) for placement.  The caller branches on the FLAGS left
 * by 0xAB4 (js -> break, jne -> step), not on ax, so this returns the flag outcome:
 *   -1 = break (GM SF=1), 0 = step to neighbour (GM ZF=0, SF=0), 1 = place here.
 * *pdl receives the sub-mode bias (0 or 8). */
static int gm_ab4(CaptiveGmWork *w, uint8_t cl, uint8_t ch, uint8_t *pdl) {
    uint8_t mask = gm_1bf5(w, cl, ch);
    *pdl = 0u;
    if (mask & 4u) return -1;                                      /* 0xAD2: or ax,0xffff -> SF=1 */
    if (mask != 0x0Au && mask != 0x11u)                           /* 0xAD1: flags = cmp mask,0x11 */
        return (mask < 0x11u) ? -1 : 0;                          /* SF=1 -> break; SF=0 -> step */
    if (mask == 0x11u) *pdl = 8u;                                 /* 0xAC7 */
    uint8_t t = w->b[(uint16_t)(GM_MAP_TYPE + captive_gm_map_index(cl, ch))]; /* 0xACE cmp byte,7 */
    if (t < 7u) return -1;                                        /* SF=1 -> break */
    return (t == 7u) ? 1 : 0;                                     /* ZF=1 -> place; else step */
}

/* GM 0xAD6: return 1 if any 4-neighbour of (cl,ch) already holds a type-0x15 item. */
static int gm_ad6(CaptiveGmWork *w, uint8_t cl, uint8_t ch) {
    uint16_t bx = (uint16_t)(GM_MAP_TYPE + captive_gm_map_index(cl, ch));
    if (w->b[(uint16_t)(bx + 2u)] == 0x15u) return 1;
    if (w->b[(uint16_t)(bx - 2u)] == 0x15u) return 1;
    if (w->b[(uint16_t)(bx - 0x80u)] == 0x15u) return 1;
    if (w->b[(uint16_t)(bx + 0x80u)] == 0x15u) return 1;
    return 0;
}

void captive_gm_pass_a2a_firstloop(CaptiveGmWork *w) {
    /* First loop: 25 records from word[0x3586]. */
    uint16_t bx = captive_gm_wget(w, 0x3586u);
    for (int cnt = 0x18; cnt >= 0; --cnt) {
        uint16_t cx = captive_gm_wget(w, bx); bx = (uint16_t)(bx + 2u);
        if ((int8_t)(uint8_t)cx < 0) continue;                     /* 0xA3A or cl,cl; js */
        uint16_t bp = captive_gm_map_index((uint8_t)cx, (uint8_t)(cx >> 8));
        uint8_t dl = w->b[(uint16_t)(GM_MAP_TYPE + bp + 2u)];       /* east-neighbour type */
        uint8_t code = 9u;
        if (dl > 0x19u && dl < 0x1Eu) code = 1u;                    /* 0x1A..0x1D */
        gm_b00(w, code, cx, 0u);
    }
}

void captive_gm_pass_a2a(CaptiveGmWork *w) {
    captive_gm_pass_a2a_firstloop(w);
    /* Second loop: 300 RNG-seeded probe walks. */
    for (int outer = 0x12B; outer >= 0; --outer) {
        uint16_t r = captive_gm_rng_next(w);                       /* 0xA67 */
        r = (uint16_t)((r >> 1) | (r << 15));                      /* ror ax,1 */
        uint16_t x = (uint16_t)(r & 0x3Fu);                        /* 0x1C9A pos(ax) */
        uint16_t ror6 = (uint16_t)((r >> 6) | (r << 10));
        uint16_t y = (uint16_t)(ror6 & 0x1Fu);
        uint8_t cl = (uint8_t)x, ch = (uint8_t)y;
        for (int inner = 0x10; inner >= 0; --inner) {
            uint8_t dl;
            int st = gm_ab4(w, cl, ch, &dl);                       /* 0xA76 */
            if (st < 0) break;                                    /* js 0xAAF */
            if (st > 0) {                                         /* jne skips to step */
                if (gm_ad6(w, cl, ch)) break;                    /* 0xA80 je 0xAAF */
                uint16_t rr = captive_gm_rng_next(w);            /* 0xA82 */
                uint16_t idx = (uint16_t)(gm_ror16(rr, 5) & 7u);
                uint8_t code = (uint8_t)(w->b[(uint16_t)(idx + 0x6AF4u)] + dl);
                if (gm_aee(w, code, (uint16_t)(((uint16_t)ch << 8) | cl), rr)) break; /* 0xA9F je */
            }
            gm_2a59(w, &cl, &ch);                                 /* 0xAA1 */
            if ((int8_t)ch < 0) break;                            /* 0xAA4 js 0xAAF */
        }
    }
}

/* ==== Post-A2A decoration/spawn group (orchestrator 0x417..0x43A) ==== */

/* GM 0x2595: over the 0x800 selector-map words at di, replace every word == bp with ax.
 * The orchestrator calls it with di=0x38, bp=0xFFC3, ax=0 (clear the nest markers). */
void captive_gm_pass_2595(CaptiveGmWork *w, uint16_t bpval, uint16_t axval) {
    uint16_t di = GM_MAP_SEL;
    for (int cx = 0x800; cx > 0; --cx) {
        if (captive_gm_wget(w, di) == bpval) captive_gm_wset(w, di, axval);
        di = (uint16_t)(di + 2u);
    }
}

/* GM 0x245B: draw a random cell (0x1C97) into (cl,ch); return 1 (GM ZF=1) when it is a
 * valid floor (0x2468) or already type 0x28. */
static int gm_245b(CaptiveGmWork *w, uint8_t *pcl, uint8_t *pch) {
    uint16_t cx = captive_gm_rng_pos(w);
    uint8_t cl = (uint8_t)cx, ch = (uint8_t)(cx >> 8);
    *pcl = cl; *pch = ch;
    if (gm_2468(w, cl, ch)) return 1;
    return (w->b[(uint16_t)(GM_MAP_TYPE + captive_gm_map_index(cl, ch))] == 0x28u);
}

/* GM 0x2346: chest placer.  If 0x245B says the drawn cell is valid, place a chest
 * (type 0x22, high byte = 2*direction) at a 0x168D-found neighbour; else if the cell
 * already holds a chest, extend it.  Then insert the chest contents via 0x122C.
 * Returns 1 on success (GM ZF=1). */
static int gm_2346(CaptiveGmWork *w) {
    uint8_t cl, ch;
    int z = gm_245b(w, &cl, &ch);
    if (z) {
        uint8_t dh;
        if (!gm_168d(w, &cl, &ch, &dh)) return 0;         /* 0x235D jne 0x2377 */
        dh = (uint8_t)(dh << 1);
        uint16_t bp = captive_gm_map_index(cl, ch);
        captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + bp), (uint16_t)(((uint16_t)dh << 8) | 0x22u));
    } else {
        uint16_t bp = captive_gm_map_index(cl, ch);
        if (w->b[(uint16_t)(GM_MAP_TYPE + bp)] != 0x22u) return 0;  /* 0x2352 jne 0x2377 */
    }
    uint16_t dx = captive_gm_wget(w, 0x356Au);            /* 0x2366 */
    uint8_t al = w->b[0x3568u];
    w->b[0x356Eu] = 0xFFu;
    gm_1226_body(w, (uint16_t)(((uint16_t)ch << 8) | cl), al, (uint8_t)(dx >> 8), (uint8_t)dx);
    return 1;
}

/* GM 0x9C3: place up to word[0x3078] (capped 0x14) type-0x16 items.  Each iteration
 * seeds a probe walk and steps up to 19 times looking for a type-7 junction cell not
 * adjacent to a 0x15 item. */
void captive_gm_pass_9c3(CaptiveGmWork *w) {
    uint16_t bxo = captive_gm_wget(w, 0x3078u);
    if (bxo == 0u) return;
    if (bxo > 0xAu) bxo = 0x14u;
    for (; (int16_t)bxo >= 0; --bxo) {
        uint16_t r = gm_ror16(captive_gm_rng_next(w), 4);
        uint8_t cl = (uint8_t)(r & 0x3Fu), ch = (uint8_t)(((r >> 6) | (r << 10)) & 0x1Fu);
        gm_2a59(w, &cl, &ch);
        if ((uint8_t)(cl + ch) & 1u) gm_2a59(w, &cl, &ch);
        for (int bi = 0x12; bi >= 0; --bi) {
            uint16_t bp = captive_gm_map_index(cl, ch);
            /* GM 0x2675 returns ZF=0 (proceed) iff the CENTRE cell's selector is VALID
             * (the neighbour count it also computes is not what 0x9C3 tests). */
            if (w->b[(uint16_t)(GM_MAP_TYPE + bp)] == 7u
                && captive_gm_cell_check(w, GM_MAP_SEL, cl, ch) == CAPTIVE_GM_CELL_VALID) {
                if (!gm_ad6(w, cl, ch)) {
                    captive_gm_wset(w, 0x354Eu, (uint16_t)(captive_gm_wget(w, 0x354Eu) + 1u));
                    w->b[(uint16_t)(GM_MAP_TYPE + bp)] = 0x16u;
                }
                break;
            }
            gm_2a59(w, &cl, &ch); gm_2a59(w, &cl, &ch);
        }
    }
}

/* GM 0x967: place type-0x1E markers.  For up to (mission<<2, capped 0x50) seeds, scan 5
 * cells rightward for a cell whose 0x1BF5 mask fits and whose selector/type are clear. */
void captive_gm_pass_967(CaptiveGmWork *w) {
    uint16_t bxo = captive_gm_wget(w, 0x3078u);
    if (bxo >= 0x15u) bxo = 0x50u; else bxo = (uint16_t)(bxo << 2);
    for (; (int16_t)bxo >= 0; --bxo) {
        uint16_t r = gm_ror16(captive_gm_rng_next(w), 4);
        uint8_t cl = (uint8_t)(r & 0x3Fu), ch = (uint8_t)(((r >> 6) | (r << 10)) & 0x1Fu);
        for (int bi = 4; bi >= 0; --bi) {
            uint8_t dl = gm_1bf5(w, cl, ch);
            if ((dl & 4u) && ((dl & 0x11u) == 0u || (dl & 0x0Au) == 0u)) {
                uint16_t bp = captive_gm_map_index(cl, ch);
                captive_gm_wset(w, 0x3550u, (uint16_t)(captive_gm_wget(w, 0x3550u) + 1u));
                if ((int16_t)captive_gm_wget(w, (uint16_t)(GM_MAP_SEL + bp)) >= 0
                    && w->b[(uint16_t)(GM_MAP_TYPE + bp)] == 0u) {
                    w->b[(uint16_t)(GM_MAP_TYPE + bp)] = 0x1Eu;
                    break;
                }
            }
            cl = (uint8_t)((cl + 1u) & 0x3Fu);
        }
    }
}

/* GM 0xF61: for 300 random cells whose type is a wall/feature and whose selector > 0xA,
 * spawn a wall creature/item via 0xFE2 (0xFD2) and set the type high bit.  A spawn
 * failure (0xFFFF) ends the pass early. */
void captive_gm_pass_f61(CaptiveGmWork *w) {
    for (int bx = 0x12B; bx >= 0; --bx) {
        uint16_t cx = captive_gm_rng_pos(w);
        uint16_t bp = captive_gm_map_index((uint8_t)cx, (uint8_t)(cx >> 8));
        uint8_t dl = w->b[(uint16_t)(GM_MAP_TYPE + bp)];
        if (dl == 0u) continue;
        if (captive_gm_wget(w, (uint16_t)(GM_MAP_SEL + bp)) <= 0xAu) continue;
        uint16_t sbx = 0u;
        int match = (dl == 7u || dl == 0x0Fu || dl == 0x0Cu || dl == 0x1Fu || dl == 0x33u
                     || dl == 9u || dl == 0x11u || dl == 0x14u);
        if (dl == 0x23u) { sbx = 0x40u; match = 1; }
        if (!match) continue;
        captive_gm_wset(w, 0x36u, 0u);
        w->b[0x3564u] &= 0xEFu;
        captive_gm_wset(w, 0x3566u, 0u);
        if (gm_fe2(w, cx, sbx, 0u) == 0xFFFFu) return;             /* 0xFD2 -> 0xFE2 */
        w->b[(uint16_t)(GM_MAP_TYPE + bp)] |= 0x80u;
    }
}

/* GM 0x2284: place up to (0x165B(mission)+0xA) floor items, type 0x2A over existing
 * 0x28 else 0x24; stops after 0xB placed. */
void captive_gm_pass_2284(CaptiveGmWork *w) {
    uint16_t bx = captive_gm_wget(w, 0x3078u);
    if (bx <= 5u) return;
    bx = (uint16_t)(gm_165b(w, bx) + 0xAu);
    for (; (int16_t)bx >= 0; --bx) {
        uint8_t cl, ch;
        if (!gm_245b(w, &cl, &ch)) continue;
        uint16_t bp = captive_gm_map_index(cl, ch);
        w->b[(uint16_t)(GM_MAP_TYPE + bp)] = (w->b[(uint16_t)(GM_MAP_TYPE + bp)] == 0x28u) ? 0x2Au : 0x24u;
        captive_gm_wset(w, 0x3546u, (uint16_t)(captive_gm_wget(w, 0x3546u) + 1u));
        if (captive_gm_wget(w, 0x3546u) >= 0xBu) return;
    }
}

/* GM 0x22BA: attempt up to 0x8D chest placements (0x2346) at group 0x19; count in
 * word[0x3532], stop after 0x15. */
void captive_gm_pass_22ba(CaptiveGmWork *w) {
    captive_gm_wset(w, 0x3532u, 0u);
    for (int bx = 0x8C; bx >= 0; --bx) {
        captive_gm_wset(w, 0x356Au, (uint16_t)(w->b[0x3074u] + 0x40u));
        captive_gm_wset(w, 0x3568u, 0x19u);
        if (gm_2346(w)) {
            captive_gm_wset(w, 0x3532u, (uint16_t)(captive_gm_wget(w, 0x3532u) + 1u));
            if (captive_gm_wget(w, 0x3532u) >= 0x15u) return;
        }
    }
}

/* GM 0x22EB: place a single chest of group 0x5F (mission != 0), up to 0x65 attempts. */
void captive_gm_pass_22eb(CaptiveGmWork *w) {
    if (captive_gm_wget(w, 0x3078u) == 0u) return;
    for (int bx = 0x64; bx >= 0; --bx) {
        captive_gm_wset(w, 0x3568u, 0x5Fu);
        captive_gm_wset(w, 0x356Au, 0u);
        if (gm_2346(w)) return;
    }
}

/* GM 0x2310: place up to 6 chests of group 0x61 (mission != 0), up to 0x15 attempts. */
void captive_gm_pass_2310(CaptiveGmWork *w) {
    if (captive_gm_wget(w, 0x3078u) == 0u) return;
    uint8_t bh = 0u;
    for (int bl = 0x14; bl >= 0; --bl) {
        captive_gm_wset(w, 0x3568u, 0x61u);
        captive_gm_wset(w, 0x356Au, 0u);
        if (gm_2346(w)) { bh = (uint8_t)(bh + 1u); if (bh >= 6u) return; }
    }
}

/* GM 0x2400: place up to ((0x165B(mission)+0xA)<<1) floor items, type 0x2B over 0x28
 * else 0x25. */
void captive_gm_pass_2400(CaptiveGmWork *w) {
    uint16_t bx = captive_gm_wget(w, 0x3078u);
    if (bx <= 1u) return;
    bx = (uint16_t)((gm_165b(w, bx) + 0xAu) << 1);
    for (; (int16_t)bx >= 0; --bx) {
        uint8_t cl, ch;
        if (!gm_245b(w, &cl, &ch)) continue;
        uint16_t bp = captive_gm_map_index(cl, ch);
        w->b[(uint16_t)(GM_MAP_TYPE + bp)] = (w->b[(uint16_t)(GM_MAP_TYPE + bp)] == 0x28u) ? 0x2Bu : 0x25u;
        captive_gm_wset(w, 0x3548u, (uint16_t)(captive_gm_wget(w, 0x3548u) + 1u));
    }
}

/* GM 0x242E: place up to (mission+0xA) type-0x26 items on valid non-floor cells; stops
 * after 7 placed. */
void captive_gm_pass_242e(CaptiveGmWork *w) {
    uint16_t bx = captive_gm_wget(w, 0x3078u);
    if (bx <= 2u) return;
    bx = (uint16_t)(bx + 0xAu);
    for (; (int16_t)bx >= 0; --bx) {
        uint16_t cx = captive_gm_rng_pos(w);
        uint8_t cl = (uint8_t)cx, ch = (uint8_t)(cx >> 8);
        if (!gm_2468(w, cl, ch)) continue;
        uint16_t bp = captive_gm_map_index(cl, ch);
        if (w->b[(uint16_t)(GM_MAP_TYPE + bp)] == 7u) continue;
        w->b[(uint16_t)(GM_MAP_TYPE + bp)] = 0x26u;
        captive_gm_wset(w, 0x3544u, (uint16_t)(captive_gm_wget(w, 0x3544u) + 1u));
        if (captive_gm_wget(w, 0x3544u) > 7u) return;
    }
}

/* ==== Final decoration group (orchestrator 0x43A..0x452) ==== */

/* GM 0x1548: classify a neighbour cell for 0x1513.  (The `jmp 0x683` targets are a
 * shared `ret` — they return from 0x1548, not the pass.)  Returns 1 = ok (GM ZF=0,
 * *pal = contribution: 1 for EMPTY/BLOCKED/0x23, else 0), 0 = reject (GM ZF=1: a
 * 0x15/0x1E/0x12/0x13 type). */
static int gm_1548(CaptiveGmWork *w, uint8_t cl, uint8_t ch, uint8_t *pal) {
    CaptiveGmCellStatus s = captive_gm_cell_check(w, GM_MAP_SEL, cl, ch);
    if (s == CAPTIVE_GM_CELL_EMPTY || s == CAPTIVE_GM_CELL_BLOCKED) { *pal = 1u; return 1; }
    uint8_t t = w->b[(uint16_t)(GM_MAP_TYPE + captive_gm_map_index(cl, ch))];
    if (t == 0x23u) { *pal = 1u; return 1; }
    if (t == 0x15u || t == 0x1Eu || t == 0x12u || t == 0x13u) return 0;
    *pal = 0u; return 1;
}

/* GM 0x1513: the centre cell must be EMPTY; then sum the 4 neighbours via 0x1548.
 * Returns 1 = accept (GM ZF=0, *pdl = neighbour sum), 0 = reject. */
static int gm_1513(CaptiveGmWork *w, uint8_t cl, uint8_t ch, uint8_t *pdl) {
    *pdl = 0;
    if (captive_gm_cell_check(w, GM_MAP_SEL, cl, ch) != CAPTIVE_GM_CELL_EMPTY) return 0;
    static const int8_t dx[4] = { -1, +1, 0, 0 }, dy[4] = { 0, 0, -1, +1 };
    uint8_t dl = 0;
    for (int i = 0; i < 4; ++i) {
        uint8_t al;
        if (!gm_1548(w, (uint8_t)(cl + dx[i]), (uint8_t)(ch + dy[i]), &al)) return 0;
        dl = (uint8_t)(dl + al);
    }
    *pdl = dl;
    return 1;
}

/* GM 0x157E: scatter up to 0x64 type-0x2C markers.  For each of 0x64 outer slots, probe
 * up to 0x65 random cells for an EMPTY cell whose 0x1513 classification accepts and
 * whose neighbour-sum != 4, then record it (0x15F2) with a wall-mask high byte and an
 * item id = (rng_high mod 6, avoiding 1). */
void captive_gm_pass_157e(CaptiveGmWork *w) {
    for (int outer = 0x63; outer >= 0; --outer) {
        for (int bx = 0x64; bx >= 0; --bx) {
            uint16_t r = captive_gm_rng_next(w);                    /* 0x1C97 */
            captive_gm_wset(w, 0x33E4u, r);
            uint8_t cl = (uint8_t)(r & 0x3Fu), ch = (uint8_t)(((r >> 6) | (r << 10)) & 0x1Fu);
            uint8_t dl;
            if (!gm_1513(w, cl, ch, &dl)) continue;                 /* 0x158E je 0x15A0 */
            if (dl == 4u) continue;                                 /* 0x1590 */
            if (captive_gm_cell_check(w, GM_MAP_SEL, cl, ch) != CAPTIVE_GM_CELL_EMPTY) continue; /* 0x1595 */
            uint16_t bp = captive_gm_map_index(cl, ch);
            if (captive_gm_wget(w, (uint16_t)(GM_MAP_TYPE + bp)) != 0u) continue;  /* 0x159A */
            uint8_t mask = gm_1bf5(w, cl, ch);                      /* 0x15A5 */
            uint8_t dh = 0;
            if (!(mask & 0x10u)) dh |= 4u;
            if (!(mask & 0x08u)) dh |= 2u;
            if (!(mask & 0x02u)) dh |= 8u;
            if (!(mask & 0x01u)) dh |= 1u;
            uint16_t av = (uint16_t)(r >> 8);                       /* al=ah; ah=0 */
            while (av > 5u) av -= 6u;
            if (av == 1u) av += 1u;
            gm_15f2(w, cl, ch, (uint16_t)(((uint16_t)ch << 8) | cl), (uint8_t)av, 0u, dh);
            if (captive_gm_wget(w, 0x354Cu) >= 0x64u) return;       /* 0x15E7 */
            break;                                                  /* placed -> next outer */
        }
    }
}

/* GM 0x13E3: pair up stairs.  For up to (mission, capped 0x28) seeds, scan a row for a
 * type-4/0x35 cell (step forward to its 5/0x36 partner) or a type-5/0x36 cell (step
 * back to its 4/0x35 partner), and rewrite the pair to 0x2E/0x2F. */
void captive_gm_pass_13e3(CaptiveGmWork *w) {
    uint16_t bx = captive_gm_wget(w, 0x3078u);
    if (bx > 0x28u) bx = 0x28u;
    for (; (int16_t)bx >= 0; --bx) {
        uint16_t cx = captive_gm_rng_pos(w);
        uint8_t cl = (uint8_t)cx, ch = (uint8_t)(cx >> 8);
        for (;;) {
            uint16_t bp = captive_gm_map_index(cl, ch);
            uint8_t t = w->b[(uint16_t)(GM_MAP_TYPE + bp)];
            if (t == 4u || t == 0x35u) {                            /* 0x1419 */
                uint8_t scl = cl, sch = ch, dh = captive_gm_grid_cell(w, cl, ch);
                gm_h286e(w, &scl, &sch, &dh);                       /* 0x2A89 step forward */
                uint16_t bp2 = captive_gm_map_index(scl, sch);
                uint8_t t2 = w->b[(uint16_t)(GM_MAP_TYPE + bp2)];
                if (t2 == 0x36u || t2 == 5u) {
                    w->b[(uint16_t)(GM_MAP_TYPE + bp2)] = 0x2Fu;
                    w->b[(uint16_t)(GM_MAP_TYPE + bp)] = 0x2Eu;
                    captive_gm_wset(w, 0x3552u, (uint16_t)(captive_gm_wget(w, 0x3552u) - 1u));
                    captive_gm_wset(w, 0x3554u, (uint16_t)(captive_gm_wget(w, 0x3554u) + 1u));
                }
                break;
            }
            if (t == 5u || t == 0x36u) {                            /* 0x1441 */
                uint8_t scl = cl, sch = ch, dh = captive_gm_grid_cell(w, cl, ch);
                gm_2854(w, &scl, &sch, &dh);                        /* 0x2A93 step back */
                uint16_t bp2 = captive_gm_map_index(scl, sch);
                uint8_t t2 = w->b[(uint16_t)(GM_MAP_TYPE + bp2)];
                if (t2 == 0x35u || t2 == 4u) {
                    w->b[(uint16_t)(GM_MAP_TYPE + bp2)] = 0x2Eu;
                    w->b[(uint16_t)(GM_MAP_TYPE + bp)] = 0x2Fu;
                    captive_gm_wset(w, 0x3552u, (uint16_t)(captive_gm_wget(w, 0x3552u) - 1u));
                    captive_gm_wset(w, 0x3554u, (uint16_t)(captive_gm_wget(w, 0x3554u) + 1u));
                }
                break;
            }
            cl = (uint8_t)(cl + 1u);
            if (cl > 0x3Fu) break;                                  /* 0x1410 cmp cl,0x3f */
        }
    }
}

/* GM 0x237F: place up to ((mission-5, capped 0x3C)>>1) type-0x32 items on type-7 floor
 * cells, redrawing while the cell is empty. */
void captive_gm_pass_237f(CaptiveGmWork *w) {
    uint16_t bx = captive_gm_wget(w, 0x3078u);
    if (bx <= 5u) return;
    bx = (uint16_t)(bx - 5u);
    if (bx > 0x3Cu) bx = 0x3Cu;
    bx >>= 1;
    for (; (int16_t)bx >= 0; --bx) {
        uint16_t bp;
        for (;;) {                                                 /* redraw while type word == 0 */
            uint16_t cx = captive_gm_rng_pos(w);
            bp = captive_gm_map_index((uint8_t)cx, (uint8_t)(cx >> 8));
            if (captive_gm_wget(w, (uint16_t)(GM_MAP_TYPE + bp)) != 0u) break;
        }
        if (captive_gm_wget(w, (uint16_t)(GM_MAP_TYPE + bp)) == 7u) {
            captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + bp), 0x32u);
            captive_gm_wset(w, 0x353Au, (uint16_t)(captive_gm_wget(w, 0x353Au) + 1u));
        }
    }
}

/* GM 0x23B4: place up to ((mission-3, capped 0x28)>>1) type-0x30/0x31 items on 0x1F/0x33
 * cells, or type-7 cells whose 0x1BF5 mask is 0. */
void captive_gm_pass_23b4(CaptiveGmWork *w) {
    uint16_t bx = captive_gm_wget(w, 0x3078u);
    if (bx <= 3u) return;
    bx = (uint16_t)(bx - 3u);
    if (bx > 0x28u) bx = 0x28u;
    bx >>= 1;
    for (; (int16_t)bx >= 0; --bx) {
        uint16_t r, bp; uint8_t cl, ch;
        for (;;) {                                                 /* redraw while type word == 0 */
            r = captive_gm_rng_next(w);
            cl = (uint8_t)(r & 0x3Fu); ch = (uint8_t)(((r >> 6) | (r << 10)) & 0x1Fu);
            bp = captive_gm_map_index(cl, ch);
            if (captive_gm_wget(w, (uint16_t)(GM_MAP_TYPE + bp)) != 0u) break;
        }
        uint8_t t = w->b[(uint16_t)(GM_MAP_TYPE + bp)];
        int place = 0; uint16_t axbit = 0;
        if (t == 0x1Fu || t == 0x33u) { place = 1; axbit = (uint16_t)(r & 1u); }
        else if (captive_gm_wget(w, (uint16_t)(GM_MAP_TYPE + bp)) == 7u) {
            if (gm_1bf5(w, cl, ch) == 0u) { place = 1; axbit = 0u; }  /* ax&1 == mask&1 == 0 */
        }
        if (place) {
            captive_gm_wset(w, (uint16_t)(GM_MAP_TYPE + bp), (uint16_t)(0x30u + axbit));
            captive_gm_wset(w, 0x3538u, (uint16_t)(captive_gm_wget(w, 0x3538u) + 1u));
        }
    }
}

/* GM 0x1460: mission-0 only (returns immediately when word[0x3078] != 0).  For mission
 * 0 it places the fixed objective cell (0x1E,1) as 0x34 (over 0x33) or 0x0D, then
 * inserts two spawn records (ids 0x1B then 0x66) via 0x1226, preserving the 2nd RNG
 * state across the first insert. */
void captive_gm_pass_1460(CaptiveGmWork *w) {
    if (captive_gm_wget(w, 0x3078u) != 0u) return;
    uint16_t bp = captive_gm_map_index(0x1Eu, 1u);
    w->b[(uint16_t)(GM_MAP_TYPE + bp)] = (w->b[(uint16_t)(GM_MAP_TYPE + bp)] == 0x33u) ? 0x34u : 0x0Du;
    captive_gm_wset(w, 0x356Eu, 0u);
    uint16_t cx = (uint16_t)((1u << 8) | 0x1Eu);
    uint16_t saved = captive_gm_wget(w, 0x355Cu);
    gm_1226(w, cx, 0x1Bu, 0u, 0u);
    captive_gm_wset(w, 0x355Cu, saved);
    gm_1226(w, cx, 0x66u, 0u, 0u);
}

void captive_gm_generate_output(CaptiveGmWork *w) {
    /* GM 0xEE: the final translate driver.  For each of the 2048 cells it reads the
     * cell type at word[0x1048+2k], the selector at word[0x38+2k], and the aux at
     * word[0x2058+2k], and writes:
     *   output map  (0x5A68+k) = translate(type_lo, type_hi, selector)
     *   second map  (0x6288+k) = aux_hi, expanded to (v<<3)|v unless the selector
     *                            is 0 or 0xFFFF (an empty cell). */
    uint16_t out = captive_gm_wget(w, 0x3578u);   /* 0x5A68 */
    uint16_t out2 = captive_gm_wget(w, 0x357Au);  /* 0x6288 */
    for (uint16_t k = 0; k < 0x800u; ++k) {
        uint16_t si = (uint16_t)(k * 2u);
        uint16_t type = captive_gm_wget(w, (uint16_t)(0x1048u + si));
        uint16_t ax = captive_gm_wget(w, (uint16_t)(0x0038u + si));
        w->b[(uint16_t)(out + k)] =
            captive_gm_translate_cell((uint8_t)(type & 0xFFu),
                                      (uint8_t)(type >> 8), ax);
        uint8_t bh = (uint8_t)(captive_gm_wget(w, (uint16_t)(0x2058u + si)) >> 8);
        if (ax != 0u && ax != 0xFFFFu)               /* GM 0x10C/0x10E */
            bh = (uint8_t)((bh << 3) | bh);
        w->b[(uint16_t)(out2 + k)] = bh;
    }
}

void captive_gm_run(CaptiveGmWork *w, uint16_t mission) {
    /* The whole GM.EXE dungeon generator: seed -> the full pass chain (0x3B1..0x45E) ->
     * the 0xEE translate driver.  Deterministic per mission param; on return the output
     * level map lives at work[0x5A68] (2048 bytes) and the second map at work[0x6288].
     * Verified byte-identical to the real GM.EXE (test_full_pipeline_output). */
    captive_gm_generate(w, mission, 0u, 0u, 0u);
}

void captive_gm_generate(CaptiveGmWork *w, uint16_t ax, uint16_t bx,
                         uint16_t cx, uint16_t dx) {
    /* Faithful transcription of GM.EXE's generation orchestrator (0x3B1..0x45E),
     * including its conditional branches, so it is correct for any mission-param set,
     * not just the (bx=cx=dx=0) case the oracle tests exercise. */
    captive_gm_init(w);
    captive_gm_entry_setup(w, ax, bx, cx, dx);
    captive_gm_seed(w);
    captive_gm_pass_14c9(w); captive_gm_pass_45f(w);
    captive_gm_pass_526(w); captive_gm_pass_5d4(w);
    captive_gm_wset(w, 0x3070u, 1u);                                   /* 0x3BD */
    { uint16_t rng = captive_gm_wget(w, 0x3074u);                      /* 0x3C9 push */
      captive_gm_pass_1cb5(w);
      captive_gm_wset(w, 0x3074u, rng); }                             /* 0x3D0 pop */
    captive_gm_pass_1617(w); captive_gm_pass_d12(w);
    captive_gm_pass_2589(w); captive_gm_pass_26be(w); captive_gm_pass_28b2(w);
    captive_gm_pass_29f6(w); captive_gm_pass_28b2(w); captive_gm_pass_2888(w);
    captive_gm_pass_164c(w); captive_gm_pass_2940(w);
    if (captive_gm_wget(w, 0x33DCu) & 1u) return;                      /* 0x3F2 early ret */
    if (captive_gm_wget(w, 0x307Cu) == 1u) captive_gm_pass_1314(w);    /* 0x3FB conditional */
    captive_gm_pass_e12(w);
    captive_gm_pass_1736(w); captive_gm_pass_1806(w); captive_gm_pass_2a9d(w);
    captive_gm_pass_2abc(w); captive_gm_pass_a2a(w);
    captive_gm_pass_2595(w, 0xFFC3u, 0x0000u); captive_gm_pass_9c3(w);
    captive_gm_pass_967(w); captive_gm_pass_f61(w); captive_gm_pass_2284(w);
    captive_gm_pass_22ba(w); captive_gm_pass_22eb(w); captive_gm_pass_2310(w);
    captive_gm_pass_2400(w); captive_gm_pass_242e(w);
    captive_gm_pass_2595(w, 0xFFC4u, 0xFFF6u); captive_gm_pass_157e(w);
    captive_gm_pass_13e3(w); captive_gm_pass_237f(w); captive_gm_pass_23b4(w);
    captive_gm_pass_1460(w);
    captive_gm_generate_output(w);
}

void captive_gm_pass_5d4(CaptiveGmWork *w) {
    /* GM 0x5D4..0x640: expand the 4x4 room grid into the 2048-word input map at
     * work[0x38..].  Each of the 4 grid rows (dh) writes:
     *   Part1 (0x5DC): 7 x 4 groups of {15 zero words, then a horizontal-wall word
     *                  = 0xFFFF when the grid cell differs from its right neighbour}
     *   Part2 (0x608): 4 x 16 words = a vertical-wall word derived from the grid
     *                  cell vs the cell one grid-row down and the value one map-row
     *                  up (word[di-0x80]).
     * si advances by 4 (one grid row) each outer pass. */
    uint16_t si = 0u;
    uint16_t di = 0x38u;
    for (int dh = 0; dh < 4; ++dh) {
        for (int cxo = 7; cxo >= 1; --cxo) {            /* GM 0x5DC loop */
            for (int bp = 0; bp < 4; ++bp) {            /* GM 0x5E1 loop */
                for (int k = 0; k < 15; ++k) {          /* GM 0x5E7 rep stosw */
                    captive_gm_wset(w, di, 0u); di = (uint16_t)(di + 2u);
                }
                uint16_t val = 0u;                       /* GM 0x5EA */
                if (bp < 3 &&
                    w->b[(uint16_t)(bp + si + 1)] != w->b[(uint16_t)(bp + si)])
                    val = 0xFFFFu;
                captive_gm_wset(w, di, val); di = (uint16_t)(di + 2u);
            }
        }
        for (int bp = 0; bp < 4; ++bp) {                /* GM 0x608/0x60A loop */
            uint8_t dl = 0u;                             /* GM 0x60D */
            if (dh < 3 &&
                w->b[(uint16_t)(bp + si + 4)] != w->b[(uint16_t)(bp + si)])
                dl = 0xFFu;
            for (int cx2 = 0; cx2 < 16; ++cx2) {        /* GM 0x61F loop */
                uint16_t val;
                if (captive_gm_wget(w, (uint16_t)(di - 0x80u)) == 0u)
                    val = (uint16_t)(dl | (dl << 8));
                else
                    val = 0xFFFFu;
                captive_gm_wset(w, di, val); di = (uint16_t)(di + 2u);
            }
        }
        si = (uint16_t)(si + 4u);
    }
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
