#include "rnc_decoder.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_rejects_truncated_forward_stream(void) {
    uint8_t stream[18] = {0};
    uint8_t output[1] = {0};
    memcpy(stream, "RNC\x01", 4);
    stream[7] = 1; /* one byte of advertised output */

    assert(rnc_decode(stream, (int)sizeof(stream), output,
                      (int)sizeof(output)) == -3);
}

static void test_rejects_missing_input_and_output_buffers(void) {
    uint8_t stream[18] = {0};
    uint8_t output[1] = {0};
    memcpy(stream, "RNC\x01", 4);

    assert(rnc_decode(NULL, (int)sizeof(stream), output, 1) == -1);
    assert(rnc_decode(stream, (int)sizeof(stream), NULL, 1) == -1);
}

int main(void) {
    test_rejects_truncated_forward_stream();
    test_rejects_missing_input_and_output_buffers();
    puts("All RNC decoder tests passed");
    return 0;
}
