#include "liberation_data.h"
#include "sha256.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* This inventory deliberately reports only content hashes. ISO path names are
 * not stable identifiers for game resources. */
static uint32_t be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

static uint16_t be16(const uint8_t *p) {
    return ((uint16_t)p[0] << 8) | p[1];
}

static void hex_digest(const uint8_t digest[32], char out[65]) {
    static const char digits[] = "0123456789abcdef";
    for (unsigned i = 0; i < 32; ++i) {
        out[i * 2] = digits[digest[i] >> 4];
        out[i * 2 + 1] = digits[digest[i] & 15U];
    }
    out[64] = '\0';
}

static void print_form(const uint8_t *bytes, size_t size) {
    if (size < 12 || memcmp(bytes, "FORM", 4) != 0) return;
    uint32_t declared = be32(bytes + 4);
    size_t end = declared <= size - 8 ? (size_t)declared + 8 : size;
    char type[5] = {0};
    memcpy(type, bytes + 8, 4);
    uint8_t digest[32];
    char hex[65];
    sha256_digest(bytes, size, digest);
    hex_digest(digest, hex);

    unsigned chunks = 0;
    char chunk_types[161] = {0};
    size_t chunk_types_len = 0;
    const uint8_t *bmhd = NULL;
    for (size_t offset = 12; offset + 8 <= end;) {
        uint32_t chunk_size = be32(bytes + offset + 4);
        size_t next = offset + 8U + chunk_size + (chunk_size & 1U);
        if (next > end) break;
        ++chunks;
        if (chunk_types_len + 5 < sizeof(chunk_types)) {
            if (chunk_types_len) chunk_types[chunk_types_len++] = ',';
            memcpy(chunk_types + chunk_types_len, bytes + offset, 4);
            chunk_types_len += 4;
            chunk_types[chunk_types_len] = '\0';
        }
        if (memcmp(bytes + offset, "BMHD", 4) == 0 && chunk_size >= 20)
            bmhd = bytes + offset + 8;
        offset = next;
    }
    if (bmhd) {
        printf("%s form=%s chunks=%u types=%s image=%ux%u depth=%u compression=%u\n",
               hex, type, chunks, chunk_types, be16(bmhd), be16(bmhd + 2), bmhd[8], bmhd[10]);
    } else {
        printf("%s form=%s chunks=%u types=%s\n", hex, type, chunks, chunk_types);
    }
}

static void inventory_dir(const ISOImage *iso, uint32_t lba, uint32_t size,
                          unsigned depth) {
    if (depth > 16) return;
    ISOEntry entries[256];
    int count = iso_list_dir(iso, lba, size, entries, 256);
    for (int i = 0; i < count; ++i) {
        if (entries[i].is_dir) {
            inventory_dir(iso, entries[i].lba, entries[i].size, depth + 1);
            continue;
        }
        uint8_t *bytes = iso_read_file(iso, entries[i].lba, entries[i].size);
        if (bytes) {
            print_form(bytes, entries[i].size);
            free(bytes);
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <data-dir>\n", argv[0]);
        return 2;
    }
    DataVFS vfs;
    LiberationData data = {0};
    bool ok = vfs_init(&vfs, argv[1]) && liberation_data_open(&data, &vfs);
    if (!ok) {
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
