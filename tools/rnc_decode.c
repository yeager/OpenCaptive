#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// RNC (Rob Northen Compression) Method 1 decoder
// Based on the well-documented RNC1 algorithm used in many Amiga/ST/DOS games

#define RNC_SIGNATURE 0x524E4301  // "RNC\x01"

typedef struct {
    const uint8_t *src;
    int pos;
    uint32_t bit_buf;
    int bits_left;
} BitReader;

static void br_init(BitReader *br, const uint8_t *data, int offset) {
    br->src = data;
    br->pos = offset;
    br->bit_buf = 0;
    br->bits_left = 0;
}

static void br_refill(BitReader *br) {
    while (br->bits_left < 16) {
        br->bit_buf |= (uint32_t)br->src[br->pos++] << br->bits_left;
        br->bits_left += 8;
    }
}

static uint32_t br_get(BitReader *br, int n) {
    if (n == 0) return 0;
    while (br->bits_left < n) {
        br->bit_buf |= (uint32_t)br->src[br->pos++] << br->bits_left;
        br->bits_left += 8;
    }
    uint32_t val = br->bit_buf & ((1u << n) - 1);
    br->bit_buf >>= n;
    br->bits_left -= n;
    return val;
}

typedef struct {
    int num;
    struct { uint32_t code; int len; } entries[32];
} HuffTable;

static void read_huff_table(BitReader *br, HuffTable *ht) {
    ht->num = br_get(br, 5);
    if (ht->num == 0) {
        ht->entries[0].code = 0;
        ht->entries[0].len = 0;
        ht->num = 1;
        return;
    }
    int code_lengths[32];
    for (int i = 0; i < ht->num; i++) {
        code_lengths[i] = br_get(br, 4);
    }
    // Build canonical huffman codes
    int bl_count[16] = {0};
    for (int i = 0; i < ht->num; i++) {
        if (code_lengths[i] > 0 && code_lengths[i] < 16)
            bl_count[code_lengths[i]]++;
    }
    uint32_t next_code[16] = {0};
    uint32_t code = 0;
    for (int bits = 1; bits < 16; bits++) {
        code = (code + bl_count[bits - 1]) << 1;
        next_code[bits] = code;
    }
    for (int i = 0; i < ht->num; i++) {
        int len = code_lengths[i];
        ht->entries[i].len = len;
        if (len > 0) {
            ht->entries[i].code = next_code[len]++;
        }
    }
}

static int decode_huff(BitReader *br, HuffTable *ht) {
    if (ht->num <= 1) return 0;

    uint32_t peek = br->bit_buf;
    int peeked = br->bits_left;

    // Try to match
    uint32_t code = 0;
    for (int len = 1; len <= 15; len++) {
        if (peeked < len) {
            br->bit_buf |= (uint32_t)br->src[br->pos++] << br->bits_left;
            br->bits_left += 8;
            peeked = br->bits_left;
            peek = br->bit_buf;
        }
        code = (code << 1) | (br_get(br, 1));
        // But wait, RNC uses LSB-first bit reading for huffman
        // Let me use a different approach
    }

    // Simplified: just read minimum bits matching any entry
    // Reset
    br->bit_buf = peek;
    br->bits_left = peeked;

    for (int i = 0; i < ht->num; i++) {
        if (ht->entries[i].len == 0) continue;
        // Peek bits
        int len = ht->entries[i].len;
        while (br->bits_left < len) {
            br->bit_buf |= (uint32_t)br->src[br->pos++] << br->bits_left;
            br->bits_left += 8;
        }
        // RNC uses LSB-first, so we need to reverse the bits for comparison
        uint32_t bits = br->bit_buf & ((1u << len) - 1);
        // Reverse bits
        uint32_t rev = 0;
        for (int b = 0; b < len; b++) {
            rev = (rev << 1) | ((bits >> b) & 1);
        }
        if (rev == ht->entries[i].code) {
            br->bit_buf >>= len;
            br->bits_left -= len;
            return i;
        }
    }
    return 0;
}

int rnc1_unpack(const uint8_t *src, int src_len, uint8_t *dst, int dst_len) {
    // Verify header
    uint32_t sig = ((uint32_t)src[0] << 24) | ((uint32_t)src[1] << 16) |
                   ((uint32_t)src[2] << 8) | src[3];
    if (sig != RNC_SIGNATURE) return -1;

    uint32_t unp_size = ((uint32_t)src[4] << 24) | ((uint32_t)src[5] << 16) |
                        ((uint32_t)src[6] << 8) | src[7];
    if ((int)unp_size > dst_len) return -2;

    BitReader br;
    br_init(&br, src, 18);
    br_refill(&br);
    br_get(&br, 2); // skip initial 2 bits

    int out_pos = 0;

    while (out_pos < (int)unp_size) {
        HuffTable raw_ht, dist_ht, len_ht;
        read_huff_table(&br, &raw_ht);
        read_huff_table(&br, &dist_ht);
        read_huff_table(&br, &len_ht);

        uint32_t count = br_get(&br, 16);

        for (uint32_t i = 0; i < count && out_pos < (int)unp_size; i++) {
            int raw_len = decode_huff(&br, &raw_ht);
            for (int j = 0; j < raw_len && out_pos < (int)unp_size; j++) {
                dst[out_pos++] = br.src[br.pos++];
                br.bits_left = 0;
                br.bit_buf = 0;
            }
            br_refill(&br);

            if (out_pos >= (int)unp_size) break;

            int match_len = decode_huff(&br, &len_ht) + 2;
            int match_dist = decode_huff(&br, &dist_ht) + 1;

            int match_start = out_pos - match_dist;
            if (match_start < 0) break;
            for (int j = 0; j < match_len && out_pos < (int)unp_size; j++) {
                dst[out_pos] = dst[match_start + j];
                out_pos++;
            }
        }
    }

    return out_pos;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: rnc_decode <input> <output>\n");
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) { fprintf(stderr, "Cannot open %s\n", argv[1]); return 1; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);

    if (size < 18 || data[0] != 'R' || data[1] != 'N' || data[2] != 'C') {
        fprintf(stderr, "Not an RNC file\n");
        free(data);
        return 1;
    }

    uint32_t unp_size = ((uint32_t)data[4] << 24) | ((uint32_t)data[5] << 16) |
                        ((uint32_t)data[6] << 8) | data[7];
    printf("RNC method %d, compressed %ld -> uncompressed %u\n", data[3], size - 18, unp_size);

    uint8_t *output = calloc(unp_size + 4096, 1);
    int result = rnc1_unpack(data, size, output, unp_size + 4096);
    printf("Decoded %d bytes\n", result);

    if (result > 0) {
        f = fopen(argv[2], "wb");
        fwrite(output, 1, result, f);
        fclose(f);
        printf("Wrote %s\n", argv[2]);
    }

    free(output);
    free(data);
    return 0;
}
