#include "amiga_hunk.h"
#include <assert.h>
#include <stdio.h>

static void put_be32(uint8_t *data, size_t *offset, uint32_t value) {
    data[(*offset)++] = (uint8_t)(value >> 24);
    data[(*offset)++] = (uint8_t)(value >> 16);
    data[(*offset)++] = (uint8_t)(value >> 8);
    data[(*offset)++] = (uint8_t)value;
}

int main(void) {
    uint8_t hunk[64] = {0};
    size_t pos = 0;
    put_be32(hunk, &pos, 0x3F3); /* header */
    put_be32(hunk, &pos, 0);     /* resident names */
    put_be32(hunk, &pos, 1);     /* table size */
    put_be32(hunk, &pos, 0);
    put_be32(hunk, &pos, 0);
    put_be32(hunk, &pos, 2);     /* allocation: two longwords */
    put_be32(hunk, &pos, 0x3E9); /* code */
    put_be32(hunk, &pos, 2);
    put_be32(hunk, &pos, 0x4E75);
    put_be32(hunk, &pos, 0);
    put_be32(hunk, &pos, 0x3F2);

    AmigaHunkInfo info;
    assert(amiga_hunk_parse(hunk, pos, &info));
    assert(info.hunk_count == 1 && info.code_count == 1);
    assert(info.first_code_size == 8);
    assert(!amiga_hunk_parse(hunk, pos - 1, &info));
    hunk[3] = 0;
    assert(!amiga_hunk_parse(hunk, pos, &info));
    puts("All Amiga HUNK tests passed");
    return 0;
}
