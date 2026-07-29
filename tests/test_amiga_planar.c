#include "amiga_planar.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    uint8_t source[AMIGA_PLANAR_BYTES] = {0};
    uint8_t pixels[AMIGA_PLANAR_WIDTH * AMIGA_PLANAR_HEIGHT];
    const size_t plane_size = (AMIGA_PLANAR_WIDTH / 8U) * AMIGA_PLANAR_HEIGHT;
    /* Pixel 0 has planes 0, 2 and 4 set; pixel 8 has plane 3 set. */
    source[0] = 0x80U;
    source[2U * plane_size] = 0x80U;
    source[4U * plane_size] = 0x80U;
    source[3U * plane_size + 1U] = 0x80U;
    assert(amiga_planar_decode_320x200_5(source, sizeof(source), pixels));
    assert(pixels[0] == 21U);
    assert(pixels[8] == 8U);
    assert(pixels[1] == 0U);
    assert(!amiga_planar_decode_320x200_5(source, sizeof(source) - 1U, pixels));
    puts("All Amiga planar tests passed");
    return 0;
}
