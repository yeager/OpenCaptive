#include "pl5_decoder.h"
#include <stdlib.h>
#include <string.h>

bool pl5_decode(const uint8_t *data, size_t size, PL5Image *out) {
    if (!data || size < 16 || !out) return false;
    memset(out, 0, sizeof(*out));
    // TODO: reverse-engineer PL5 format from DOS data files
    // Initial analysis shows non-standard headers, likely planar VGA data
    return false;
}

void pl5_free(PL5Image *img) {
    if (img && img->pixel_data) {
        free(img->pixel_data);
        img->pixel_data = NULL;
    }
}
