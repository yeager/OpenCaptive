#include "amiga_planar.h"

bool amiga_planar_decode_320x200_5(const uint8_t *data, size_t size,
                                   uint8_t output[AMIGA_PLANAR_WIDTH * AMIGA_PLANAR_HEIGHT]) {
    if (!data || !output || size != AMIGA_PLANAR_BYTES) return false;
    const size_t row_bytes = AMIGA_PLANAR_WIDTH / 8U;
    const size_t plane_size = row_bytes * AMIGA_PLANAR_HEIGHT;
    for (size_t y = 0; y < AMIGA_PLANAR_HEIGHT; ++y) {
        for (size_t x = 0; x < AMIGA_PLANAR_WIDTH; ++x) {
            uint8_t index = 0;
            size_t source_byte = y * row_bytes + x / 8U;
            uint8_t bit = (uint8_t)(7U - (x & 7U));
            for (size_t plane = 0; plane < AMIGA_PLANAR_DEPTH; ++plane)
                index |= (uint8_t)(((data[plane * plane_size + source_byte] >> bit) & 1U) << plane);
            output[y * AMIGA_PLANAR_WIDTH + x] = index;
        }
    }
    return true;
}
