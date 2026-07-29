#include "data_vfs.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s <data-dir> <sha256> <output>\n", argv[0]);
        return 2;
    }
    DataVFS vfs;
    if (!vfs_init(&vfs, argv[1])) return 1;
    size_t size = 0;
    uint8_t *data = vfs_find_sha256(&vfs, argv[2], &size);
    vfs_free(&vfs);
    if (!data) {
        fprintf(stderr, "content hash not found\n");
        return 1;
    }

    FILE *output = fopen(argv[3], "wb");
    if (!output) {
        free(data);
        return 1;
    }
    bool ok = fwrite(data, 1, size, output) == size && fclose(output) == 0;
    free(data);
    if (!ok) {
        fprintf(stderr, "could not write output\n");
        return 1;
    }
    printf("extracted %zu bytes from verified hash\n", size);
    return 0;
}
