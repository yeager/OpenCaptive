#include "object_sprite.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void test_load(void) {
    PL5Image sheet;
    sheet.width = PL5_WIDTH + 16;
    sheet.height = PL5_HEIGHT;
    sheet.data_size = (size_t)sheet.width * sheet.height;
    sheet.pixel_data = calloc(sheet.data_size, 1);
    assert(sheet.pixel_data);
    memset(sheet.palette, 0, sizeof(sheet.palette));

    sheet.pixel_data[3 * sheet.width + 3] = 5;

    ObjectSpriteSet oss;
    assert(object_sprite_load(&sheet, &oss));
    assert(oss.num_frames == OBJECT_MAX_FRAMES);
    assert(oss.frames[0].valid);
    assert(oss.frames[0].pixels[3][3] == 5);
    assert(!oss.frames[1].valid);

    free(sheet.pixel_data);
}

static void test_load_rejects_short_pixel_buffer(void) {
    PL5Image sheet = {0};
    sheet.width = PL5_WIDTH;
    sheet.height = PL5_HEIGHT;
    sheet.pixel_data = (uint8_t *)1;
    sheet.data_size = PL5_PIXEL_COUNT - 1;
    ObjectSpriteSet sprites;
    assert(!object_sprite_load(&sheet, &sprites));
}

static void test_blit(void) {
    ObjectFrame frame;
    memset(&frame, 0, sizeof(frame));
    frame.valid = true;
    frame.pixels[0][0] = 1;

    uint32_t palette[PL5_COLORS] = {0};
    palette[1] = 0xFFDDEEFF;

    uint32_t fb[32 * 32];
    memset(fb, 0, sizeof(fb));
    object_sprite_blit(&frame, palette, fb, 32, 32, 5, 5, 1);
    assert(fb[5 * 32 + 5] == 0xFFDDEEFF);
    assert(fb[0] == 0);
}

int main(void) {
    test_load();
    test_load_rejects_short_pixel_buffer();
    test_blit();
    printf("All object sprite tests passed.\n");
    return 0;
}
