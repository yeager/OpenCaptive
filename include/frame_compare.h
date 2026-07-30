#ifndef FRAME_COMPARE_H
#define FRAME_COMPARE_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    size_t pixel_count;
    size_t different_pixels;
    uint64_t total_channel_difference;
    uint8_t maximum_channel_difference;
} FrameComparison;

/* Compare two same-sized ARGB8888 frames. Alpha is deliberately ignored:
 * captured original raster frames are opaque presentation data. */
FrameComparison frame_compare_argb(const uint32_t *expected,
                                   const uint32_t *actual,
                                   size_t pixel_count);

#endif
