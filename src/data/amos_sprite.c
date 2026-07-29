#include "amos_sprite.h"
#include <string.h>

static uint16_t be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }
static uint32_t rgb(uint16_t c) {
    uint32_t r = (c >> 8) & 15, g = (c >> 4) & 15, b = c & 15;
    return 0xff000000u | (r * 17u << 16) | (g * 17u << 8) | b * 17u;
}

bool amos_sprite_get(const uint8_t *data, size_t size, unsigned index, AmosSprite *out) {
    if (!data || !out || size < 70 || memcmp(data, "AmSp", 4)) return false;
    unsigned count = be16(data + 4); size_t pos = 6;
    if (index >= count) return false;
    for (unsigned n = 0; n < count; ++n) {
        if (pos + 10 > size - 64) return false;
        unsigned words = be16(data + pos), height = be16(data + pos + 2), depth = be16(data + pos + 4);
        if (!words || !height || !depth || depth > 5) return false;
        /* Liberation's AMOS banks retain the optional mask plane. */
        size_t bytes = (size_t)words * height * (depth + 1) * 2;
        if (bytes > size - 64 - pos - 10) return false;
        if (n == index) {
            memset(out, 0, sizeof(*out)); out->width = words * 16; out->height = height; out->depth = depth; out->planes = data + pos + 10;
            for (int p = 0; p < 32; ++p) out->palette[p] = rgb(be16(data + size - 64 + p * 2));
            return true;
        }
        pos += 10 + bytes;
    }
    return false;
}

bool amos_sprite_decode_argb(const AmosSprite *sprite, uint32_t *pixels, size_t count) {
    if (!sprite || !pixels || !sprite->planes || count < (size_t)sprite->width * sprite->height) return false;
    size_t words = sprite->width / 16;
    for (unsigned y = 0; y < sprite->height; ++y) for (unsigned x = 0; x < sprite->width; ++x) {
        size_t word = x / 16, offset = y * words + word; unsigned bit = 15 - (x & 15), colour = 0;
        for (unsigned p = 0; p < sprite->depth; ++p)
            if (be16(sprite->planes + ((size_t)p * sprite->height * words + offset) * 2) & (1u << bit)) colour |= 1u << p;
        size_t mask = (size_t)sprite->depth * sprite->height * words + offset;
        bool visible = be16(sprite->planes + mask * 2) & (1u << bit);
        pixels[(size_t)y * sprite->width + x] = visible ? sprite->palette[colour] : 0;
    }
    return true;
}
