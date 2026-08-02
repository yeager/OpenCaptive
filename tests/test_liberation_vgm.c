#include "liberation_vgm.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static void build_amsp_bank(uint8_t *buf, size_t *out_size,
                            unsigned count, unsigned w_words, unsigned h,
                            unsigned depth) {
    size_t pos = 0;
    buf[pos++] = 'A'; buf[pos++] = 'm'; buf[pos++] = 'S'; buf[pos++] = 'p';
    buf[pos++] = (uint8_t)(count >> 8);
    buf[pos++] = (uint8_t)(count & 0xFF);

    for (unsigned i = 0; i < count; i++) {
        unsigned bpr = w_words * 2;
        unsigned planes = depth + 1;
        size_t pixel_bytes = (size_t)bpr * h * planes;

        buf[pos++] = (uint8_t)(w_words >> 8);
        buf[pos++] = (uint8_t)(w_words & 0xFF);
        buf[pos++] = (uint8_t)(h >> 8);
        buf[pos++] = (uint8_t)(h & 0xFF);
        buf[pos++] = (uint8_t)(depth >> 8);
        buf[pos++] = (uint8_t)(depth & 0xFF);
        buf[pos++] = 0x80; buf[pos++] = 0x00;
        buf[pos++] = 0x00; buf[pos++] = 0x00;

        for (size_t j = 0; j < pixel_bytes; j++)
            buf[pos++] = (uint8_t)(i + j);

        if (pixel_bytes & 1)
            buf[pos++] = 0;
    }

    *out_size = pos;
}

static void test_single_bank(void) {
    uint8_t buf[4096];
    size_t sz;
    build_amsp_bank(buf, &sz, 3, 2, 8, 4);

    VgmFile vgm;
    assert(vgm_open(&vgm, buf, sz));
    assert(vgm.num_banks == 1);
    assert(vgm.total_sprites == 3);
    assert(vgm.bank_count[0] == 3);
}

static void test_multi_bank(void) {
    uint8_t buf[16384];
    size_t sz1, sz2, sz3;

    build_amsp_bank(buf, &sz1, 5, 2, 4, 4);
    build_amsp_bank(buf + sz1, &sz2, 3, 4, 8, 4);
    build_amsp_bank(buf + sz1 + sz2, &sz3, 2, 2, 4, 4);

    size_t total = sz1 + sz2 + sz3;

    VgmFile vgm;
    assert(vgm_open(&vgm, buf, total));
    assert(vgm.num_banks == 3);
    assert(vgm.total_sprites == 10);
    assert(vgm.bank_count[0] == 5);
    assert(vgm.bank_count[1] == 3);
    assert(vgm.bank_count[2] == 2);
    assert(vgm.bank_first_index[0] == 0);
    assert(vgm.bank_first_index[1] == 5);
    assert(vgm.bank_first_index[2] == 8);
}

static void test_sprite_access(void) {
    uint8_t buf[16384];
    size_t sz1, sz2;

    build_amsp_bank(buf, &sz1, 2, 2, 4, 4);
    build_amsp_bank(buf + sz1, &sz2, 3, 4, 8, 4);

    VgmFile vgm;
    assert(vgm_open(&vgm, buf, sz1 + sz2));

    AmosSprite sprite;
    assert(vgm_get_sprite(&vgm, 0, &sprite));
    assert(sprite.width == 32);
    assert(sprite.height == 4);

    assert(vgm_get_sprite(&vgm, 2, &sprite));
    assert(sprite.width == 64);
    assert(sprite.height == 8);

    assert(!vgm_get_sprite(&vgm, 5, &sprite));
}

static void test_invalid(void) {
    VgmFile vgm;
    assert(!vgm_open(&vgm, NULL, 0));
    assert(!vgm_open(&vgm, (const uint8_t *)"bad", 3));

    uint8_t bad[8] = {'N', 'O', 'P', 'E', 0, 0, 0, 0};
    assert(!vgm_open(&vgm, bad, sizeof(bad)));
}

int main(void) {
    test_single_bank();
    test_multi_bank();
    test_sprite_access();
    test_invalid();
    printf("All VGM tests passed\n");
    return 0;
}
