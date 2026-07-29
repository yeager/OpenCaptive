#include "liberation_data.h"
#include "iso9660_reader.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s <data-dir> <resource-sha256> <output>\n", argv[0]);
        return 2;
    }
    DataVFS vfs;
    LiberationData data = {0};
    if (!vfs_init(&vfs, argv[1]) || !liberation_data_open(&data, &vfs)) {
        liberation_data_close(&data);
        vfs_free(&vfs);
        fprintf(stderr, "verified Liberation CD32 data was not found\n");
        return 1;
    }

    size_t size = 0;
    uint8_t *bytes = iso_read_file_sha256(&data.iso, argv[2], &size);
    liberation_data_close(&data);
    vfs_free(&vfs);
    if (!bytes) {
        fprintf(stderr, "resource hash not found in verified CD32 ISO\n");
        return 1;
    }
    FILE *output = fopen(argv[3], "wb");
    bool ok = false;
    if (output) {
        ok = fwrite(bytes, 1, size, output) == size;
        if (fclose(output) != 0) ok = false;
    }
    free(bytes);
    if (!ok) {
        fprintf(stderr, "could not write output\n");
        return 1;
    }
    printf("extracted %zu bytes from verified ISO resource hash\n", size);
    return 0;
}
