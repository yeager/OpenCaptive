#include "ctv_decoder.h"
#include <assert.h>
#include <string.h>

static void test_reject_too_small(void) {
    CtvFile ctv;
    uint8_t data[10] = {0};
    assert(!ctv_decode(&ctv, data, sizeof(data)));
}

static void test_reject_bad_magic(void) {
    CtvFile ctv;
    uint8_t data[32] = {0};
    memcpy(data, "Not a Creative Voice", 20);
    assert(!ctv_decode(&ctv, data, sizeof(data)));
}

static void test_decode_minimal(void) {
    /* Build a minimal valid CTV file with one type-1 sound block. */
    uint8_t buf[64];
    memset(buf, 0, sizeof(buf));

    /* Header: "Creative Voice File\x1a" */
    memcpy(buf, "Creative Voice File\x1a", 20);
    /* Data offset = 26 */
    buf[20] = 26; buf[21] = 0;
    /* Version 1.10 */
    buf[22] = 0x0a; buf[23] = 0x01;
    /* Version check */
    buf[24] = 0x29; buf[25] = 0x11;

    /* Type 1 block: 2 bytes header + 4 bytes PCM = 6 bytes total length */
    buf[26] = 1;            /* block type */
    buf[27] = 6; buf[28] = 0; buf[29] = 0;  /* block length = 6 */
    buf[30] = 131;          /* sr_code: 1000000/(256-131) = 8000 Hz */
    buf[31] = 0;            /* codec: unsigned PCM */
    buf[32] = 0x40;         /* sample data */
    buf[33] = 0x60;
    buf[34] = 0x80;
    buf[35] = 0x20;

    /* Terminator */
    buf[36] = 0;

    CtvFile ctv;
    assert(ctv_decode(&ctv, buf, 37));
    assert(ctv.count == 1);
    assert(ctv.entries[0].length == 4);
    assert(ctv.entries[0].sample_rate == 8000);
    ctv_free(&ctv);
}

static void test_null_input(void) {
    CtvFile ctv;
    assert(!ctv_decode(&ctv, NULL, 0));
    assert(!ctv_decode(NULL, (const uint8_t *)"", 1));
}

int main(void) {
    test_reject_too_small();
    test_reject_bad_magic();
    test_decode_minimal();
    test_null_input();
    return 0;
}
