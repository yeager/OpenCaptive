#include "liberation_data.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <data-dir> <output-file>\n", argv[0]);
        return 2;
    }
    DataVFS vfs;
    LiberationData data = {0};
    if (!vfs_init(&vfs, argv[1]) || !liberation_data_open(&data, &vfs)) {
        fprintf(stderr, "failed to open liberation data\n");
        liberation_data_close(&data);
        vfs_free(&vfs);
        return 1;
    }
    size_t size = 0;
    uint8_t *bin = liberation_data_read(&data, LIBERATION_RESOURCE_PLOT_GENERATOR, &size);
    if (!bin) {
        fprintf(stderr, "failed to read PlotGen\n");
        liberation_data_close(&data);
        vfs_free(&vfs);
        return 1;
    }
    printf("PlotGen: %zu bytes\n", size);
    FILE *f = fopen(argv[2], "wb");
    if (f) {
        fwrite(bin, 1, size, f);
        fclose(f);
        printf("written to %s\n", argv[2]);
    }
    free(bin);
    liberation_data_close(&data);
    vfs_free(&vfs);
    return 0;
}
