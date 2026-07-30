#include "frame_compare.h"

FrameComparison frame_compare_argb(const uint32_t *expected,
                                   const uint32_t *actual,
                                   size_t pixel_count) {
    FrameComparison result = {.pixel_count = pixel_count};
    if (!expected || !actual) {
        result.pixel_count = 0;
        return result;
    }
    for (size_t i = 0; i < pixel_count; ++i) {
        uint32_t a = expected[i];
        uint32_t b = actual[i];
        uint8_t largest = 0;
        for (int shift = 0; shift <= 16; shift += 8) {
            int delta = (int)((a >> shift) & 0xffu) -
                        (int)((b >> shift) & 0xffu);
            uint8_t difference = (uint8_t)(delta < 0 ? -delta : delta);
            result.total_channel_difference += difference;
            if (difference > largest) largest = difference;
        }
        if (largest) {
            ++result.different_pixels;
            if (largest > result.maximum_channel_difference)
                result.maximum_channel_difference = largest;
        }
    }
    return result;
}
