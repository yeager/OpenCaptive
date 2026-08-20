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

void captive_gm_init(CaptiveGmWork *w) {
    memset(w, 0, sizeof(*w));
    memcpy(&w->b[0x6D16u], GM_TBL_6D16, sizeof(GM_TBL_6D16));
    memcpy(&w->b[0x6D20u], GM_TBL_6D20, sizeof(GM_TBL_6D20));
    memcpy(&w->b[0x6D2Eu], GM_TBL_6D2E, sizeof(GM_TBL_6D2E));
    memcpy(&w->b[0x6D36u], GM_TBL_6D36, sizeof(GM_TBL_6D36));
    memcpy(&w->b[0x6D3Eu], GM_TBL_6D3E, sizeof(GM_TBL_6D3E));
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
    for (int k = 0; k < 0x4B; ++k)
        captive_gm_wset(w, (uint16_t)(0x3430u + k * 2), 0xFFFFu);
    gm_h1cd8(w);
    uint16_t dst = captive_gm_wget(w, 0x3586u);
    for (int k = 0; k < 0x19; ++k)
        captive_gm_wset(w, (uint16_t)(dst + k * 2),
                        captive_gm_wget(w, (uint16_t)(0x3494u + k * 2)));
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
