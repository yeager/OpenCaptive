#include "data_vfs.h"
#include "liberation_data.h"
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    DataVFS vfs;
    LiberationData data = {0};
    int ok = vfs_init(&vfs, argv[1]) && liberation_data_open(&data, &vfs);
    printf("Liberation CD32 data: %s\n", ok ? "verified" : "not verified");
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
