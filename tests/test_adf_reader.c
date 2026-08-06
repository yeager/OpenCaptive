#include "adf_reader.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void put_be32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)(value >> 24);
    p[1] = (uint8_t)(value >> 16);
    p[2] = (uint8_t)(value >> 8);
    p[3] = (uint8_t)value;
}

int main(void) {
    static uint8_t image[ADF_DISK_SIZE + ADF_SECTOR_SIZE];
    ADFDisk disk;
    memset(&disk, 0xA5, sizeof(disk));
    assert(!adf_open(&disk, image, ADF_DISK_SIZE - 1U));
    assert(disk.data == NULL && disk.size == 0);

    memcpy(image, "DOS\0", 4);
    uint8_t *root = image + ADF_ROOT_BLOCK * ADF_SECTOR_SIZE;
    put_be32(root, 2);                         /* T_HEADER */
    put_be32(root + ADF_SECTOR_SIZE - 4, 1);  /* ST_ROOT */
    put_be32(root + 24, ADF_ROOT_BLOCK + 1U);

    uint8_t *entry = image + (ADF_ROOT_BLOCK + 1U) * ADF_SECTOR_SIZE;
    put_be32(entry, 2);                        /* T_HEADER */
    put_be32(entry + 4, ADF_ROOT_BLOCK + 1U);
    put_be32(entry + 496, ADF_ROOT_BLOCK + 1U); /* cyclic hash chain */
    put_be32(entry + ADF_SECTOR_SIZE - 4, 0xfffffffdu);

    assert(adf_open(&disk, image, ADF_DISK_SIZE));
    ADFEntry entries[2];
    /* The bounded traversal must terminate even though the chain cycles. */
    assert(adf_list_root(&disk, entries, 2) == 2);

    put_be32(entry + 324, 0);
    uint32_t empty_size = 99;
    uint8_t *empty = adf_read_file(&disk, ADF_ROOT_BLOCK + 1U, &empty_size);
    assert(empty != NULL);
    assert(empty_size == 0);
    free(empty);

    /* Extra bytes after a standard ADF are not additional disk blocks. */
    put_be32(image + ADF_DISK_SIZE, 2);
    assert(adf_read_file(&disk, ADF_DISK_SIZE / ADF_SECTOR_SIZE, NULL) == NULL);

    put_be32(entry, 8); /* T_DATA is not a file header */
    assert(adf_read_file(&disk, ADF_ROOT_BLOCK + 1U, NULL) == NULL);

    image[0] = 'X'; /* A failed reopen must not leave a usable-looking disk. */
    assert(!adf_open(&disk, image, ADF_DISK_SIZE));
    assert(disk.data == NULL && disk.size == 0);
    puts("All ADF reader tests passed");
    return 0;
}
