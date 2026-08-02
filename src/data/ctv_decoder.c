#include "ctv_decoder.h"
#include <stdlib.h>
#include <string.h>

/* Creative Voice File format:
 * Header: "Creative Voice File\x1a" (20 bytes) + 2-byte data offset + 2-byte version + 2-byte check
 * Data blocks follow, each starting with a type byte:
 *   0 = terminator
 *   1 = sound data: 3-byte length, 1-byte sample rate code, 1-byte codec, then PCM
 *   2 = sound continue: 3-byte length, then PCM
 *   3 = silence: 2-byte duration, 1-byte sample rate code
 *   4 = marker
 *   5 = ASCII text
 *   6 = repeat start
 *   7 = repeat end
 *   8 = extended: new sample rate/codec for subsequent blocks
 *   9 = new format sound data (VOC 1.20+)
 *
 * Sample rate = 1000000 / (256 - sr_code) for type 1/3 blocks. */

static uint16_t read_le16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t read_le24(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
}

static bool ctv_add_sample(CtvFile *ctv, const int8_t *data, uint32_t len,
                            uint32_t rate) {
    if (ctv->count >= ctv->capacity) {
        int new_cap = ctv->capacity ? ctv->capacity * 2 : 16;
        CtvSample *new_entries = realloc(ctv->entries, new_cap * sizeof(CtvSample));
        if (!new_entries) return false;
        ctv->entries = new_entries;
        ctv->capacity = new_cap;
    }
    CtvSample *s = &ctv->entries[ctv->count];
    s->samples = malloc(len);
    if (!s->samples) return false;
    memcpy(s->samples, data, len);
    s->length = len;
    s->sample_rate = rate;
    ctv->count++;
    return true;
}

bool ctv_decode(CtvFile *out, const uint8_t *data, uint32_t size) {
    if (!out || !data) return false;
    memset(out, 0, sizeof(*out));

    if (size < 26) return false;
    if (memcmp(data, "Creative Voice File\x1a", 20) != 0) return false;

    uint16_t data_offset = read_le16(data + 20);
    if (data_offset > size) return false;

    uint32_t pos = data_offset;
    uint32_t current_rate = 8000;

    while (pos < size) {
        uint8_t block_type = data[pos++];
        if (block_type == 0) break;

        if (pos + 3 > size) break;
        uint32_t block_len = read_le24(data + pos);
        pos += 3;

        if (pos + block_len > size) break;

        switch (block_type) {
        case 1: {
            if (block_len < 2) break;
            uint8_t sr_code = data[pos];
            uint8_t codec = data[pos + 1];
            if (codec != 0) { pos += block_len; break; }
            current_rate = 1000000 / (256 - sr_code);
            ctv_add_sample(out, (const int8_t *)(data + pos + 2),
                           block_len - 2, current_rate);
            break;
        }
        case 2:
            if (out->count > 0) {
                CtvSample *last = &out->entries[out->count - 1];
                int8_t *new_data = realloc(last->samples,
                                            last->length + block_len);
                if (new_data) {
                    memcpy(new_data + last->length, data + pos, block_len);
                    last->samples = new_data;
                    last->length += block_len;
                }
            }
            break;
        case 8: {
            if (block_len >= 4) {
                uint16_t tc = read_le16(data + pos);
                current_rate = 256000000 / (65536 - tc);
            }
            break;
        }
        case 9: {
            if (block_len < 12) break;
            uint32_t sr = (uint32_t)data[pos] | ((uint32_t)data[pos+1] << 8) |
                          ((uint32_t)data[pos+2] << 16) | ((uint32_t)data[pos+3] << 24);
            uint8_t bits = data[pos + 4];
            uint8_t channels = data[pos + 5];
            uint16_t codec9 = read_le16(data + pos + 6);
            if (codec9 != 0 || bits != 8 || channels != 1) {
                pos += block_len;
                break;
            }
            current_rate = sr;
            ctv_add_sample(out, (const int8_t *)(data + pos + 12),
                           block_len - 12, current_rate);
            break;
        }
        default:
            break;
        }
        pos += block_len;
    }

    return out->count > 0;
}

void ctv_free(CtvFile *ctv) {
    if (!ctv) return;
    for (int i = 0; i < ctv->count; i++)
        free(ctv->entries[i].samples);
    free(ctv->entries);
    memset(ctv, 0, sizeof(*ctv));
}
