#include "pl5_decoder.h"
#include <stdio.h>
#include <assert.h>

static void test_null_input(void) {
    PL5Image img;
    assert(!pl5_decode(NULL, 0, &img));
    assert(!pl5_decode(NULL, 100, &img));
}

static void test_too_small(void) {
    uint8_t data[4] = {0};
    PL5Image img;
    assert(!pl5_decode(data, sizeof(data), &img));
}

int main(void) {
    test_null_input();
    test_too_small();
    printf("All PL5 decoder tests passed\n");
    return 0;
}
