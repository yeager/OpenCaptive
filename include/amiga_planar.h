#ifndef AMIGA_PLANAR_H
#define AMIGA_PLANAR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define AMIGA_PLANAR_WIDTH 320
#define AMIGA_PLANAR_HEIGHT 200
#define AMIGA_PLANAR_DEPTH 5
#define AMIGA_PLANAR_BYTES (AMIGA_PLANAR_WIDTH / 8 * AMIGA_PLANAR_HEIGHT * AMIGA_PLANAR_DEPTH)

/* Converts the standard contiguous Amiga five-bitplane layout used by
 * Captive's Fed7 resources to one 5-bit palette index per pixel. */
bool amiga_planar_decode_320x200_5(const uint8_t *data, size_t size,
                                   uint8_t output[AMIGA_PLANAR_WIDTH * AMIGA_PLANAR_HEIGHT]);

#endif
