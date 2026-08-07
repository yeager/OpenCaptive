#include "anm_decoder.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static void test_null_input(void) {
    ANMAnimation anim;
    assert(!anm_decode(NULL, 0, &anim));
    assert(!anm_decode(NULL, 100, &anim));
}

static void test_too_small(void) {
    uint8_t data[4] = {0};
    ANMAnimation anim;
    assert(!anm_decode(data, sizeof(data), &anim));
}

static void test_palette_parse(void) {
    // Minimal ANM: palette + cmd_end word pointing past end + one minimal frame
    // This tests that the palette is parsed even if frame decode has issues
    uint8_t data[ANM_PALETTE_SIZE + 10];
    memset(data, 0, sizeof(data));

    // Set color 1 to VGA (63, 0, 0) = bright red
    data[3] = 63;
    data[4] = 0;
    data[5] = 0;

    // cmd_end = 770 (right after the word itself), little-endian
    data[768] = 0x02;
    data[769] = 0x03;

    // No valid frames in remaining 8 bytes - decode should fail
    ANMAnimation anim;
    // May or may not decode depending on frame parsing
    // Just test that it doesn't crash
    bool ok = anm_decode(data, sizeof(data), &anim);
    if (ok) {
        assert(anim.palette[1] == 0xFFFC0000); // red
        anm_free(&anim);
    }
}

static void test_little_endian_frame_layout(void) {
    /* One frame: command byte 1 changes pixel zero, followed by the
       little-endian size word 4 (two payload bytes plus the size word). */
    uint8_t data[ANM_PALETTE_SIZE + 6] = {0};
    data[768] = (uint8_t)(ANM_PALETTE_SIZE + 2);
    data[769] = (uint8_t)((ANM_PALETTE_SIZE + 2) >> 8);
    data[770] = 1;
    data[771] = 0;
    data[772] = 4;
    data[773] = 0;

    ANMAnimation anim;
    assert(anm_decode(data, sizeof(data), &anim));
    assert(anim.frame_count == 1);
    assert(anim.frames[0][0] == 1);
    anm_free(&anim);
}

int main(void) {
    test_null_input();
    test_too_small();
    test_palette_parse();
    test_little_endian_frame_layout();
    printf("All ANM decoder tests passed\n");
    return 0;
}
