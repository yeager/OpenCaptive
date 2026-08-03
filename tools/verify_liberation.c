#include "data_vfs.h"
#include "liberation_data.h"
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    DataVFS vfs;
    LiberationData data = {0};
    int ok = vfs_init(&vfs, argv[1]) && liberation_data_open(&data, &vfs);
    const char *src_name = data.source == LIBERATION_SOURCE_AMIGA_ADF ? "Amiga ADF" : "CD32";
    printf("Liberation data: %s (%s)\n", ok ? "verified" : "not verified", src_name);
    if (ok) {
        printf("Original presentation: intro=%s/%s city=%s/%s\n",
               data.intro_frame.bitplanes ? "decoded" : "unavailable",
               data.intro_script.bytes ? "SCPT" : "no-SCPT",
               data.city_frame.bitplanes ? "decoded" : "unavailable",
               data.city_script.bytes ? "SCPT" : "no-SCPT");
    }
    liberation_data_close(&data);
    vfs_free(&vfs);
    return ok ? 0 : 1;
}
