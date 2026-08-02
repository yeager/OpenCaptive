#include "liberation_img.h"
#include <stdlib.h>
#include <string.h>

static uint16_t read_be16(const uint8_t *p) {
    return (uint16_t)(p[0] << 8 | p[1]);
}

static uint32_t read_be32(const uint8_t *p) {
    return (uint32_t)(p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]);
}

bool img_open(ImgFile *img, const uint8_t *data, size_t size) {
    if (!img || !data || size < 6) return false;
    if (memcmp(data, "ImgA", 4) != 0) return false;

    memset(img, 0, sizeof(*img));
    img->data = data;
    img->size = size;
    img->sprite_count = read_be16(data + 4);

    if (img->sprite_count > IMG_MAX_SPRITES) return false;
    if (6 + (size_t)img->sprite_count * 4 > size) return false;

    for (unsigned i = 0; i < img->sprite_count; i++) {
        img->offsets[i] = read_be32(data + 6 + i * 4);
        if (img->offsets[i] >= size) return false;
    }

    for (unsigned i = 0; i < img->sprite_count; i++) {
        uint16_t flags = read_be16(data + img->offsets[i]);
        img->multi_frame[i] = (flags > 0);
    }

    return true;
}

static int ceil_div16(int v) { return (v + 15) / 16; }

static bool decode_single(const uint8_t *spr, size_t spr_size,
                           const uint32_t *palette, unsigned pal_size,
                           int header_overhead, ImgSprite *out) {
    if (spr_size < (size_t)header_overhead) return false;

    int base = (header_overhead == 22) ? 2 : 0;
    uint16_t w = read_be16(spr + base);
    uint16_t h = read_be16(spr + base + 2);
    if (w == 0 || h == 0 || w > 1024 || h > 1024) return false;

    int word_width = ceil_div16(w);
    int plane_size = word_width * 2 * h;
    if (plane_size == 0) return false;

    int data_start = header_overhead;
    int total_planes = (int)(spr_size - header_overhead) / plane_size;
    if (total_planes < 1 || total_planes > IMG_MAX_PLANES) return false;

    int color_planes = total_planes - 1;
    if (color_planes < 0) color_planes = 0;

    out->width = w;
    out->height = h;
    out->num_color_planes = (uint8_t)color_planes;
    out->total_planes = (uint8_t)total_planes;
    out->pixels = calloc((size_t)w * h, sizeof(uint32_t));
    if (!out->pixels) return false;

    const uint8_t *planes[IMG_MAX_PLANES];
    for (int p = 0; p < total_planes; p++)
        planes[p] = spr + data_start + p * plane_size;

    int mask_plane = total_planes - 1;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int word_idx = x / 16;
            int bit = 15 - (x % 16);
            int byte_off = y * word_width * 2 + word_idx * 2;

            unsigned color_idx = 0;
            for (int p = 0; p < color_planes; p++) {
                if ((planes[p][byte_off] << 8 | planes[p][byte_off + 1]) & (1 << bit))
                    color_idx |= (1u << p);
            }

            int mask_val = (planes[mask_plane][byte_off] << 8 |
                            planes[mask_plane][byte_off + 1]) & (1 << bit);

            if (mask_val && color_idx < pal_size && palette)
                out->pixels[y * w + x] = palette[color_idx] | 0xFF000000;
            else if (mask_val)
                out->pixels[y * w + x] = 0xFF000000 | (color_idx * 4);
            else
                out->pixels[y * w + x] = 0x00000000;
        }
    }

    return true;
}

bool img_decode_sprite(const ImgFile *img, unsigned index,
                       const uint32_t *palette, unsigned palette_size,
                       ImgSprite *out) {
    if (!img || index >= img->sprite_count || !out) return false;
    if (img->multi_frame[index]) return false;

    uint32_t off = img->offsets[index];
    size_t end = (index + 1 < img->sprite_count)
                     ? img->offsets[index + 1]
                     : img->size;
    size_t spr_size = end - off;

    memset(out, 0, sizeof(*out));
    return decode_single(img->data + off, spr_size, palette, palette_size, 22, out);
}

bool img_decode_multi(const ImgFile *img, unsigned index,
                      const uint32_t *palette, unsigned palette_size,
                      ImgMultiSprite *out) {
    if (!img || index >= img->sprite_count || !out) return false;
    if (!img->multi_frame[index]) return false;

    memset(out, 0, sizeof(*out));

    uint32_t off = img->offsets[index];
    size_t spr_end = (index + 1 < img->sprite_count)
                         ? img->offsets[index + 1]
                         : img->size;
    const uint8_t *spr = img->data + off;
    size_t spr_size = spr_end - off;

    uint16_t frame_count = read_be16(spr);
    if (frame_count > IMG_MAX_FRAMES || frame_count == 0) return false;
    if (2 + (size_t)frame_count * 2 > spr_size) return false;

    out->frame_count = frame_count;

    uint16_t frame_offsets[IMG_MAX_FRAMES];
    for (unsigned i = 0; i < frame_count; i++)
        frame_offsets[i] = read_be16(spr + 2 + i * 2);

    for (unsigned i = 0; i < frame_count; i++) {
        size_t fstart = frame_offsets[i];
        size_t fend = (i + 1 < frame_count) ? frame_offsets[i + 1] : spr_size;
        if (fstart >= spr_size || fend > spr_size || fend <= fstart) {
            img_multi_free(out);
            return false;
        }
        if (!decode_single(spr + fstart, fend - fstart,
                           palette, palette_size, 20, &out->frames[i])) {
            img_multi_free(out);
            return false;
        }
    }

    return true;
}

void img_sprite_free(ImgSprite *sprite) {
    if (sprite) { free(sprite->pixels); sprite->pixels = NULL; }
}

void img_multi_free(ImgMultiSprite *multi) {
    if (!multi) return;
    for (unsigned i = 0; i < multi->frame_count; i++)
        img_sprite_free(&multi->frames[i]);
    multi->frame_count = 0;
}

void img_close(ImgFile *img) {
    if (img) memset(img, 0, sizeof(*img));
}
