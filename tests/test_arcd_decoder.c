#include "arcd_decoder.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

static void test_bad_magic(void) {
    uint8_t data[12] = {0};
    assert(arcd_decompressed_size(data, 12) == 0);
    uint8_t out[16];
    assert(arcd_decode(data, 12, out, 16) == -1);
}

static void test_too_small(void) {
    uint8_t data[8] = {0x41, 0x72, 0x63, 0x44, 0,0,0,10};
    assert(arcd_decompressed_size(data, 8) == 0);
    assert(arcd_decode(data, 8, NULL, 0) == -1);
    uint8_t data2[12] = {0x41, 0x72, 0x63, 0x44, 0,0,0,10, 0,0,0,5};
    assert(arcd_decompressed_size(data2, 12) == 10);
    uint8_t out[4];
    assert(arcd_decode(data2, 12, out, 4) == -1);
}

static void test_decompressed_size(void) {
    uint8_t data[12] = {
        0x41, 0x72, 0x63, 0x44,
        0x00, 0x00, 0x3F, 0xB0,
        0x00, 0x00, 0x1B, 0xAD
    };
    assert(arcd_decompressed_size(data, 12) == 16304);
}

static void test_rejects_unreasonable_output_size(void) {
    uint8_t data[12] = {
        0x41, 0x72, 0x63, 0x44,
        0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x1B, 0xAD
    };
    assert(arcd_decompressed_size(data, sizeof(data)) == 0);
}

static void test_game_data_if_available(void) {
    const char *paths[] = {
        "PGE.txt", "DTE.txt", "CTE.txt"
    };
    size_t expected[] = { 16304, 14136, 17809 };
    const char *basedir = getenv("OPENCAPTIVE_TEST_DATA");
    if (!basedir) basedir = getenv("HOME");
    if (!basedir) {
        printf("SKIP: game_data (no OPENCAPTIVE_TEST_DATA or HOME)\n");
        return;
    }

    int verified = 0;
    for (int i = 0; i < 3; i++) {
        char path[512];
        snprintf(path, sizeof(path), "%s/.opencaptive/liberation/disk3/%s", basedir, paths[i]);
        FILE *f = fopen(path, "rb");
        if (!f) {
            /* A bare `continue` here let all three files be absent while
             * main() still printed "PASS: all arcd tests", reporting success
             * for a run that verified nothing. */
            printf("SKIP: game_data %s not available\n", paths[i]);
            continue;
        }
        verified++;

        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);
        uint8_t *src = malloc(fsize);
        fread(src, 1, fsize, f);
        fclose(f);

        size_t dsize = arcd_decompressed_size(src, fsize);
        assert(dsize == expected[i]);

        uint8_t *dst = malloc(dsize);
        int result = arcd_decode(src, fsize, dst, dsize);
        assert(result == (int)expected[i]);
        if (fsize > 12) {
            assert(arcd_decode(src, fsize - 1, dst, dsize) == -1);
        }

        printf("  %s: %d bytes OK\n", paths[i], result);

        free(dst);
        free(src);
    }
    if (verified == 0)
        printf("SKIP: game_data (0 of 3 files present, nothing verified)\n");
    else
        printf("PASS: game_data (%d of 3 files verified)\n", verified);
}

int main(void) {
    test_bad_magic();
    printf("PASS: bad_magic\n");
    test_too_small();
    printf("PASS: too_small\n");
    test_decompressed_size();
    printf("PASS: decompressed_size\n");
    test_rejects_unreasonable_output_size();
    printf("PASS: unreasonable_output_size\n");
    test_game_data_if_available();
    printf("PASS: arcd unit tests (see game_data line above for data coverage)\n");
    return 0;
}
