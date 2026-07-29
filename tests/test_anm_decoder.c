#include "anm_decoder.h"
#include <stdio.h>
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

int main(void) {
    test_null_input();
    test_too_small();
    printf("All ANM decoder tests passed\n");
    return 0;
}
