#ifndef PL5_DECODER_H
#define PL5_DECODER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PL5_WIDTH       320
#define PL5_HEIGHT      200
#define PL5_COLORS      32
#define PL5_FILE_SIZE   40000
#define PL5_PIXEL_COUNT (PL5_WIDTH * PL5_HEIGHT)

typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t *pixel_data;
    uint32_t palette[PL5_COLORS];
    size_t data_size;
} PL5Image;

bool pl5_decode(const uint8_t *data, size_t size, PL5Image *out);
void pl5_free(PL5Image *img);

extern const uint32_t pl5_default_palette[PL5_COLORS];

#endif
