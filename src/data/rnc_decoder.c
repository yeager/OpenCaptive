#include "rnc_decoder.h"
#include <limits.h>
#include <string.h>

#define RNC1_SIG 0x524E4301
#define RNC2_SIG 0x524E4302

typedef struct {
    const uint8_t *data;
    size_t begin;
    size_t pos;
    uint8_t bits;
    unsigned bit_count;
} RncBack;

static bool rb_byte(RncBack *reader, uint8_t *value) {
    if (reader->pos <= reader->begin) return false;
    *value = reader->data[--reader->pos];
    return true;
}

static bool rb_bits(RncBack *r, unsigned n, uint32_t *v) {
    uint32_t out = 0;
    while (n--) {
        if (!r->bit_count && (!rb_byte(r, &r->bits) || !(r->bit_count = 8))) return false;
        out = (out << 1) | ((r->bits >> (r->bit_count - 1)) & 1u);
        --r->bit_count;
    }
    *v = out;
    return true;
}

static bool rb_vlc(RncBack *r, const uint8_t *lens, const uint32_t *offs, unsigned count, bool cascade, uint32_t base, uint32_t *v) {
    if (cascade) for (unsigned i = 0; i < count; ++i) {
        uint32_t value;
        if (!lens[i] || !rb_bits(r, lens[i], &value)) return false;
        if (i + 1 == count || value != ((1u << lens[i]) - 1)) {
            *v = offs[i] - i + value;
            return true;
        }
    }
    if (base >= count || !rb_bits(r, lens[base], v)) return false;
    *v += offs[base];
    return true;
}

static bool rb_huff(RncBack *r, bool distance, uint32_t *v) {
    static const uint8_t ll[] = {1,2,3,4,4}, lc[] = {0,2,6,14,15}, lv[] = {0,1,2,3,4};
    static const uint8_t dl[] = {1,2,2}, dc[] = {0,2,3}, dv[] = {1,0,2};
    const uint8_t *lens = distance ? dl : ll, *codes = distance ? dc : lc, *vals = distance ? dv : lv;
    unsigned count = distance ? 3 : 5;
    uint32_t code = 0;
    for (unsigned n = 1; n <= 4; ++n) {
        uint32_t bit;
        if (!rb_bits(r, 1, &bit)) return false;
        code = (code << 1) | bit;
        for (unsigned i = 0; i < count; ++i) if (lens[i] == n && codes[i] == code) {
            *v = vals[i];
            return true;
        }
    }
    return false;
}
static int rnc_old_decode(const uint8_t *src, int len, uint8_t *dst, int cap, bool rnc2) {
    if (!src || !dst || len < 13 || cap < 0) return -1;
    uint32_t raw = ((uint32_t)src[4]<<24)|((uint32_t)src[5]<<16)|((uint32_t)src[6]<<8)|src[7];
    uint32_t packed = ((uint32_t)src[8]<<24)|((uint32_t)src[9]<<16)|((uint32_t)src[10]<<8)|src[11];
    if (raw > (uint32_t)cap || packed > (uint32_t)len - 12) return -2;
    RncBack in = {src, 12, 12 + packed, 0, 0};
    unsigned distbits = 12;
    unsigned lenbits = 10;
    uint8_t t;
    if (rnc2) {
        if (!rb_byte(&in, &t)) return -3;
        t++;
        distbits = t & 15;
        lenbits = (t >> 4) + 1;
    }
    if (!rb_byte(&in, &t)) return -3;
    for (unsigned i = 0; i < 7; ++i) if (t & (1u << i)) {
        in.bit_count = 7 - i;
        in.bits = (uint8_t)(t >> (i + 1));
        break;
    }
    uint8_t litlens[18] = {1,1,2,2,3,10,1,2,3,4,5,6,7,8,9,10,11,12};
    const unsigned litcount = rnc2 ? 18 : 6;
    if (rnc2) {
        static const uint8_t rnc2_litlens[18] = {1,1,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
        memcpy(litlens, rnc2_litlens, sizeof(litlens));
    }
    uint32_t litoff[18], lo = 0;
    for (unsigned i = 0; i < litcount; ++i) {
        litoff[i] = lo;
        lo += 1u << litlens[i];
    }
    uint8_t llen[5] = {0,0,1,2,(uint8_t)lenbits};
    uint32_t llo[5], ddo[3];
    lo = 0;
    for (unsigned i = 0; i < 5; ++i) {
        llo[i] = lo;
        lo += 1u << llen[i];
    }
    uint8_t dlen[3] = {5,8,(uint8_t)distbits};
    lo = 0;
    for (unsigned i = 0; i < 3; ++i) {
        ddo[i] = lo;
        lo += 1u << dlen[i];
    }
    /* Old RNC1 and RNC2 streams are read and written backwards. Their structure was
       independently checked against Ancient Format Decompressor (BSD-2-Clause,
       Teemu Suutari); this is a standalone C implementation. */
    size_t out = raw;
    while (out) {
        uint32_t n; if (!rb_vlc(&in,litlens,litoff,litcount,true,0,&n) || n > out) return -3;
        while(n--) { uint8_t b; if(!rb_byte(&in,&b))return -3; dst[--out]=b; } if(!out)break;
        uint32_t base,count,distance,bit; if(!rb_huff(&in,false,&base)||!rb_vlc(&in,llen,llo,5,false,base,&count))return -3; count+=2;
        if(count!=2){ if(!rb_huff(&in,true,&base)||!rb_vlc(&in,dlen,ddo,3,false,base,&distance))return -3; } else {
            if (!rb_bits(&in, 1, &bit) || !rb_bits(&in, bit ? 9 : 6, &distance)) return -3;
            if (bit) distance += 64;
        }
        size_t d = distance ? distance + count - 1 : 1; if(count>out || out+d>raw)return -3;
        while(count--) { --out; dst[out]=dst[out+d]; }
    }
    return (int)raw;
}

typedef struct {
    const uint8_t *data;
    int pos;
    int len;
    uint32_t buf;
    int bits;
} Bits;

static void bits_init(Bits *b, const uint8_t *data, int pos, int len) {
    b->data = data;
    b->pos = pos;
    b->len = len;
    b->buf = 0;
    b->bits = 0;
}

static void bits_load(Bits *b) {
    while (b->bits < 16) {
        uint8_t next = b->pos < b->len ? b->data[b->pos] : 0;
        b->pos++;
        b->buf |= (uint32_t)next << b->bits;
        b->bits += 8;
    }
}

static uint32_t bits_read(Bits *b, int n) {
    if (n == 0) return 0;
    while (b->bits < n) {
        uint8_t next = b->pos < b->len ? b->data[b->pos] : 0;
        b->pos++;
        b->buf |= (uint32_t)next << b->bits;
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

static bool ht_read(Bits *b, HuffTab *h) {
    if (!b || !h) return false;
    h->num = bits_read(b, 5);
    if (h->num == 0) {
        h->num = 1;
        h->lens[0] = 0;
        return true;
    }
    if (h->num > MAX_HT) return false;
    for (int i = 0; i < h->num; i++)
        h->lens[i] = bits_read(b, 4);
    return true;
}

static int ht_decode(Bits *b, HuffTab *h) {
    if (!b || !h || h->num <= 0 || h->num > MAX_HT) return -1;
    if (h->num == 1) return 0;

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
        if (b->pos >= b->len) return -1;
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
    return -1;
}

int rnc_decode(const uint8_t *src, int src_len, uint8_t *dst, int dst_cap) {
    if (!src || !dst || src_len < 13 || dst_cap < 0) return -1;

    uint32_t sig = ((uint32_t)src[0] << 24) | ((uint32_t)src[1] << 16) |
                   ((uint32_t)src[2] << 8) | src[3];
    if (sig == RNC2_SIG) return rnc_old_decode(src, src_len, dst, dst_cap, true);
    if (sig != RNC1_SIG) return -1;

    /* RNC1 has two incompatible stream layouts under the same signature.
       Old Amiga streams start at byte 12 and are decoded backwards; new
       streams start at byte 18 and are decoded forwards. The final old-stream
       byte is a literal marker (high bit set), which distinguishes the
       verified Captive Amiga Federation payload from the new layout. */
    uint32_t packed_size = ((uint32_t)src[8] << 24) | ((uint32_t)src[9] << 16) |
                           ((uint32_t)src[10] << 8) | src[11];
    if (packed_size <= (uint32_t)src_len - 12U &&
        (src[packed_size + 11U] & 0x80U))
        return rnc_old_decode(src, src_len, dst, dst_cap, false);

    if (src_len < 18) return -1;

    uint32_t unp = ((uint32_t)src[4] << 24) | ((uint32_t)src[5] << 16) |
                   ((uint32_t)src[6] << 8) | src[7];
    if (unp > (uint32_t)dst_cap || unp > INT_MAX) return -2;

    Bits b;
    bits_init(&b, src, 18, src_len);
    bits_load(&b);
    bits_read(&b, 2);

    int out = 0;
    int safety = 0;

    while (out < (int)unp && safety++ < 1000000) {
        HuffTab raw, dist, len;
        if (!ht_read(&b, &raw) || !ht_read(&b, &dist) || !ht_read(&b, &len))
            return -3;

        uint32_t count = bits_read(&b, 16);

        for (uint32_t i = 0; i < count && out < (int)unp; i++) {
            int raw_len = ht_decode(&b, &raw);
            if (raw_len < 0) return -3;
            for (int j = 0; j < raw_len && out < (int)unp; j++) {
                if (b.pos >= b.len) return -3;
                dst[out++] = b.data[b.pos++];
            }
            b.buf = 0;
            b.bits = 0;
            bits_load(&b);

            if (out >= (int)unp) break;

            int decoded_len = ht_decode(&b, &len);
            int decoded_dist = ht_decode(&b, &dist);
            if (decoded_len < 0 || decoded_dist < 0) return -3;
            int match_len = decoded_len + 2;
            int match_dist = decoded_dist + 1;
            if (match_len < 2 || match_dist < 1) return -3;

            int src_pos = out - match_dist;
            if (src_pos < 0) return -3;
            for (int j = 0; j < match_len && out < (int)unp; j++) {
                dst[out] = dst[src_pos + j];
                out++;
            }
        }
    }

    return out == (int)unp ? out : -3;
}

uint32_t rnc_uncompressed_size(const uint8_t *src, int src_len) {
    if (!src || src_len < 12) return 0;
    if (src[0] != 'R' || src[1] != 'N' || src[2] != 'C') return 0;
    return ((uint32_t)src[4] << 24) | ((uint32_t)src[5] << 16) |
           ((uint32_t)src[6] << 8) | src[7];
}

bool rnc_is_compressed(const uint8_t *data, int len) {
    return data && len >= 4 && data[0] == 'R' && data[1] == 'N' && data[2] == 'C';
}
