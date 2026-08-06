#include "arcd_decoder.h"
#include <stdbool.h>
#include <limits.h>
#include <string.h>

typedef struct {
    const uint8_t *src;
    size_t src_len;
    size_t pos;
    uint32_t d6;
    uint8_t d7;
    uint8_t *dst;
    size_t dst_pos;
    size_t dst_len;
    bool input_truncated;
    uint8_t tables[384];
} ArcD;

static uint8_t read_src_byte(ArcD *s) {
    if (s->pos >= s->src_len) {
        s->input_truncated = true;
        return 0;
    }
    return s->src[s->pos++];
}

static uint8_t refill(ArcD *s, uint8_t d1) {
    s->d7 += d1;
    s->d6 >>= s->d7;
    d1 -= s->d7;
    s->d7 = 16 - d1;
    uint16_t lo = s->d6 & 0xFFFF;
    uint16_t hi = (s->d6 >> 16) & 0xFFFF;
    s->d6 = ((uint32_t)lo << 16) | hi;
    uint8_t b0 = read_src_byte(s);
    uint8_t b1 = read_src_byte(s);
    s->d6 = (s->d6 & 0xFFFF0000u) | ((uint32_t)b0 << 8) | b1;
    lo = s->d6 & 0xFFFF;
    hi = (s->d6 >> 16) & 0xFFFF;
    s->d6 = ((uint32_t)lo << 16) | hi;
    return d1;
}

static uint16_t get_bits(ArcD *s, uint16_t mask, uint8_t count) {
    uint16_t val = (uint16_t)(s->d6 & mask);
    s->d7 -= count;
    if (s->d7 & 0x80) {
        count = refill(s, count);
    }
    s->d6 >>= count;
    return val;
}

static bool build_table(ArcD *s, uint8_t *tbl, uint16_t sc) {
    /* The table has room for 32 code-length entries.  sc comes directly from
       the compressed stream, so reject malformed counts before indexing
       syms[] below. */
    if (sc == 0 || sc > 32) return false;
    memset(tbl, 0, 128);
    int n = sc - 1;

    uint16_t d3 = 0;
    uint8_t syms[32];
    for (int i = 0; i <= n; i++) {
        uint8_t sv = (uint8_t)get_bits(s, 0xF, 4);
        d3 |= (1u << sv);
        syms[i] = sv;
    }

    if (syms[n] == 0) {
        tbl[64] = 0;
        tbl[65] = (uint8_t)n;
        uint16_t em = (uint16_t)((n >= 2) ? ((1u << (n - 1)) - 1) : 0);
        tbl[66] = (em >> 8) & 0xFF;
        tbl[67] = em & 0xFF;
        return true;
    }

    int entry = 0;
    uint16_t d4_hi = 2, d4_lo = 0;

    for (int bl = 1; bl < 16; bl++) {
        if (!(d3 & (1u << bl))) {
            uint32_t d4_full = ((uint32_t)d4_hi << 16) | d4_lo;
            d4_full <<= 1;
            d4_hi = (d4_full >> 16) & 0xFFFF;
            d4_lo = d4_full & 0xFFFF;
            continue;
        }
        for (int si = 0; si <= n; si++) {
            if (syms[si] != bl) continue;
            if (entry >= 16) return false;
            uint16_t mask = d4_hi - 1;
            uint16_t rev = 0;
            uint16_t tmp = d4_lo;
            for (int b = 0; b < bl; b++) {
                rev = (uint16_t)((rev << 1) | (tmp & 1));
                tmp >>= 1;
            }
            tbl[entry * 4 + 0] = (mask >> 8) & 0xFF;
            tbl[entry * 4 + 1] = mask & 0xFF;
            tbl[entry * 4 + 2] = (rev >> 8) & 0xFF;
            tbl[entry * 4 + 3] = rev & 0xFF;
            tbl[64 + entry * 4 + 0] = (uint8_t)bl;
            tbl[64 + entry * 4 + 1] = (uint8_t)si;
            uint16_t em = (uint16_t)((si >= 2) ? ((1u << (si - 1)) - 1) : 0);
            tbl[64 + entry * 4 + 2] = (em >> 8) & 0xFF;
            tbl[64 + entry * 4 + 3] = em & 0xFF;
            entry++;
            d4_lo++;
        }
        uint32_t d4_full = ((uint32_t)d4_hi << 16) | d4_lo;
        d4_full <<= 1;
        d4_hi = (d4_full >> 16) & 0xFFFF;
        d4_lo = d4_full & 0xFFFF;
    }
    return true;
}

static int16_t huff_decode(ArcD *s, const uint8_t *tbl) {
    for (int idx = 0; idx < 16; idx++) {
        uint16_t mask = (uint16_t)(((uint16_t)tbl[idx * 4] << 8) | tbl[idx * 4 + 1]);
        uint16_t match = (uint16_t)(((uint16_t)tbl[idx * 4 + 2] << 8) | tbl[idx * 4 + 3]);
        if ((uint16_t)(s->d6 & mask) != match) continue;

        uint8_t shift = tbl[64 + idx * 4];
        uint8_t extra = tbl[64 + idx * 4 + 1];
        uint16_t extra_mask = (uint16_t)(((uint16_t)tbl[64 + idx * 4 + 2] << 8) |
                                         tbl[64 + idx * 4 + 3]);

        s->d7 -= shift;
        if (s->d7 & 0x80) shift = refill(s, shift);
        s->d6 >>= shift;

        if (extra < 2) return (int16_t)extra;

        uint8_t eb = extra - 1;
        uint16_t d0 = (uint16_t)(s->d6 & extra_mask);
        s->d7 -= eb;
        if (s->d7 & 0x80) eb = refill(s, eb);
        s->d6 >>= eb;
        d0 |= (1u << (extra - 1));
        return (int16_t)d0;
    }
    return -1;
}

size_t arcd_decompressed_size(const uint8_t *src, size_t src_size) {
    if (!src || src_size < 12) return 0;
    uint32_t magic = ((uint32_t)src[0] << 24) | ((uint32_t)src[1] << 16) |
                     ((uint32_t)src[2] << 8) | src[3];
    if (magic != ARCD_MAGIC) return 0;
    uint32_t size = ((uint32_t)src[4] << 24) | ((uint32_t)src[5] << 16) |
                    ((uint32_t)src[6] << 8) | src[7];
    if (size == 0 || size > ARCD_MAX_DECOMPRESSED_SIZE) return 0;
    return size;
}

int arcd_decode(const uint8_t *src, size_t src_size,
                uint8_t *dst, size_t dst_size) {
    if (!src || !dst || src_size < 12) return -1;

    uint32_t magic = ((uint32_t)src[0] << 24) | ((uint32_t)src[1] << 16) |
                     ((uint32_t)src[2] << 8) | src[3];
    if (magic != ARCD_MAGIC) return -1;

    size_t decomp_size = arcd_decompressed_size(src, src_size);
    if (decomp_size == 0 || decomp_size > dst_size || decomp_size > INT_MAX) return -1;

    ArcD s;
    s.src = src + 12;
    s.src_len = src_size - 12;
    s.pos = 0;
    s.dst = dst;
    s.dst_pos = 0;
    s.dst_len = decomp_size;
    s.input_truncated = false;

    s.d6 = ((uint32_t)read_src_byte(&s) << 8) | read_src_byte(&s);
    s.d7 = 0;

    while (s.dst_pos < decomp_size) {
        uint16_t bc = get_bits(&s, 0xFFFF, 16);
        if (bc == 0) break;
        bc--;

        uint16_t ot = get_bits(&s, 0x1F, 5);

        if (ot == 31) {
            for (uint16_t i = 0; i <= bc; i++) {
                if (s.dst_pos < decomp_size)
                    s.dst[s.dst_pos++] = read_src_byte(&s);
            }
            continue;
        }

        if (!build_table(&s, s.tables + 256, ot)) return -1;
        uint16_t off_sc = get_bits(&s, 0x1F, 5);
        if (!build_table(&s, s.tables + 0, off_sc)) return -1;
        uint16_t len_sc = get_bits(&s, 0x1F, 5);
        if (!build_table(&s, s.tables + 128, len_sc)) return -1;

        int16_t lc = huff_decode(&s, s.tables + 256);
        if (lc < 0) return -1;
        lc--;
        for (int i = 0; i <= lc; i++) {
            if (s.dst_pos < decomp_size)
                s.dst[s.dst_pos++] = read_src_byte(&s);
        }

        for (int bi = 0; bi < (int)bc; bi++) {
            int16_t off = huff_decode(&s, s.tables + 0);
            if (off <= 0) return -1;
            int64_t ref_value = (int64_t)s.dst_pos - off - 1;
            if (ref_value < 0 || (size_t)ref_value >= s.dst_pos) return -1;
            size_t ref = (size_t)ref_value;

            if (off >= 512) {
                if (s.dst_pos >= decomp_size || ref >= s.dst_pos) return -1;
                s.dst[s.dst_pos++] = s.dst[ref++];
            }

            int16_t ml = huff_decode(&s, s.tables + 128);
            if (ml < 0) return -1;

            if (s.dst_pos >= decomp_size || ref >= s.dst_pos) return -1;
            s.dst[s.dst_pos++] = s.dst[ref++];
            for (int j = 0; j <= ml; j++) {
                if (s.dst_pos >= decomp_size || ref >= s.dst_pos) return -1;
                s.dst[s.dst_pos++] = s.dst[ref++];
            }

            lc = huff_decode(&s, s.tables + 256);
            if (lc < 0) return -1;
            lc--;
            for (int i = 0; i <= lc; i++) {
                if (s.dst_pos < decomp_size)
                    s.dst[s.dst_pos++] = read_src_byte(&s);
            }
        }
    }

    if (s.input_truncated || s.dst_pos != decomp_size) return -1;
    return (int)s.dst_pos;
}
