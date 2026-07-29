#include "rnc_decoder.h"
#include <string.h>

#define RNC1_SIG 0x524E4301

typedef struct {
    const uint8_t *data;
    int pos;
    uint32_t buf;
    int bits;
} Bits;

static void bits_init(Bits *b, const uint8_t *data, int pos) {
    b->data = data;
    b->pos = pos;
    b->buf = 0;
    b->bits = 0;
}

static void bits_load(Bits *b) {
    while (b->bits < 16) {
        b->buf |= (uint32_t)b->data[b->pos++] << b->bits;
        b->bits += 8;
    }
}

static uint32_t bits_read(Bits *b, int n) {
    if (n == 0) return 0;
    while (b->bits < n) {
        b->buf |= (uint32_t)b->data[b->pos++] << b->bits;
        b->bits += 8;
    }
    uint32_t v = b->buf & ((1u << n) - 1);
    b->buf >>= n;
    b->bits -= n;
    return v;
}

#define MAX_HT 32

typedef struct {
    int num;
    int lens[MAX_HT];
} HuffTab;

static void ht_read(Bits *b, HuffTab *h) {
    h->num = bits_read(b, 5);
    if (h->num == 0) {
        h->num = 1;
        h->lens[0] = 0;
        return;
    }
    for (int i = 0; i < h->num; i++)
        h->lens[i] = bits_read(b, 4);
}

static int ht_decode(Bits *b, HuffTab *h) {
    if (h->num <= 1) return 0;

    int bl_count[16] = {0};
    for (int i = 0; i < h->num; i++)
        if (h->lens[i] > 0 && h->lens[i] < 16)
            bl_count[h->lens[i]]++;

    uint32_t next_code[16] = {0};
    uint32_t code = 0;
    for (int bits = 1; bits < 16; bits++) {
        code = (code + bl_count[bits - 1]) << 1;
        next_code[bits] = code;
    }

    uint32_t sym_codes[MAX_HT];
    for (int i = 0; i < h->num; i++) {
        if (h->lens[i] > 0)
            sym_codes[i] = next_code[h->lens[i]]++;
        else
            sym_codes[i] = 0xFFFFFFFF;
    }

    while (b->bits < 16) {
        b->buf |= (uint32_t)b->data[b->pos++] << b->bits;
        b->bits += 8;
    }

    uint32_t acc = 0;
    for (int len = 1; len <= 15; len++) {
        acc = (acc << 1) | (bits_read(b, 1));
        for (int i = 0; i < h->num; i++) {
            if (h->lens[i] == len && sym_codes[i] == acc)
                return i;
        }
    }
    return 0;
}

int rnc_decode(const uint8_t *src, int src_len, uint8_t *dst, int dst_cap) {
    if (src_len < 18) return -1;

    uint32_t sig = ((uint32_t)src[0] << 24) | ((uint32_t)src[1] << 16) |
                   ((uint32_t)src[2] << 8) | src[3];
    if (sig != RNC1_SIG) return -1;

    uint32_t unp = ((uint32_t)src[4] << 24) | ((uint32_t)src[5] << 16) |
                   ((uint32_t)src[6] << 8) | src[7];
    if ((int)unp > dst_cap) return -2;

    Bits b;
    bits_init(&b, src, 18);
    bits_load(&b);
    bits_read(&b, 2);

    int out = 0;
    int safety = 0;

    while (out < (int)unp && safety++ < 1000000) {
        HuffTab raw, dist, len;
        ht_read(&b, &raw);
        ht_read(&b, &dist);
        ht_read(&b, &len);

        uint32_t count = bits_read(&b, 16);

        for (uint32_t i = 0; i < count && out < (int)unp; i++) {
            int raw_len = ht_decode(&b, &raw);
            for (int j = 0; j < raw_len && out < (int)unp; j++) {
                dst[out++] = b.data[b.pos++];
            }
            b.buf = 0;
            b.bits = 0;
            bits_load(&b);

            if (out >= (int)unp) break;

            int match_len = ht_decode(&b, &len) + 2;
            int match_dist = ht_decode(&b, &dist) + 1;

            int src_pos = out - match_dist;
            if (src_pos < 0) return -3;
            for (int j = 0; j < match_len && out < (int)unp; j++) {
                dst[out] = dst[src_pos + j];
                out++;
            }
        }
    }

    return out;
}

uint32_t rnc_uncompressed_size(const uint8_t *src, int src_len) {
    if (src_len < 12) return 0;
    if (src[0] != 'R' || src[1] != 'N' || src[2] != 'C') return 0;
    return ((uint32_t)src[4] << 24) | ((uint32_t)src[5] << 16) |
           ((uint32_t)src[6] << 8) | src[7];
}

bool rnc_is_compressed(const uint8_t *data, int len) {
    return len >= 4 && data[0] == 'R' && data[1] == 'N' && data[2] == 'C';
}
