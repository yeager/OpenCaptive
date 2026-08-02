#include "adlib_data.h"
#include <assert.h>
#include <stdio.h>

static void test_patch_count(void) {
    assert(CAPTIVE_OPL2_PATCH_COUNT == 26);
}

static void test_sfx_count(void) {
    assert(CAPTIVE_SFX_COUNT == 49);
}

static void test_patches_not_empty(void) {
    for (int i = 0; i < CAPTIVE_OPL2_PATCH_COUNT; i++) {
        int nonzero = 0;
        for (int j = 0; j < 16; j++)
            if (captive_opl2_patches[i][j]) nonzero++;
        assert(nonzero > 0);
    }
}

static void test_sfx_entries_valid(void) {
    for (int i = 0; i < CAPTIVE_SFX_COUNT; i++) {
        assert(captive_sfx_table[i].data != NULL);
        assert(captive_sfx_table[i].size > 0);
        assert(captive_sfx_table[i].data[captive_sfx_table[i].size - 1] == 0xFF);
    }
}

static void test_ftable_populated(void) {
    int nonzero = 0;
    for (int i = 0; i < 128; i++)
        if (captive_opl2_ftable[i]) nonzero++;
    assert(nonzero > 64);
}

int main(void) {
    test_patch_count();
    test_sfx_count();
    test_patches_not_empty();
    test_sfx_entries_valid();
    test_ftable_populated();
    printf("All adlib_data tests passed.\n");
    return 0;
}
