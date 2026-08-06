#include "creature_sprite.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void test_load_synthetic(void) {
    PL5Image sheet;
    sheet.width = PL5_WIDTH + 16;
    sheet.height = PL5_HEIGHT;
    sheet.data_size = (size_t)sheet.width * sheet.height;
    sheet.pixel_data = calloc(sheet.data_size, 1);
    assert(sheet.pixel_data);
    memset(sheet.palette, 0, sizeof(sheet.palette));
    sheet.palette[1] = 0xFFFF0000;

    /* Draw a non-zero pixel in frames 0 and 1 */
    sheet.pixel_data[5 * sheet.width + 5] = 1;
    sheet.pixel_data[5 * sheet.width + (CREATURE_FRAME_W + 5)] = 1;

    CreatureSpriteSet css;
    assert(creature_sprite_load(&sheet, &css));
    assert(css.num_frames == CREATURE_MAX_FRAMES);
    assert(css.frames[0].valid);
    assert(css.frames[1].valid);
    assert(!css.frames[2].valid);
    assert(css.frames[0].pixels[5][5] == 1);

    free(sheet.pixel_data);
}

static void test_blit(void) {
    CreatureFrame frame;
    memset(&frame, 0, sizeof(frame));
    frame.valid = true;
    frame.pixels[0][0] = 1;
    frame.pixels[1][1] = 2;

    uint32_t palette[PL5_COLORS] = {0};
    palette[1] = 0xFFAABBCC;
    palette[2] = 0xFF112233;

    uint32_t fb[64 * 64];
    memset(fb, 0, sizeof(fb));
    creature_sprite_blit(&frame, palette, fb, 64, 64, 10, 10, 1);
    assert(fb[10 * 64 + 10] == 0xFFAABBCC);
    assert(fb[11 * 64 + 11] == 0xFF112233);
    assert(fb[0] == 0);
}

static void test_blit_scale(void) {
    CreatureFrame frame;
    memset(&frame, 0, sizeof(frame));
    frame.valid = true;
    frame.pixels[0][0] = 1;

    uint32_t palette[PL5_COLORS] = {0};
    palette[1] = 0xFFFF0000;

    uint32_t fb[128 * 128];
    memset(fb, 0, sizeof(fb));
    creature_sprite_blit(&frame, palette, fb, 128, 128, 0, 0, 2);
    assert(fb[0 * 128 + 0] == 0xFFFF0000);
    assert(fb[0 * 128 + 1] == 0xFFFF0000);
    assert(fb[1 * 128 + 0] == 0xFFFF0000);
    assert(fb[1 * 128 + 1] == 0xFFFF0000);
    assert(fb[0 * 128 + 2] == 0);
}

static void test_null_safety(void) {
    CreatureSpriteSet css;
    assert(!creature_sprite_load(NULL, &css));
    assert(!creature_sprite_load(NULL, NULL));
    creature_sprite_blit(NULL, NULL, NULL, 0, 0, 0, 0, 1);
}

int main(void) {
    test_load_synthetic();
    test_blit();
    test_blit_scale();
    test_null_safety();
    printf("All creature sprite tests passed.\n");
    return 0;
}
