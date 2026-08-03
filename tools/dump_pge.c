#include "liberation_data.h"
#include "arcd_decoder.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <data-dir> [resource: pge|dte|cte]\n", argv[0]);
        return 2;
    }
    LiberationResource res = LIBERATION_RESOURCE_PLOT_TEXT;
    const char *name = "PGE";
    if (argc >= 3) {
        if (argv[2][0] == 'd' || argv[2][0] == 'D') {
            res = LIBERATION_RESOURCE_DIALOGUE_TEXT;
            name = "DTE";
        } else if (argv[2][0] == 'c' || argv[2][0] == 'C') {
            res = LIBERATION_RESOURCE_CITY_TEXT;
            name = "CTE";
        }
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
    uint8_t *bin = liberation_data_read(&data, res, &size);
    if (!bin) {
        fprintf(stderr, "failed to read %s\n", name);
        liberation_data_close(&data);
        vfs_free(&vfs);
        return 1;
    }
    printf("%s: %zu bytes (compressed)\n", name, size);

    size_t dec_size = arcd_decompressed_size(bin, size);
    if (dec_size == 0) {
        printf("Not ArcD compressed, dumping raw:\n");
        fwrite(bin, 1, size, stdout);
    } else {
        uint8_t *dec = malloc(dec_size);
        int result = arcd_decode(bin, size, dec, dec_size);
        if (result < 0) {
            fprintf(stderr, "ArcD decode failed\n");
        } else {
            printf("Decompressed: %d bytes\n\n", result);
            fwrite(dec, 1, (size_t)result, stdout);
            printf("\n");
        }
        free(dec);
    }
    free(bin);
    liberation_data_close(&data);
    vfs_free(&vfs);
    return 0;
}
