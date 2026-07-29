#ifndef PL5_DECODER_H
#define PL5_DECODER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// PL5 is the DOS Captive graphics format (CAPICS/*.PL5)
// Appears to be a planar image format

typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t planes;
    uint8_t *pixel_data;
    uint32_t palette[256];
    size_t data_size;
} PL5Image;

bool pl5_decode(const uint8_t *data, size_t size, PL5Image *out);
void pl5_free(PL5Image *img);

#endif
