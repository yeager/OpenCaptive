#include "iso9660_reader.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void put_le32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16); p[3] = (uint8_t)(value >> 24);
}

static uint8_t *sector(uint8_t *image, int lba) {
    return image + lba * ISO_RAW_SECTOR + 16;
}

int main(void) {
    uint8_t image[20 * ISO_RAW_SECTOR];
    memset(image, 0, sizeof(image));
    uint8_t *pvd = sector(image, 16);
    pvd[0] = 1; memcpy(pvd + 1, "CD001", 5); memcpy(pvd + 40, "TEST_DISC", 9);
    pvd[156] = 34;
    put_le32(pvd + 158, 17);
    put_le32(pvd + 166, ISO_SECTOR_SIZE);

    uint8_t *root = sector(image, 17);
    root[0] = 41; put_le32(root + 2, 18); put_le32(root + 10, 4);
    root[32] = 8; memcpy(root + 33, "GAME.DAT", 8);
    memcpy(sector(image, 18), "DATA", 4);

    ISOImage iso;
    assert(iso_open_raw(&iso, image, sizeof(image)));
    assert(strcmp(iso.volume_id, "TEST_DISC") == 0);
    ISOEntry entries[2];
    assert(iso_list_root(&iso, entries, 2) == 1);
    assert(strcmp(entries[0].name, "GAME.DAT") == 0);
    uint8_t *file = iso_read_file(&iso, entries[0].lba, entries[0].size);
    assert(file && memcmp(file, "DATA", 4) == 0);
    free(file);

    root[0] = 250; root[32] = 250;
    assert(iso_list_root(&iso, entries, 2) == 0);
    puts("All ISO9660 reader tests passed");
    return 0;
}
