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

void captive_gm_init(CaptiveGmWork *w) {
    memset(w, 0, sizeof(*w));
    memcpy(&w->b[0x6D16u], GM_TBL_6D16, sizeof(GM_TBL_6D16));
    memcpy(&w->b[0x6D20u], GM_TBL_6D20, sizeof(GM_TBL_6D20));
    memcpy(&w->b[0x6D2Eu], GM_TBL_6D2E, sizeof(GM_TBL_6D2E));
    memcpy(&w->b[0x6D36u], GM_TBL_6D36, sizeof(GM_TBL_6D36));
    memcpy(&w->b[0x6D3Eu], GM_TBL_6D3E, sizeof(GM_TBL_6D3E));
    memcpy(&w->b[0x6D4Eu], GM_TBL_6D4E, sizeof(GM_TBL_6D4E));
    memcpy(&w->b[0x6D56u], GM_TBL_6D56, sizeof(GM_TBL_6D56));
    memcpy(&w->b[0x6D5Eu], GM_TBL_6D5E, sizeof(GM_TBL_6D5E));
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
        uint32_t t = (uint32_t)ax * 0x05E5u + 0x0029u;
        ax = (uint16_t)t;
        if (t > 0xFFFFu) break;                        /* jb (carry) */
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
            if (mode == 1u && rch == 0u) goto abort;      /* 0x2118 */
            uint8_t dl = gm_2681(w, rcl, rch);            /* 0x2133 */
            if (captive_gm_wget(w, 0x3092u) != 0u) dl = (uint8_t)(dl - 1u);
            if (dl != 3u) goto abort;                     /* 0x2144 */
            captive_gm_wset(w, 0x3092u, (uint16_t)(captive_gm_wget(w, 0x3092u) + 1u));
            gm_226e(w, &rcl, &rch);
        }
        gm_2272(w, &vcl, &vch, 0);
    }

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

void captive_gm_pass_1617(CaptiveGmWork *w) {
    captive_gm_wset(w, 0x3510u, 1u);
    uint16_t bx = captive_gm_wget(w, 0x3078u);         /* mission */
    if (bx != 0u) {
        if ((bx & 2u) == 0u) return;                   /* GM 0x1625 */
        bx = (uint16_t)(~bx);
        bx = gm_165b(w, bx);
    }
    /* loop bx+1 times: draw one room from a random cell.  GM 0x1632/0x1647 wraps
     * the loop with push/pop word[0x3074], so the RNG use here is discarded. */
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
