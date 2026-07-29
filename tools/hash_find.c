#include "data_vfs.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr, "usage: %s <data-dir> <sha256>\n", argv[0]); return 2; }
    DataVFS vfs;
    if (!vfs_init(&vfs, argv[1])) return 1;
    size_t size = 0;
    uint8_t *data = vfs_find_sha256(&vfs, argv[2], &size);
    vfs_free(&vfs);
    if (!data) return 1;
    printf("found %zu bytes\n", size);
    free(data);
    return 0;
}
