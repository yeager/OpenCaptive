#ifndef LIBERATION_IMG_H
#define LIBERATION_IMG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define IMG_MAX_SPRITES 256
#define IMG_MAX_FRAMES 16
#define IMG_MAX_PLANES 7

typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t num_color_planes;
    uint8_t total_planes;
    uint32_t *pixels;
} ImgSprite;

typedef struct {
    uint16_t frame_count;
    ImgSprite frames[IMG_MAX_FRAMES];
} ImgMultiSprite;

typedef struct {
    const uint8_t *data;
    size_t size;
    uint16_t sprite_count;
    uint32_t offsets[IMG_MAX_SPRITES];
    bool multi_frame[IMG_MAX_SPRITES];
} ImgFile;

bool img_open(ImgFile *img, const uint8_t *data, size_t size);
bool img_decode_sprite(const ImgFile *img, unsigned index,
                       const uint32_t *palette, unsigned palette_size,
                       ImgSprite *out);
bool img_decode_multi(const ImgFile *img, unsigned index,
                      const uint32_t *palette, unsigned palette_size,
                      ImgMultiSprite *out);
void img_sprite_free(ImgSprite *sprite);
void img_multi_free(ImgMultiSprite *multi);
void img_close(ImgFile *img);

#endif
