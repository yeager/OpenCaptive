#include "iso9660_reader.h"
#include "sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc != 2 && !(argc == 3 && strcmp(argv[2], "--hash") == 0)) {
        fprintf(stderr, "usage: %s <mode1-2352-track.bin> [--hash]\n", argv[0]);
        return 2;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 1; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0) { fclose(f); return 1; }
    uint8_t *data = malloc((size_t)size);
    if (!data || fread(data, 1, (size_t)size, f) != (size_t)size) {
        free(data); fclose(f); return 1;
    }
    fclose(f);

    ISOImage iso;
    if (!iso_open_raw(&iso, data, (size_t)size)) {
        fprintf(stderr, "not a valid MODE1/2352 ISO9660 image\n");
        free(data);
        return 1;
    }
    printf("volume: %s\n", iso.volume_id);
    ISOEntry entries[256];
    int count = iso_list_root(&iso, entries, 256);
    for (int i = 0; i < count; i++) {
        printf("%c %8u %8u %s\n", entries[i].is_dir ? 'd' : 'f',
               entries[i].lba, entries[i].size, entries[i].name);
        if (argc == 3 && !entries[i].is_dir) {
            uint8_t *file = iso_read_file(&iso, entries[i].lba, entries[i].size);
            uint8_t digest[32];
            if (file) {
                sha256_digest(file, entries[i].size, digest);
                for (int j = 0; j < 32; j++) printf("%02x", digest[j]);
                printf("\n");
                free(file);
            }
        }
    }
    free(data);
    return count > 0 ? 0 : 1;
}
