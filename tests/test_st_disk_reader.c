#include "st_disk_reader.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void put_le16(uint8_t *p, uint16_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

int main(void) {
    static uint8_t image[737280];
    STDisk disk;
    memset(&disk, 0xA5, sizeof(disk));
    assert(!st_disk_open(&disk, image, sizeof(image) - 1U));
    assert(disk.data == NULL && disk.size == 0);

    image[11] = 0x00; image[12] = 0x02;
    image[13] = 1;
    put_le16(image + 14, 1);
    image[16] = 2;
    put_le16(image + 17, 16);
    put_le16(image + 19, 1440);
    put_le16(image + 22, 1);

    image[512 + 3] = 0xFF;
    image[512 + 4] = 0x0F;
    image[2048] = 'O'; image[2049] = 'K';

    assert(st_disk_open(&disk, image, sizeof(image)));
    uint32_t out_size = 123;
    uint8_t *file = st_disk_read_file(&disk, 2, 2, &out_size);
    assert(file && out_size == 2 && memcmp(file, "OK", 2) == 0);
    free(file);

    /* A non-sector-aligned root directory must still round up before the
     * first data cluster.  With 17 entries, cluster 2 starts at byte 2560,
     * not at the unrounded byte 2080. */
    put_le16(image + 17, 17);
    image[2048] = 'B'; image[2049] = 'A';
    image[2560] = 'O'; image[2561] = 'K';
    assert(st_disk_open(&disk, image, sizeof(image)));
    out_size = 123;
    file = st_disk_read_file(&disk, 2, 2, &out_size);
    assert(file && out_size == 2 && memcmp(file, "OK", 2) == 0);
    free(file);
    file = st_disk_read_file(&disk, 2, 0, &out_size);
    assert(file && out_size == 0);
    free(file);
    put_le16(image + 17, 16);

    out_size = 123;
    assert(st_disk_read_file(&disk, 2, 513, &out_size) == NULL);
    assert(out_size == 0);
    assert(st_disk_list_root(&disk, NULL, 1) == 0);

    put_le16(image + 19, 1); /* BPB geometry cannot contain the data area. */
    assert(!st_disk_open(&disk, image, sizeof(image)));
    put_le16(image + 19, 1440);

    put_le16(image + 14, UINT16_MAX);
    image[16] = UINT8_MAX;
    put_le16(image + 22, UINT16_MAX);
    assert(!st_disk_open(&disk, image, sizeof(image)));
    puts("All ST disk reader tests passed");
    return 0;
}
