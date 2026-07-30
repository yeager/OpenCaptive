#include "frame_compare.h"

#include <assert.h>

int main(void) {
    const uint32_t expected[] = {0xFF000000, 0xFF102030, 0xFFAABBCC};
    const uint32_t same[] = {0xFF000000, 0xFF102030, 0xFFAABBCC};
    const uint32_t changed[] = {0xFF000001, 0xFF102030, 0xFFAA00CC};
    FrameComparison comparison = frame_compare_argb(expected, same, 3);
    assert(comparison.pixel_count == 3);
    assert(comparison.different_pixels == 0);
    assert(comparison.total_channel_difference == 0);
    comparison = frame_compare_argb(expected, changed, 3);
    assert(comparison.pixel_count == 3);
    assert(comparison.different_pixels == 2);
    assert(comparison.total_channel_difference == 188);
    assert(comparison.maximum_channel_difference == 187);
    comparison = frame_compare_argb(NULL, changed, 3);
    assert(comparison.pixel_count == 0);
    return 0;
}
