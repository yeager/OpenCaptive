#include "liberation_img.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Minimal ImgA file: 1 simple sprite, 16x2, 1 color plane + 1 mask plane.
 * Header: "ImgA" + count(1) + offset(0x000A)
 * Sprite at 0x0A: flags=0, w=16, h=2, fmt=0x0600,
 *   data_size=18 (2+6offsets=14, + 1plane×4bytes=4 → 14+4=18)
 *   offsets: 000e 0000 0000 0000 0000 0000
 *   color plane 0: 4 bytes (16px × 2 lines, 1 word/line × 2 bytes)
 *   mask plane: 4 bytes
 *
 * Layout: header=22 bytes, 1 color plane (4 bytes), mask (4 bytes) = 30 bytes
 * total sprite = 22 + 4 (color) + 4 (mask) = 30
 * data_size = 18 (from byte 8 to end of color data: 14 offset_table + 4 plane)
 */
static const uint8_t test_img[] = {
    /* ImgA header */
    'I','m','g','A',
    0x00, 0x01,             /* sprite count = 1 */
    0x00, 0x00, 0x00, 0x0A, /* offset[0] = 10 */

    /* Sprite 0 at offset 10 */
    0x00, 0x00,             /* flags = 0 (simple) */
    0x00, 0x10,             /* width = 16 */
    0x00, 0x02,             /* height = 2 */
    0x06, 0x00,             /* format tag 0x0600 */
    0x00, 0x12,             /* data_size = 18 */
    /* 6 offset entries (from byte 8 = data_size field) */
    0x00, 0x0E,             /* offset[0] = 14 → plane 0 */
    0x00, 0x00,             /* offset[1] = 0 (unused) */
    0x00, 0x00,             /* offset[2] = 0 */
    0x00, 0x00,             /* offset[3] = 0 */
    0x00, 0x00,             /* offset[4] = 0 */
    0x00, 0x00,             /* offset[5] = 0 */
    /* Color plane 0 (4 bytes): alternating bits */
    0xAA, 0xAA,             /* line 0: 1010101010101010 */
    0x55, 0x55,             /* line 1: 0101010101010101 */
    /* Mask plane (4 bytes): all visible */
    0xFF, 0xFF,             /* line 0: all set */
    0xFF, 0xFF,             /* line 1: all set */
};

/* Multi-frame ImgA: 1 sprite with 2 frames, each 8x1.
 * Sub-sprite (no flags): 20 bytes header + 2 color + 2 mask = 24 bytes each.
 * Multi header: 2 (count) + 4 (2 offsets) = 6 bytes.
 * Frame 0 at +6, Frame 1 at +30. Total sprite = 6 + 24 + 24 = 54. */
static const uint8_t test_multi_img[] = {
    'I','m','g','A',
    0x00, 0x01,             /* count = 1 */
    0x00, 0x00, 0x00, 0x0A, /* offset = 10 */

    /* Multi-frame sprite at offset 10 */
    0x00, 0x02,             /* frame_count = 2 */
    0x00, 0x06,             /* frame[0] offset = 6 */
    0x00, 0x1E,             /* frame[1] offset = 30 */

    /* Frame 0 at +6: 8x1, 1 color + 1 mask */
    0x00, 0x08,             /* width = 8 */
    0x00, 0x01,             /* height = 1 */
    0x06, 0x00,             /* format */
    0x00, 0x10,             /* data_size = 16 (14 offsets + 2 color plane) */
    0x00, 0x0E,             /* offset[0] = 14 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Color plane 0 (2 bytes) */
    0xF0, 0x00,
    /* Mask plane (2 bytes) */
    0xFF, 0x00,

    /* Frame 1 at +30: 8x1, 1 color + 1 mask */
    0x00, 0x08,
    0x00, 0x01,
    0x06, 0x00,
    0x00, 0x10,
    0x00, 0x0E,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0F, 0x00,
    0xFF, 0x00,
};

static void test_open(void) {
    ImgFile img;
    assert(img_open(&img, test_img, sizeof(test_img)));
    assert(img.sprite_count == 1);
    assert(img.offsets[0] == 10);
    assert(!img.multi_frame[0]);
    assert(!img_open(&img, NULL, 0));
    assert(img.data == NULL && img.size == 0 && img.sprite_count == 0);
    img_close(&img);
}

static void test_bad_magic(void) {
    uint8_t bad[10] = {'N','O','P','E', 0,1, 0,0,0,6};
    ImgFile img;
    assert(!img_open(&img, bad, sizeof(bad)));
}

static void test_reject_unsorted_offsets(void) {
    uint8_t bad[14] = {
        'I','m','g','A', 0,2,
        0,0,0,10,
        0,0,0,9
    };
    ImgFile img;
    assert(!img_open(&img, bad, sizeof(bad)));
}

static void test_rejects_offset_without_flags(void) {
    uint8_t bad[11] = {
        'I','m','g','A', 0,1,
        0,0,0,10,
        0
    };
    ImgFile img;
    assert(!img_open(&img, bad, sizeof(bad)));
}

static void test_decode_simple(void) {
    ImgFile img;
    assert(img_open(&img, test_img, sizeof(test_img)));

    uint32_t palette[2] = {0x00FF0000, 0x0000FF00};
    ImgSprite sprite;
    assert(img_decode_sprite(&img, 0, palette, 2, &sprite));

    assert(sprite.width == 16);
    assert(sprite.height == 2);
    assert(sprite.num_color_planes == 1);
    assert(sprite.total_planes == 2);

    /* Line 0: 0xAAAA = bit pattern 1010... → colors 1,0,1,0... */
    assert((sprite.pixels[0] & 0x00FFFFFF) == 0x0000FF00);  /* color 1 = green */
    assert((sprite.pixels[1] & 0x00FFFFFF) == 0x00FF0000);  /* color 0 = red */
    assert((sprite.pixels[2] & 0x00FFFFFF) == 0x0000FF00);
    assert((sprite.pixels[3] & 0x00FFFFFF) == 0x00FF0000);

    /* All pixels should be opaque (mask all set) */
    for (int i = 0; i < 32; i++)
        assert((sprite.pixels[i] & 0xFF000000) == 0xFF000000);

    img_sprite_free(&sprite);
    img_close(&img);
}

static void test_decode_multi(void) {
    ImgFile img;
    assert(img_open(&img, test_multi_img, sizeof(test_multi_img)));
    assert(img.multi_frame[0]);

    uint32_t palette[2] = {0x00000000, 0x00FFFFFF};
    ImgMultiSprite multi;
    assert(img_decode_multi(&img, 0, palette, 2, &multi));

    assert(multi.frame_count == 2);
    assert(multi.frames[0].width == 8);
    assert(multi.frames[0].height == 1);
    assert(multi.frames[1].width == 8);

    /* Frame 0: 0xF0 = 11110000 → pixels 0-3 color 1, 4-7 color 0 */
    assert((multi.frames[0].pixels[0] & 0x00FFFFFF) == 0x00FFFFFF);
    assert((multi.frames[0].pixels[4] & 0x00FFFFFF) == 0x00000000);

    /* Frame 1: 0x0F = 00001111 → pixels 0-3 color 0, 4-7 color 1 */
    assert((multi.frames[1].pixels[0] & 0x00FFFFFF) == 0x00000000);
    assert((multi.frames[1].pixels[4] & 0x00FFFFFF) == 0x00FFFFFF);

    img_multi_free(&multi);
    img_close(&img);
}

static void test_mask_transparency(void) {
    ImgFile img;
    assert(img_open(&img, test_multi_img, sizeof(test_multi_img)));

    uint32_t palette[2] = {0x00000000, 0x00FFFFFF};
    ImgMultiSprite multi;
    assert(img_decode_multi(&img, 0, palette, 2, &multi));

    /* Mask 0xFF00: first 8 bits set, last 8 clear. But width=8, so all visible */
    for (int i = 0; i < 8; i++)
        assert((multi.frames[0].pixels[i] & 0xFF000000) == 0xFF000000);

    img_multi_free(&multi);
    img_close(&img);
}

static void test_reject_multi_as_simple(void) {
    ImgFile img;
    assert(img_open(&img, test_multi_img, sizeof(test_multi_img)));
    ImgSprite sprite;
    assert(!img_decode_sprite(&img, 0, NULL, 0, &sprite));
    img_close(&img);
}

int main(void) {
    test_open();
    test_bad_magic();
    test_reject_unsorted_offsets();
    test_rejects_offset_without_flags();
    test_decode_simple();
    test_decode_multi();
    test_mask_transparency();
    test_reject_multi_as_simple();

    ImgFile reopened;
    assert(img_open(&reopened, test_multi_img, sizeof(test_multi_img)));
    uint8_t invalid[10] = {0};
    memcpy(invalid, "ImgA", 4);
    invalid[5] = 1; /* one sprite, but its offset is outside the buffer */
    assert(!img_open(&reopened, invalid, sizeof(invalid)));
    assert(reopened.data == NULL && reopened.size == 0 && reopened.sprite_count == 0);
    printf("All liberation_img tests passed\n");
    return 0;
}
