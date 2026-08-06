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

    uint8_t wrapped_table[64] = {0};
    pos = 0;
    put_be32(wrapped_table, &pos, 0x3F3);
    put_be32(wrapped_table, &pos, 0);
    put_be32(wrapped_table, &pos, 0);          /* wrapped table size */
    put_be32(wrapped_table, &pos, 0);
    put_be32(wrapped_table, &pos, 0xFFFFFFFFU);
    put_be32(wrapped_table, &pos, 0x3E9);      /* would be code */
    put_be32(wrapped_table, &pos, 0);
    put_be32(wrapped_table, &pos, 0x3F2);
    assert(!amiga_hunk_parse(wrapped_table, pos, &info));

    uint8_t flagged_bss[64] = {0};
    pos = 0;
    put_be32(flagged_bss, &pos, 0x3F3);       /* header */
    put_be32(flagged_bss, &pos, 0);           /* resident names */
    put_be32(flagged_bss, &pos, 1);           /* table size */
    put_be32(flagged_bss, &pos, 0);
    put_be32(flagged_bss, &pos, 0);
    put_be32(flagged_bss, &pos, 1);           /* allocation table */
    put_be32(flagged_bss, &pos, 0x400003EB);  /* flagged HUNK_BSS */
    put_be32(flagged_bss, &pos, 4);
    put_be32(flagged_bss, &pos, 0x3F2);       /* end */
    assert(amiga_hunk_parse(flagged_bss, pos, &info));
    assert(info.bss_count == 1);

    uint8_t out_of_order[96] = {0};
    pos = 0;
    put_be32(out_of_order, &pos, 0x3F3);      /* header */
    put_be32(out_of_order, &pos, 0);          /* resident names */
    put_be32(out_of_order, &pos, 2);          /* two hunks */
    put_be32(out_of_order, &pos, 0);
    put_be32(out_of_order, &pos, 1);
    put_be32(out_of_order, &pos, 0);
    put_be32(out_of_order, &pos, 0x3F2);      /* closes no hunk */
    put_be32(out_of_order, &pos, 0x3E9);      /* code hunk */
    put_be32(out_of_order, &pos, 0);
    put_be32(out_of_order, &pos, 0x3EA);      /* data hunk */
    put_be32(out_of_order, &pos, 0);
    put_be32(out_of_order, &pos, 0x3F2);
    assert(!amiga_hunk_parse(out_of_order, pos, &info));
    puts("All Amiga HUNK tests passed");
    return 0;
}
