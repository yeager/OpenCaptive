#include "amiga_ofs.h"
#include "data_vfs.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "usage: %s <data-dir> <adf-sha256> <file-sha256> <output>\n", argv[0]);
        return 2;
    }
    DataVFS vfs;
    if (!vfs_init(&vfs, argv[1])) return 1;
    size_t adf_size = 0;
    uint8_t *adf = vfs_find_sha256(&vfs, argv[2], &adf_size);
    if (!adf) {
        vfs_free(&vfs);
        fprintf(stderr, "verified ADF image was not found\n");
        return 1;
    }
    size_t file_size = 0;
    uint8_t *file = amiga_ofs_find_file_sha256(adf, adf_size, argv[3], &file_size);
    free(adf);
    vfs_free(&vfs);
    if (!file) {
        fprintf(stderr, "verified OFS file was not found\n");
        return 1;
    }
    FILE *output = fopen(argv[4], "wb");
    int ok = output && fwrite(file, 1, file_size, output) == file_size;
    if (output && fclose(output) != 0) ok = 0;
    free(file);
    if (!ok) {
        fprintf(stderr, "could not write output\n");
        return 1;
    }
    printf("extracted %zu bytes from verified OFS data\n", file_size);
    return 0;
}
