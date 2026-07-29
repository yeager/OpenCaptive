#include "liberation_data.h"
#include "sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Inventory by content, never by ISO filename.  This is intentionally an
 * analysis boundary: it lets reverse-engineering work select stable resources
 * by SHA-256 even when media dumps use different directory naming. */
static const char *container_kind(const uint8_t *bytes, size_t size) {
    if (size >= 12 && memcmp(bytes, "FORM", 4) == 0) {
        if (memcmp(bytes + 8, "ILBM", 4) == 0) return "IFF/ILBM";
        if (memcmp(bytes + 8, "ANIM", 4) == 0) return "IFF/ANIM";
        if (memcmp(bytes + 8, "8SVX", 4) == 0) return "IFF/8SVX";
        return "IFF/FORM";
    }
    if (size >= 4 && memcmp(bytes, "RNC\1", 4) == 0) return "RNC1";
    if (size >= 4 && memcmp(bytes, "RNC\2", 4) == 0) return "RNC2-old";
    if (size >= 4 && memcmp(bytes, "AmSp", 4) == 0) return "AMOS-sprite-bank";
    if (size >= 4 && memcmp(bytes, "AmIc", 4) == 0) return "AMOS-icon-bank";
    if (size >= 4 && bytes[0] == 0 && bytes[1] == 0 && bytes[2] == 3 &&
        (bytes[3] == 0xf3 || bytes[3] == 0xe7)) return "Amiga-HUNK";
    return "raw";
}

static void digest_hex(const uint8_t digest[32], char hex[65]) {
    static const char digits[] = "0123456789abcdef";
    for (int i = 0; i < 32; ++i) {
        hex[i * 2] = digits[digest[i] >> 4];
        hex[i * 2 + 1] = digits[digest[i] & 0x0f];
    }
    hex[64] = '\0';
}

static void print_file(const ISOImage *iso, const ISOEntry *entry) {
    uint8_t *bytes = iso_read_file(iso, entry->lba, entry->size);
    if (!bytes) return;
    uint8_t digest[32];
    char hex[65];
    sha256_digest(bytes, entry->size, digest);
    digest_hex(digest, hex);
    printf("%s size=%u kind=%s\n", hex, entry->size,
           container_kind(bytes, entry->size));
    free(bytes);
}

static void inventory_dir(const ISOImage *iso, uint32_t lba, uint32_t size,
                          unsigned depth) {
    if (depth > 8) return;
    ISOEntry entries[256];
    int count = iso_list_dir(iso, lba, size, entries, 256);
    for (int i = 0; i < count; ++i) {
        /* Deliberately ignore entries[i].name: resource identity is content. */
        if (entries[i].is_dir)
            inventory_dir(iso, entries[i].lba, entries[i].size, depth + 1);
        else
            print_file(iso, &entries[i]);
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
        fprintf(stderr, "verified Liberation CD32 data was not found\n");
        liberation_data_close(&data);
        vfs_free(&vfs);
        return 1;
    }
    inventory_dir(&data.iso, data.iso.root_lba, data.iso.root_size, 0);
    liberation_data_close(&data);
    vfs_free(&vfs);
    return 0;
}
