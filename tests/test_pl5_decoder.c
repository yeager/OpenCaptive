#include "pl5_decoder.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static void test_null_input(void) {
    PL5Image img;
    assert(!pl5_decode(NULL, 0, &img));
    assert(!pl5_decode(NULL, 100, &img));
}

static void test_wrong_size(void) {
    uint8_t data[4] = {0};
    PL5Image img;
    assert(!pl5_decode(data, sizeof(data), &img));

    uint8_t big[50000];
    memset(big, 0, sizeof(big));
    assert(!pl5_decode(big, sizeof(big), &img));
}

static void test_decode_zeros(void) {
    uint8_t data[PL5_FILE_SIZE];
    memset(data, 0, sizeof(data));
    PL5Image img;
    assert(pl5_decode(data, sizeof(data), &img));
    assert(img.width == PL5_WIDTH);
    assert(img.height == PL5_HEIGHT);
    assert(img.data_size == PL5_PIXEL_COUNT);
    for (int i = 0; i < PL5_PIXEL_COUNT; i++) {
        assert(img.pixel_data[i] == 0);
    }
    pl5_free(&img);
}

static void test_decode_known_pattern(void) {
    // 5 bytes -> 8 pixels using the documented algorithm
    uint8_t data[PL5_FILE_SIZE];
    memset(data, 0, sizeof(data));

    // Set first 5 bytes to known values: 0xE0, 0xB7, 0xFB, 0xB7, 0xB7
    // Expected output: 0, 23, 23, 23, 23, 23, 23, 23
    data[0] = 0xE0;
    data[1] = 0xB7;
    data[2] = 0xFB;
    data[3] = 0xB7;
    data[4] = 0xB7;

    PL5Image img;
    assert(pl5_decode(data, sizeof(data), &img));

    uint8_t expected[8] = {0, 23, 23, 23, 23, 23, 23, 23};
    for (int i = 0; i < 8; i++) {
        assert(img.pixel_data[i] == expected[i]);
    }
    pl5_free(&img);
}

static void test_palette_nonzero(void) {
    assert(pl5_default_palette[17] == 0xFFFCFCFC); // white
    assert(pl5_default_palette[0] == 0xFF000000);   // black
    assert(pl5_default_palette[6] == 0xFF50F050);   // bright green
}

int main(void) {
    test_null_input();
    test_wrong_size();
    test_decode_zeros();
    test_decode_known_pattern();
    test_palette_nonzero();
    printf("All PL5 decoder tests passed\n");
    return 0;
}
