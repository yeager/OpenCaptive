#include "amiga_ofs.h"
#include "data_vfs.h"

#include <stdio.h>
#include <stdlib.h>

static void print_hash(const uint8_t digest[32]) {
    for (int i = 0; i < 32; ++i) printf("%02x", digest[i]);
}

static void print_class(const uint8_t *data, size_t size) {
    if (size >= 18 && data[0] == 'R' && data[1] == 'N' && data[2] == 'C') {
        unsigned unpacked = ((unsigned)data[4] << 24) | ((unsigned)data[5] << 16) |
            ((unsigned)data[6] << 8) | data[7];
        printf("%s unpacked=%u", data[3] == 1 ? "RNC1" :
               data[3] == 2 ? "RNC2" : "RNC", unpacked);
        return;
    }
    if (size >= 4 && data[0] == 0x60 && data[1] == 0x1a) {
        printf("M68K-PRG");
        return;
    }
    if (size >= 4 && data[0] == 'F' && data[1] == 'O' && data[2] == 'R' && data[3] == 'M')
        printf("IFF");
    else
        printf("raw");
}

static int print_entry(const uint8_t *data, size_t size,
                       const uint8_t digest[32], void *context) {
    (void)context;
    printf("%8zu ", size);
    print_hash(digest);
    printf(" ");
    print_class(data, size);
    printf("\n");
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <data-dir> <adf-sha256>\n", argv[0]);
        return 2;
    }
    DataVFS vfs;
    if (!vfs_init(&vfs, argv[1])) return 1;
    size_t adf_size = 0;
    uint8_t *adf = vfs_find_sha256(&vfs, argv[2], &adf_size);
    vfs_free(&vfs);
    if (!adf) {
        fprintf(stderr, "verified ADF image was not found\n");
        return 1;
    }
    int count = amiga_ofs_visit_file_hashes(adf, adf_size, print_entry, NULL);
    free(adf);
    if (!count) {
        fprintf(stderr, "no reconstructible OFS files were found\n");
        return 1;
    }
    fprintf(stderr, "inventoried %d content-addressed OFS payload(s)\n", count);
    return 0;
}
