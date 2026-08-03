#include "amos_sprite.h"
#include <string.h>

static uint16_t be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }
static uint32_t rgb(uint16_t c) {
    uint32_t r = (c >> 8) & 15, g = (c >> 4) & 15, b = c & 15;
    return 0xff000000u | (r * 17u << 16) | (g * 17u << 8) | b * 17u;
}

bool amos_sprite_get(const uint8_t *data, size_t size, unsigned index, AmosSprite *out) {
    if (!data || !out || size < 6 || memcmp(data, "AmSp", 4)) return false;
    unsigned count = be16(data + 4); size_t pos = 6;
    if (index >= count) return false;
    bool has_palette = size >= 70;
    for (unsigned n = 0; n < count; ++n) {
        if (pos + 10 > size) return false;
        unsigned width_field = be16(data + pos), height = be16(data + pos + 2), depth = be16(data + pos + 4);
        bool has_mask = (be16(data + pos + 6) & 0x8000u) != 0;
        unsigned bytes_per_row = (width_field & 0x7fff) * 2 - (width_field >> 15);
        if (!bytes_per_row || !height || !depth || depth > 5) return false;
        size_t bytes = (size_t)bytes_per_row * height * (depth + (has_mask ? 1 : 0));
        if (pos + 10 + bytes > size) return false;
        if (n == index) {
            memset(out, 0, sizeof(*out));
            out->width = bytes_per_row * 8;
            out->height = height;
            out->depth = depth;
            out->bytes_per_row = bytes_per_row;
            out->has_mask = has_mask;
            out->planes = data + pos + 10;
            if (has_palette && size >= 64) {
                for (int p = 0; p < 32; ++p)
                    out->palette[p] = rgb(be16(data + size - 64 + p * 2));
            }
            return true;
        }
        pos += (10 + bytes + 1) & ~(size_t)1;
    }
    return false;
}

bool amos_sprite_decode_argb(const AmosSprite *sprite, uint32_t *pixels, size_t count) {
    if (!sprite || !pixels || !sprite->planes || count < (size_t)sprite->width * sprite->height) return false;
    for (unsigned y = 0; y < sprite->height; ++y) for (unsigned x = 0; x < sprite->width; ++x) {
        size_t offset = (size_t)y * sprite->bytes_per_row + x / 8; unsigned bit = 7 - (x & 7), colour = 0;
        for (unsigned p = 0; p < sprite->depth; ++p)
            if (sprite->planes[(size_t)p * sprite->height * sprite->bytes_per_row + offset] & (1u << bit)) colour |= 1u << p;
        bool visible = true;
        if (sprite->has_mask) {
            size_t mask = (size_t)sprite->depth * sprite->height * sprite->bytes_per_row + offset;
            visible = sprite->planes[mask] & (1u << bit);
        }
        pixels[(size_t)y * sprite->width + x] = visible ? sprite->palette[colour] : 0;
    }
    return true;
}
