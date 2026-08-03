#include "liberation_data.h"
#include "liberation_vgm.h"
#include "sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void digest_hex(const uint8_t digest[32], char hex[65]) {
    static const char digits[] = "0123456789abcdef";
    for (int i = 0; i < 32; ++i) {
        hex[i * 2] = digits[digest[i] >> 4];
        hex[i * 2 + 1] = digits[digest[i] & 0x0f];
    }
    hex[64] = '\0';
}

static int verify_vgm(const ISOImage *iso, const ISOEntry *entry,
                      unsigned *pass, unsigned *fail) {
    uint8_t *bytes = iso_read_file(iso, entry->lba, entry->size);
    if (!bytes) return 0;

    if (entry->size != 167766 || memcmp(bytes, "AmSp", 4) != 0) {
        free(bytes);
        return 0;
    }

    uint8_t digest[32];
    char hex[65];
    sha256_digest(bytes, entry->size, digest);
    digest_hex(digest, hex);

    VgmFile vgm;
    if (!vgm_open(&vgm, bytes, entry->size)) {
        printf("FAIL: %.16s... vgm_open failed\n", hex);
        (*fail)++;
        free(bytes);
        return 1;
    }

    if (vgm.num_banks != 4) {
        printf("FAIL: %.16s... expected 4 banks, got %u\n", hex, vgm.num_banks);
        (*fail)++;
        free(bytes);
        return 1;
    }

    AmosSprite sprite;
    unsigned sprites_ok = 0;
    for (unsigned i = 0; i < vgm.total_sprites; i++) {
        if (vgm_get_sprite(&vgm, i, &sprite))
            sprites_ok++;
    }

    if (sprites_ok == vgm.total_sprites) {
        printf("OK:   %.16s... banks=%u sprites=%u\n",
               hex, vgm.num_banks, vgm.total_sprites);
        (*pass)++;
    } else {
        printf("FAIL: %.16s... %u/%u sprites decoded\n",
               hex, sprites_ok, vgm.total_sprites);
        (*fail)++;
    }

    free(bytes);
    return 1;
}

static void scan_dir(const ISOImage *iso, uint32_t lba, uint32_t size,
                     unsigned depth, unsigned *pass, unsigned *fail) {
    if (depth > 8) return;
    ISOEntry entries[256];
    int count = iso_list_dir(iso, lba, size, entries, 256);
    for (int i = 0; i < count; ++i) {
        if (entries[i].is_dir)
            scan_dir(iso, entries[i].lba, entries[i].size, depth + 1, pass, fail);
        else
            verify_vgm(iso, &entries[i], pass, fail);
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <data-dir>\n", argv[0]);
        return 2;
    }
    DataVFS vfs;
    LiberationData data = {0};
    if (!vfs_init(&vfs, argv[1]) || !liberation_data_open(&data, &vfs)) {
        fprintf(stderr, "verified Liberation data was not found\n");
        liberation_data_close(&data);
        vfs_free(&vfs);
        return 1;
    }

    if (data.source != LIBERATION_SOURCE_CD32) {
        fprintf(stderr, "CD32 ISO required for VGM verification\n");
        liberation_data_close(&data);
        vfs_free(&vfs);
        return 1;
    }

    unsigned pass = 0, fail = 0;
    scan_dir(&data.iso, data.iso.root_lba, data.iso.root_size, 0, &pass, &fail);
    printf("\nVGM wall textures: %u passed, %u failed, %u total\n",
           pass, fail, pass + fail);

    liberation_data_close(&data);
    vfs_free(&vfs);
    return fail > 0 ? 1 : 0;
}
