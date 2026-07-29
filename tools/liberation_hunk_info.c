#include "amiga_hunk.h"
#include "liberation_data.h"
#include "iso9660_reader.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <data-dir> <resource-sha256>\n", argv[0]);
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

    AmigaHunkInfo info;
    bool ok = amiga_hunk_parse(bytes, size, &info);
    free(bytes);
    if (!ok) {
        fprintf(stderr, "resource hash is not a valid supported Amiga HUNK stream\n");
        return 1;
    }
    printf("hunks=%zu code=%zu data=%zu bss=%zu reloc32=%zu first-code-offset=%zu first-code-size=%zu\n",
           info.hunk_count, info.code_count, info.data_count, info.bss_count,
           info.reloc32_count, info.first_code_offset, info.first_code_size);
    return 0;
}
