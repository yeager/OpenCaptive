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
    uint8_t image[24 * ISO_RAW_SECTOR];
    memset(image, 0, sizeof(image));
    uint8_t *pvd = sector(image, 16);
    pvd[0] = 1; memcpy(pvd + 1, "CD001", 5); memcpy(pvd + 40, "TEST_DISC", 9);
    pvd[156] = 34;
    put_le32(pvd + 158, 17);
    put_le32(pvd + 166, ISO_SECTOR_SIZE);
    pvd[156 + 25] = 0x02; /* root extent is a directory */

    uint8_t *root = sector(image, 17);
    root[0] = 41; put_le32(root + 2, 18); put_le32(root + 10, 4);
    root[32] = 8; memcpy(root + 33, "GAME.DAT", 8);
    memcpy(sector(image, 18), "DATA", 4);
    root[41] = 36; put_le32(root + 43, 19); put_le32(root + 51, ISO_SECTOR_SIZE);
    root[41 + 25] = 0x02;
    root[41 + 32] = 3; memcpy(root + 41 + 33, "SUB", 3);
    uint8_t *sub = sector(image, 19);
    sub[0] = 36; put_le32(sub + 2, 20); put_le32(sub + 10, 6);
    sub[32] = 3; memcpy(sub + 33, "HUD", 3);
    memcpy(sector(image, 20), "NESTED", 6);

    ISOImage iso;
    assert(iso_open_raw(&iso, image, sizeof(image)));
    assert(strcmp(iso.volume_id, "TEST_DISC") == 0);
    ISOEntry entries[2];
    assert(iso_list_root(&iso, entries, 2) == 2);
    assert(strcmp(entries[0].name, "GAME.DAT") == 0);
    uint8_t *file = iso_read_file(&iso, entries[0].lba, entries[0].size);
    assert(file && memcmp(file, "DATA", 4) == 0);
    free(file);
    file = iso_read_file_sha256(&iso,
        "c97c29c7a71b392b437ee03fd17f09bb10b75e879466fc0eb757b2c4a78ac938", NULL);
    assert(file && memcmp(file, "DATA", 4) == 0);
    free(file);
    file = iso_read_file_sha256(&iso,
        "a5b612c96f272ac9ba5651c3326c6f74ec8212df0cf956c553a5ce9909594edf", NULL);
    assert(file && memcmp(file, "NESTED", 6) == 0);
    free(file);
    file = iso_read_file(&iso, entries[0].lba, 0);
    assert(file != NULL);
    free(file);

    assert(!iso_open_raw(&iso, NULL, 0));
    assert(iso.data == NULL && iso.size == 0 && iso.root_lba == 0);
    assert(!iso_open(&iso, NULL, 0));
    assert(iso.data == NULL && iso.size == 0 && iso.root_lba == 0);

    pvd[0] = 0; /* A failed reopen must leave no partially opened image. */
    assert(!iso_open_raw(&iso, image, sizeof(image)));
    assert(iso.data == NULL && iso.size == 0 && iso.root_lba == 0);

    pvd[0] = 1;
    pvd[156] = 33; /* Root records must include the fixed 34-byte prefix. */
    assert(!iso_open_raw(&iso, image, sizeof(image)));
    assert(iso.data == NULL && iso.size == 0 && iso.root_lba == 0);
    pvd[156] = 34;
    pvd[156 + 25] = 0; /* The root extent must be marked as a directory. */
    assert(!iso_open_raw(&iso, image, sizeof(image)));
    assert(iso.data == NULL && iso.size == 0 && iso.root_lba == 0);
    pvd[156 + 25] = 0x02;
    put_le32(pvd + 158, UINT32_MAX); /* Extent must fit in the image. */
    assert(!iso_open_raw(&iso, image, sizeof(image)));
    assert(iso.data == NULL && iso.size == 0 && iso.root_lba == 0);

    root[0] = 250; root[32] = 250;
    assert(iso_list_root(&iso, entries, 2) == 0);
    assert(iso_list_dir(&iso, UINT32_MAX, UINT32_MAX, entries, 2) == 0);
    assert(!iso_read_file(&iso, UINT32_MAX, ISO_SECTOR_SIZE));
    puts("All ISO9660 reader tests passed");
    return 0;
}
